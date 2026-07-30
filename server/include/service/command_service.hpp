#ifndef COMMAND_SERVICE_HPP
#define COMMAND_SERVICE_HPP

#include <optional>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "repository/command_repository.hpp"
#include "repository/database_connection.hpp"
#include "session_manager.hpp"

class ICommandService {
 public:
  virtual ~ICommandService() = default;
  virtual void save(DashboardCommand& commandDashboard) = 0;
};

// command_repository.hpp
class CommandService : public ICommandService {
 private:
  ICommandRepository& repository_;
  SessionManager& sessionManager_;

 public:
  explicit CommandService(ICommandRepository& repository,
                          SessionManager& sessionManager)
      : repository_(repository), sessionManager_(sessionManager) {}

  void save(DashboardCommand& commandDashboard) override;
};

#endif