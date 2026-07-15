#include "system_monitor/linux_system_monitor.hpp"

#include <gtest/gtest.h>

#if defined(__linux__)

// getOsInfo() reads the real system but os_type is always  LINUX
TEST(LinuxSystemMonitor, should_report_linux_as_os_type) {
  // TODO : create a monitor, call getOs Info(), check os_type
  LinuxSystemMonitor monitor;
  OsInfoPayload info = monitor.getOsInfo();

  EXPECT_EQ(info.os_type, OSType::LINUX);
}

// executeShell() runs the command through the shell and returns its stdout
TEST(LinuxSystemMonitor, should_return_command_output) {
  LinuxSystemMonitor monitor;

  EXPECT_EQ(monitor.executeShell("echo hello"), "hello\n");
}


// stderr is not captured, so a failing command returns no output
TEST(LinuxSystemMonitor, should_return_empty_output_when_command_fails) {
  LinuxSystemMonitor monitor;

  EXPECT_EQ(monitor.executeShell("inferno_does_not_exist"), "");
}

#endif
