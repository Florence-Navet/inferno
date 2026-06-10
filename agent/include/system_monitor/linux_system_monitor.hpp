#ifndef LINUX_SYSTEM_MONITOR_HPP
#define LINUX_SYSTEM_MONITOR_HPP

#include "system_monitor/i_system_monitor.hpp"

class LinuxSystemMonitor : public ISystemMonitor {
public:
    LinuxSystemMonitor();
    ~LinuxSystemMonitor() override;

    RegisterPayload       getOsInfo()                          override;
    std::vector<ProcessInfo>  getProcessList()                     override;
    // MetricsSample sampleMetrics()                     override;
    std::string  executeShell(const std::string& command) override;

    private:
    // Reads the machine"s hostname ("inferno-agent-1")
    std::string readHostName();
};


#endif // LINUX_SYSTEM_MONITOR_HPP