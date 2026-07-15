#ifndef PROTOCOL_RESPONSE_HPP
#define PROTOCOL_RESPONSE_HPP
#include <cstdint>
#include <vector>

constexpr std::size_t RESPONSE_FIXED_BYTES =
    sizeof(std::uint16_t) * 2 +
    sizeof(std::uint8_t) *
        3;  // id + data_len + status + total_chunks + chunk_index

enum class ResponseStatus : std::uint8_t {
  OK = 0,
  ERROR = 1,
  UNKNOWN  // must be the last one
};

struct ResponsePayload {
  std::uint16_t id = 0;
  ResponseStatus status = ResponseStatus::UNKNOWN;
  std::uint8_t total_chunks = 0;
  std::uint8_t chunk_index = 0;
  std::vector<std::uint8_t> data = {};

  bool operator==(const ResponsePayload& other) const {
    return id == other.id && status == other.status &&
           total_chunks == other.total_chunks &&
           chunk_index == other.chunk_index && data == other.data;
  }
};

struct DashboardResponse {
  std::string target;        // which agent sent this response
  ResponsePayload response;

  bool operator==(const DashboardResponse& other) const {
    return target == other.target && response == other.response;
  }
};

#endif