# lptf binary protocol

## 1. Overview

LPTF is a custom binary agent-server protocol:

- Agent connects and registers itself.
- Server sends COMMANDS.
- Agent executes commands and sends RESPONSES (possibly chunked).
- Agent may send unsolicited DATA.
- TCP sockets are used as the transport.

The protocol is designed to be:

- Cross-platform
- Lightweight
- Extendable

## 2. Transport

- Use TCP sockets (SOCK_STREAM) at low level.
- TCP provides reliability, ordered delivery, and byte-stream semantics.
- Your protocol is responsible for parsing bytes into messages, including chunked responses.

## 3. Message Format

All messages have a fixed-size header followed by a payload:

```text
[Identifier][Version][Type][Size] 
 4 bytes     1B      1B     2B
```

| Field      | Size | Description                          |
| ---------- | ---- | ------------------------------------ |
| Identifier | 4B   | ASCII string (e.g. `"LPTF"`)         |
| Version    | 1B   | Protocol version (current: `1`)      |
| Type       | 1B   | Message type                         |
| Size       | 2B   | Payload length (uint16, max `65535`) |

Payload immediately follows the header.

### 3.1 Message Types

| Value | Name               | Description                      |
| ----- | ------------------ | -------------------------------- |
| 0     | REGISTER           | Agent → Server registration      |
| 1     | DASHBOARD_REGISTER | dashboard -> Server registration |
| 2     | DATA               | Unsolicited agent data           |
| 3     | COMMAND            | Server → Agent instruction       |
| 4     | RESPONSE           | Agent → Server result            |
| 5     | DISCONNECT         | Close connection                 |
| 6     | ERROR              | Error reporting                  |

## 4. Payload Structures

### 4.1 REGISTER

Agent sends REGISTER immediately after connecting.

```c++
    struct OsInfoPayload {
        uint8_t os_type;        // 0=Windows, 1=Linux, 2=macOS
        uint8_t arch;           // 0=x86, 1=x64, 2=ARM
        uint16_t hostname_len;
        uint16_t os_version_len
        uint16_t current_user_len;
        uint16_t ip_len;
        uint16_t mac_len;
        char hostname[hostname_len]; // UTF-8
        char os_version[os_version_len];
        char current_user[current_user_len];
        char ip[ip_len];
        char mac[mac_len];
    };
```

Rules:

- Server ignores other messages until REGISTER is received.
- Invalid or missing REGISTER → connection may be closed.
mac adress is meant to recognize an agent in between connection and if an agent process is stopped (computer shutdown)

### 4.2 COMMAND

Server sends COMMAND to request action:

```c++
struct CommandPayload {
    uint32_t id;        // unique id for this command
    uint8_t type;       // OS_INFO, RUNNING_PROCESSES, SHELL
    uint16_t data_len;  // optional command string length (SHELL only)
    char data[data_len];
};
```

#### Command types

| Value | Name              |
| ----- | ----------------- |
| 0     | OS_INFO           |
| 1     | RUNNING_PROCESSES |
| 2     | SHELL             |
| 3     | START_METRICS     |
| 4     | STOP_METRICS      |

### 4.3 RESPONSE

Agent sends RESPONSE after executing a command. Supports chunking:

```c++
struct ResponsePayload {
    uint32_t id;          // command id this response belongs to
    uint8_t status;       // 0=OK, 1=ERROR
    uint8_t total_chunks; // total number of chunks
    uint8_t chunk_index;  // 0-based index of this chunk
    uint16_t data_len;    // length of this chunk
    uint8_t data[data_len];  
};
```
the max length for a chunk is the response payload fixed byte ( 4 * uint8_t size + uint16_t + uint32_t ) + the target id size (uint32_t) given by server to allow dashboard <-> server communication (and agent identification)

if process info , response.data will be a vector of processInfo:
```c++
struct ProcessInfo {
    uint32_t pid;
    float cpu_percent;  // 0.0 – 100.0
    uint64_t mem_bytes;
    uint16_t name_len
    char name[name_len];
};
```

vector will look like this :
```
std::vector<uint8_t> | processCount (2 bytes) | processList (N bytes) |
```

#### Chunking rules

- Each chunk ≤ 65535 bytes
- Server reassembles using:
  - `id`
  - `chunk_index`
  - `total_chunks`
- Large outputs must be split into multiple chunks

### 4.4 DATA

#### Datasubtype
| Value | Name              |
| ----- | ----------------- |
| 0     | METRICS_SAMPLE    |
| 1     | HEALTH_CHECK      |
| 2     | REGISTRATION      |
| 3     | AGENTS            |
| 4     | UNKNOWN           |

registration = server push RegisterPayload of a first ever seen agent
agents = list of RegisterPayload on dashboard connection, retreived from db

