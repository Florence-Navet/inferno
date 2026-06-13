#ifndef FAKE_SYSTEM_MONITOR_HPP
#define FAKE_SYSTEM_MONITOR_HPP

#include "system_monitor/i_system_monitor.hpp"

class FakeSystemMonitor : public ISystemMonitor {
        public:
        RegisterPayload getOsInfo() override {
                RegisterPayload info;
                info.os_type = OSType::LINUX;
                info.arch = ArchType::X64;
                info.hostname = "test-hostname";
                 info.os_version ="test-osversion";
                info.current_user = "test-currentuser";
                return info;
        }

        std::vector<ProcessInfo> getProcessList() override {
    // Implement process list retrieval logic here
    return std::vector<ProcessInfo>{};
}

std::string executeShell(const std::string& cmd) override {
    // Implement shell command execution logic here
    (void) cmd; // not used yed
    return std::string{};
}

};


#endif