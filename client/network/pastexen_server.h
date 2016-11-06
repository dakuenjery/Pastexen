#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QSslError>
#include <QSettings>
#include <QHostAddress>
#include <QHostInfo>
#include "network.h"
#include "defines.h"

class PastexenServer : public Network
{
    Q_OBJECT
public:
    explicit PastexenServer(QObject *parent = nullptr);

    void init() override;
    void upload(QString filename, QByteArray data, QString type) override;

    const QTcpSocket& socket() const {
        return _socket;
    }

private slots:
    void onDataReceived();
    void lookedUp(const QHostInfo &host);

protected:
    QByteArray readFile(const QString& fileName);
    void timerEvent(QTimerEvent *) override;

private:
    QString _hostName = "";
    quint16 _port = 0;
    bool _ready;
    QTcpSocket _socket;
    QHostAddress _serverAddr;
    int _lookupId;
};
