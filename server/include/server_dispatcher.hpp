#ifndef SERVER_DISPATCHER_HPP
#define SERVER_DISPATCHER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>
// #include "agent_session.hpp"
#include "agent_connection.hpp"
#include "dispatcher/dispatcher.hpp"
#include "protocol/lptf_protocol.hpp"
#include "dashboard_connection.hpp"

class ServerDispatcher : public Dispatcher {
 public:
  explicit ServerDispatcher();
  ServerDispatcher(const ServerDispatcher&) = delete;
  ~ServerDispatcher() = default;
  ServerDispatcher& operator=(const ServerDispatcher&) = delete;

  void handleFrame(FrameTransport& agent, const Frame& frame) override;
  void sendCommand(AgentConnection& agent, CommandType type,
                   const std::string& data = "");
  void setDashboard(std::shared_ptr<DashboardConnection> dashboard) ;

 private:
  std::shared_ptr<DashboardConnection> dashboard_;
  // ── Incoming message handlers ───────────────────────
  void onRegister(AgentConnection& agent,
                  const std::vector<std::uint8_t>& payload);
  void onResponse(AgentConnection& agent,
                  const std::vector<std::uint8_t>& payload);
  void onData(const std::vector<std::uint8_t>& payload);

  void sendDisconnect(AgentConnection& agent);

  std::uint32_t nextId();

  struct IncompleteResponse {
    ResponsePayload baseResponse;
    std::vector<std::uint8_t> data;  // indexed by chunk_index
    // std::uint8_t chunksReceived = 0;
  };
  std::map<std::uint16_t, IncompleteResponse>
      pendingResponses_;  // keyed by response id

  void processCompleteResponse(const ResponsePayload& response);
  void createResponseEntry(const ResponsePayload& response);
  void tryCompleteResponse(const ResponsePayload& response);
  std::uint32_t nextCmdId_ = 0;
};

#endif