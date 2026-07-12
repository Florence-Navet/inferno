#ifndef AGENT_CONNECTION_HPP
#define AGENT_CONNECTION_HPP

#include <memory>
#include <stdexcept>

#include "frame_transport.hpp"
#include "logger.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/i_socket.hpp"

class AgentConnection : public FrameTransport {
 public:
  explicit AgentConnection() {};
  explicit AgentConnection(std::unique_ptr<ISocket> sock)
      : FrameTransport(std::move(sock)) {}
  AgentConnection(std::nullptr_t) = delete;

  AgentConnection(const AgentConnection&) = delete;
  AgentConnection& operator=(const AgentConnection&) = delete;
  AgentConnection(AgentConnection&&) = default;

  // ===== Register payload related methods =====
  const OsInfoPayload& getAgentInfo() const override;
  void setAgentInfo(const OsInfoPayload& info) override;

  // ===== Register and registration state related method =====
  bool getIsRegistered() const { return isRegistered_; }

  // ===== Buffer related methods =====

 private:
  bool isRegistered_ = false;
};

#endif