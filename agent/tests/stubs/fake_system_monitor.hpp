#ifndef FAKE_SYSTEM_MONITOR_HPP
#define FAKE_SYSTEM_MONITOR_HPP

#include "builders/process_builder.hpp"
#include "builders/os_info_builder.hpp"
#include "fixtures/protocol.hpp"
#include "system_monitor/i_system_monitor.hpp"

class FakeSystemMonitor : public ISystemMonitor {
 public:
  RegisterPayload getOsInfo() override {
    return OsInfoBuilder::create();
    // RegisterPayload info;
    // info.os_type = OSType::LINUX;
    // info.arch = ArchType::X64;
    // // info.hostname = "agent-01";
    // info.hostname = Protocol::TEST_HOSTNAME_STR;
    // info.os_version = Protocol::TEST_OS_VERSION_STR;
    // // info.os_version = "test-osversion";
    // // info.current_user = "test-currentuser";
    // info.current_user = Protocol::TEST_CURRENT_USER_STR;
    // return info;
  }

  std::vector<ProcessInfo> getProcessList() override {
    ProcessInfo process1 =
        ProcessBuilder::createProcessInfo(1001, 12.5f, 123456, "proc-a");
    ProcessInfo process2 =
        ProcessBuilder::createProcessInfo(1002, 7.25f, 654321, "proc-b");
    return {process1, process2};
  }

  std::string executeShell(const std::string& cmd) override {
    // Implement shell command execution logic here
    (void)cmd;  // not used yet
    return std::string{};
  }
};

#endif