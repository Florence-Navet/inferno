#ifndef LINUX_SYSTEM_MONITOR_HPP
#define LINUX_SYSTEM_MONITOR_HPP

#include "system_monitor/i_system_monitor.hpp"

class LinuxSystemMonitor : public ISystemMonitor {
public:
    LinuxSystemMonitor();
    ~LinuxSystemMonitor() override;

    OsInfo       getOsInfo()                          override;
    ProcessList  getProcessList()                     override;
    MetricsSample sampleMetrics()                     override;
    std::string  executeShell(const std::string& cmd) override;
};


#endif // LINUX_SYSTEM_MONITOR_HPP