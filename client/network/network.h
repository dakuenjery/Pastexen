#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QSslError>
#include <QSettings>
#include <QHostAddress>
#include <QHostInfo>
#include "defines.h"

class Network : public QObject
{
    Q_OBJECT
public:
    explicit Network(QObject *parent = nullptr);

    virtual bool isInited();
    virtual void init();
    virtual void deinit();
    virtual void upload(QString filename, QByteArray data, QString type) = 0;

signals:
    void connectionError();
    void linkReceived(const QString &link);
    void trayMessage(const QString &caption, const QString &text);
    void ready(); // dns was resolve
};
