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
    info.os_version = "test-osversion";
    info.current_user = "test-currentuser";
    return info;
  }

  std::vector<ProcessInfo> getProcessList() override {
    ProcessInfo p1;
    p1.pid = 1001;
    p1.name = "proc-a";
    p1.cpu_percent = 12.5f;
    p1.mem_bytes = 123456;

    ProcessInfo p2;
    p2.pid = 1002;
    p2.name = "proc-b";
    p2.cpu_percent = 7.25f;
    p2.mem_bytes = 654321;

    return {p1, p2};
  }

  std::string executeShell(const std::string& cmd) override {
    // Implement shell command execution logic here
    (void)cmd; // not used yet
    return std::string{};
  }
};

#endif