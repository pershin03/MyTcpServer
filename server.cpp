#include "server.h"
#include "clienthandler.h"

#include <QThread>

Server::Server(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

bool Server::startServer(quint16 port)
{
    if(!m_server->listen(QHostAddress::Any, port))
    {
        //не удалось запустить сервер
        return false;
    }
    //удалось запустить сервер
    return true;
}

void Server::onNewConnection()
{
    QTcpSocket* socket = m_server->nextPendingConnection();

    QThread* thread = new QThread();
    ClientHandler* handler = new ClientHandler(socket);
    handler->moveToThread(thread);

    //    connects

    thread->start();
}
