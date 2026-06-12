#include "system_monitor/linux_system_monitor.hpp"
#include <unistd.h>  // gethostname()

#include <cstdlib>   // getenv()

#include <sys/utsname.h>  // uname()

#include <fstream>   // std::ifstream

#include <pwd.h>
#include <unistd.h>

#include <dirent.h>
#include <cctype>    // std::isdigit
#include <sstream>
#include <string>
#include <vector>

LinuxSystemMonitor::LinuxSystemMonitor() {
    // Initialization code, if needed
}

LinuxSystemMonitor::~LinuxSystemMonitor() {
    // Cleanup code, if needed
}

// ── getProcessList helpers (file-local) ──────────────────────
// Each helper opens exactly one /proc file per process.
// If the file can't be opened (process vanished), it returns a
// safe default — the caller skips the entry when name is empty.
 
namespace {
 
// /proc/uptime  →  seconds since boot (first field)
double getUptime() {
  std::ifstream f("/proc/uptime");
  double up = 0.0;
  f >> up;
  return up;
}
 
// /proc/[pid]/comm  →  executable name, kernel-limited to 15 chars
std::string readComm(uint32_t pid) {
  std::ifstream f("/proc/" + std::to_string(pid) + "/comm");
  std::string name;
  std::getline(f, name);
  return name;  // empty = process vanished or kernel thread without comm
}
 
// /proc/[pid]/status  →  VmRSS line  →  resident memory in kB
std::size_t readVmRss(int pid) {
  std::ifstream f("/proc/" + std::to_string(pid) + "/status");
  std::string line;
  while (std::getline(f, line)) {
    if (line.compare(0, 6, "VmRSS:") == 0) {
      std::istringstream iss(line.substr(6));
      std::size_t kb = 0;
      iss >> kb;
      return kb;
    }
  }
  return 0;  // kernel threads and zombie processes have no VmRSS
}
 
// /proc/[pid]/stat  →  utime + stime (fields 14 and 15, 1-indexed)
//
// The stat line format is:
//   pid (comm) state ppid pgrp session tty tty_pgrp flags
//   minflt cminflt majflt cmajflt utime stime ...
//
// (comm) can contain spaces and '(' ')' characters, so we search
// for the *last* ')' to find where the fixed-position fields start.
// After that closing paren the fields are:
//   state(1) ppid(2) pgrp(3) session(4) tty(5) tty_pgrp(6)
//   flags(7) minflt(8) cminflt(9) majflt(10) cmajflt(11)
//   utime(12) stime(13)   <- 0-indexed from the token after ')'
unsigned long long readCpuTicks(uint32_t pid) {
  std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
  std::string line;
  if (!std::getline(f, line)) return 0;
 
  const auto closing = line.rfind(')');
  if (closing == std::string::npos) return 0;
 
  std::istringstream iss(line.substr(closing + 2));
  std::string tok;
  for (int i = 0; i < 11 && iss >> tok; ++i) {}  // skip state..cmajflt
 
  unsigned long long utime = 0, stime = 0;
  if (iss >> utime >> stime) return utime + stime;
  return 0;
}
 
// CPU lifetime average: what fraction of total uptime did this
// process consume? Multiply by 100 for a percentage.
// hz = CLK_TCK (typically 100 on Linux) converts jiffies to seconds.
float toCpuPercent(unsigned long long ticks, double uptime, long hz) {
  if (uptime <= 0.0 || hz <= 0) return 0.0f;
  return static_cast<float>(ticks)
       / static_cast<float>(hz)
       / static_cast<float>(uptime)
       * 100.0f;
}
 
}  // namespace

RegisterPayload LinuxSystemMonitor::getOsInfo() {
        RegisterPayload info;

        info.os_type = OSType::LINUX;
        info.hostname = readHostName();
        info.current_user = readCurrentUser();
        info.arch = readArch();
        info.os_version = readOsVersion();
   
    return info;
}
 
