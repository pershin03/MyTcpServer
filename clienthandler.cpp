#include "clienthandler.h"
#include <QThread>


ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket)
{
    QString connName = "db_conn_" + QString::number((quintptr)QThread::currentThreadId());
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName("users.db");
}

ClientHandler::~ClientHandler()
{
    if(m_socket) m_socket->deleteLater();
    QString connName = m_db.connectionName();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
}

void ClientHandler::startProcessing()
{

}

void ClientHandler::onReadyRead()
{

}

void ClientHandler::onDisconnected()
{

}
