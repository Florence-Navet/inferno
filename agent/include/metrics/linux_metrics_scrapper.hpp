#ifndef LINUX_METRICS_SCRAPPER_HPP
#define LINUX_METRICS_SCRAPPER_HPP

#include "metrics/i_metrics_scrapper.hpp"
#include <map>

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
};

#endif