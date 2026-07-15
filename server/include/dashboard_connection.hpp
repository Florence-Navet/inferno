#ifndef DASHBOARD_CONNECTION_HPP
#define DASHBOARD_CONNECTION_HPP

#include <memory>
#include <stdexcept>

#include "codec/protocol_parser.hpp"
#include "frame_transport.hpp"
#include "logger.hpp"
#include "protocol/lptf_protocol.hpp"
#include "socket/i_socket.hpp"

class DashboardConnection : public FrameTransport {
 public:
  explicit DashboardConnection() {};
  explicit DashboardConnection(std::unique_ptr<ISocket> sock)
      : FrameTransport(std::move(sock)) {}
  DashboardConnection(std::nullptr_t) = delete;

  DashboardConnection(const DashboardConnection&) = delete;
  DashboardConnection& operator=(const DashboardConnection&) = delete;
  DashboardConnection(DashboardConnection&&) = default;

  // ===== Register payload related methods =====
  const OsInfoPayload& getAgentInfo() const override { return agentInfo_; };
  void setAgentInfo(const OsInfoPayload& info) override { agentInfo_ = info; };

  // ===== Register and registration state related method =====
  bool getIsRegistered() const { return isRegistered_; }

  // ===== Buffer related methods =====

 private:
  bool isRegistered_ = false;
};

#endif