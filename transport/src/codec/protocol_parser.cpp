#include "codec/protocol_parser.hpp"

#include <cstring>
#include <string>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_parser.hpp"
#include "exception/lptf_exception.hpp"

namespace ProtocolParser {
LptfHeader parseHeader(const std::vector<std::uint8_t>& input) {
  if (input.size() < 8) {
    throw InvalidSize(std::string("header"), std::to_string(input.size()));
  }

  const std::string_view inputIdentifier(
      reinterpret_cast<const char*>(input.data()), 4);
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(std::string(inputIdentifier));
  }

  LptfHeader header;
  std::size_t offset{4};
  // for (std::size_t i = 0; i < offset; ++i) {
  //   header.identifier[i] = static_cast<char>(input[i]);
  // }
  std::copy(input.begin(), input.begin() + 4, header.identifier.begin());

  header.version = input[offset];
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  offset++;
  header.type = ProtocolHelper::EnumConversion::toMessageType(input[offset]);
  offset++;
  header.size = ConvertEndian::readU16BE(input, offset);
  return header;
}

OsInfoPayload parseOsInfoPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < OS_INFO_FIXED_BYTES) {
    throw InvalidSize("os info payload", std::to_string(input.size()));
  }

  std::size_t offset{2};
  const std::uint16_t hostnameLen{ConvertEndian::readU16BE(input, offset)};
  const std::uint16_t osVersionLen{ConvertEndian::readU16BE(input, offset)};
  const std::uint16_t currentUserLen{ConvertEndian::readU16BE(input, offset)};
  const std::uint16_t ipLen{ConvertEndian::readU16BE(input, offset)};
  const std::uint16_t macLen{ConvertEndian::readU16BE(input, offset)};

  const std::size_t maxFieldLen{MAX_VALUE_INT16 - OS_INFO_FIXED_BYTES};

  ProtocolHelper::validateNotNullLength(hostnameLen, maxFieldLen);
  ProtocolHelper::validateNotNullLength(osVersionLen, maxFieldLen);
  ProtocolHelper::validateNotNullLength(currentUserLen, maxFieldLen);
  ProtocolHelper::validateNotNullLength(ipLen, maxFieldLen);
  ProtocolHelper::validateNotNullLength(macLen, maxFieldLen);

  const std::size_t expectedSize{OS_INFO_FIXED_BYTES + hostnameLen +
                                 osVersionLen + currentUserLen + ipLen + macLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);
  // validateStringLength(hostnameLen, input, maxHostnameLength, expectedSize);
  // TODO validateStringLength needs to check each string or all payload

  OsInfoPayload payload;
  payload.os_type = ProtocolHelper::EnumConversion::toOsType(input[0]);
  payload.arch = ProtocolHelper::EnumConversion::toArchType(input[1]);
  payload.hostname.assign(
      reinterpret_cast<const char*>(input.data() + OS_INFO_FIXED_BYTES),
      hostnameLen);

  payload.os_version.assign(
      reinterpret_cast<const char*>(input.data() + OS_INFO_FIXED_BYTES +
                                    hostnameLen),
      osVersionLen);

  payload.current_user.assign(
      reinterpret_cast<const char*>(input.data() + OS_INFO_FIXED_BYTES +
                                    hostnameLen + osVersionLen),
      currentUserLen);

  payload.ip.assign(reinterpret_cast<const char*>(
                        input.data() + OS_INFO_FIXED_BYTES + hostnameLen +
                        osVersionLen + currentUserLen),
                    ipLen);

  payload.mac.assign(reinterpret_cast<const char*>(
                         input.data() + OS_INFO_FIXED_BYTES + hostnameLen +
                         osVersionLen + currentUserLen + ipLen),
                     macLen);
  return payload;
}

DataPayload parseDataPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < DATA_FIXED_BYTES) {
    throw InvalidSize("data payload", std::to_string(input.size()));
  }
  std::size_t offset{1};
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, offset)};
  const std::size_t expectedSize{DATA_FIXED_BYTES + dataLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);

  DataPayload payload;
  payload.subtype = ProtocolHelper::EnumConversion::toDataType(input[0]);
  payload.data =
      std::vector<std::uint8_t>(input.begin() + DATA_FIXED_BYTES,
                                input.begin() + DATA_FIXED_BYTES + dataLen);

  return payload;
}

CommandPayload parseCommandPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < COMMAND_FIXED_BYTES) {
    throw InvalidSize("command payload", std::to_string(input.size()));
  }
  // std::size_t typeOffset{4};
  std::size_t typeOffset{0};
  CommandPayload payload;
  payload.id = ConvertEndian::readU32BE(input, typeOffset);

  const CommandType type =
      ProtocolHelper::EnumConversion::toCommandType(input[typeOffset]);
  payload.type = type;
  typeOffset++;

  const std::uint16_t dataLen = ConvertEndian::readU16BE(input, typeOffset);
  const std::size_t expectedSize{COMMAND_FIXED_BYTES + dataLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);

  // std::size_t payloadOffset{0};

  if (payload.type == CommandType::SHELL && dataLen != 0) {
    payload.data.assign(
        reinterpret_cast<const char*>(input.data() + COMMAND_FIXED_BYTES),
        dataLen);
  }

  return payload;
}

