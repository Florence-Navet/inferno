#include "system_monitor/linux_system_monitor.hpp"

LinuxSystemMonitor::LinuxSystemMonitor() {
    // Initialization code, if needed
}

LinuxSystemMonitor::~LinuxSystemMonitor() {
    // Cleanup code, if needed
}

RegisterPayload LinuxSystemMonitor::getOsInfo() {
    // Implement OS info retrieval logic here
    return RegisterPayload{};
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

