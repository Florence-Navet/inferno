#include "protocol/protocol_serializer.hpp"

#include <cstddef>
#include <string>

#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"

namespace {

void ensureFitsU16(std::size_t sourceSize, const std::string& source) {
  if (sourceSize > KMAX_U16_VALUE) {
    throw InvalidSize(source, std::to_string(sourceSize));
  }
}

void validateHeader(const LptfHeader& header) {
  const std::string inputIdentifier(header.identifier,
                                    sizeof(header.identifier));
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(inputIdentifier);
  }
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  if (header.type >= MessageType::END) {
    throw InvalidType(std::to_string(static_cast<std::uint8_t>(header.type)));
  }
}

void copyString(std::vector<std::uint8_t>& out, std::size_t offset,
                const std::string& value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    out[offset + i] = static_cast<std::uint8_t>(value[i]);
  }
}

void validateOsInfoPayload(const OsInfoPayload& payload) {
  if (payload.os_type >= OSType::END) {
    throw InvalidFieldValue(
        "os_type", std::to_string(static_cast<std::uint8_t>(payload.os_type)));
  }
  if (payload.arch >= ArchType::END) {
    throw InvalidFieldValue(
        "arch", std::to_string(static_cast<std::uint8_t>(payload.arch)));
  }
  if (payload.hostname.empty()) {
    throw InvalidSize("register hostname length", "0");
  }

  if (payload.os_version.empty()) {
    throw InvalidSize("register os_version length", "0");
  }

  if (payload.current_user.empty()) {
    throw InvalidSize("register current_user length", "0");
  }

  if (payload.ip.empty()) {
    throw InvalidSize("register ip length", "0");
  }

  if (payload.hostname.size() > REGISTER_MAX_HOSTNAME_LEN) {
    throw InvalidSize("register hostname length",
                      std::to_string(payload.hostname.size()));
  }

  ensureFitsU16(payload.hostname.size(), "register hostname length");

  ensureFitsU16(payload.os_version.size(), "register os_version length");

  ensureFitsU16(payload.current_user.size(), "register current_user length");

  ensureFitsU16(payload.ip.size(), "register ip length");
}

void validateCommandPayload(const CommandPayload& payload) {
  if (payload.type >= CommandType::END) {
    throw InvalidFieldValue(
        "command_type",
        std::to_string(static_cast<std::uint8_t>(payload.type)));
  }

  ensureFitsU16(payload.data.size(), "command data length");

  if (payload.type == CommandType::SHELL && payload.data.empty()) {
    throw InvalidFieldValue(
        "Data payload empty for a shell command, payload.data content : ",
        payload.data);
  }

  if (payload.type != CommandType::SHELL && !payload.data.empty()) {
    throw InvalidSize("command data length",
                      std::to_string(payload.data.size()));
  }
}

void validateResponsePayload(const ResponsePayload& payload) {
  ensureFitsU16(payload.data.size(), "response data length");

  if (payload.status >= ResponseStatus::END) {
    throw InvalidFieldValue(
        "response_status",
        std::to_string(static_cast<std::uint8_t>(payload.status)));
  }
  if (payload.total_chunks == 0) {
    throw InvalidFieldValue("total_chunks", "0");
  }
  if (payload.chunk_index >= payload.total_chunks) {
    throw InvalidFieldValue(
        "chunk_index",
        std::to_string(static_cast<std::uint8_t>(payload.chunk_index)));
  }
}

void validateDataPayload(const DataPayload& payload) {
  if (payload.subtype >= DataType::END) {
    throw InvalidFieldValue(
        "data_type",
        std::to_string(static_cast<std::uint8_t>(payload.subtype)));
  }
  ensureFitsU16(payload.data.size(), "data length");
}

void validateErrorPayload(const ErrorPayload& payload) {
  if (payload.code >= ErrorType::END) {
    throw InvalidFieldValue(
        "error_code", std::to_string(static_cast<std::uint8_t>(payload.code)));
  }
  if (payload.message.empty()) {
    throw InvalidSize("error message length", "0");
  }
  ensureFitsU16(payload.message.size(), "error message length");
}
}  // namespace

