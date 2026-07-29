#include <cstring>
#include <string>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_parser.hpp"
#include "exception/lptf_exception.hpp"

namespace ProtocolParser {

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

RegisterPayload parseRegisterPayload(const std::vector<std::uint8_t>& input) {
  RegisterPayload payload;
  size_t offset = 0;

  std::uint16_t idLen = ConvertEndian::readU16BE(input, offset);
  std::uint16_t registeredAtLen = ConvertEndian::readU16BE(input, offset);
  std::uint16_t lastSeenLen = ConvertEndian::readU16BE(input, offset);

  if (offset + idLen + registeredAtLen + lastSeenLen > input.size()) {
    throw InvalidSize("id", std::to_string(idLen));
  }

  payload.id = ConvertEndian::getString(input, offset, idLen);
  payload.registered_at =
      ConvertEndian::getString(input, offset, registeredAtLen);
  payload.last_seen = ConvertEndian::getString(input, offset, lastSeenLen);

  if (offset >= input.size()) {
    throw InvalidSize("register payload id", "0");
  }
  payload.system = parseOsInfoPayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return payload;
}

std::vector<RegisterPayload> parseRegisterPayloadList(
    const std::vector<std::uint8_t>& input) {
  std::vector<RegisterPayload> registerPayloadList;
  std::size_t offset{0};
  std::uint16_t registerCount = ConvertEndian::readU16BE(input, offset);

  for (size_t i{0}; i < registerCount; ++i) {
    RegisterPayload registerPayload = parseRegisterPayload(
        std::vector<uint8_t>(input.begin() + offset, input.end()));
    registerPayloadList.push_back(registerPayload);

    // Offset calculation mirrors serialization
    offset +=
        (3 * sizeof(std::uint16_t) + registerPayload.id.size() +
         registerPayload.registered_at.size() +
         registerPayload.last_seen.size() + OS_INFO_FIXED_BYTES +
         registerPayload.system.hostname.size() +
         registerPayload.system.os_version.size() +
         registerPayload.system.current_user.size() +
         registerPayload.system.ip.size() + registerPayload.system.mac.size());
  }

  return registerPayloadList;
}
}  // namespace ProtocolParser
