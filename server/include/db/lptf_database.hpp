#ifndef LPTF_DATABASE_HPP
#define LPTF_DATABASE_HPP


#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "db/i_agent_repository.hpp"
#include "db/i_command_repository.hpp"
// #include "db/i_metrics_repository.hpp"
// #include "db/i_process_repository.hpp"

// LPTF_Database is the single point of contact with PostgreSQL.
// The class declaration lives here; each interface's methods are
// defined in their own .cpp file (lptf_database_agents.cpp, etc.)
// so no single translation unit grows unwieldy.
//
// Usage in main.cpp:
//   LPTF_Database db("host=localhost dbname=inferno user=inferno
//   password=..."); db.applyMigrations(); ServerDispatcher dispatcher(db, db,
//   db, db);
class LPTF_Database : public IAgentRepository,
                      public ICommandRepository
                      {
 public:
  explicit LPTF_Database(const std::string& connectionString);

  // Runs schema.sql idempotently at server startup.
  // Safe to call every time — all statements use CREATE TABLE IF NOT EXISTS.
  // void applyMigrations();

  // ── IAgentRepository ──────────────────────────────────────
  void save(const RegisterPayload& agent) override;
  // void setLastSeen(const std::string& id, bool online) override;
  std::vector<RegisterPayload> findAll() override;
  std::optional<RegisterPayload> findById(const std::string& id) override;

  // ── ICommandRepository ────────────────────────────────────
  int save(const std::string& agentId, const CommandPayload& cmd) override;
  void saveResponse(int commandDbId, const std::string& agentId,
                    const ResponsePayload& response) override;

  std::vector<CommandPayload> findByAgent(const std::string& agentId,
                                          int limit = 50) override;
  // std::vector<ResponsePayload> findByAgent(const std::string& agentId,
  //                                          int limit = 50) override;

  // ── IMetricsRepository ────────────────────────────────────
  // void save(const std::string& agentId, const MetricsSample& sample) override;
  // std::vector<MetricsSample> findLatest(const std::string& agentId,
  //                                       int limit = 60) override;

  // ── IProcessRepository ────────────────────────────────────
  // void save(const std::string& agentId,
  //           const std::vector<ProcessInfo>& processes) override;
  // std::vector<ProcessInfo> findLatest(const std::string& agentId) override;

 private:
  pqxx::connection conn_;
};

#endif  // LPTF_DATABASE_HPP