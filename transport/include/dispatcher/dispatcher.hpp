#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <iostream>
#include <string>

#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "dispatcher/i_dispatcher.hpp"
#include "exception/lptf_exception.hpp"
#include "exception/socket_exception.hpp"
#include "frame_transport.hpp"
#include "protocol/lptf_protocol.hpp"
#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"

class Dispatcher : public IDispatcher {
 public:
  Dispatcher() {}
  virtual ~Dispatcher() = default;

  // Must be overriden by children
  // virtual void handleFrame(FrameTransport& agent, const Frame& frame) = 0;

  // ── Incoming message handlers ───────────────────────
  void onError(const std::vector<std::uint8_t>& payload);

  // ── Outgoing message builders ───────────────────────
  void sendError(FrameTransport& agent, ErrorType code,
                 const std::string& msg) override;

  //  protected:
  //   Logger logger_;
};

#endif