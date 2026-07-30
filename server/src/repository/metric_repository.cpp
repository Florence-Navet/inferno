// #include "repository/metrics_repository.hpp"

// void MetricsRepository::save(const std::string& agent_id,
//                              const MetricsSample& sample,
//                              const std::string& timestamp_iso) {
//   // Base metrics (CPU + Memory)
//   pqxx::params params;
//   params.append(timestamp_iso);
//   params.append(agent_id);
//   params.append(sample.cpu.total_percent);
//   params.append(static_cast<int>(sample.cpu.per_core.size()));
//   // pqxx array syntax for per_core
//   params.append(sample.mem.phys_total);
//   params.append(sample.mem.phys_used);
//   params.append(sample.mem.phys_available);
//   params.append(sample.mem.swap_total);
//   params.append(sample.mem.swap_used);

//   db_.executeParams(
//       "INSERT INTO metrics "
//       "  (time, agent_id, cpu_total_percent, cpu_core_count, "
//       "cpu_per_core, mem_phys_total, mem_phys_used, mem_phys_available, "
//       "mem_swap_total, mem_swap_used) "
//       "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)",
//       params);

//   // Disk and Network via private helpers
//   saveDiskMetrics(agent_id, timestamp_iso, sample.disks);
//   saveNetMetrics(agent_id, timestamp_iso, sample.interfaces);
// }

// void MetricsRepository::saveDiskMetrics(
//     const std::string& agent_id, const std::string& time_iso,
//     const std::vector<DiskSample>& disks) {
//   for (const auto& disk : disks) {
//     pqxx::params params;
//     params.append(time_iso);
//     params.append(agent_id);
//     params.append(disk.device);
//     params.append(disk.read_bytes_per_sec);
//     params.append(disk.write_bytes_per_sec);

//     db_.executeParams(
//         "INSERT INTO metrics_disk "
//         "  (time, agent_id, device, read_bytes_per_sec, "
//         "write_bytes_per_sec) "
//         "VALUES ($1, $2, $3, $4, $5)",
//         params);
//   }
// }

// void MetricsRepository::saveNetMetrics(
//     const std::string& agent_id, const std::string& time_iso,
//     const std::vector<NetSample>& interfaces) {
//   for (const auto& iface : interfaces) {
//     pqxx::params params;
//     params.append(time_iso);
//     params.append(agent_id);
//     params.append(iface.iface);
//     params.append(iface.rx_bytes_per_sec);
//     params.append(iface.tx_bytes_per_sec);

//     db_.executeParams(
//         "INSERT INTO metrics_net "
//         "  (time, agent_id, iface, rx_bytes_per_sec, tx_bytes_per_sec) "
//         "VALUES ($1, $2, $3, $4, $5)",
//         params);
//   }
// }

// std::vector<std::tuple<std::string, CpuSample, MemSample>>
// MetricsRepository::findBaseMetrics(const std::string& agent_id,
//                                    const std::string& since_iso,
//                                    const std::string& until_iso) {
//   pqxx::params params;
//   params.append(agent_id);
//   params.append(since_iso);
//   params.append(until_iso);

//   pqxx::result rows = db_.executeParams(
//       "SELECT time, cpu_total_percent, cpu_core_count, cpu_per_core, "
//       "       mem_phys_total, mem_phys_used, mem_phys_available, "
//       "       mem_swap_total, mem_swap_used "
//       "FROM metrics "
//       "WHERE agent_id = $1 AND time >= $2 AND time <= $3 "
//       "ORDER BY time DESC",
//       params);

//   std::vector<std::tuple<std::string, CpuSample, MemSample>> results;
//   results.reserve(rows.size());

//   for (const auto& row : rows) {
//     std::string time = row["time"].as<std::string>();
    
//     CpuSample cpu;
//     cpu.total_percent = row["cpu_total_percent"].as<float>();
//     // Parse cpu_per_core array from PostgreSQL
//     // (pqxx handles this, but depends on your pqxx version)

//     MemSample mem;
//     mem.phys_total = row["mem_phys_total"].as<std::uint64_t>();
//     mem.phys_used = row["mem_phys_used"].as<std::uint64_t>();
//     mem.phys_available = row["mem_phys_available"].as<std::uint64_t>();
//     mem.swap_total = row["mem_swap_total"].as<std::uint64_t>();
//     mem.swap_used = row["mem_swap_used"].as<std::uint64_t>();

//     results.emplace_back(time, cpu, mem);
//   }

//   return results;
// }

// std::vector<std::tuple<std::string, DiskSample>>
// MetricsRepository::findDiskMetrics(const std::string& agent_id,
//                                    const std::string& since_iso,
//                                    const std::string& until_iso) {
//   pqxx::params params;
//   params.append(agent_id);
//   params.append(since_iso);
//   params.append(until_iso);

//   pqxx::result rows = db_.executeParams(
//       "SELECT time, device, read_bytes_per_sec, write_bytes_per_sec "
//       "FROM metrics_disk "
//       "WHERE agent_id = $1 AND time >= $2 AND time <= $3 "
//       "ORDER BY time DESC, device",
//       params);

//   std::vector<std::tuple<std::string, DiskSample>> results;
//   results.reserve(rows.size());

//   for (const auto& row : rows) {
//     std::string time = row["time"].as<std::string>();
//     DiskSample disk;
//     disk.device = row["device"].as<std::string>();
//     disk.read_bytes_per_sec = row["read_bytes_per_sec"].as<float>();
//     disk.write_bytes_per_sec = row["write_bytes_per_sec"].as<float>();

//     results.emplace_back(time, disk);
//   }

//   return results;
// }

// std::vector<std::tuple<std::string, NetSample>>
// MetricsRepository::findNetMetrics(const std::string& agent_id,
//                                   const std::string& since_iso,
//                                   const std::string& until_iso) {
//   pqxx::params params;
//   params.append(agent_id);
//   params.append(since_iso);
//   params.append(until_iso);

//   pqxx::result rows = db_.executeParams(
//       "SELECT time, iface, rx_bytes_per_sec, tx_bytes_per_sec "
//       "FROM metrics_net "
//       "WHERE agent_id = $1 AND time >= $2 AND time <= $3 "
//       "ORDER BY time DESC, iface",
//       params);

//   std::vector<std::tuple<std::string, NetSample>> results;
//   results.reserve(rows.size());

//   for (const auto& row : rows) {
//     std::string time = row["time"].as<std::string>();
//     NetSample net;
//     net.iface = row["iface"].as<std::string>();
//     net.rx_bytes_per_sec = row["rx_bytes_per_sec"].as<float>();
//     net.tx_bytes_per_sec = row["tx_bytes_per_sec"].as<float>();

//     results.emplace_back(time, net);
//   }

//   return results;
// }

// std::optional<MetricsSample> MetricsRepository::getLatest(
//     const std::string& agent_id) {
//   pqxx::params params;
//   params.append(agent_id);

//   pqxx::result row = db_.executeParams(
//       "SELECT time, cpu_total_percent, cpu_core_count, cpu_per_core, "
//       "       mem_phys_total, mem_phys_used, mem_phys_available, "
//       "       mem_swap_total, mem_swap_used "
//       "FROM metrics "
//       "WHERE agent_id = $1 "
//       "ORDER BY time DESC LIMIT 1",
//       params);

//   if (row.empty()) {
//     return std::nullopt;
//   }

//   // Reconstruct MetricsSample from row + joined disk/net data
//   // (Implementation omitted for brevity, but same pattern)
  
//   return std::nullopt;  // placeholder
// }