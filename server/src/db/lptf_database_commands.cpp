// lptf_database_commands.cpp — ICommandRepository

#include "db/lptf_database.hpp"

int LPTF_Database::save(const std::string& agentId, const CommandPayload& cmd) {
  pqxx::work txn(conn_);
  pqxx::result r = txn.exec_params(
      "INSERT INTO command_history (agent_id, command_type, command_data) "
      "VALUES ($1, $2, $3) RETURNING db_id",
      agentId, static_cast<std::uint8_t>(cmd.type), cmd.data);
  txn.commit();
  return r[0][0].as<int>();
}

void LPTF_Database::saveResponse(int commandDbId, const std::string& agentId,
                                  const ResponsePayload& response) {
  pqxx::work txn(conn_);
  txn.exec_params(
      "INSERT INTO responses "
      "  (command_id, status, total_chunks, chunk_index, data) "
      "VALUES ($1, $2, $3, $4, $5)",
      commandDbId, static_cast<uint8_t>(response.status),
      static_cast<uint8_t>(response.total_chunks),
      static_cast<uint8_t>(response.chunk_index), response.data);
  txn.commit();
}

std::vector<CommandPayload> LPTF_Database::findByAgent(
    const std::string& agentId, int limit) {
  pqxx::work txn(conn_);
  pqxx::result rows = txn.exec_params(
      "SELECT id, command_type, command_data, sent_at FROM command_history"
      "WHERE agent_id = $1 ORDER BY created_at DESC LIMIT $2",
      agentId, limit);
  txn.commit();

  // TODO handle sent_at field (timestamp) through DashboardCommand struct
  std::vector<CommandPayload> commands;
  commands.reserve(rows.size());
  for (const auto& row : rows) {
    CommandPayload cmd;
    cmd.id = static_cast<uint32_t>(row["id"].as<long long>());
    cmd.type = static_cast<uint8_t>(row["command_type"].as<int>());
    cmd.data = row["command_data"].as<std::string>();
    commands.push_back(std::move(cmd));
  }
  return commands;
}
