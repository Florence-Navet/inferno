#include "codec/protocol_serializer.hpp"

#include <cstddef>
#include <string>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "exception/lptf_exception.hpp"

namespace ProtocolSerializer {
std::vector<std::uint8_t> serializeHeader(const LptfHeader& header) {
  ProtocolHelper::validateHeader(header);

  std::vector<std::uint8_t> headerInByte(LPTF_HEADER_SIZE);

  // for (std::size_t i = 0; i < sizeof(header.identifier); ++i) {
  //   headerInByte[i] = header.identifier[i];
  // }
  std::copy(header.identifier.begin(), header.identifier.end(),
            headerInByte.begin());

  headerInByte[4] = header.version;
  headerInByte[5] = static_cast<std::uint8_t>(header.type);

  std::size_t offset{6};
  ConvertEndian::writeU16BE(headerInByte, offset, header.size);

  return headerInByte;
}

std::vector<std::uint8_t> serializeOsInfoPayload(const OsInfoPayload& payload) {
  ProtocolHelper::validateOsInfoPayload(payload);

  const std::size_t finalSize{OS_INFO_FIXED_BYTES + payload.hostname.size() +
                              payload.os_version.size() +
                              payload.current_user.size() + payload.ip.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.os_type);
  payloadInByte[1] = static_cast<std::uint8_t>(payload.arch);
  std::size_t offset{2};
  ConvertEndian::writeU16BE(
      payloadInByte, offset,
      static_cast<std::uint16_t>(payload.hostname.size()));

  ConvertEndian::writeU16BE(
      payloadInByte, offset,
      static_cast<std::uint16_t>(payload.os_version.size()));

  ConvertEndian::writeU16BE(
      payloadInByte, offset,
      static_cast<std::uint16_t>(payload.current_user.size()));

  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.ip.size()));

  ProtocolHelper::copyString(payloadInByte, OS_INFO_FIXED_BYTES,
                             payload.hostname);

  ProtocolHelper::copyString(payloadInByte,
                             OS_INFO_FIXED_BYTES + payload.hostname.size(),
                             payload.os_version);

  ProtocolHelper::copyString(
      payloadInByte,
      OS_INFO_FIXED_BYTES + payload.hostname.size() + payload.os_version.size(),
      payload.current_user);

  ProtocolHelper::copyString(payloadInByte,
                             OS_INFO_FIXED_BYTES + payload.hostname.size() +
                                 payload.os_version.size() +
                                 payload.current_user.size(),
                             payload.ip);

  return payloadInByte;
}

