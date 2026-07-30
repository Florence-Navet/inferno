#include "service/command_service.hpp"

void CommandService::save(DashboardCommand& commandDashboard) {
  commandDashboard.command.id =
      repository_.save(commandDashboard);
//   return commandDashboard.id;
}