#include "system_monitor/linux_system_monitor.hpp"
#include <unistd.h>  // gethostname()

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