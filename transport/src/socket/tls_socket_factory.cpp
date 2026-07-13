#include "socket/tls_socket_factory.hpp"

#include "socket/socket_factory.hpp"
#include "socket/tls_socket.hpp"

std::unique_ptr<ISocket> TLSSocketFactory::createServer(
    const std::string& cert, const std::string& key) {
  // Step 1 — create the SSL_CTX, immediately wrap in unique_ptr
  std::unique_ptr<SSL_CTX, SSLContextDeleter> context(
      SSL_CTX_new(TLS_server_method()),
      SSLContextDeleter{});  // create context, server role

  // Step 2 — configure it
  SSL_CTX_use_certificate_file(context.get(), cert.c_str(),
                               SSL_FILETYPE_PEM);  // load server cert
  SSL_CTX_use_PrivateKey_file(context.get(), key.c_str(),
                              SSL_FILETYPE_PEM);  // load server key
  SSL_CTX_set_verify(context.get(), SSL_VERIFY_NONE,
                     nullptr);  // don't verify clients

  // Step 3 — create raw TCP socket
  auto raw = SocketFactory::createTCP();

  // Step 4 — wrap both into TLSSocket
  return std::make_unique<TLSSocket>(std::move(raw), std::move(context));
}

std::unique_ptr<ISocket> TLSSocketFactory::createClient(
    const std::string& caFile) {
  std::unique_ptr<SSL_CTX, SSLContextDeleter> context(
      SSL_CTX_new(TLS_client_method()), SSLContextDeleter{});

  // Step 2 — configure it

  SSL_CTX_load_verify_locations(context.get(), caFile.c_str(),
                                nullptr);  // load CA cert to trust
  SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER,
                     nullptr);  // verify server cert ← critical

  // Step 3 — create raw TCP socket
  auto raw = SocketFactory::createTCP();

  // Step 4 — wrap both into TLSSocket
  return std::make_unique<TLSSocket>(std::move(raw), std::move(context));
}
