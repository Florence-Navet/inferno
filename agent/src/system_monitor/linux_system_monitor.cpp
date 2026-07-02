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
double getSystemUptimeSeconds() {
  std::ifstream uptimeFile("/proc/uptime");
  double uptimeSeconds = 0.0;
  uptimeFile >> uptimeSeconds;
  return uptimeSeconds;
}
 
// /proc/[pid]/comm  →  executable name, kernel-limited to 15 chars
std::string readProcessCommName(uint32_t processId) {
  std::ifstream commFile("/proc/" + std::to_string(processId) + "/comm");
  std::string processName;
  std::getline(commFile, processName);
  return processName;  // empty = process vanished or kernel thread without comm
}
 
// /proc/[pid]/status  →  VmRSS line  →  resident memory in kB
std::size_t readProcessVmRssKb(int processId) {
  std::ifstream statusFile("/proc/" + std::to_string(processId) + "/status");
  std::string currentLine;
  while (std::getline(statusFile, currentLine)) {
    if (currentLine.compare(0, 6, "VmRSS:") == 0) {
      std::istringstream rssValueStream(currentLine.substr(6));
      std::size_t residentMemoryKb = 0;
      rssValueStream >> residentMemoryKb;
      return residentMemoryKb;
    }
  }
  return 0;  // kernel threads and zombie processes have no VmRSS
}
 
// /proc/[pid]/stat  →  utime + stime (fields 14 and 15, 1-indexed)
unsigned long long readProcessCpuTicks(uint32_t processId) {
  std::ifstream statFile("/proc/" + std::to_string(processId) + "/stat");
  std::string statLine;
  if (!std::getline(statFile, statLine)) return 0;
 
  const auto closingParenthesisPos = statLine.rfind(')');
  if (closingParenthesisPos == std::string::npos) return 0;
 
  std::istringstream fieldsStream(statLine.substr(closingParenthesisPos + 2));
  std::string skippedToken;
  for (int i = 0; i < 11 && fieldsStream >> skippedToken; ++i) {}  // skip state..cmajflt
 
  unsigned long long userModeTicks = 0, kernelModeTicks = 0;
  if (fieldsStream >> userModeTicks >> kernelModeTicks) {
    return userModeTicks + kernelModeTicks;
  }
  return 0;
}
 
// CPU lifetime average: what fraction of total uptime did this
// process consume? Multiply by 100 for a percentage.
// clockTicksPerSecond = CLK_TCK (typically 100 on Linux) converts jiffies to seconds.
float calculateCpuPercentage(unsigned long long totalProcessTicks, double systemUptimeSeconds, long clockTicksPerSecond) {
  if (systemUptimeSeconds <= 0.0 || clockTicksPerSecond <= 0) return 0.0f;
  return static_cast<float>(totalProcessTicks)
       / static_cast<float>(clockTicksPerSecond)
       / static_cast<float>(systemUptimeSeconds)
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
// silently (readProcessCommName returns empty string).
std::vector<ProcessInfo> LinuxSystemMonitor::getProcessList() {
  const double systemUptimeSeconds = getSystemUptimeSeconds();
  const long   clockTicksPerSecond = sysconf(_SC_CLK_TCK);
 
  std::vector<ProcessInfo> processListResult;
 
  DIR* procDirectory = opendir("/proc");
  if (!procDirectory) return processListResult;
 
  struct dirent* directoryEntry;
  while ((directoryEntry = readdir(procDirectory)) != nullptr) {
    // Only process directory entries that start with a digit (representing PIDs)
    if (std::isdigit(static_cast<unsigned char>(directoryEntry->d_name[0]))) {
      const std::uint32_t         processId   = static_cast<std::uint32_t>(std::stoi(directoryEntry->d_name));
      const std::string processName = readProcessCommName(processId);
      
      // Ensure the process has not vanished (valid non-empty name)
      if (!processName.empty()) {
        const auto totalProcessTicks = readProcessCpuTicks(processId);
        const float cpuUsagePercentage = calculateCpuPercentage(totalProcessTicks, systemUptimeSeconds, clockTicksPerSecond);
        const std::size_t residentMemoryKb = readProcessVmRssKb(processId);

        processListResult.push_back({
          processId,
          processName,
          cpuUsagePercentage,
          residentMemoryKb
        });
      }
    }
  }
 
  closedir(procDirectory);
  return processListResult;
}

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