#include "serverclient.h"
#include <QTcpSocket>

ServerClient::ServerClient(QObject *parent)
    : QObject{parent}
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, []() {
        qDebug() << "connected to server";
    });

    connect(m_socket, &QTcpSocket::errorOccurred, this, [](QAbstractSocket::SocketError e) {
        qDebug() << "socket error:" << e;
    });
}

void ServerClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

