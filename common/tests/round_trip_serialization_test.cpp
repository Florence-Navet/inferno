#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/process_builder.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_process_info_through_serialize_then_parse) {
  // Arrange
  ProcessInfo info = ProcessBuilder::createProcessInfo();

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeProcessInfo(info);
  const ProcessInfo result = ProtocolParser::parseProcessInfo(bytes);

  // Assert
  EXPECT_EQ(result.pid, info.pid);
  EXPECT_EQ(result.cpu_percent, info.cpu_percent);
  EXPECT_EQ(result.mem_bytes, info.mem_bytes);
  EXPECT_EQ(result.name, info.name);
}

TEST(ProtocolRoundTrip,
     should_preserve_process_info_list_through_serialize_then_parse) {
  // Arrange
  std::vector<ProcessInfo> infos = ProcessBuilder::createProcessInfoList();

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeProcessInfoList(infos);
  const std::vector<ProcessInfo> result =
      ProtocolParser::parseProcessInfoList(bytes);

  // Assert
  EXPECT_EQ(result.size(), infos.size());
  EXPECT_EQ(result[0].pid, infos[0].pid);
  EXPECT_EQ(result[1].pid, infos[1].pid);
  EXPECT_EQ(result[3].pid, infos[3].pid);
}
