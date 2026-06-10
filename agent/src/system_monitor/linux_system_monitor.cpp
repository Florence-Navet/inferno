#include "system_monitor/linux_system_monitor.hpp"
#include <unistd.h>  // gethostname()

#include <cstdlib>   // getenv()

#include <sys/utsname.h>  // uname()

LinuxSystemMonitor::LinuxSystemMonitor() {
    // Initialization code, if needed
}

LinuxSystemMonitor::~LinuxSystemMonitor() {
    // Cleanup code, if needed
}

RegisterPayload LinuxSystemMonitor::getOsInfo() {
        RegisterPayload info;

        info.os_type = OSType::LINUX;
        info.hostname = readHostName();
        info.current_user = readCurrentUser();
        info.arch = readArch();
   
    return info;
}
std::vector<ProcessInfo> LinuxSystemMonitor::getProcessList() {
    // Implement process list retrieval logic here
    return std::vector<ProcessInfo>{};
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

    if (user == nullptr) {
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