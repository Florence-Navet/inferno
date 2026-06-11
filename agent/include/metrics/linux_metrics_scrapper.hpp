#ifndef LINUX_METRICS_SCRAPPER_HPP
#define LINUX_METRICS_SCRAPPER_HPP

#include <map>

#include "metrics/i_metrics_scrapper.hpp"

class LinuxMetricsScrapper : public IMetricsScrapper {
 public:
  LinuxMetricsScrapper();
  MetricsSample sample() override;

 private:
  // delta state
  RawCpuSnapshot previousCpu_;
  std::map<std::string, RawDiskSnapshot> previousDisks_;
  std::map<std::string, RawNetSnapshot> previousNet_;
  bool firstSample_;

  // per-subsystem private readers
  CpuSample readCpu();
  MemSample readMem();
  std::vector<DiskSample> readDisks();
  std::vector<NetSample> readNet();
  std::ifstream openFile(const std::string& path);
  float computeCpuPercent(const RawCpuSnapshot& snapshot, const std::size_t& index);
  RawCpuSnapshot getRawCpuSnapshot();
};

#endif