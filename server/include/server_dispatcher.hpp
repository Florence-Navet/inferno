#ifndef SERVER_DISPATCHER_HPP
#define SERVER_DISPATCHER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "agent_connection.hpp"
#include "dispatcher/dispatcher.hpp"
#include "protocol/lptf_protocol.hpp"
#include "repository_manager.hpp"
#include "session_manager.hpp"

class ServerDispatcher : public Dispatcher {
 public:
  explicit ServerDispatcher(SessionManager& sessionManager,
                            RepositoryManager& repositoryManager)
      : sessionManager_(sessionManager),
        repositoryManager_(repositoryManager) {};
  ServerDispatcher(const ServerDispatcher&) = delete;
  ~ServerDispatcher() = default;
  ServerDispatcher& operator=(const ServerDispatcher&) = delete;

  void handleFrame(FrameTransport& agent, const Frame& frame) override;
  void sendCommand(AgentConnection& agent, CommandType type,
                   const std::string& data = "");
  SessionManager& getSessionManager() { return sessionManager_; }

 private:
  SessionManager& sessionManager_;
  RepositoryManager& repositoryManager_;
  // ── Incoming message handlers ───────────────────────
  void onRegister(AgentConnection& agent,
                  const std::vector<std::uint8_t>& payload);
  void onDashboardRegister(AgentConnection& dashboard,
                           const std::vector<std::uint8_t>& payload);
  void onResponse(AgentConnection& agent,
                  const std::vector<std::uint8_t>& payload);
  void onData(const std::vector<std::uint8_t>& payload);

  void onDashboardCommand(AgentConnection& dashboard,
                          const std::vector<std::uint8_t>& payload);

  void onDisconnect(AgentConnection& connection);

  void sendDisconnect(AgentConnection& agent);

  std::uint32_t nextId();

  // map<command.id, target string = mac adress>
  std::map<std::uint32_t, std::string> commandTargets_;

  std::uint32_t nextCmdId_ = 0;

  void registerDashboard(AgentConnection& dashboard,
                         const OsInfoPayload& dashboardInfo);


void registerAgent(AgentConnection& agent,
                                     const OsInfoPayload& agentInfo, RegisterPayload& registerToSent);
};

#endif

// TODO DASHBOARD SHOULD GET THESE
// pendingResponses_;  // keyed by response id
// struct IncompleteResponse {
//   ResponsePayload baseResponse;
//   std::vector<std::uint8_t> data;  // indexed by chunk_index
//   // std::uint8_t chunksReceived = 0;
// };
// std::map<std::uint16_t, IncompleteResponse>
//     pendingResponses_;  // keyed by response id

// void processCompleteResponse(AgentConnection& agent,
//                              const ResponsePayload& response);
// void createResponseEntry(const ResponsePayload& response);
// void tryCompleteResponse(AgentConnection& agent,
//                          const ResponsePayload& response);
