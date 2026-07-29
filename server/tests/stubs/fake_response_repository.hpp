#ifndef FAKE_RESPONSE_REPOSITORY_HPP
#define FAKE_RESPONSE_REPOSITORY_HPP

#include "repository/response_repository.hpp"

class FakeResponseRepository : public IResponseRepository {
 private:
  IDatabaseConnection& db_;  // same connection, different instance

 public:
  explicit FakeResponseRepository(IDatabaseConnection& db) : db_(db) {}

  void save(const ResponsePayload& response) override;

  //   std::vector<DashboardResponse> findByAgentId(int commandId);

  std::vector<DashboardResponse> findByCommandId(std::uint32_t commandId,
                                                 int limit = 50);
  std::vector<DashboardResponse> findByAgentId(const std::string& agentId,
                                               int limit = 50);
};

#endif