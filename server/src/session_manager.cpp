#include "session_manager.hpp"

const std::unordered_map<int, AgentConnection>& SessionManager::getAgents() const {
  return agents_;
}

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

bool SessionManager::isDashboard() {
  return dashboardFd_ != -1 && agents_.find(dashboardFd_) != agents_.end();
}

void SessionManager::setDashboardFd(int fileDescriptor) {
  if (dashboardFd_ != -1 ) return; // set it once
  dashboardFd_ = fileDescriptor;
}

void SessionManager::resetDashboard() {
  if (dashboardFd_ == -1) return;
  deleteAgentTarget(dashboardFd_, getTargetByFd(dashboardFd_));
  dashboardFd_ = -1;
}

int SessionManager::getDashboardFd() { return dashboardFd_ ; }

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