```c++
struct DataPayload {
    uint8_t subtype;        // custom type
    uint16_t data_len;
    uint8_t data[data_len];
};
```

### 4.4.1 Metric Sample
```c++
struct CpuSample {
    float total_percent;
    uint8_t core_number;
    float per_core[core_number]
}

struct MemSample {
    uint64_t phys_total;
    uint64_t phys_used;
    uint64_t phys_available;
    uint64_t swap_total;
    uint64_t swap_used;
}


struct DiskSample {
    float read_bytes_per_sec;
    float write_bytes_per_sec;
    uint16_t device_len;
    uint8_t   device[device_len];           // e.g. "sda", "C:"
};

struct NetSample {
    float rx_bytes_per_sec;
    float tx_bytes_per_sec;
    uint16_t iface_len
    uint8_t   iface[iface_len];            // e.g. "eth0", "Ethernet"
};

struct MetricsSample {
    CpuSample             cpu;
    MemSample             mem;
    uint8_t disk_count;
    uint8_t interface_count;
    DiskSample disks[disk_count];
    NetSample  interfaces[interface_count];
};

```


### 4.5 ERROR

```c++
struct ErrorPayload {
    uint8_t code;           // see table below
    uint16_t message_len;
    char message[message_len]; // UTF-8
};
```

#### Error codes

| Code | Meaning          |
| ---- | ---------------- |
| 0    | UNKNOWN_TYPE     |
| 1    | INVALID_FORMAT   |
| 2    | UNKNOWN_COMMAND  |
| 3    | EXECUTION_FAILED |
| 4    | SIZE_EXCEEDED    |

### 4.6 DISCONNECT

- No payload
- Server may force disconnect
- Agent must:
  - Close socket
  - Optionally reconnect

## 5. Parsing Guidelines (TCP Stream)

TCP provides a continuous byte stream. You must reconstruct messages:

- Maintain a receive buffer.
- Append all received bytes.
- While buffer contains at least 8 bytes (header):
  - Peek header → read size
  - Compute:  
        `total_size = 8 + size`
  - If buffer < total_size → wait for more data
  - Else:
    - extract full message
    - process it
    - Repeat (handle multiple messages in the buffer)

Important: bytes from different messages never interleave, but a single message may be fragmented across multiple recv() calls.

## 6. Versioning

- Agent version < protocol version → reject
- Agent version > protocol version → accept (backward-compatible)
- Always check version in REGISTER

## 7. Limits & Timeouts

- Max payload = 65535 bytes (chunking for larger data)
- Optional: limit number of concurrent agents

### Optional constraints

- Limit number of concurrent agents
- Disconnect if message incomplete after timeout

## 8. Notes

- All integers are big-endian
- All strings are UTF-8
- COMMAND id ensures correct mapping of RESPONSES, even with multiple simultaneous commands
- Chunking ensures no single message exceeds the max payload size


## Communication between dashboard and server
dashboard will wrap the previous structure with the target identifier (hostname or ip) so server can route dashboard queries to the rightful agent

```c++
struct DashboardCommand {
  std::string target;
  CommandPayload command;
};

struct DashboardData {
  std::string target;
  DataPayload data;
};

struct DashboardResponse {
  std::string target;        // which agent sent this response
  ResponsePayload response;
};

struct RegisterPayload {
    std::string id;
    OsInfoPayload system
}
```
On the network : 
```c++
uint16_t target_len;  // optional command string length (SHELL only)
char target[target_len];
std::vector<std::uint8_t> Commandpayload / DashboardData / DashboardResponse serialized
```

```c++
uint16_t id_len;  // optional command string length (SHELL only)
char id[id_len];
std::vector<std::uint8_t> OsInfoPayload serialized
```

- Disconnect (côté dashboard) envoie un payload de l'agent id (target dans le protocole dans command.hpp data.hpp et response.hpp) + pas de feedback (on considère qu'une déconnexion se passera toujours bien)
- L'envoie des `OsInfoPayload` au dashboard se fait via `DataPayload`, `DataType::REGISTRATION `sous forme de `RegisterPayload` lorsqu'il se connecte pour la toute première fois. Se fait par `DataType::AGENTS` pour récupérer une liste de `RegisterPayload` de la db
- CommandPayload : le dashboard ne génère pas l'ID, c'est le server qui s'en charge, le dashboard laisse à 0 (initialisation par défaut)
- Le dashboard reçoit la réponse du server d'une commande via `DashboardResponse` qui contient "target", l'agent d'où provient la réponse 


```c++
struct RegisterPayload {
  std::string id;
  OsInfoPayload system;

  bool operator==(const RegisterPayload& other) const {
    return id == other.id && system == other.system;
  }
};
```