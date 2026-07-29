#ifndef SERVERCLIENT_H
#define SERVERCLIENT_H
#include "protocol/lptf_protocol.hpp"
#include <QHash>

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
    void sendCommand(const QString &target, CommandType type, const QString &data);

signals:
     void agentReceived(const QString &id, const QString &name, const QString &details);

private:
    std::unique_ptr<DashboardSession> m_session;
    QSocketNotifier *m_notifier = nullptr;
    std::uint32_t m_nextCommandId = 0;
    QHash<std::uint32_t, CommandType> m_pendingCommands;
    void onReadyRead();
    void sendRegister();
    void handleFrame(const Frame &frame);
    void handleData(const std::vector<std::uint8_t> &payload);
};


#endif // SERVERCLIENT_H
