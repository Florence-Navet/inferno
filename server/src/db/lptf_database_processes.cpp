// // lptf_database_processes.cpp — IProcessRepository

// #include "db/lptf_database.hpp"

// void LPTF_Database::save(const std::string& agentId,
//                           const std::vector<ProcessInfo>& processes) {
//   pqxx::work txn(conn_);

//   pqxx::result snap = txn.exec_params(
//       "INSERT INTO process_snapshots (agent_id) VALUES ($1) RETURNING db_id",
//       agentId);
//   int snapshotId = snap[0][0].as<int>();

//   // One INSERT per process is simplest and keeps the whole snapshot in a
//   // single transaction; if process lists get large (thousands of rows),
//   // switch this to a multi-row INSERT or pqxx::stream_to for throughput.
//   for (const auto& p : processes) {
//     txn.exec_params(
//         "INSERT INTO process_entries "
//         "  (snapshot_id, pid, cpu_percent, mem_bytes, name) "
//         "VALUES ($1, $2, $3, $4, $5)",
//         snapshotId, static_cast<long long>(p.pid), p.cpu_percent,
//         static_cast<long long>(p.mem_bytes), p.name);
//   }

//   txn.commit();
// }

// std::vector<ProcessInfo> LPTF_Database::findLatest(
//     const std::string& agentId) {
//   pqxx::work txn(conn_);

//   pqxx::result snap = txn.exec_params(
//       "SELECT db_id FROM process_snapshots "
//       "WHERE agent_id = $1 ORDER BY ts DESC LIMIT 1",
//       agentId);

//   if (snap.empty()) {
//     txn.commit();
//     return {};
//   }
//   int snapshotId = snap[0][0].as<int>();

//   pqxx::result rows = txn.exec_params(
//       "SELECT pid, cpu_percent, mem_bytes, name FROM process_entries "
//       "WHERE snapshot_id = $1 ORDER BY mem_bytes DESC",
//       snapshotId);
//   txn.commit();

//   std::vector<ProcessInfo> processes;
//   processes.reserve(rows.size());
//   for (const auto& row : rows) {
//     ProcessInfo p;
//     p.pid = static_cast<uint32_t>(row["pid"].as<long long>());
//     p.cpu_percent = row["cpu_percent"].as<float>();
//     p.mem_bytes = static_cast<uint64_t>(row["mem_bytes"].as<long long>());
//     p.name = row["name"].as<std::string>();
//     processes.push_back(std::move(p));
//   }
//   return processes;
// }
