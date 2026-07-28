// lptf_database.cpp
//
// Constructor + applyMigrations() only. Each repository interface's methods
// live in their own translation unit:
//   lptf_database_agents.cpp    -> IAgentRepository
//   lptf_database_commands.cpp  -> ICommandRepository
//   lptf_database_metrics.cpp   -> IMetricsRepository
//   lptf_database_processes.cpp -> IProcessRepository
//
// NOTE on assumptions: the fields below (RegisterPayload::id / ::system,
// CpuSample::per_core as std::vector<float>, ResponsePayload::data /
// CommandPayload::data as UTF-8 text) are inferred from the struct
// definitions documented in lptf_binary_protocol.md, since
// protocol/lptf_protocol.hpp itself wasn't available. If the real header
// names things differently, the fixes are purely mechanical field-name
// substitutions in these five files.

#include "db/lptf_database.hpp"

#include <stdexcept>

LPTF_Database::LPTF_Database(const std::string& connectionString)
    : conn_(connectionString) {
  if (!conn_.is_open()) {
    throw std::runtime_error(
        "LPTF_Database: failed to open connection to PostgreSQL");
  }
}

// void LPTF_Database::applyMigrations() {
//   // Mirrors db/schema.sql. Kept inline (rather than read from disk) so the
//   // binary has no runtime dependency on the source tree's file layout.
//   static const char* kSchema = R"SQL(
// CREATE TABLE IF NOT EXISTS agents (
//     id              TEXT PRIMARY KEY,
//     hostname        TEXT NOT NULL,
//     os_type         SMALLINT NOT NULL,
//     arch            SMALLINT NOT NULL,
//     os_version      TEXT NOT NULL,
//     "current_user"  TEXT NOT NULL,
//     ip              TEXT NOT NULL,
//     online          BOOLEAN NOT NULL DEFAULT FALSE,
//     first_seen      TIMESTAMPTZ NOT NULL DEFAULT now(),
//     last_seen       TIMESTAMPTZ NOT NULL DEFAULT now()
// );

// CREATE TABLE IF NOT EXISTS commands (
//     db_id       SERIAL PRIMARY KEY,
//     agent_id    TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
//     proto_id    BIGINT NOT NULL,
//     cmd_type    SMALLINT NOT NULL,
//     data        TEXT NOT NULL DEFAULT '',
//     created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
// );
// CREATE INDEX IF NOT EXISTS idx_commands_agent_created
//     ON commands (agent_id, created_at DESC);

// CREATE TABLE IF NOT EXISTS responses (
//     db_id           SERIAL PRIMARY KEY,
//     command_db_id   INT NOT NULL REFERENCES commands(db_id) ON DELETE CASCADE,
//     agent_id        TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
//     status          SMALLINT NOT NULL,
//     total_chunks    SMALLINT NOT NULL,
//     chunk_index     SMALLINT NOT NULL,
//     data            TEXT NOT NULL DEFAULT '',
//     created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
// );
// CREATE INDEX IF NOT EXISTS idx_responses_command
//     ON responses (command_db_id, chunk_index);

// CREATE TABLE IF NOT EXISTS metrics_samples (
//     db_id               SERIAL PRIMARY KEY,
//     agent_id            TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
//     ts                  TIMESTAMPTZ NOT NULL DEFAULT now(),
//     cpu_total_percent   REAL NOT NULL,
//     cpu_core_number     SMALLINT NOT NULL,
//     cpu_per_core        JSONB NOT NULL,
//     mem_phys_total      BIGINT NOT NULL,
//     mem_phys_used       BIGINT NOT NULL,
//     mem_phys_available  BIGINT NOT NULL,
//     mem_swap_total      BIGINT NOT NULL,
//     mem_swap_used       BIGINT NOT NULL,
//     disks               JSONB NOT NULL,
//     interfaces          JSONB NOT NULL
// );
// CREATE INDEX IF NOT EXISTS idx_metrics_agent_ts
//     ON metrics_samples (agent_id, ts DESC);

// CREATE TABLE IF NOT EXISTS process_snapshots (
//     db_id       SERIAL PRIMARY KEY,
//     agent_id    TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
//     ts          TIMESTAMPTZ NOT NULL DEFAULT now()
// );
// CREATE INDEX IF NOT EXISTS idx_process_snapshots_agent_ts
//     ON process_snapshots (agent_id, ts DESC);

// CREATE TABLE IF NOT EXISTS process_entries (
//     db_id           SERIAL PRIMARY KEY,
//     snapshot_id     INT NOT NULL REFERENCES process_snapshots(db_id) ON DELETE CASCADE,
//     pid             BIGINT NOT NULL,
//     cpu_percent     REAL NOT NULL,
//     mem_bytes       BIGINT NOT NULL,
//     name            TEXT NOT NULL
// );
// CREATE INDEX IF NOT EXISTS idx_process_entries_snapshot
//     ON process_entries (snapshot_id);
// )SQL";

//   pqxx::work txn(conn_);
//   txn.exec(kSchema);
//   txn.commit();
// }
