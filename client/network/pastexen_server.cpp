#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QDate>
#include <QTime>
#include "network.h"
#include "utils.h"
#include "application.h"
#include "pastexen_server.h"

#include "../utils/udebug.h"

PastexenServer::PastexenServer(QObject *parent) :
    Network(parent)
{

}

void PastexenServer::init()
{
    if (!_serverAddr.isNull())
        return;

    _hostName = Application::GetHostName();
    _port = Application::GetPort();

    connect(&_socket, SIGNAL(readyRead()), SLOT(onDataReceived()));
    this->startTimer(30000);
    timerEvent(NULL);
}

void PastexenServer::lookedUp(const QHostInfo &host)
{
    if (host.error() != QHostInfo::NoError) {
        qDebug() << "Lookup failed:" << host.errorString();
        return;
    }
    _serverAddr = host.addresses().at(0);
    emit ready();
}

void PastexenServer::timerEvent(QTimerEvent *) {
    if (_serverAddr.isNull()) {
        QHostInfo::abortHostLookup(_lookupId);
        _lookupId = QHostInfo::lookupHost(_hostName,
                              this, SLOT(lookedUp(QHostInfo)));
    }
}


void PastexenServer::upload(QString filename, QByteArray data, QString type)
{
    if (_serverAddr.isNull()) {
        UDebug << "Unable to upload data: host not resolved";
        throw UException("Not connected - host not resolved");
        return; // If no internet connection
    }
    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        UDebug << "Error connecting to server!";
        throw UException("Sending previous request");
        return;
    }

    UDebug << "Server addr: " << _serverAddr.toString() << ":" << _port;
    _socket.connectToHost(_serverAddr, _port);
    _socket.waitForConnected(4000);

    QByteArray arr;
    arr.append("proto=pastexen\n");
    arr.append("version=" + APP_RELEASE + "\n");
    arr.append("uuid=");
    QString uuid = Application::settings().GetParameter("uuid");

    UDebug << Q_FUNC_INFO << " uuid = " << uuid;

    Q_ASSERT(uuid.length() == 24 * 2);
    arr.append(uuid.toLocal8Bit());
    arr.append("\n");
    arr.append("type=" + type + "\n");
    arr.append(QString("size=%1\n\n").arg(data.size()));
    arr.append(data);
    _socket.write(arr);
}

QByteArray PastexenServer::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error opening temporary file!";
        exit(1);
    }
    return file.readAll();
}

void PastexenServer::onDataReceived()
{
    const QByteArray arr = _socket.readAll();
    const QString link = getValue(arr, "url");
    emit linkReceived(link);
    _socket.disconnectFromHost();
}
