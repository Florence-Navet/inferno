#include "agent_dispatcher.hpp"

#include <sstream>

#include "logger.hpp"
#include "protocol/protocol_helper.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/i_socket.hpp"

AgentDispatcher::AgentDispatcher(ISystemMonitor& monitor)
    : Dispatcher(), monitor_(monitor) {}

void AgentDispatcher::handleFrame(FrameTransport& agent, const Frame& frame) {
  AgentSession& session = static_cast<AgentSession&>(agent);
  switch (frame.header.type) {
    case MessageType::COMMAND:
      return onCommand(session, frame.payload);
    case MessageType::DISCONNECT:
      return onDisconnect(session);
    case MessageType::ERROR: {
      return onError(frame.payload);
    }
    default:
      return sendError(session, ErrorType::UNKNOWN_TYPE,
                       "Unexpected message type for agent");
  }
}

void AgentDispatcher::startMetrics(AgentSession& session,
                                   const CommandPayload& command) {
  Logger::info("agent dispatcher", "received COMMAND START_METRICS id=" +
                                       std::to_string(command.id));
  metricsController_->start(session);
  return send(session, command.id, ResponseStatus::OK, {});
}

void AgentDispatcher::stopMetrics(AgentSession& session,
                                  const CommandPayload& command) {
  Logger::info("agent dispatcher", "received COMMAND STOP_METRICS id=" +
                                       std::to_string(command.id));
  metricsController_->stop();
  return send(session, command.id, ResponseStatus::OK, {});
}

void AgentDispatcher::osInfo(AgentSession& session,
                             const CommandPayload& command) {
  std::ostringstream what;
  what << "received COMMAND OS_INFO id=" << command.id;
  Logger::info("agent dispatcher", what.str());

  OsInfoPayload payload = monitor_.getOsInfo();

  const std::vector<std::uint8_t> OsInfoPayload =
      ProtocolSerializer::serializeOsInfoPayload(payload);

  send(session, command.id, ResponseStatus::OK, OsInfoPayload);
}

void AgentDispatcher::processesList(AgentSession& session,
                                    const CommandPayload& command) {
  std::ostringstream what;
  what << "received COMMAND RUNNING_PROCESSES id=" << command.id;
  Logger::info("agent dispatcher", what.str());

  const std::vector<ProcessInfo> processes = monitor_.getProcessList();
  const std::vector<std::uint8_t> processBytes =
      ProtocolSerializer::serializeProcessInfoList(processes);

  return send(session, command.id, ResponseStatus::OK, processBytes);
}

void AgentDispatcher::onError(const std::vector<std::uint8_t>& payload) {
  std::ostringstream what;
  try {
    const ErrorPayload errorPayload =
        ProtocolParser::parseErrorPayload(payload);
    what << "server ERROR code=" << static_cast<int>(errorPayload.code)
         << " message=" << errorPayload.message;
    Logger::error("agent dispatcher", what.str());
  } catch (...) {
    Logger::error("agent dispatcher", "received malformed ERROR payload");
  }
}
void AgentDispatcher::onDisconnect(AgentSession& session) {
  Logger::info("agent dispatcher", "received DISCONNECT");
  session.close();  // check for valid socket already inside close method
                    // session.socket.reset();

}

void AgentDispatcher::onCommand(AgentSession& session,
                                const std::vector<std::uint8_t>& payload) {
  try {
    const CommandPayload cmd = ProtocolParser::parseCommandPayload(payload);

    switch (cmd.type) {
      case CommandType::OS_INFO:
        return osInfo(session, cmd);
      case CommandType::RUNNING_PROCESSES:
        return processesList(session, cmd);
      case CommandType::START_METRICS:
        return startMetrics(session, cmd);
      case CommandType::STOP_METRICS:
        return stopMetrics(session, cmd);
      default:
        return sendError(session, ErrorType::UNKNOWN_COMMAND,
                         "Command not implemented in minimal agent");
    }

  } catch (const std::exception& ex) {
    std::ostringstream what;
    what << "invalid COMMAND payload: " << ex.what();
    Logger::error("agent dispatcher", what.str());
    return sendError(session, ErrorType::INVALID_FORMAT,
                     "Invalid COMMAND payload");
  }
}

void AgentDispatcher::sendRegister(AgentSession& session) {
  std::ostringstream what;
  CommandPayload command;
  command.id = 0;
  command.type = CommandType::OS_INFO;
  OsInfoPayload payload = monitor_.getOsInfo();
  session.setAgentInfo(payload);
  session.setRegistered_(RegisterState::SENT);
  what << "at the end of sendRegister";
  Logger::info("agent dispatcher", what.str());
  send(session, command.id, ResponseStatus::OK,
       ProtocolSerializer::serializeOsInfoPayload(payload),
       MessageType::REGISTER);
}

void AgentDispatcher::setMetricsController(
    std::shared_ptr<MetricsController> controller) {
  metricsController_ = controller;
}

void AgentDispatcher::send(AgentSession& session, std::uint16_t id,
                           ResponseStatus status,
                           const std::vector<std::uint8_t>& data,
                           MessageType type) {
  std::vector<uint8_t> responsePayload;
  if (type == MessageType::RESPONSE) {
    ResponsePayload payload;
    payload.id = id;
    payload.status = status;
    payload.total_chunks = 1;
    payload.chunk_index = 0;
    payload.data = data;

    responsePayload = ProtocolSerializer::serializeResponsePayload(payload);
  }

  if (type == MessageType::REGISTER) {
    responsePayload = data;
  }

  Frame frame = {ProtocolHelper::createHeader(type, responsePayload),
                 responsePayload};
  session.sendFrame(frame);
}