std::vector<std::uint8_t> ProtocolSerializer::serializeHeader(
    const LptfHeader& header) {
  validateHeader(header);

  std::vector<std::uint8_t> headerInByte(LPTF_HEADER_SIZE);

  for (std::size_t i = 0; i < sizeof(header.identifier); ++i) {
    headerInByte[i] = header.identifier[i];
  }

  headerInByte[4] = header.version;
  headerInByte[5] = static_cast<std::uint8_t>(header.type);

  std::size_t offset{6};
  ConvertEndian::writeU16BE(headerInByte, offset, header.size);

  return headerInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeOsInfoPayload(
    const OsInfoPayload& payload) {
  validateOsInfoPayload(payload);

  const std::size_t finalSize{REGISTER_FIXED_BYTES + payload.hostname.size() +
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

  copyString(payloadInByte, REGISTER_FIXED_BYTES, payload.hostname);

  copyString(payloadInByte, REGISTER_FIXED_BYTES + payload.hostname.size(),
             payload.os_version);

  copyString(payloadInByte,
             REGISTER_FIXED_BYTES + payload.hostname.size() +
                 payload.os_version.size(),
             payload.current_user);

  copyString(payloadInByte,
             REGISTER_FIXED_BYTES + payload.hostname.size() +
                 payload.os_version.size() + payload.current_user.size(),
             payload.ip);

  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeCommandPayload(
    const CommandPayload& payload) {
  validateCommandPayload(payload);

  const std::size_t finalSize{COMMAND_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);
  std::size_t offset{0};
  ConvertEndian::writeU16BE(payloadInByte, offset, payload.id);
  payloadInByte[offset] = static_cast<std::uint8_t>(payload.type);
  offset++;
  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  copyString(payloadInByte, COMMAND_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeResponsePayload(
    const ResponsePayload& payload) {
  validateResponsePayload(payload);

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

std::vector<std::uint8_t> ProtocolSerializer::serializeDataPayload(
    const DataPayload& payload) {
  validateDataPayload(payload);

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

std::vector<std::uint8_t> ProtocolSerializer::serializeErrorPayload(
    const ErrorPayload& payload) {
  validateErrorPayload(payload);

  const std::size_t message_length{payload.message.size()};
  const std::size_t finalSize{ERROR_FIXED_BYTES + message_length};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.code);
  std::size_t offset{1};
  ConvertEndian::writeU16BE(payloadInByte, offset, message_length);

  copyString(payloadInByte, ERROR_FIXED_BYTES, payload.message);
  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeProcessInfo(
    const ProcessInfo& info) {
  std::vector<uint8_t> payload(PROCESS_INFO_FIXED_SIZE + info.name.size());
  std::size_t offset{0};
  ConvertEndian::writeU32BE(payload, offset, info.pid);
  ConvertEndian::writeFloat(payload, offset, info.cpu_percent);
  ConvertEndian::writeU64BE(payload, offset, info.mem_bytes);
  ConvertEndian::writeU16BE(payload, offset, info.name.size());
  std::copy(info.name.begin(), info.name.end(), payload.begin() + offset);

  return payload;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeProcessInfoList(
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
    std::vector<uint8_t> infoPayload =
        ProtocolSerializer::serializeProcessInfo(info);
    std::copy(infoPayload.begin(), infoPayload.end(),
              finalList.begin() + offset);
    offset += infoPayload.size();
  }

  return finalList;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeFrame(
    const Frame& frame) {
  if (frame.payload.size() > MAX_VALUE_INT16) {
    throw InvalidSize("payload", std::to_string(frame.payload.size()));
  }

  const std::vector<std::uint8_t> headerBytes =
      ProtocolSerializer::serializeHeader(frame.header);

  std::vector<uint8_t> frameBytes;
  frameBytes.reserve(headerBytes.size() + frame.payload.size());
  frameBytes.insert(frameBytes.end(), headerBytes.begin(), headerBytes.end());
  frameBytes.insert(frameBytes.end(), frame.payload.begin(),
                    frame.payload.end());
  return frameBytes;
}