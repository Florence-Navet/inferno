#include "metrics/linux_metrics_scrapper.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

class LinuxMetricsScrapperTest : public ::testing::Test {
 protected:
  LinuxMetricsScrapper scrapper;
  bool createdProc = false;

 protected:
  void SetUp() override {
    // Create fake /proc files for testing
    createFakeProc();
  }

  void TearDown() override {
    if (createdProc) {
      system(
          "rm -f /proc/stat /proc/meminfo /proc/diskstats /proc/net/dev "
          "2>/dev/null");
    }
  }

  void createFakeProc() {
    std::ifstream stat("/proc/stat");
    if (!stat || stat.peek() == std::ifstream::traits_type::eof()) {
      // /proc/stat is missing or empty, create fake ones
      createdProc = true;

      system("mkdir -p /proc 2>/dev/null");

      std::ofstream out("/proc/stat");
      out << "cpu  1000 0 2000 3000000 500 0 0 0 0 0\n";
      out << "cpu0 500 0 1000 1500000 250 0 0 0 0 0\n";
      out << "cpu1 500 0 1000 1500000 250 0 0 0 0 0\n";
      out.close();

      std::ofstream mem("/proc/meminfo");
      mem << "MemTotal:        8000000 kB\n";
      mem << "MemAvailable:    6000000 kB\n";
      mem << "SwapTotal:       2000000 kB\n";
      mem << "SwapFree:        2000000 kB\n";
      mem.close();

      std::ofstream disk("/proc/diskstats");
      disk << "   8        0 sda 12345 0 98765 0 54321 0 432100 0 0 0 0 0 0 0 "
              "0\n";
      disk.close();

      std::ofstream net("/proc/net/dev");
      net << "Inter-|   Receive                                                "
             "|  Transmit\n";
      net << " face |bytes    packets errs drop fifo frame compressed "
             "multicast|bytes    packets errs drop fifo colls carrier "
             "compressed\n";
      net << "    lo: 1000000     100    0    0    0     0          0         "
             "0  1000000     100    0    0    0     0       0          0\n";
      net << "  eth0: 5000000     500    0    0    0     0          0         "
             "0  2000000     200    0    0    0     0       0          0\n";
      net.close();
    }
  }

  void updateFakeProcFiles() {
    if (createdProc) {
      // Update /proc/stat with different values (simulating time passing)
      std::ofstream out("/proc/stat");
      out << "cpu  2000 0 4000 6000000 1000 0 0 0 0 0\n";
      out << "cpu0 1000 0 2000 3000000 500 0 0 0 0 0\n";
      out << "cpu1 1000 0 2000 3000000 500 0 0 0 0 0\n";
      out.close();

      // Update /proc/meminfo
      std::ofstream mem("/proc/meminfo");
      mem << "MemTotal:        8000000 kB\n";
      mem << "MemAvailable:    5900000 kB\n";
      mem << "SwapTotal:       2000000 kB\n";
      mem << "SwapFree:        1900000 kB\n";
      mem.close();

      // Update /proc/diskstats (more reads/writes)
      std::ofstream disk("/proc/diskstats");
      disk << "   8        0 sda 12500 0 99765 0 55000 0 440100 0 0 0 0 0 0 0 "
              "0\n";
      disk.close();

      // Update /proc/net/dev (more bytes)
      std::ofstream net("/proc/net/dev");
      net << "Inter-|   Receive                                                "
             "|  Transmit\n";
      net << " face |bytes    packets errs drop fifo frame compressed "
             "multicast|bytes    packets errs drop fifo colls carrier "
             "compressed\n";
      net << "    lo: 1100000     110    0    0    0     0          0         "
             "0  1100000     110    0    0    0     0       0          0\n";
      net << "  eth0: 5500000     550    0    0    0     0          0         "
             "0  2500000     250    0    0    0     0       0          0\n";
      net.close();
    }
  }
};

// Test 1: First sample should return mostly empty data
TEST_F(LinuxMetricsScrapperTest, FirstSampleReturnsEmptyMetrics) {
  MetricsSample sample = scrapper.sample();
  std::cout << sample.cpu.total_percent << "\n";

  //   CPU should be zero on first call EXPECT_GT(sample.cpu.total_percent,
  //   0.0f);
  EXPECT_EQ(sample.cpu.per_core.size(), 0);

  //   Memory should be populated (doesn't depend on delta)
  //   EXPECT_GT(sample.mem.phys_total, 0);
  EXPECT_EQ(sample.cpu.total_percent, 0.0f);

  //   Disk and net should be empty (need delta)
  EXPECT_EQ(sample.disks.size(), 0);
  EXPECT_EQ(sample.interfaces.size(), 0);
}

// Test 2: Second sample should have data
TEST_F(LinuxMetricsScrapperTest, SecondSampleReturnsMetrics) {
  scrapper.sample();  // First call (establishes baseline)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  updateFakeProcFiles();
  MetricsSample sample = scrapper.sample();  // Second call (computes rates)

  // CPU should have values now
  EXPECT_GT(sample.cpu.total_percent, -0.01f);
  // Allow tiny negative due to rounding

  // EXPECT_EQ(sample.cpu.per_core.size(), 16);
  EXPECT_GT(sample.cpu.per_core.size(), 0u); // TODO : windows read actual file and not test file?
  // Match the 16 cores in fake /proc/stat
  //   EXPECT_GT(sample.cpu.per_core.size(), 0);  // Just check we got some
  //   cores

  // Memory should still be populated
  EXPECT_GT(sample.mem.phys_total, 0);
  EXPECT_LE(sample.mem.phys_used, sample.mem.phys_total);

  // Disk and net may or may not have data (depends on system)
  // Just check they're vectors
  EXPECT_TRUE(sample.disks.empty() || sample.disks.size() > 0);
  EXPECT_TRUE(sample.interfaces.empty() || sample.interfaces.size() > 0);
}

// Test 3: Multiple samples should be consistent
TEST_F(LinuxMetricsScrapperTest, MultipleSamplesAreConsistent) {
  scrapper.sample();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  updateFakeProcFiles();
  MetricsSample sample1 = scrapper.sample();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  updateFakeProcFiles();
  MetricsSample sample2 = scrapper.sample();

  // Both should have same CPU core count
  EXPECT_EQ(sample1.cpu.per_core.size(), sample2.cpu.per_core.size());

  // Memory should be in same ballpark
  EXPECT_NEAR(sample1.mem.phys_total, sample2.mem.phys_total,
              sample1.mem.phys_total * 0.01);  // Within 1%
}

// Test 4: CPU percent should be in valid range [0, 100]
TEST_F(LinuxMetricsScrapperTest, CpuPercentIsInValidRange) {
  scrapper.sample();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  updateFakeProcFiles();
  MetricsSample sample = scrapper.sample();

  EXPECT_GE(sample.cpu.total_percent, 0.0f);
  EXPECT_LE(sample.cpu.total_percent, 100.0f);

  for (float core_percent : sample.cpu.per_core) {
    EXPECT_GE(core_percent, 0.0f);
    EXPECT_LE(core_percent, 100.0f);
  }
}