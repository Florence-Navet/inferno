#ifndef SERVERCLIENT_H
#define SERVERCLIENT_H
#include "protocol/lptf_protocol.hpp"


#include <QObject>
#include <memory>

class DashboardSession;
class QSocketNotifier;


/// Owns the TCP connection to the Inferno server.
class ServerClient : public QObject
{
    Q_OBJECT

public:
    explicit ServerClient(QObject *parent = nullptr);
    ~ServerClient();

    /// Connects to the server and starts listening for frames.
    bool connectToServer(const QString &host, quint16 port);

private:
    std::unique_ptr<DashboardSession> m_session;
    QSocketNotifier *m_notifier = nullptr;
    void onReadyRead();
    void sendRegister();
    void handleFrame(const Frame &frame);
    void handleData(const std::vector<std::uint8_t> &payload);
};


#endif // SERVERCLIENT_H
