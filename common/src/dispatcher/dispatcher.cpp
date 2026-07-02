#include "dispatcher/dispatcher.hpp"

#include <sstream>

void Dispatcher::onError(const std::vector<std::uint8_t>& payload) {
  const ErrorPayload error = ProtocolParser::parseErrorPayload(payload);
  std::ostringstream what;
  what << "code=" << static_cast<int>(error.code) << "  msg=" << error.message;
  logger_.error(what.str());
  // std::cerr << "[← ERROR] code=" << static_cast<int>(error.code)
  //           << "  msg=" << error.message << "\n";
  //   running = false;
}

void Dispatcher::sendError(AgentSession& agent, ErrorType code,
                           const std::string& msg) {
  ErrorPayload error;
  error.code = code;
  error.message = msg;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeErrorPayload(error);
  Frame frame = {ProtocolHelper::createHeader(MessageType::ERROR, payload),
                 payload};
  // sendRaw(agent, MessageType::ERROR, payload);
  agent.sendFrame(frame);
}