std::vector<std::uint8_t> serializeCommandPayload(
    const CommandPayload& payload) {
  ProtocolHelper::validateCommandPayload(payload);

  const std::size_t finalSize{COMMAND_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);
  std::size_t offset{0};
  ConvertEndian::writeU16BE(payloadInByte, offset, payload.id);
  payloadInByte[offset] = static_cast<std::uint8_t>(payload.type);
  offset++;
  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  ProtocolHelper::copyString(payloadInByte, COMMAND_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeResponsePayload(
    const ResponsePayload& payload) {
  ProtocolHelper::validateResponsePayload(payload);

  const std::size_t finalSize{RESPONSE_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);
  std::size_t offset{0};
  ConvertEndian::writeU16BE(payloadInByte, offset, payload.id);
  payloadInByte[2] = static_cast<std::uint8_t>(payload.status);
  payloadInByte[3] = payload.total_chunks;
  payloadInByte[4] = payload.chunk_index;
  offset = 5;
  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  // copyString(payloadInByte, RESPONSE_FIXED_BYTES, payload.data);
  std::copy(payload.data.begin(), payload.data.end(),
            payloadInByte.begin() + RESPONSE_FIXED_BYTES);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeDataPayload(const DataPayload& payload) {
  ProtocolHelper::validateDataPayload(payload);

  const std::size_t finalSize{DATA_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.subtype);
  std::size_t offset{1};
  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  // copyString(payloadInByte, DATA_FIXED_BYTES, payload.data);
  std::copy(payload.data.begin(), payload.data.end(),
            payloadInByte.begin() + DATA_FIXED_BYTES);

  return payloadInByte;
}

std::vector<std::uint8_t> serializeErrorPayload(const ErrorPayload& payload) {
  ProtocolHelper::validateErrorPayload(payload);

  const std::size_t message_length{payload.message.size()};
  const std::size_t finalSize{ERROR_FIXED_BYTES + message_length};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.code);
  std::size_t offset{1};
  ConvertEndian::writeU16BE(payloadInByte, offset, message_length);

  ProtocolHelper::copyString(payloadInByte, ERROR_FIXED_BYTES, payload.message);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeProcessInfo(const ProcessInfo& info) {
  ProtocolHelper::validateProcessInfo(info);
  std::vector<uint8_t> payload(PROCESS_INFO_FIXED_SIZE + info.name.size());
  std::size_t offset{0};
  ConvertEndian::writeU32BE(payload, offset, info.pid);
  ConvertEndian::writeFloat(payload, offset, info.cpu_percent);
  ConvertEndian::writeU64BE(payload, offset, info.mem_bytes);
  ConvertEndian::writeU16BE(payload, offset, info.name.size());
  std::copy(info.name.begin(), info.name.end(), payload.begin() + offset);

  return payload;
}

std::vector<std::uint8_t> serializeProcessInfoList(
    const std::vector<ProcessInfo>& infos) {
  std::size_t totalSize = sizeof(std::uint16_t);  // processCount

  for (const ProcessInfo& info : infos) {
    totalSize += (PROCESS_INFO_FIXED_SIZE + info.name.size());
    // totalSize += info.name.size();
  }

  std::vector<std::uint8_t> finalList(totalSize);
  std::size_t offset{0};
  std::uint16_t processCount = static_cast<std::uint16_t>(infos.size());

  ConvertEndian::writeU16BE(finalList, offset, processCount);

  for (const ProcessInfo& info : infos) {
    std::vector<uint8_t> infoPayload = serializeProcessInfo(info);
    std::copy(infoPayload.begin(), infoPayload.end(),
              finalList.begin() + offset);
    offset += infoPayload.size();
  }

  return finalList;
}

std::vector<std::uint8_t> serializeDashboardCommand(
    const DashboardCommand& command) {
  std::uint16_t target_len = command.target.size();
  std::vector<uint8_t> commandPayload =
      serializeCommandPayload(command.command);

  std::size_t totalSize{sizeof(std::uint16_t) + target_len +
                        commandPayload.size()};
  std::vector<uint8_t> payload(totalSize);
  std::size_t offset{0};
  ConvertEndian::writeU16BE(payload, offset, target_len);
  std::copy(command.target.begin(), command.target.end(),
            payload.begin() + offset);
  offset += target_len;
  std::copy(commandPayload.begin(), commandPayload.end(),
            payload.begin() + offset);
  return payload;
}

std::vector<std::uint8_t> serializeDashboardData(const DashboardData& data) {
  std::uint16_t target_len = data.target.size();
  std::vector<uint8_t> dataPayload = serializeDataPayload(data.data);

  std::size_t totalSize{sizeof(std::uint16_t) + target_len +
                        dataPayload.size()};
  std::vector<std::uint8_t> payload(totalSize);
  std::size_t offset{0};

  ConvertEndian::writeU16BE(payload, offset, target_len);
  std::copy(data.target.begin(), data.target.end(), payload.begin() + offset);
  offset += target_len;
  std::copy(dataPayload.begin(), dataPayload.end(), payload.begin() + offset);

  return payload;
}

std::vector<std::uint8_t> serializeDashboardResponse(
    const DashboardResponse& response) {
  std::uint16_t target_len = response.target.size();
  std::vector<uint8_t> responsePayload =
      serializeResponsePayload(response.response);

  std::size_t totalSize{sizeof(std::uint16_t) + target_len +
                        responsePayload.size()};
  std::vector<std::uint8_t> payload(totalSize);
  std::size_t offset{0};

  ConvertEndian::writeU16BE(payload, offset, target_len);
  std::copy(response.target.begin(), response.target.end(),
            payload.begin() + offset);
  offset += target_len;
  std::copy(responsePayload.begin(), responsePayload.end(),
            payload.begin() + offset);

  return payload;
}

std::vector<std::uint8_t> serializeRegisterPayload(
    const RegisterPayload& payload) {
  std::uint16_t idLen = payload.id.size();
  std::vector<uint8_t> registerPayload = serializeOsInfoPayload(payload.system);

  std::size_t totalSize{sizeof(std::uint16_t) + idLen + registerPayload.size()};
  std::vector<std::uint8_t> finalPayload(totalSize);
  std::size_t offset{0};

  ConvertEndian::writeU16BE(finalPayload, offset, idLen);
  std::copy(payload.id.begin(), payload.id.end(),
            finalPayload.begin() + offset);
  offset += idLen;
  std::copy(registerPayload.begin(), registerPayload.end(),
            finalPayload.begin() + offset);

  return finalPayload;
}

std::vector<std::uint8_t> serializeFrame(const Frame& frame) {
  if (frame.payload.size() > MAX_VALUE_INT16) {
    throw InvalidSize("payload", std::to_string(frame.payload.size()));
  }

  const std::vector<std::uint8_t> headerBytes = serializeHeader(frame.header);

  std::vector<uint8_t> frameBytes;
  frameBytes.reserve(headerBytes.size() + frame.payload.size());
  frameBytes.insert(frameBytes.end(), headerBytes.begin(), headerBytes.end());
  frameBytes.insert(frameBytes.end(), frame.payload.begin(),
                    frame.payload.end());
  return frameBytes;
}
}  // namespace ProtocolSerializer