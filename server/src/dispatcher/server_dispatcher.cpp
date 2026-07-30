#include "dispatcher/server_dispatcher.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "codec/metrics_parser.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"
#include "exception/lptf_exception.hpp"
#include "logger.hpp"
#include "socket/i_socket.hpp"

void ServerDispatcher::handleFrame(FrameTransport& agent, const Frame& frame) {
  AgentConnection& connection = static_cast<AgentConnection&>(agent);
  std::cout << "type = " << static_cast<int>(frame.header.type) << std::endl;

  switch (frame.header.type) {
    case MessageType::REGISTER:
      Logger::info("server dispatcher", "register received");
      onRegister(connection, frame.payload);
      break;
    case MessageType::DASHBOARD_REGISTER:
      Logger::info("server dispatcher", "dashboard register received");
      onDashboardRegister(connection, frame.payload);
      break;
    case MessageType::RESPONSE:
      Logger::info("server dispatcher", "response received");
      onResponse(connection, frame.payload);
      break;
    case MessageType::DATA:
      onData(frame.payload);
      break;
    case MessageType::DISCONNECT:
      if (sessionManager_.isDashboardConnection(connection.getFd())) {
        // DashboardDisconnect target =
        // ProtocolParser::parseDashboardDisconnect(frame.payload);
        std::string target =
            ProtocolParser::parseDashboardDisconnect(frame.payload).target;
        onDisconnect(sessionManager_.getAgentByTarget(target));
      } else {
        if (sessionManager_.isDashboard()) {
          sessionManager_.getDashboard().sendError(
              ErrorType::UNKNOWN_TYPE, "Agents cannot ask for disconnection");
        }
        connection.sendError(ErrorType::UNKNOWN_TYPE,
                             "Agents cannot ask for disconnection");
      }
      break;

      // onDisconnect(connection);
    case MessageType::INFERNO_ERROR:
      if (sessionManager_.isDashboard()) {
        sessionManager_.getDashboard().onError(frame.payload);
      }
      break;
    case MessageType::COMMAND: {
      if (sessionManager_.isDashboardConnection(connection.getFd())) {
        onDashboardCommand(connection, frame.payload);
      } else {
        if (sessionManager_.isDashboard()) {
          sessionManager_.getDashboard().sendError(
              ErrorType::UNKNOWN_TYPE, "Agents send RESPONSE, not COMMAND");
        }
        connection.sendError(ErrorType::UNKNOWN_TYPE,
                             "Agents send RESPONSE, not COMMAND");
      }
      break;
    }
    default:
      if (sessionManager_.isDashboard()) {
        sessionManager_.getDashboard().sendError(ErrorType::UNKNOWN_TYPE,
                                                 "Unexpected message type");
      }
      connection.sendError(ErrorType::UNKNOWN_TYPE, "Unexpected message type");
  }
}

// ── Incoming handlers ────────────────────────────────────────

// The first message from the agent must always be REGISTER.
// Once we know who it is, we kick off the command sequence.
void ServerDispatcher::onRegister(AgentConnection& agent,
                                  const std::vector<std::uint8_t>& payload) {
  OsInfoPayload agentInfo = ProtocolParser::parseOsInfoPayload(payload);
  RegisterPayload registerToSent;
  agentService_.registerAgent(agent, agentInfo, registerToSent);

  DataPayload registration;
  registration.subtype = DataType::REGISTRATION;
  registration.data =
      ProtocolSerializer::serializeRegisterPayload(registerToSent);
  std::vector<std::uint8_t> registerPayload =
      ProtocolSerializer::serializeDataPayload(registration);
  Frame frame{ProtocolHelper::createHeader(MessageType::DATA, registerPayload),
              registerPayload};

  if (sessionManager_.isDashboard()) {
    try {
      sessionManager_.getDashboard().sendFrame(frame);
    } catch (const std::exception& e) {
      Logger::error(
          "server dispatcher",
          "Failed to send registration to dashboard: " + std::string(e.what()));
    }
  }
}

void ServerDispatcher::onDashboardRegister(
    AgentConnection& dashboard, const std::vector<std::uint8_t>& payload) {
  agentService_.registerDashboard(dashboard,
                                  ProtocolParser::parseOsInfoPayload(payload));
  DataPayload data;
  data.subtype = DataType::AGENTS;

  data.data = ProtocolSerializer::serializeRegisterPayloadList(
      agentService_.getAllAgents(dashboard));

  std::vector<std::uint8_t> finalPayload =
      ProtocolSerializer::serializeDataPayload(data);

  Frame frame{ProtocolHelper::createHeader(MessageType::DATA, finalPayload),
              finalPayload};
  dashboard.sendFrame(frame);
}

