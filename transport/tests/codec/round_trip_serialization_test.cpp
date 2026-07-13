#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/process_builder.hpp"
#include "protocol/lptf_protocol.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_process_info_through_serialize_then_parse) {
  // Arrange
  ProcessInfo info = ProcessBuilder::createProcessInfo();

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeProcessInfo(info);
  const ProcessInfo result = ProtocolParser::parseProcessInfo(bytes);

  // Assert
  EXPECT_EQ(result, info);
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
  EXPECT_EQ(result, infos);
}
