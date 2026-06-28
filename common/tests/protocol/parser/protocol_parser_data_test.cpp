#include <gtest/gtest.h>

#include "builders/frame_builder.hpp"
#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"

TEST(ProtocolParserData, should_parse_data_payload_when_input_is_valid) {
  const std::string data = "keys";
  const std::vector<std::uint8_t> input = FrameBuilder::makeDataPayload(
      static_cast<std::uint8_t>(DataType::METRICS_SAMPLE),
      static_cast<std::uint16_t>(data.size()), Common::bytesFromString(data));

  const DataPayload result = ProtocolParser::parseDataPayload(input);

  EXPECT_EQ(result.subtype, DataType::METRICS_SAMPLE);
  EXPECT_EQ(result.data, Common::bytesFromString(data));
}

TEST(ProtocolParserData, should_reject_payload_shorter_than_fixed_fields) {
  const std::vector<std::uint8_t> input = {0x00, 0x00};

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidSize);
}

TEST(ProtocolParserData, should_reject_null_data_size) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeDataPayload(
      static_cast<std::uint8_t>(DataType::METRICS_SAMPLE), 0, {});

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidSize);
}

TEST(ProtocolParserData, should_reject_unknown_data_subtype) {
  const std::string data = "whatever";
  const std::vector<std::uint8_t> input = FrameBuilder::makeDataPayload(
      static_cast<std::uint8_t>(DataType::END),
      static_cast<std::uint16_t>(data.size()), Common::bytesFromString(data));

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserData,
     should_reject_when_declared_data_length_mismatches_payload_size) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeDataPayload(
      static_cast<std::uint8_t>(DataType::METRICS_SAMPLE), 10,
      Common::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidSize);
}
