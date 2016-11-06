#pragma once

#include <QObject>
#include "network.h"

class NetworkManager : public Network
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);

    void init() override;
    void upload(QString filename, QByteArray data, QString type) override;

    QStringList getAvailableNetworks();
    bool setNetwork(QString networkName);

private:
    QScopedPointer<Network> network;
    QMap<QString, Network*> networks;
};
