#include "session_manager.hpp"

void SessionManager::addAgent(int fileDescriptor,
                              std::unique_ptr<ISocket> incoming) {
  agents_.emplace(fileDescriptor, AgentConnection(std::move(incoming)));
}

void SessionManager::recordAgentTarget(int fileDescriptor,
                                       const std::string& target) {
  agentsByTarget_.emplace(target, fileDescriptor);
  targetByFd_.emplace(fileDescriptor, target);
}

void SessionManager::deleteAgentTarget(int fileDescriptor,
                                       const std::string& target) {
  agentsByTarget_.erase(target);
  targetByFd_.erase(fileDescriptor);
}

AgentConnection& SessionManager::getAgent(int fileDescriptor) {
  return agents_.at(fileDescriptor);
}

void SessionManager::setDashboardFd(int fileDescriptor) {
  dashboardFd_ = fileDescriptor;
}

AgentConnection& SessionManager::getDashboard() {
  return agents_.at(dashboardFd_);
}

std::string SessionManager::getTargetByFd(int fileDescriptor) {
  return targetByFd_.at(fileDescriptor);
}

int SessionManager::getFdByTarget(const std::string& target) {
  return agentsByTarget_.at(target);
}

void SessionManager::removeAgent(int fileDescriptor) {
  AgentConnection& agent = getAgent(fileDescriptor);
  deleteAgentTarget(fileDescriptor, agent.getId());
  agents_.erase(fileDescriptor);
}