// ── getProcessList ────────────────────────────────────────────
//
// Reads /proc at the moment of the call — the kernel maintains
// the virtual filesystem in real time, so the snapshot is current.
// Processes that die between readdir() and file open are skipped
// silently (readComm returns empty string → continue).
std::vector<ProcessInfo> LinuxSystemMonitor::getProcessList() {
  const double uptime = getUptime();
  const long   hz     = sysconf(_SC_CLK_TCK);
 
  std::vector<ProcessInfo> result;
 
  DIR* proc = opendir("/proc");
  if (!proc) return result;
 
  struct dirent* entry;
  while ((entry = readdir(proc)) != nullptr) {
    if (!std::isdigit(static_cast<unsigned char>(entry->d_name[0]))) continue;
 
    const int         pid  = std::stoi(entry->d_name);
    const std::string name = readComm(pid);
    if (name.empty()) continue;
 
    const auto ticks = readCpuTicks(pid);
    result.push_back({pid,
                      name,
                      toCpuPercent(ticks, uptime, hz),
                      readVmRss(pid)});
  }
 
  closedir(proc);
  return result;
}

// MetricsSample LinuxSystemMonitor::sampleMetrics() {
//     // Implement metrics sampling logic here
//     return MetricsSample{};
// }
std::string LinuxSystemMonitor::executeShell(const std::string& cmd) {
    // Implement shell command execution logic here
    (void) cmd; // not used yed
    return std::string{};
}


std::string LinuxSystemMonitor::readHostName(){
    char buffer[256]; // Linux hosnmaes ->255 chars

    //gethosname() fills the buffer and returns 0 on success  // -1 if error
    if (gethostname(buffer, sizeof(buffer)) != 0) {
        return std::string{};  // return empty string rather trhan crashing
    }

    return  std::string{buffer};

}

std::string LinuxSystemMonitor::readCurrentUser(){

    //getenv() returns  a raw char* - or a nullptr if the variable is not set
    const char* user = getenv("USER");

    //USER is no always set - inside docker
    // sy stem user db always knows  the current user

    if (user == nullptr) {
        const struct passwd* pw = getpwuid(getuid());
        if(pw != nullptr) {
            return std::string  {pw ->pw_name};// ex root
        }
        return std::string {};
    }


    return std::string{user};
}

ArchType LinuxSystemMonitor::readArch() {
    // Todo : detect CPU architecture via uname()
    struct utsname info;

    // uname() fills the structure and returns 0(success) / -1 (error)
    if (uname(&info) != 0) {
        return ArchType::X64; // fall backs to a senisible default on failure
    }

    std::string machine = info.machine;

    //TODO -> translate the machien string into an ArchType
    if (machine == "x86_64") {
        return ArchType::X64;
    } else if (machine == "i386" || machine == "i686"){
        return ArchType::X86;
    } else if ((machine.rfind("arm", 0) == 0) || (machine == "aarch64")) {
        return ArchType::ARM;
    }
    return ArchType::X64; // default if the string is not recognized
}

std::string LinuxSystemMonitor::readOsVersion() {
    // TODO : read Pretty_NAME  form /etc/os-release/blabla

    std::ifstream file("/etc/os-release");

    if(!file.is_open()) {
        return  std::string{};
    }

    // TODO : read the file line by line and find PRETTY_NAME

    std::string line;

    // read file line by line
    while (std::getline(file, line)) {
        //TODO : check if this line stars with PRETTY_NAME= and extract it
        if (line.rfind("PRETTY_NAME=", 0) == 0) {
            // TOTO : extract the value after the '=' and strip the quote
            // cut off the "PRETTY_NAME=" to keep only the value
            std::string value = line.substr(std::string("PRETTY_NAME=").length());

            //the value is usually wrapped in double quotes - remove them
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            return value;
       
        }
    }

    return std::string{};
}