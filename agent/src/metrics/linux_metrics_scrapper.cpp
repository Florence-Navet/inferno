#include "metrics/linux_metrics_scrapper.hpp"

#include <fstream>
#include <sstream>

std::ifstream LinuxMetricsScrapper::openFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open" + path);
  }
  return file;
}

MemSample LinuxMetricsScrapper::readMem() {
  std::ifstream file = openFile("/proc/meminfo");
  MemSample mem{};

  std::string line;
  std::string key;
  std::uint64_t value;

  std::uint64_t swapFree{0};
  while (std::getline(file, line)) {
    std::istringstream lineStream(line);
    lineStream >> key >> value;

    // all value are in kiloBytes, * 1024 convert them back to bytes
    if (key == "MemTotal:") {
      mem.phys_total = value * 1024;
    } else if (key == "MemAvailable:") {
      mem.phys_available = value * 1024;
    } else if (key == "SwapTotal:") {
      mem.swap_total = value * 1024;
    } else if (key == "SwapFree:") {
      swapFree = value * 1024;
    }
  }

  mem.phys_used = mem.phys_total - mem.phys_available;
  mem.swap_used = mem.swap_total - swapFree;

  return mem;
}

CpuSample LinuxMetricsScrapper::readCpu() {
  CpuSample cpu{};
  RawCpuSnapshot snapshot = getRawCpuSnapshot();

  if (firstSample_) {
    previousCpu_ = snapshot;
    firstSample_ = false;
    return cpu;  // first snapshot returns 0
  }
  cpu.total_percent = computeCpuPercent(snapshot, 0);

  for (std::size_t i{1}; i < snapshot.total.size(); ++i) {
    cpu.per_core.push_back(computeCpuPercent(snapshot, i));
  }
  previousCpu_ = snapshot;

  return cpu;
}

float LinuxMetricsScrapper::computeCpuPercent(const RawCpuSnapshot& snapshot,
                                              const std::size_t& index) {
  std::uint64_t deltTotal = snapshot.total[index] - previousCpu_.total[index];
  std::uint64_t deltaIdle = snapshot.idle[index] - previousCpu_.idle[index];
  if (deltTotal == 0) {
    return 0.0f;
  }
  return (1.0f -
          static_cast<float>(deltaIdle) / static_cast<float>(deltTotal)) *
         100.0f;
}

RawCpuSnapshot LinuxMetricsScrapper::getRawCpuSnapshot() {
  std::ifstream file = openFile("/proc/stat");
  RawCpuSnapshot snapshot;
  std::string line;
  std::string key;
  std::uint64_t user;
  std::uint64_t nice;
  std::uint64_t system;
  std::uint64_t idle;
  std::uint64_t iowait;
  std::uint64_t irq;
  std::uint64_t softirq;
  std::uint64_t steal;
  std::uint64_t guest;
  std::uint64_t guest_nice;

  while (std::getline(file, line)) {
    if (line.substr(0, 3) == "cpu") {
      std::istringstream lineStream(line);
      lineStream >> key >> user >> nice >> system >> idle >> iowait >> irq >>
          softirq >> steal >> guest >> guest_nice;

      std::uint64_t total{user + nice + system + idle + iowait + irq + softirq +
                          steal + guest + guest_nice};

      snapshot.total.push_back(total);
      snapshot.idle.push_back(idle);
    }
  }

  return snapshot;
}