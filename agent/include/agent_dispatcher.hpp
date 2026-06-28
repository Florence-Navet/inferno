#ifndef AGENT_DISPATCHER_HPP
#define AGENT_DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "dispatcher/dispatcher.hpp"
#include "metrics/metrics_controller.hpp"
#include "protocol/lptf_protocol.hpp"
#include "system_monitor/i_system_monitor.hpp"

// enum class StatusRegister : std::uint8_t {
//   SENT,
//   OK,
//   REJECTED
// };

class AgentDispatcher : public Dispatcher {
 public:
  explicit AgentDispatcher(ISystemMonitor& monitor);

  AgentDispatcher(const AgentDispatcher&) = delete;
  ~AgentDispatcher() = default;
  AgentDispatcher& operator=(const AgentDispatcher&) = delete;

  void handleFrame(AgentSession& agent, const Frame& frame) override;

  // StatusRegister getRegistered_() const { return registered_; };
  void sendRegister(AgentSession& session);
  // in AgentDispatcher
  void setMetricsController(std::shared_ptr<MetricsController> controller);

 private:
  ISystemMonitor& monitor_;  // injected -used to read OS info for REGISTER
  std::shared_ptr<MetricsController> metricsController_{nullptr};

  // StatusRegister registered_{StatusRegister::REJECTED};
  void sendResponse(AgentSession& session, std::uint16_t id,
                    ResponseStatus status,
                    const std::vector<std::uint8_t>& data);
  void onCommand(AgentSession& session,
                 const std::vector<std::uint8_t>& payload);
  void onDisconnect(AgentSession& session);
  void onError(const std::vector<std::uint8_t>& payload);

  // Helpers for each command type
  void startMetrics(AgentSession& session, const CommandPayload& command);
  void stopMetrics(AgentSession& session, const CommandPayload& command);
  void osInfo(AgentSession& session, const CommandPayload& command);
  void processesList(AgentSession& session, const CommandPayload& command);
};

#endif