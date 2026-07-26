# Inferno

## Project description

Inferno is a distributed remote monitoring and diagnostics platform built in C++.

Originally derived from an academic specification focused on progressively complex networking and system programming challenges, the project has been fully reframed into a legitimate monitoring and orchestration platform.

The project is composed of:

- a central server responsible for orchestration and analysis,
- remote agents deployed on monitored machines,
- and a future Qt desktop dashboard for visualization and control.

The system focuses on:

- low-level network communication,
- custom binary protocol design,
- cross-platform system monitoring,
- and distributed telemetry processing.

## Planned architecture

Inferno is designed as a distributed monitoring system composed of three main components:

### Agent (remote node)

A lightweight system daemon responsible for:

- collecting machine metrics,
- executing diagnostic commands,
- streaming telemetry to the server,
- maintaining persistent connection with automatic reconnection.

### Server (central coordinator)

A central service responsible for:

- managing multiple connected agents,
- receiving and parsing telemetry streams,
- coordinating requests and responses,
- preparing data for visualization and analysis.

### Desktop dashboard (future)

A Qt-based interface providing:

- real-time monitoring dashboard,
- agent management interface,
- visualization of system metrics and diagnostics.

## Features

### Current

- Custom binary communication protocol
- Multi-agent server architecture
- Shared C++ networking library
- Docker-based development pipeline
- Automated test execution during builds
- Multi-agent scaling support via Docker Compose

### Planned

- Continuous metrics streaming
- Remote diagnostics execution
- Cross-platform monitoring agent
- Qt desktop monitoring interface
- PostgreSQL data persistence
- Health analysis and anomaly detection
- Agent reconnection resilience
- Background daemon/service deployment

## Tech stack

| Layer                    | Technology              |
| ------------------------ | ----------------------- |
| Backend                  | C++17                   |
| Build system             | CMake                   |
| Containerization         | Docker & Docker Compose |
| Testing                  | Google Test             |
| Database (planned)       | PostgreSQL              |
| Desktop dashboard (planned) | Qt 6                    |

## Prerequisites

