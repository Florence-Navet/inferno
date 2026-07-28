// lptf_database_agents.cpp — IAgentRepository

#include "db/lptf_database.hpp"

namespace {

RegisterPayload rowToRegisterPayload(const pqxx::row& row) {
  RegisterPayload agent;
  agent.id = row["id"].as<std::string>();
  agent.registered_at = row["registered_at"].as<std::string>();
  agent.last_seen = row["last_seen"].as<std::string>();
  agent.system.os_type = static_cast<OSType>(row["os_type"].as<int>());
  agent.system.arch = static_cast<ArchType>(row["architecture"].as<int>());
  agent.system.hostname = row["hostname"].as<std::string>();
  agent.system.os_version = row["os_version"].as<std::string>();
  agent.system.current_user = row["current_username"].as<std::string>();
  agent.system.ip = row["ip_address"].as<std::string>();
  agent.system.mac = agent.id;  // MAC address is the agent identity == id
  return agent;
}

}  // namespace

void LPTF_Database::save(const RegisterPayload& agent) {
  pqxx::work txn(conn_);
  txn.exec_params(
      "INSERT INTO agents "
      "  (id, hostname, os_type, architecture, os_version, current_username, "
      "ip_address, mac_address) "
      "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) ",
      agent.id, agent.system.hostname, static_cast<int>(agent.system.os_type),
      static_cast<int>(agent.system.arch), agent.system.os_version,
      agent.system.current_user, agent.system.ip, agent.system.mac);
  txn.commit();
}

// void LPTF_Database::setLastSeen(const std::string& id, bool online) {
//   pqxx::work txn(conn_);
//   txn.exec_params(
//       "UPDATE agents SET online = $2, last_seen = now() WHERE id = $1", id,
//       online);
//   txn.commit();
// }

std::vector<RegisterPayload> LPTF_Database::findAll() {
  pqxx::work txn(conn_);
  pqxx::result rows = txn.exec(
      "SELECT id, registered_at, last_seen, hostname, os_type, arch, "
      "os_version, current_username, ip "
      "FROM agents ORDER BY hostname");
  txn.commit();

  std::vector<RegisterPayload> agents;
  agents.reserve(rows.size());
  for (const auto& row : rows) {
    agents.push_back(rowToRegisterPayload(row));
  }
  return agents;
}

std::optional<RegisterPayload> LPTF_Database::findById(
    const std::string& id) {
  pqxx::work txn(conn_);
  pqxx::result rows = txn.exec_params(
      "SELECT registered_at, last_seen, hostname, os_type, arch, "
      "os_version, current_username, ip "
      "FROM agents WHERE id = $1",
      id);
  txn.commit();

  if (rows.empty()) {
    return std::nullopt;
  }
  return rowToRegisterPayload(rows[0]);
}