// A RESPONSE carries the same id as the COMMAND it answers,
// plus chunk metadata for large payloads split across messages.
void ServerDispatcher::onResponse(AgentConnection& agent,
                                  const std::vector<std::uint8_t>& payload) {
  std::ostringstream what;
  what << "Response from agent :" << agent.getAgentInfo().ip;
  Logger::info("server dispatcher", what.str());

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(payload);

  DashboardResponse dashResponse;
  dashResponse.target = commandService_.getTarget(response.id);
  dashResponse.response = response;

  std::vector<std::uint8_t> dashPayload =
      ProtocolSerializer::serializeDashboardResponse(dashResponse);
  Frame frame = {
      ProtocolHelper::createHeader(MessageType::RESPONSE, dashPayload),
      dashPayload};

  if (sessionManager_.isDashboard()) {
    sessionManager_.getDashboard().sendFrame(frame);
  }

  // Clean up when last chunk received
  if (response.chunk_index + 1 == response.total_chunks) {
    // commandTargets_.erase(commandId);
    commandService_.deleteTarget(response.id);
  }
}

// DATA messages are pushed by the agent without a prior COMMAND
// (e.g. keylogger stream). Handle them independently of the
// request/response cycle.
void ServerDispatcher::onData(const std::vector<std::uint8_t>& payload) {
  const DataPayload data = ProtocolParser::parseDataPayload(payload);
  std::ostringstream what;
  what << "[DATA] subtype=" << static_cast<int>(data.subtype) << "\n";
  Frame frame;
  int dashboardFd;

  if (sessionManager_.isDashboard()) {
    dashboardFd = sessionManager_.getDashboardFd();
    // frame.payload = payload;
    // sessionManager_.getDashboard().sendFrame(frame);
  }

  switch (data.subtype) {
    case DataType::METRICS_SAMPLE: {
      frame.header = ProtocolHelper::createHeader(MessageType::DATA, payload);
      frame.payload = payload;
      sessionManager_.getAgent(dashboardFd).sendFrame(frame);
      // TODO For log only, doesn't need to parse it once dashboard will
      // retrieve it
      {
        MetricsSample sample = MetricsParser::parseMetricsSample(data.data);

        what << "[DATA] METRICS_SAMPLE\n";

        what << "CPU: " << sample.cpu.total_percent << "%\n";
        what << "CPU cores: ";
        for (float core : sample.cpu.per_core) what << core << "% ";
        what << '\n';

        what << "Memory: " << sample.mem.phys_used << "/"
             << sample.mem.phys_total << " bytes used\n";

        for (const auto& disk : sample.disks) {
          what << "Disk " << disk.device << " R=" << disk.read_bytes_per_sec
               << " W=" << disk.write_bytes_per_sec << '\n';
        }

        for (const auto& iface : sample.interfaces) {
          what << "Net " << iface.iface << " RX=" << iface.rx_bytes_per_sec
               << " TX=" << iface.tx_bytes_per_sec << '\n';
        }
      }
    } break;
    case DataType::HEALTH_CHECK: {
      what << "health check not implemented yet";
      if (sessionManager_.isDashboard()) {
        sessionManager_.getDashboard().sendError(
            ErrorType::NOT_IMPLEMENTED, "health check isn't implemented yet");
      }
    } break;
    case DataType::AGENTS:
    case DataType::REGISTRATION: {
      what << "DataType::AGENTS or REGISTRATION are generated by server only";
      if (sessionManager_.isDashboard()) {
        sessionManager_.getDashboard().sendError(
            ErrorType::INVALID_TYPE,
            "DataType::AGENTS or REGISTRATION are generated by server only");
      }
    } break;
    case DataType::UNKNOWN: {
      what << "unknown data type";
    }
  }

  if (sessionManager_.isDashboard()) {
    frame.payload = payload;
    sessionManager_.getDashboard().sendFrame(frame);
  }
  Logger::info("server dispatcher", what.str());
}

