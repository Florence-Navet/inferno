// #ifndef METRIC_REPOSITORY_HPP
// #define METRIC_REPOSITORY_HPP

// #include <optional>
// #include <string>
// #include <vector>

// #include "protocol/lptf_protocol.hpp"
// #include "repository/database_connection.hpp"

// class IMetricsRepository {
//  public:
//   virtual ~IMetricsRepository() = default;

//   virtual void save(const std::string& agent_id, 
//                     const MetricsSample& sample) = 0;

//   virtual std::vector<MetricsSample> 
//   findByAgent(const std::string& agent_id, 
//               const std::string& since_iso,
//               const std::string& until_iso) = 0;

//   virtual std::optional<MetricsSample> 
//   getLatest(const std::string& agent_id) = 0;
// };

// // agent_repository.hpp
// class MetricRepository : public IMetricRepository {
//  private:
//   IDatabaseConnection& db_;  // shared reference
//                              // Private helpers for N:1 relationships
//   void saveDiskMetrics(const std::string& agent_id, const std::string& time_iso,
//                        const std::vector<DiskSample>& disks);

//   void saveNetMetrics(const std::string& agent_id, const std::string& time_iso,
//                       const std::vector<NetSample>& interfaces);

//   MetricsSample rowToMetricsSample(const pqxx::row& row);

//  public:
//   explicit MetricRepository(IDatabaseConnection& db) : db_(db) {}
//   void save(const std::string& agent_id, 
//                     const MetricsSample& sample);

//   std::vector<MetricsSample> 
//   findByAgent(const std::string& agent_id, 
//               const std::string& since_iso,
//               const std::string& until_iso);

//   std::optional<MetricsSample> 
//   getLatest(const std::string& agent_id);

//   // etc.
// };

// #endif