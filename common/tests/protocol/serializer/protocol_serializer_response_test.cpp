#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolSerializerResponse,
     should_produce_corresponding_byteArray_when_response_is_valid) {
  ResponsePayload input;
  input.id = 9;
  input.status = ResponseStatus::OK;
  input.total_chunks = 2;
  input.chunk_index = 1;
  input.data = {'c', 'h', 'u', 'n', 'k'};

  const std::vector<std::uint8_t> expected = {
      0x00, 0x09, static_cast<std::uint8_t>(ResponseStatus::OK),
      0x02, 0x01, 0x00, 0x05,
      'c', 'h', 'u', 'n', 'k'};

  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeResponsePayload(input);

  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerResponse,
     should_throw_InvalidFieldValue_when_status_is_unknown) {
  ResponsePayload input;
  input.id = 9;
  input.status = static_cast<ResponseStatus>(Common::INVALID_ENUM_VALUE);
  input.total_chunks = 1;
  input.chunk_index = 0;
  input.data = {};

  EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerResponse,
     should_throw_InvalidFieldValue_when_total_chunks_is_zero) {
  ResponsePayload input;
  input.id = 9;
  input.status = ResponseStatus::OK;
  input.total_chunks = 0;
  input.chunk_index = 0;
  input.data = {};

  EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerResponse,
     should_throw_InvalidFieldValue_when_chunk_index_is_out_of_range) {
  ResponsePayload input;
  input.id = 9;
  input.status = ResponseStatus::OK;
  input.total_chunks = 2;
  input.chunk_index = 2;
  input.data = {};

  EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerResponse,
     should_throw_InvalidSize_when_response_data_is_too_large) {
  ResponsePayload input;
  input.id = 9;
  input.status = ResponseStatus::OK;
  input.total_chunks = 1;
  input.chunk_index = 0;
  input.data = std::vector<std::uint8_t>(KMAX_U16_VALUE + 1, 'a');

  EXPECT_THROW(ProtocolSerializer::serializeResponsePayload(input),
               InvalidSize);
}