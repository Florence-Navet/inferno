// repository_manager.cpp
#include "repository_manager.hpp"

RepositoryManager::RepositoryManager()
    : db_(std::make_unique<DatabaseConnection>()),
      agents(std::make_unique<AgentRepository>(*db_)),
      commands(std::make_unique<CommandRepository>(*db_)) {}

RepositoryManager::RepositoryManager(std::unique_ptr<IDatabaseConnection> db)
    : db_(std::move(db)),
      agents(std::make_unique<AgentRepository>(*db_)),
      commands(std::make_unique<CommandRepository>(*db_)) {}