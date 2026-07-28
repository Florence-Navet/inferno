#include "env_helper.hpp"

#include <algorithm>
#include <cctype>

std::uint16_t EnvHelper::resolvePort(const std::string& portName) {
  // const char* portEnv = std::getenv("SERVER_PORT");
  const char* portEnv = std::getenv(portName.c_str());
  if (!portEnv) {
    std::cout << "SERVER_PORT not found in .env" << SERVER_PORT << '\n';
    return SERVER_PORT;
  }

  try {
    const int parsed = std::stoi(portEnv);
    if (parsed > 0 && parsed <= 65535) {
      std::cout << "Resolved server port from environment: " << parsed << '\n';
      return static_cast<std::uint16_t>(parsed);
    }
  } catch (...) {
  }
  std::cout << "Invalid SERVER_PORT value in .env: " << portEnv
            << ", using default " << SERVER_PORT << '\n';
  return SERVER_PORT;
}

std::string EnvHelper::resolveServerHost() {
  const char* hostEnv = std::getenv("SERVER_HOST");
  if (hostEnv && hostEnv[0] != '\0') {
    std::cout << "Resolved server host from environment: " << hostEnv << '\n';
    return std::string(hostEnv);
  }
  return "server";
}

std::string EnvHelper::resolveString(const std::string& variableName) {
  const char* envVariable = std::getenv(variableName.c_str());
  if (envVariable && envVariable[0] != '\0') {
    std::cout << "Resolved " << variableName
              << " from environment: " << envVariable << '\n';
    return std::string(envVariable);
  }
  return "";
}

// std::string EnvHelper::resolveDbName() {
//   const char* dbNameEnv = std::getenv("POSTGRES_DB");
//   if (dbNameEnv && dbNameEnv[0] != '\0') {
//     std::cout << "Resolved database name from environment: " << dbNameEnv
//               << '\n';
//     return std::string(dbNameEnv);
//   }
//   return "inferno-db";
// }

// std::string EnvHelper::resolveDbUser() {
//   const char* dbUser = std::getenv("POSTGRES_USER");
//   if (dbUser && dbUser[0] != '\0') {
//     std::cout << "Resolved database name from environment: " << dbUser <<
//     '\n'; return std::string(dbUser);
//   }
//   return "timescale-user";
// }

// std::string EnvHelper::resolveDbPassword() {
//   const char* dbPassword = std::getenv("POSTGRES_PASSWORD");
//   if (dbPassword && dbPassword[0] != '\0') {
//     std::cout << "Resolved database name from environment: " << dbPassword
//               << '\n';
//     return std::string(dbPassword);
//   }
//   return "timescale-password";
// }

bool EnvHelper::resolveTlsEnabled() {
  const char* tlsEnv = std::getenv("TLS");
  if (tlsEnv == nullptr) {
    return false;  // default value
  }

  std::string value(tlsEnv);

  std::transform(value.begin(), value.end(), value.begin(), ::tolower);

  return value == "true" || value == "1" || value == "yes" || value == "on";
}