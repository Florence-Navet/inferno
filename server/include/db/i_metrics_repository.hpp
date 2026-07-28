#ifndef I_METRICS_REPOSITORY_HPP
#define I_METRICS_REPOSITORY_HPP

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class IMetricsRepository {
 public:
  virtual ~IMetricsRepository() = default;

  // Persists one full MetricsSample (cpu + mem + disks + interfaces).
  // Called by ServerDispatcher::onData() when subtype == METRICS_SAMPLE.
  virtual void save(const std::string& agentId,
                    const MetricsSample& sample) = 0;

  // Latest N samples for one agent — used by the dashboard graph.
  virtual std::vector<MetricsSample> findLatest(const std::string& agentId,
                                                int limit = 60) = 0;
};

#endif