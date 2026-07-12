#include "agent_session.hpp"

#include <sstream>

#include "exception/socket_exception.hpp"
#include "protocol/protocol_helper.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/socket_factory.hpp"
#include "socket/tls_socket_factory.hpp"

// AgentSession::AgentSession() :  {}

void AgentSession::resetSession() {
  registered_ = RegisterState::PENDING;
  buffer_.clear();
  header_.reset();
  if (encryption_) {
    socket_ = TLSSocketFactory::createClient("certs/ca.crt");
  } else {
    socket_ = SocketFactory::createTCP();
  }
}

// ===== socket related methods =====
// bool AgentSession::isValid() const { return socket_ && socket_->isValid(); }

// void AgentSession::close() {
//   if (socket_) {
//     socket_->close();
//   }
// }

bool AgentSession::connect(const std::string& host, std::uint16_t port) {
  return socket_ && socket_->connect(host, port);
}



// ===== Register payload related methods =====
const OsInfoPayload& AgentSession::getAgentInfo() const {
    // TODO : if agentInfo = empty or 0 filled values, need to retreive actual value
  return agentInfo_;
}

void AgentSession::setAgentInfo(const OsInfoPayload& info) {
  agentInfo_ = info;
}
