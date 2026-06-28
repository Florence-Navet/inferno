#ifndef LPTF_PROTOCOL_HPP
#define LPTF_PROTOCOL_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// ! send() second arg is size_t
constexpr std::uint8_t LPTF_VERSION = 1;
constexpr std::uint8_t LPTF_HEADER_SIZE =
    sizeof(std::uint8_t) * 6 +
    sizeof(std::uint16_t);  // identifier + version + type + size
constexpr char LPTF_IDENTIFIER[4] = {'L', 'P', 'T', 'F'};
constexpr std::string_view LPTF_IDENTIFIER_STR(LPTF_IDENTIFIER, 4);

constexpr std::size_t REGISTER_FIXED_BYTES =
    3 * sizeof(std::uint16_t) +
    2 * sizeof(std::uint8_t);  // hostname_len + os_version_len +
                               // current_user_len + os_type + arch

constexpr std::size_t KMAX_U16_VALUE = 65535u;
constexpr std::uint16_t MAX_VALUE_INT16 =
    static_cast<std::uint16_t>(KMAX_U16_VALUE);
constexpr std::size_t REGISTER_MAX_HOSTNAME_LEN =
    MAX_VALUE_INT16 - REGISTER_FIXED_BYTES;
constexpr std::size_t COMMAND_FIXED_BYTES =
    sizeof(std::uint16_t) * 2 + sizeof(std::uint8_t);  // id + type + data_len
constexpr std::size_t RESPONSE_FIXED_BYTES =
    sizeof(std::uint16_t) * 2 +
    sizeof(std::uint8_t) *
        3;  // id + data_len + status + total_chunks + chunk_index
constexpr std::size_t DATA_FIXED_BYTES =
    sizeof(std::uint16_t) + sizeof(std::uint8_t);
constexpr std::size_t ERROR_FIXED_BYTES =
    sizeof(std::uint16_t) + sizeof(std::uint8_t);

constexpr std::size_t PROCESS_INFO_FIXED_SIZE = sizeof(std::uint32_t) +
    sizeof(float) +
    sizeof(std::uint64_t) +
    sizeof(std::uint16_t);

    // CPU: float total + uint8_t count  (per_core is variable)
constexpr std::size_t CPU_SAMPLE_FIXED_SIZE = sizeof(float) + sizeof(std::uint8_t);

// MEM: 5 × uint64_t, fully fixed
constexpr std::size_t MEM_SAMPLE_FIXED_SIZE = 5 * sizeof(std::uint64_t);

// DISK: uint16_t device_len + 2 × uint64_t  (device string is variable)
constexpr std::size_t DISK_SAMPLE_FIXED_SIZE = sizeof(std::uint16_t) + 2 * sizeof(float);

// NET: uint16_t iface_len + 2 × uint64_t  (iface string is variable)
constexpr std::size_t NET_SAMPLE_FIXED_SIZE = sizeof(std::uint16_t) + 2 * sizeof(float);

// METRICS top-level: 2 × uint8_t for disk_count + interface_count
// (CpuSample and MemSample are inlined, variable themselves)
constexpr std::size_t METRICS_SAMPLE_FIXED_SIZE = sizeof(std::uint8_t) * 2;

constexpr int METRICS_INTERVAL_MS = 1000;

enum class MessageType : std::uint8_t {
  REGISTER = 0,
  DATA = 1,
  COMMAND = 2,
  RESPONSE = 3,
  DISCONNECT = 4,
  ERROR = 5,
  END,  // must always be the last !!
};

enum class CommandType : std::uint8_t {
  OS_INFO = 0,
  RUNNING_PROCESSES = 1,
  SHELL = 2,
  START_METRICS = 3,
  STOP_METRICS = 4,
  END,  // must be the last one
};

enum class DataType : std::uint8_t {
  METRICS_SAMPLE = 0,
  END,  // must be the last one
};

enum class ErrorType : std::uint8_t {
  UNKNOWN_TYPE = 0,
  INVALID_FORMAT = 1,
  UNKNOWN_COMMAND = 2,
  EXECUTION_FAILED = 3,
  SIZE_EXCEEDED = 4,
  END,  // must be the last one
};

enum class OSType : std::uint8_t {
  WINDOWS = 0,
  LINUX = 1,
  MAC = 2,
  END,  // must be the last one
};

enum class ArchType : std::uint8_t {
  X86 = 0,
  X64 = 1,
  ARM = 2,
  END,  // must be the last one
};

enum class ResponseStatus : std::uint8_t {
  OK = 0,
  ERROR = 1,
  END,  // must be the last one
};

struct LptfHeader {
  char identifier[4];
  std::uint8_t version;
  MessageType type;
  std::uint16_t size;
};

struct RegisterPayload {
  OSType os_type;
  ArchType arch;
  std::string hostname;
  std::string os_version;    // new — "Ubuntu 22.04", "Windows 11"
  std::string current_user;  // new — getenv("USER") / GetUserName()
};

struct CommandPayload {
  std::uint16_t id;
  CommandType type;
  std::string data;
};

struct ResponsePayload {
  std::uint16_t id;
  ResponseStatus status;
  std::uint8_t total_chunks;
  std::uint8_t chunk_index;
  std::vector<std::uint8_t> data;
};

struct DataPayload {
  DataType subtype;
  // std::string data;
  std::vector<std::uint8_t> data; 
};

struct ErrorPayload {
  ErrorType code;
  std::string message;
};

// Generic struct for recv()
struct Frame {
  LptfHeader header;
  std::vector<std::uint8_t> payload;
};

struct ProcessInfo {
  std::uint32_t pid;       // kernel process id
  std::string name;        // process name (up to 15 chars, from /proc/[pid]/status, -name, GetProcessImageFileName)
  float cpu_percent;       // lifetime average CPU % (utime+stime / uptime)
  std::uint64_t mem_bytes; // resident set size in kB (VmRSS from /proc/[pid]/status)
};

struct CpuSample {
    float total_percent = 0.0f;
    std::vector<float> per_core; // one per logical core
};

struct MemSample {
    std::uint64_t phys_total = 0;
    std::uint64_t phys_used = 0;
    std::uint64_t phys_available = 0;
    std::uint64_t swap_total = 0;
    std::uint64_t swap_used = 0;
};

struct DiskSample {
    std::string   device;           // e.g. "sda", "C:"
    float read_bytes_per_sec = 0.0f;
    float write_bytes_per_sec = 0.0f;
};

struct NetSample {
    std::string   iface;            // e.g. "eth0", "Ethernet"
    float rx_bytes_per_sec = 0.0f;
    float tx_bytes_per_sec = 0.0f;
};

struct MetricsSample {
    CpuSample             cpu;
    MemSample             mem;
    std::vector<DiskSample> disks;
    std::vector<NetSample>  interfaces;
};

#endif