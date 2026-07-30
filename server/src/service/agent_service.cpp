#include "service/agent_service.hpp"

#include <iostream>
#include <sstream>
#include <unordered_set>

#include "logger.hpp"

void AgentService::registerAgent(AgentConnection& agent,
                                 const OsInfoPayload& agentInfo,
                                 RegisterPayload& registerToSent) {
  agent.setAgentInfo(agentInfo);
  agent.setIsRegisered();
  agent.setId(agentInfo.mac);
  sessionManager_.recordAgentTarget(agent.getFd(), agent.getId());

  // RegisterPayload registerToSent;
  registerToSent.system = agentInfo;
  registerToSent.id = agent.getId();

  if (!repository_.findById(agent.getId()).has_value()) {
    repository_.save(registerToSent);
  }

  std::ostringstream what;
  what << "[REGISTER] \nhostname : " << agentInfo.hostname
       << "\nuser : " << agentInfo.current_user
       << "\nos : " << static_cast<int>(agentInfo.os_type)
       << "\narch : " << static_cast<int>(agentInfo.arch)
       << "\nversion : " << agentInfo.os_version << "\nip : " << agentInfo.ip
       << "\nmac : " << agentInfo.mac;
  Logger::info("server dispatcher", what.str());
}

void AgentService::registerDashboard(AgentConnection& dashboard,
                                     const OsInfoPayload& dashboardInfo) {
  dashboard.setAgentInfo(dashboardInfo);
  dashboard.setIsRegisered();
  // dashboard.setId(dashboardInfo.hostname + ":" + dashboardInfo.ip);
  dashboard.setId(dashboardInfo.mac);
  sessionManager_.setDashboardFd(dashboard.getFd());
  sessionManager_.recordAgentTarget(dashboard.getFd(), dashboard.getId());

  Logger::info("server dispatcher",
               "[DASHBOARD REGISTER] : " + dashboard.getId());
}

std::vector<RegisterPayload> AgentService::getAllAgents(
    AgentConnection& dashboard) {
  std::vector<RegisterPayload> agentsInDb = repository_.findAll();

  DataPayload data;
  data.subtype = DataType::AGENTS;
  std::vector<RegisterPayload> registerList;

  if (!sessionManager_.getAgents().empty() || !agentsInDb.empty()) {
    std::unordered_set<std::string> onlineAgents;

    for (const auto& [fd, agent] : sessionManager_.getAgents()) {
      if (fd == dashboard.getFd()) continue;
      onlineAgents.insert(agent.getId());
    }

    for (RegisterPayload& registration : agentsInDb) {
      registration.online = onlineAgents.contains(registration.id);  // C++20
      // onlineAgents.find(registration.id) != onlineAgents.end();
      registerList.push_back(registration);
    }
  }
  return registerList;
}