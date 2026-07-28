// repository_manager.hpp
#ifndef REPOSITORY_MANAGER_HPP
#define REPOSITORY_MANAGER_HPP

#include <memory>

#include "repository/agent_repository.hpp"
#include "repository/command_repository.hpp"
#include "repository/database_connection.hpp"

class RepositoryManager {
 public:
 private:
  std::unique_ptr<IDatabaseConnection> db_;
  
  RepositoryManager();
  explicit RepositoryManager(std::unique_ptr<IDatabaseConnection> db);
  // IAgentRepository& agents() { return *agents_; }
  // ICommandRepository& commands() { return *commands_; }
  std::unique_ptr<IAgentRepository> agents;
  std::unique_ptr<ICommandRepository> commands;
};

#endif