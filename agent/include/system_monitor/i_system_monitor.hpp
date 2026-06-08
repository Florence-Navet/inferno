#ifndef I_SYSTEM_MONITOR_HPP
#define I_SYSTEM_MONITOR_HPP

#include <cstdint>

class ISystemMonitor {
public:
    virtual ~ISystemMonitor() = default;
    virtual OsInfo       getOsInfo()                          = 0;
    virtual ProcessList  getProcessList()                     = 0;
    virtual MetricsSample sampleMetrics()                     = 0;
    virtual std::string  executeShell(const std::string& cmd) = 0;
};

#endif // I_SYSTEM_MONITOR_HPP