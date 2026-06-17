#include "server.h"
#include "clienthandler.h"

#include <QThread>
#include <QDebug>

Server::Server(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

bool Server::startServer(quint16 port)
{
    if(!m_server->listen(QHostAddress::Any, port))
    {
        qCritical() << "Server could not start: " << m_server->errorString();
        return false;
    }
    qDebug() << "Server started on port: " << port;
    return true;
}

void Server::onNewConnection()
{
    QTcpSocket* socket = m_server->nextPendingConnection();
    if (!socket) return;

    socket->setParent(nullptr);

    QThread* thread = new QThread();
    ClientHandler* handler = new ClientHandler(socket);
    socket->moveToThread(thread);
    handler->moveToThread(thread);

    connect(thread, &QThread::started, handler, &ClientHandler::startProcessing);
    connect(handler, &ClientHandler::finished, thread, &QThread::quit);
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
    qDebug() << "New client connected. Thread started.";
}
