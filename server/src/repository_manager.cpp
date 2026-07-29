// repository_manager.cpp
#include "repository_manager.hpp"

RepositoryManager::RepositoryManager()
    : db_(std::make_unique<DatabaseConnection>()),
      agents_(std::make_unique<AgentRepository>(*db_)),
      commands_(std::make_unique<CommandRepository>(*db_)) {}

RepositoryManager::RepositoryManager(
    std::unique_ptr<IDatabaseConnection> db,
    std::unique_ptr<IAgentRepository> agents,
    std::unique_ptr<ICommandRepository> commands)
    : db_(std::move(db)),
      agents_(std::move(agents)),
      commands_(std::move(commands)) {}


// RepositoryManager::RepositoryManager(std::unique_ptr<IDatabaseConnection> db)
//     : db_(std::move(db)),
//       agents_(std::make_unique<AgentRepository>(*db_)),
//       commands_(std::make_unique<CommandRepository>(*db_)) {}