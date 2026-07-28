#ifndef I_COMMAND_REPOSITORY_HPP
#define I_COMMAND_REPOSITORY_HPP

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class ICommandRepository {
 public:
  virtual ~ICommandRepository() = default;

  // Persists the command the server just sent.
  // Returns the DB row id — kept by ServerDispatcher to link the response.
  virtual int save(const std::string& agentId, const CommandPayload& cmd) = 0;

  // Persists the response received from the agent.
  // commandDbId is the value returned by save() above.
  virtual void saveResponse(int commandDbId, const std::string& agentId,
                            const ResponsePayload& response) = 0;

  // Latest N commands for one agent (dashboard history panel).
  virtual std::vector<CommandPayload> findByAgent(const std::string& agentId,
                                                  int limit = 50) = 0;
};

#endif