void ServerDispatcher::onDashboardCommand(
    AgentConnection& dashboard, const std::vector<std::uint8_t>& payload) {
  DashboardCommand commandDashboard =
      ProtocolParser::parseDashboardCommand(payload);

  try {
    AgentConnection& agent =
        sessionManager_.getAgentByTarget(commandDashboard.target);
    commandService_.save(commandDashboard);
    sendCommand(agent, commandDashboard.command);
    
  } catch (const std::exception& e) {
    dashboard.sendError(ErrorType::UNKNOWN_COMMAND,
                        "Agent target not found: " + commandDashboard.target);
  }
}

void ServerDispatcher::onDisconnect(AgentConnection& connection) {
  std::ostringstream what;
  what << "[DISCONNECT] " << connection.getId();
  Logger::info("server dispatcher", what.str());
  // maybe handle the rest of pending response and communication before closing
  // anything?
}

// ── Outgoing senders ─────────────────────────────────────────

void ServerDispatcher::sendCommand(AgentConnection& agent,
                                   const CommandPayload& command) {
  if (command.type == CommandType::UNKNOWN) {
    // sendError(agent, ErrorType::UNKNOWN_COMMAND, "unknown command asked");
    // // throw error here instead, catch it above so we can send it to the
    // right caller! return;
    throw InvalidType(std::to_string(static_cast<int>(command.type)));
  }

  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);

  Frame frame = {ProtocolHelper::createHeader(MessageType::COMMAND, payload),
                 payload};

  agent.sendFrame(frame);
  std::ostringstream what;

  what << "[COMMAND] target=" << agent.getId()
       << "\ntype=" << static_cast<int>(command.type)
       << "\ncmd_id=" << std::to_string(command.id);

  Logger::info("server dispatcher", what.str());
}

void ServerDispatcher::onAgentDisconnect(AgentConnection& agent) {
  DashboardDisconnect disconnection;
  disconnection.target = agent.getId();
  std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeDashboardDisconnect(disconnection);
  Frame frame;
  frame.header = ProtocolHelper::createHeader(MessageType::DATA, payload);
  if (sessionManager_.isDashboard()) {
    frame.payload = payload;
    sessionManager_.getDashboard().sendFrame(frame);
  }
}

void ServerDispatcher::sendDisconnect(AgentConnection& agent) {
  const std::vector<uint8_t> payload{};
  Frame frame = {ProtocolHelper::createHeader(MessageType::DISCONNECT, payload),
                 payload};

  agent.sendFrame(frame);
  Logger::info("server dispatcher", "[DISCONNECT]");
}


// TODO DASHBOARD SHOULD GET THESE

// void ServerDispatcher::createResponseEntry(const ResponsePayload& response) {
//   if (pendingResponses_.find(response.id) == pendingResponses_.end()) {
//     IncompleteResponse incomplete;
//     incomplete.baseResponse = response;
//     pendingResponses_[response.id] = incomplete;
//   }
// }

// void ServerDispatcher::processCompleteResponse(
//     AgentConnection& agent, const ResponsePayload& response) {
//   std::ostringstream what;
//   what << "[RESPONSE] id=" << response.id
//        << "\nstatus=" << static_cast<int>(response.status)
//        << "\n" << ProtocolParser::toString(response.data);
//   Logger::info("server dispatcher", what.str());

//   // AgentConnection dashboard = sessionManager_.getDashboard();
//   DashboardResponse finalResponse;
//   finalResponse.target = agent.getId();
//   finalResponse.response = response;
//   std::vector<std::uint8_t> payload =
//       ProtocolSerializer::serializeDashboardResponse(finalResponse);
//   Frame frame = {ProtocolHelper::createHeader(MessageType::RESPONSE,
//   payload),
//                  payload};

//   if (sessionManager_.isDashboard()) {
//     try {
//       sessionManager_.getDashboard().sendFrame(frame);
//     } catch (const std::exception& e) {
//       Logger::error(
//           "server dispatcher",
//           "Failed to send registration to dashboard: " +
//           std::string(e.what()));
//     }
//   }

//   if (pendingResponses_.find(response.id) != pendingResponses_.end()) {
//     pendingResponses_.erase(response.id);
//   }
// }

// void ServerDispatcher::tryCompleteResponse(AgentConnection& agent,
//                                            const ResponsePayload& response) {
//   const bool lastChunk = response.chunk_index + 1 == response.total_chunks;
//   if (lastChunk) {
//     IncompleteResponse& incomplete = pendingResponses_[response.id];

//     // Reassemble all chunks
//     ResponsePayload complete = incomplete.baseResponse;
//     complete.data = incomplete.data;

//     processCompleteResponse(agent, complete);
//   }
// }
