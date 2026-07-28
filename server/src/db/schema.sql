-- ============================================================================
-- Inferno / LPTF_Database schema
-- Applied idempotently by LPTF_Database::applyMigrations() at server startup.
-- All statements use IF NOT EXISTS so re-running is always safe.
-- ============================================================================

-- ── Agents ───────────────────────────────────────────────────────────────
-- id is the agent's MAC address (persistent identity across reconnections).
-- "current_user" is quoted because it collides with the reserved
-- CURRENT_USER SQL keyword/function.
CREATE TABLE IF NOT EXISTS agents (
    id              TEXT PRIMARY KEY,
    hostname        TEXT NOT NULL,
    os_type         SMALLINT NOT NULL,
    arch            SMALLINT NOT NULL,
    os_version      TEXT NOT NULL,
    "current_user"  TEXT NOT NULL,
    ip              TEXT NOT NULL,
    online          BOOLEAN NOT NULL DEFAULT FALSE,
    first_seen      TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen       TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- ── Commands / Responses ────────────────────────────────────────────────
-- proto_id is the protocol-level uint32 CommandPayload::id (server-assigned,
-- unique per session only — NOT unique across restarts, hence its own
-- surrogate db_id primary key for FK linking).
CREATE TABLE IF NOT EXISTS commands (
    db_id       SERIAL PRIMARY KEY,
    agent_id    TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    proto_id    BIGINT NOT NULL,
    cmd_type    SMALLINT NOT NULL,
    data        TEXT NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_commands_agent_created
    ON commands (agent_id, created_at DESC);

CREATE TABLE IF NOT EXISTS responses (
    db_id           SERIAL PRIMARY KEY,
    command_db_id   INT NOT NULL REFERENCES commands(db_id) ON DELETE CASCADE,
    agent_id        TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    status          SMALLINT NOT NULL,
    total_chunks    SMALLINT NOT NULL,
    chunk_index     SMALLINT NOT NULL,
    data            TEXT NOT NULL DEFAULT '',
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_responses_command
    ON responses (command_db_id, chunk_index);

-- ── Metrics ──────────────────────────────────────────────────────────────
-- Per-core CPU array and the variable-length disk/interface lists are
-- stored as JSONB rather than normalized into their own tables — samples
-- arrive at high frequency and are always read/written as a whole unit.
CREATE TABLE IF NOT EXISTS metrics_samples (
    db_id               SERIAL PRIMARY KEY,
    agent_id            TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    ts                  TIMESTAMPTZ NOT NULL DEFAULT now(),
    cpu_total_percent   REAL NOT NULL,
    cpu_core_number     SMALLINT NOT NULL,
    cpu_per_core        JSONB NOT NULL,       -- [float, float, ...]
    mem_phys_total      BIGINT NOT NULL,
    mem_phys_used       BIGINT NOT NULL,
    mem_phys_available  BIGINT NOT NULL,
    mem_swap_total      BIGINT NOT NULL,
    mem_swap_used       BIGINT NOT NULL,
    disks               JSONB NOT NULL,       -- [{device, read_bytes_per_sec, write_bytes_per_sec}, ...]
    interfaces          JSONB NOT NULL        -- [{iface, rx_bytes_per_sec, tx_bytes_per_sec}, ...]
);
CREATE INDEX IF NOT EXISTS idx_metrics_agent_ts
    ON metrics_samples (agent_id, ts DESC);

-- ── Process snapshots ────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS process_snapshots (
    db_id       SERIAL PRIMARY KEY,
    agent_id    TEXT NOT NULL REFERENCES agents(id) ON DELETE CASCADE,
    ts          TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_process_snapshots_agent_ts
    ON process_snapshots (agent_id, ts DESC);

CREATE TABLE IF NOT EXISTS process_entries (
    db_id           SERIAL PRIMARY KEY,
    snapshot_id     INT NOT NULL REFERENCES process_snapshots(db_id) ON DELETE CASCADE,
    pid             BIGINT NOT NULL,
    cpu_percent     REAL NOT NULL,
    mem_bytes       BIGINT NOT NULL,
    name            TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_process_entries_snapshot
    ON process_entries (snapshot_id);
