// // lptf_database_metrics.cpp — IMetricsRepository
// //
// // CpuSample::per_core and the variable-length DiskSample/NetSample lists
// // are stored as JSONB rather than normalized tables, since a sample is
// // always written and read as one atomic unit (never queried per-disk).
// // Requires nlohmann::json (header-only; vendor it under third_party/ or
// // pull it in via CMake FetchContent/find_package(nlohmann_json)).

// #include "db/lptf_database.hpp"

// #include <algorithm>

// #include <nlohmann/json.hpp>

// using json = nlohmann::json;

// namespace {

// json diskToJson(const DiskSample& d) {
//   return json{{"device", d.device},
//               {"read_bytes_per_sec", d.read_bytes_per_sec},
//               {"write_bytes_per_sec", d.write_bytes_per_sec}};
// }

// json netToJson(const NetSample& n) {
//   return json{{"iface", n.iface},
//               {"rx_bytes_per_sec", n.rx_bytes_per_sec},
//               {"tx_bytes_per_sec", n.tx_bytes_per_sec}};
// }

// DiskSample jsonToDisk(const json& j) {
//   DiskSample d;
//   d.device = j.at("device").get<std::string>();
//   d.read_bytes_per_sec = j.at("read_bytes_per_sec").get<float>();
//   d.write_bytes_per_sec = j.at("write_bytes_per_sec").get<float>();
//   return d;
// }

// NetSample jsonToNet(const json& j) {
//   NetSample n;
//   n.iface = j.at("iface").get<std::string>();
//   n.rx_bytes_per_sec = j.at("rx_bytes_per_sec").get<float>();
//   n.tx_bytes_per_sec = j.at("tx_bytes_per_sec").get<float>();
//   return n;
// }

// }  // namespace

// void LPTF_Database::save(const std::string& agentId,
//                           const MetricsSample& sample) {
//   json perCore = sample.cpu.per_core;  // vector<float> -> JSON array

//   json disks = json::array();
//   for (const auto& d : sample.disks) disks.push_back(diskToJson(d));

//   json interfaces = json::array();
//   for (const auto& n : sample.interfaces) interfaces.push_back(netToJson(n));

//   pqxx::work txn(conn_);
//   txn.exec_params(
//       "INSERT INTO metrics_samples "
//       "  (agent_id, cpu_total_percent, cpu_core_number, cpu_per_core, "
//       "   mem_phys_total, mem_phys_used, mem_phys_available, "
//       "   mem_swap_total, mem_swap_used, disks, interfaces) "
//       "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)",
//       agentId, sample.cpu.total_percent,
//       static_cast<int>(sample.cpu.core_number), perCore.dump(),
//       static_cast<long long>(sample.mem.phys_total),
//       static_cast<long long>(sample.mem.phys_used),
//       static_cast<long long>(sample.mem.phys_available),
//       static_cast<long long>(sample.mem.swap_total),
//       static_cast<long long>(sample.mem.swap_used), disks.dump(),
//       interfaces.dump());
//   txn.commit();
// }

// std::vector<MetricsSample> LPTF_Database::findLatest(
//     const std::string& agentId, int limit) {
//   pqxx::work txn(conn_);
//   pqxx::result rows = txn.exec_params(
//       "SELECT cpu_total_percent, cpu_core_number, cpu_per_core, "
//       "       mem_phys_total, mem_phys_used, mem_phys_available, "
//       "       mem_swap_total, mem_swap_used, disks, interfaces "
//       "FROM metrics_samples WHERE agent_id = $1 "
//       "ORDER BY ts DESC LIMIT $2",
//       agentId, limit);
//   txn.commit();

//   std::vector<MetricsSample> samples;
//   samples.reserve(rows.size());
//   for (const auto& row : rows) {
//     MetricsSample sample;
//     sample.cpu.total_percent = row["cpu_total_percent"].as<float>();
//     sample.cpu.core_number =
//         static_cast<uint8_t>(row["cpu_core_number"].as<int>());
//     sample.cpu.per_core =
//         json::parse(row["cpu_per_core"].as<std::string>())
//             .get<std::vector<float>>();

//     sample.mem.phys_total =
//         static_cast<uint64_t>(row["mem_phys_total"].as<long long>());
//     sample.mem.phys_used =
//         static_cast<uint64_t>(row["mem_phys_used"].as<long long>());
//     sample.mem.phys_available =
//         static_cast<uint64_t>(row["mem_phys_available"].as<long long>());
//     sample.mem.swap_total =
//         static_cast<uint64_t>(row["mem_swap_total"].as<long long>());
//     sample.mem.swap_used =
//         static_cast<uint64_t>(row["mem_swap_used"].as<long long>());

//     for (const auto& dj : json::parse(row["disks"].as<std::string>())) {
//       sample.disks.push_back(jsonToDisk(dj));
//     }
//     for (const auto& nj : json::parse(row["interfaces"].as<std::string>())) {
//       sample.interfaces.push_back(jsonToNet(nj));
//     }
//     sample.disk_count = static_cast<uint8_t>(sample.disks.size());
//     sample.interface_count = static_cast<uint8_t>(sample.interfaces.size());

//     samples.push_back(std::move(sample));
//   }

//   // Query is DESC (newest first) so LIMIT grabs the most recent N; reverse
//   // to chronological order, which is what a graph/table wants to render.
//   std::reverse(samples.begin(), samples.end());
//   return samples;
// }
