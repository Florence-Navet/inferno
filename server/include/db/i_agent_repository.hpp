#ifndef I_AGENT_REPOSITORY_HPP
#define I_AGENT_REPOSITORY_HPP

#include <optional>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class IAgentRepository {
 public:
  virtual ~IAgentRepository() = default;

  // Upsert — creates agent on first REGISTER, updates fields on reconnect.
  virtual void save(const RegisterPayload& agent) = 0;

  // Flipped by the Reactor on connect / disconnect.
  // virtual void setLastSeen(const std::string& id, bool online) = 0;

  // All agents — sent to the dashboard as DataType::AGENTS on connection.
  virtual std::vector<RegisterPayload> findAll() = 0;

  virtual std::optional<RegisterPayload> findById(const std::string& id) = 0;
};

#endif