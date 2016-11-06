#include "networkmanager.h"
#include "pastexen_server.h"

NetworkManager::NetworkManager(QObject *parent) :
    Network(parent)
{

}

void NetworkManager::upload(QString filename, QByteArray data, QString type)
{
    if (!network.isNull() && network->isInited()) {
        network->upload(filename, data, type);
    } else {
        emit connectionError();
    }
}

void NetworkManager::init()
{
    networks.insert("pastexen", new PastexenServer(this));
    setNetwork("pastexen");
}

QStringList NetworkManager::getAvailableNetworks()
{
    QStringList list;

    foreach (QString key , networks.keys())
        list << key;

    return list;
}

bool NetworkManager::setNetwork(QString networkName)
{
    if (networks.contains(networkName)) {
        if (!network.isNull()) {
            disconnect(network.data(), &Network::connectionError, this, &Network::connectionError);
            disconnect(network.data(), &Network::linkReceived, this, &Network::linkReceived);
            disconnect(network.data(), &Network::trayMessage, this, &Network::trayMessage);
            disconnect(network.data(), &Network::ready, this, &Network::ready);
        }

        network.reset(networks[networkName]);

        connect(network.data(), &Network::connectionError, this, &Network::connectionError);
        connect(network.data(), &Network::linkReceived, this, &Network::linkReceived);
        connect(network.data(), &Network::trayMessage, this, &Network::trayMessage);
        connect(network.data(), &Network::ready, this, &Network::ready);

        network->init();

        return true;
    }

    return false;
}