- Docker / Docker Desktop or [Podman](https://github.com/containers/podman) installed and running

## Setup

Copy `.env.template` to `.env` and adjust values if needed.

```bash
cp .env.template .env
```

## Quick start

This project uses Docker Compose profiles (`agent` and `server`) to separate build/runtime pipelines and keep logs readable.

`COMPOSE_PROFILES=agent,server` in `.env` allows `docker compose up` and `docker compose down` to work without specifying profiles manually.

### Start all services (agent + server)

```
docker compose up
```

### Start only agents

```
docker compose --profile agent up
```

### Scale agents

```
docker compose --profile agent up --scale agent=N
```

> replace N with the actual number you need

### Start only server

```
docker compose --profile server up
```

### Stop all services

```
docker compose down
```

> add the `-v` flag to remove build volumes and start from scratch

## Start the Qt dashboard on Windows

Start VcXsrv with TCP connections enabled:

```bash
powershell.exe -NoProfile -Command "Start-Process -FilePath 'C:\Program Files\VcXsrv\vcxsrv.exe' -ArgumentList ':0','-multiwindow','-clipboard','-ac','-listen','tcp'"
```

---

## Start the Qt dashboard on Windows

Windows users must first launch **XLaunch** and configure it as follows:

1. Select **Multiple windows**
2. Keep **Display number** set to `0`
3. Click **Next**
4. Select **Start no dashboard**
5. Click **Next**
6. Enable:
   - **Clipboard**
   - **Native OpenGL**
   - **Disable access control**
7. Click **Next**
8. Click **Finish**

Keep XLaunch running while using the dashboard.

Then start the server and the Qt dashboard:

```bash
docker compose --profile server --profile dashboard up --build
```

To start the complete project:

```bash
docker compose --profile server --profile agent --profile dashboard up --build
```

Then start the server and the Qt dashboard:

```bash
docker compose --profile server --profile dashboard up --build
```

To start the complete project:

```bash
docker compose --profile server --profile agent --profile dashboard up --build

```

---

### Linux

Linux users do not need XLaunch.

First, check that the graphical display is available:

```bash
echo $DISPLAY
```

The command should return a value such as `:0` or `:1`.

Allow the Docker container to access the graphical display:

```bash
xhost +SI:localuser:root
```

Then start the server and the Qt dashboard using the Linux Compose configuration:

```bash
docker compose \
  -f docker-compose.yml \
  -f docker-compose.linux.yml \
  --profile server \
  --profile dashboard \
  up --build
```

To start the complete project with an agent:

```bash
docker compose \
  -f docker-compose.yml \
  -f docker-compose.linux.yml \
  --profile server \
  --profile agent \
  --profile dashboard \
  up --build
```

After stopping the project, revoke the graphical display permission:

```bash
xhost -SI:localuser:root
```

If the `xhost` command is not installed:

```bash
sudo apt install x11-xserver-utils
```

## How to build agent and server

This project uses a **multi-stage container build pipeline** orchestrated through Docker Compose-compatible services. The pipeline has been tested with both Docker and Podman.

Builder services are only responsible for compilation and testing.
Runtime services only execute the final binaries produced during the build pipeline.

The first service, `shared`, builds the shared static library (`.a`) inside a dedicated Docker volume.
`shared` tests are executed before the service exits. If any shared test fails, the pipeline stops and dependent services are not started.

The second service (`agent-builder` or `server-builder`) compiles the final target binary using the shared `.a` artifact from the common volume. The compiled target binary is stored in another dedicated volume. Like the `shared` service, target tests are executed before the service exits.

Build artifacts are stored in Docker named volumes (`shared-build`, `server-build`, `agent-build`) so rebuilds can reuse previous outputs. Use `docker compose down -v` to remove these volumes and start from scratch.

Finally, the runtime service starts the compiled target binary directly from its build volume.

Multiple runtime instances can be started without rebuilding the binary, since all instances use the same compiled artifact.

The following diagram illustrates the pipeline:
![pipeline](./_docs/project/build_pipeline_&_artifact_flow.png)

## How to build dashboard

First, check if you already have the required tools:

```bash
qmake6 --version && cmake --version
```

If anything is missing, you have two options:

**Option A — via WSL (Windows Subsystem for Linux):**

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools
sudo apt install cmake
sudo apt install qt6-websockets-dev
```

**Option B — via the Qt official installer:**

Download Qt from [https://www.qt.io/download-open-source](https://www.qt.io/download-open-source) (free community version, account required).
Install `Qt` with `gcc`, `g++` and `cmake` to avoid path issues.
Qt **6.4.2 minimum**, **6.10.2** was used for development.

Then add these to your `PATH`:

```
C:\Qt\Tools\QtCreator\bin
C:\Qt\6.x.x\mingw_64\bin
```

**Build the dashboard:**

From **PowerShell**:
```powershell
./dashboard/build.bat
```

From **Git Bash**:
```bash
powershell.exe -NoProfile -Command "& '$(cygpath -w ./dashboard/build.bat)'"
```

**Run the dashboard:**

```bash
./dashboard/build/dashboard.exe
```
> **WSL:** if you get EGL/MESA errors, add `export LIBGL_ALWAYS_SOFTWARE=1` to your `~/.bashrc`
---

### Client (Linux)

Check your tools first:

```bash
qmake6 --version && cmake --version
```

Install if needed:

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools
sudo apt install cmake
sudo apt install qt6-websockets-dev
```

Make the scripts executable (only needed once):

```bash
chmod +x ./dashboard/build.sh
chmod +x ./dashboard/run-dashboard.sh
```

Build then run:

```bash
./dashboard/build.sh
./dashboard/run-dashboard.sh
```

**WSL only:** if you get EGL/MESA rendering errors, add this to your `~/.bashrc` and restart your terminal:
```bash
export LIBGL_ALWAYS_SOFTWARE=1
```
> WSL has no GPU access, so Qt's hardware OpenGL rendering fails. This flag forces software rendering instead.

---
