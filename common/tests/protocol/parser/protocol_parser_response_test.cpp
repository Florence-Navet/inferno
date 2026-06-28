#include <gtest/gtest.h>

#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "fixtures/common.hpp"
#include "builders/frame_builder.hpp"

TEST(ProtocolParserResponse, should_parse_response_when_input_is_valid) {
  const std::string data = "ok";
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeRawResponsePayload(9,
      static_cast<std::uint8_t>(ResponseStatus::OK), 1,
                          0, static_cast<std::uint16_t>(data.size()),
                          Common::bytesFromString(data));

  const ResponsePayload result = ProtocolParser::parseResponsePayload(input);

  EXPECT_EQ(result.id, 9);
  EXPECT_EQ(result.status, ResponseStatus::OK);
  EXPECT_EQ(result.total_chunks, 1);
  EXPECT_EQ(result.chunk_index, 0);
  EXPECT_EQ(result.data, Common::bytesFromString(data));
}

TEST(ProtocolParserResponse,
     should_reject_payload_shorter_than_fixed_response_fields) {
  const std::vector<std::uint8_t> input = {0x00, 0x09, 0x00, 0x01, 0x00,
  0x00};

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidSize);
}

TEST(ProtocolParserResponse, should_reject_unknown_status_value) {
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeRawResponsePayload(9, Common::INVALID_ENUM_VALUE,
      1, 0, 0, {});

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input),
  InvalidFieldValue);
}

TEST(ProtocolParserResponse, should_reject_zero_total_chunks) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRawResponsePayload(
      9, static_cast<std::uint8_t>(ResponseStatus::OK), 0, 0, 0, {});

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input),
  InvalidFieldValue);
}

TEST(ProtocolParserResponse, should_reject_chunk_index_out_of_range) {
  const std::vector<std::uint8_t> input = FrameBuilder::makeRawResponsePayload(
      9, static_cast<std::uint8_t>(ResponseStatus::OK), 2, 2, 0, {});

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input),
  InvalidFieldValue);
}

TEST(ProtocolParserResponse,
     should_reject_when_declared_data_length_mismatches_payload_size) {
  const std::vector<std::uint8_t> input =
      FrameBuilder::makeRawResponsePayload(9,
      static_cast<std::uint8_t>(ResponseStatus::OK), 1,
                          0, 10, Common::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidSize);
}
