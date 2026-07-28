#include "serverclient.h"
#include "dashboardsession.h"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "codec/protocol_parser.hpp"


#include "socket/socket_factory.hpp"
#include <QSocketNotifier>

#include <QDebug>
#include <QHostInfo>

ServerClient::ServerClient(QObject *parent)
    : QObject{parent}
{
}

ServerClient::~ServerClient() = default;

bool ServerClient::connectToServer(const QString &host, quint16 port)
{
    std::unique_ptr<ISocket> socket = SocketFactory::createTCP();

    if (!socket->connect(host.toStdString(), port)) {
        qDebug() << "connection failed";
        return false;
    }

    socket->setNonBlocking(true);
    m_session = std::make_unique<DashboardSession>(std::move(socket));

    qDebug() << "connected to server";
    m_notifier = new QSocketNotifier(m_session->getFd(), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &ServerClient::onReadyRead);

    sendRegister();
    return true;
}

void ServerClient::onReadyRead()
{
   // qDebug() << "data available";
    const SocketResult result = m_session->receiveIntoBuffer();


    //nothing to read for this moment
   if (!result.ok() && !result.wouldBlock()) {
       qDebug() << "receive error";
       return;
   }

   // TODO: result.ok() with 0 bytes means the server closed the connection.

   while (std::optional<Frame> frame = m_session->tryExtractFrame()) {
       handleFrame(*frame);
   }
}



void ServerClient::sendRegister()
{
    OsInfoPayload info;
    info.os_type = OSType::WINDOWS;
    info.arch = ArchType::X64;
    info.hostname = QHostInfo::localHostName().toStdString();
    info.current_user = qgetenv("USERNAME").toStdString();
    info.os_version = "Windows";
        // TODO: use the real local IP (QNetworkInterface).
    info.ip = "127.0.0.1";
    // The server uses this field as our id (setId(mac)).
    // TODO: replace with the real MAC address.
    info.mac = "dashboard";

    try
    {
          const std::vector<std::uint8_t> payload =
        ProtocolSerializer::serializeOsInfoPayload(info);

            const Frame frame{
                      ProtocolHelper::createHeader(MessageType::DASHBOARD_REGISTER, payload),
                      payload};
                
             m_session->sendFrame(frame);
    }
    catch(const std::exception& e)
    {
        // std::cerr << e.what() << '\n';
        qDebug() << "register failed:" << e.what();
    }
    
}

void ServerClient::handleFrame(const Frame &frame)
{
    switch (frame.header.type) {
    case MessageType::DATA:
        //qDebug() << "DATA received," << frame.payload.size() << "bytes";
        handleData(frame.payload);
        break;
    case MessageType::RESPONSE:
        qDebug() << "RESPONSE received";
        break;
    case MessageType::ERROR:
        qDebug() << "ERROR received";
        break;
    default:
        qDebug() << "unhandled type" << static_cast<int>(frame.header.type);
    }
}

void ServerClient::handleData(const std::vector<std::uint8_t> &payload)
{
    const DataPayload data = ProtocolParser::parseDataPayload(payload);

    switch (data.subtype) {
    case DataType::AGENTS:
        qDebug() << "AGENTS list," << data.data.size() << "bytes";
        break;
    case DataType::REGISTRATION:
        qDebug() << "one agent registered," << data.data.size() << "bytes";
        break;
    case DataType::METRICS_SAMPLE:
        qDebug() << "metrics sample";
        break;
    default:
        qDebug() << "unhandled subtype" << static_cast<int>(data.subtype);
    }
}



