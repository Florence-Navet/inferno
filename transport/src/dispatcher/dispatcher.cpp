#include "dispatcher/dispatcher.hpp"

#include <sstream>

#include "logger.hpp"

void Dispatcher::onError(const std::vector<std::uint8_t>& payload) {
  const ErrorPayload error = ProtocolParser::parseErrorPayload(payload);
  std::ostringstream what;
  what << "code=" << static_cast<int>(error.code) << "  msg=" << error.message;
  Logger::error("dispatcher", what.str());
  // std::cerr << "[← ERROR] code=" << static_cast<int>(error.code)
  //           << "  msg=" << error.message << "\n";
  //   running = false;
}

void Dispatcher::sendError(FrameTransport& agent, ErrorType code,
                           const std::string& msg) {
  ErrorPayload error;
  error.code = code;
  error.message = msg;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeErrorPayload(error);
  Frame frame = {
      ProtocolHelper::createHeader(MessageType::INFERNO_ERROR, payload),
      payload};
  // sendRaw(agent, MessageType::INFERNO_ERROR, payload);
  agent.sendFrame(frame);
}