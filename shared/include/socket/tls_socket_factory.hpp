#include "i_socket.hpp"
#include <memory>

class TLSSocketFactory {
public:
    static std::unique_ptr<ISocket> createServer(const std::string& cert, const std::string& key);
    static std::unique_ptr<ISocket> createClient(const std::string& caFile);
};