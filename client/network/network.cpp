#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QDate>
#include <QTime>
#include "network.h"
#include "utils.h"
#include "application.h"

#include "../utils/udebug.h"

Network::Network(QObject *parent) :
    QObject(parent)
{
}

bool Network::isInited()
{
    return true;
}

void Network::init()
{

}

void Network::deinit()
{

}
