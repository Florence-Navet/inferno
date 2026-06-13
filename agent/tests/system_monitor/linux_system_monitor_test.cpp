#include <gtest/gtest.h>

#include "system_monitor/linux_system_monitor.hpp"

// getOsInfo() reads the real system but os_type is always  LINUX
TEST(LinuxSystemMonitor, should_report_linux_as_os_type) {
        //TODO : create a monitor, call getOs Info(), check os_type
        LinuxSystemMonitor monitor;
        RegisterPayload info = monitor.getOsInfo();

        EXPECT_EQ(info.os_type, OSType::LINUX);
}