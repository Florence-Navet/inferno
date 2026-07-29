#ifndef FAKE_DATABASE_CONNECTION_HPP
#define FAKE_DATABASE_CONNECTION_HPP

#include <iostream>

#include "repository/database_connection.hpp"

class FakeDatabaseConnection : public IDatabaseConnection {
 public:
  explicit FakeDatabaseConnection() {};
  explicit FakeDatabaseConnection(std::string_view password,
                                  std::string_view user,
                                  std::string_view dbName,
                                  std::string_view dbHost,
                                  std::string_view dbPort) {};

  // Low-level query execution — return raw results
  pqxx::result execute(const std::string& query) override {
    std::cout << query << "\n";
    return pqxx::result();  // Empty result
  };
  pqxx::result executeParams(const std::string& query,
                             const pqxx::params& params) override {
    std::cout << query << "\n";
    return pqxx::result();  // Empty result
  };
};

#endif