ResponsePayload parseResponsePayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < RESPONSE_FIXED_BYTES) {
    throw InvalidSize("response payload", std::to_string(input.size()));
  }
  // std::size_t offset{3};
  std::size_t offset{0};
  ResponsePayload payload;
  // std::size_t payloadOffset{0};
  payload.id = ConvertEndian::readU32BE(input, offset);

  payload.status =
      ProtocolHelper::EnumConversion::toResponseStatus(input[offset]);
  offset++;
  payload.total_chunks = input[offset];
  offset++;
  payload.chunk_index = input[offset];
  offset++;
  ProtocolHelper::validateChunkFields(payload.total_chunks,
                                      payload.chunk_index);  // 4 & 5

  // offset += 2;
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, offset)};  // 5

  const std::size_t expectedSize{RESPONSE_FIXED_BYTES + dataLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);
  // payload.data.assign(
  //     reinterpret_cast<const char*>(input.data() + RESPONSE_FIXED_BYTES),
  //     dataLen);
  payload.data.assign(input.begin() + RESPONSE_FIXED_BYTES,
                      input.begin() + RESPONSE_FIXED_BYTES + dataLen);
  return payload;
}

ErrorPayload parseErrorPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < ERROR_FIXED_BYTES) {
    throw InvalidSize("error payload", std::to_string(input.size()));
  }

  std::size_t offset{1};
  const std::uint16_t messageLen{ConvertEndian::readU16BE(input, offset)};
  const std::size_t expectedSize{ERROR_FIXED_BYTES + messageLen};
  const std::size_t maxLength{MAX_VALUE_INT16 - ERROR_FIXED_BYTES};

  // if (input.size() != expectedSize) {
  //   throw InvalidSize("error payload", std::to_string(input.size()));
  // }

  ProtocolHelper::validateStringLength(messageLen, input, maxLength,
                                       expectedSize);

  ErrorPayload payload;
  payload.code = ProtocolHelper::EnumConversion::toErrorType(input[0]);
  payload.message.assign(
      reinterpret_cast<const char*>(input.data() + ERROR_FIXED_BYTES),
      messageLen);
  return payload;
}

ProcessInfo parseProcessInfo(const std::vector<std::uint8_t>& input) {
  if (input.size() < PROCESS_INFO_FIXED_SIZE) {
    throw InvalidSize("process info payload", std::to_string(input.size()));
  }

  ProcessInfo info;
  size_t offset = 0;

  info.pid = ConvertEndian::readU32BE(input, offset);
  info.cpu_percent = ConvertEndian::readFloat(input, offset);
  info.mem_bytes = ConvertEndian::readU64BE(input, offset);
  std::uint16_t nameLen = ConvertEndian::readU16BE(input, offset);
  const std::size_t maxNameLen = MAX_VALUE_INT16 - PROCESS_INFO_FIXED_SIZE;
  ProtocolHelper::validateNotNullLength(nameLen, maxNameLen);

  if (offset + nameLen > input.size()) {
    throw InvalidSize("process name", std::to_string(nameLen));
  }

  info.name = ConvertEndian::getString(input, offset, nameLen);

  if (info.cpu_percent < 0.0f) {
    throw InvalidFieldValue("cpu_percent", std::to_string(info.cpu_percent));
  }

  return info;
}

std::vector<ProcessInfo> parseProcessInfoList(
    const std::vector<std::uint8_t>& input) {
  std::vector<ProcessInfo> processInfoList;
  std::size_t offset{0};
  std::uint16_t processCount = ConvertEndian::readU16BE(input, offset);

  for (size_t i{0}; i < processCount; ++i) {
    if (offset + PROCESS_INFO_FIXED_SIZE > input.size()) {
      throw InvalidSize("process info at index " + std::to_string(i),
                        "insufficient bytes");
    }
    ProcessInfo info = parseProcessInfo(
        std::vector<uint8_t>(input.begin() + offset, input.end()));
    processInfoList.push_back(info);
    offset += PROCESS_INFO_FIXED_SIZE + info.name.size();
  }

  return processInfoList;
}
}  // namespace ProtocolParser

DashboardCommand ProtocolParser::parseDashboardCommand(
    const std::vector<std::uint8_t>& input) {
  //       if (input.size() < PROCESS_INFO_FIXED_SIZE) {
  //   throw InvalidSize("process info payload", std::to_string(input.size()));
  // }

  DashboardCommand command;
  size_t offset = 0;

  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);

  if (offset + targetLen > input.size()) {
    throw InvalidSize("target name", std::to_string(targetLen));
  }

  command.target = ConvertEndian::getString(input, offset, targetLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }
  command.command = parseCommandPayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return command;
}

DashboardData ProtocolParser::parseDashboardData(
    const std::vector<std::uint8_t>& input) {
  DashboardData data;
  size_t offset = 0;

  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);

  if (offset + targetLen > input.size()) {
    throw InvalidSize("target name", std::to_string(targetLen));
  }

  data.target = ConvertEndian::getString(input, offset, targetLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }
  data.data = parseDataPayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return data;
}

DashboardResponse ProtocolParser::parseDashboardResponse(
    const std::vector<std::uint8_t>& input) {
  DashboardResponse response;
  size_t offset = 0;

  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);

  if (offset + targetLen > input.size()) {
    throw InvalidSize("target name", std::to_string(targetLen));
  }

  response.target = ConvertEndian::getString(input, offset, targetLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }
  response.response = parseResponsePayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return response;
}

RegisterPayload ProtocolParser::parseRegisterPayload(
    const std::vector<std::uint8_t>& input) {
  RegisterPayload payload;
  size_t offset = 0;

  std::uint16_t idLen = ConvertEndian::readU16BE(input, offset);

  if (offset + idLen > input.size()) {
    throw InvalidSize("id", std::to_string(idLen));
  }

  payload.id = ConvertEndian::getString(input, offset, idLen);
  if (offset >= input.size()) {
    throw InvalidSize("register payload id", "0");
  }
  payload.system = parseOsInfoPayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return payload;
}
