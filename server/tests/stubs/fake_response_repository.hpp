#ifndef FAKE_RESPONSE_REPOSITORY_HPP
#define FAKE_RESPONSE_REPOSITORY_HPP

#include "repository/response_repository.hpp"

// records saved responses and
// returns a fixed timestamp instead of relying on a real DB's NOW().
class FakeResponseRepository : public IResponseRepository {
 private:
  std::vector<DashboardResponse> saved_;
  std::string fixedReceivedAt_ = "2026-01-01 00:00:00+00";

 public:
  explicit FakeResponseRepository() {}

  std::string save(const ResponsePayload& response) override {
    DashboardResponse dr;
    dr.response = response;
    dr.received_at = fixedReceivedAt_;
    // target/agent_id isn't known at the repository layer in the real
    // implementation either (no JOIN happens on write) - leave it empty,
    // matching what a fresh INSERT would look like before any SELECT.
    saved_.push_back(dr);
    return fixedReceivedAt_;
  }

  std::vector<DashboardResponse> findByCommandId(std::uint32_t commandId,
                                                 int limit) override {
    std::vector<DashboardResponse> result;
    for (const auto& r : saved_) {
      if (r.response.id == commandId) result.push_back(r);
      if (static_cast<int>(result.size()) >= limit) break;
    }
    return result;
  }

  std::vector<DashboardResponse> findByAgentId(
      [[maybe_unused]] const std::string& agentId, int limit) override {
    // No agent_id tracked in this fake (that link only exists via the
    // command_history JOIN in the real repository); return everything
    // saved, capped at limit, for tests that don't care about filtering.
    std::vector<DashboardResponse> result(
        saved_.begin(),
        saved_.begin() +
            std::min(saved_.size(), static_cast<std::size_t>(limit)));
    return result;
  }

  // Test helper
  const std::vector<DashboardResponse>& saved() const { return saved_; }

  // Test-only: clear state between tests
  void clear() { saved_.clear(); }
};

#endif