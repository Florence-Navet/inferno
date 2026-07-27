#ifndef SERVERCLIENT_H
#define SERVERCLIENT_H

#include <QObject>

class QTcpSocket;

/// Owns the TCP connection to the Inferno server.
class ServerClient : public QObject
{
    Q_OBJECT
public:
    explicit ServerClient(QObject *parent = nullptr);

    /// Starts an asynchronous connection attempt to the server.
    void connectToServer(const QString &host, quint16 port);

private:
    QTcpSocket *m_socket = nullptr;
};



#endif // SERVERCLIENT_H
