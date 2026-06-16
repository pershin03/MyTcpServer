#include "clienthandler.h"
#include <QThread>
#include <QSqlError>
#include <QSqlQuery>

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
    initDatabase();
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    while(m_buffer.contains('\n'))
    {
        int separatorIndex = 0;
        QByteArray command = m_buffer.left(separatorIndex);
        m_buffer.remove(0, separatorIndex + 1);

        if(!command.isEmpty())
        {
            try
            {
                json j = json::parse(command.begin(), command.end());
                processJson(j);
            }
            catch(const json::parse_error& e)
            {
                qWarning() << "JSON parse error: " << e.what();
            }
        }

    }
}

void ClientHandler::onDisconnected()
{
    emit finished();
}

void ClientHandler::processJson(const json &j)
{
    std::string action = j.value("action", "");

    if(action == "add_user")
    {
        handleAddUser(j);
    }
    else if(action == "get_users")
    {
        handleGetAllUsers();
    }

}

void ClientHandler::handleAddUser(const json &j)
{
    //work with database
}

void ClientHandler::handleGetAllUsers()
{

}

void ClientHandler::sendResponse(const json &j)
{

}

void ClientHandler::initDatabase()
{
    if(!m_db.open())
    {
        qDebug() << "Connection error for database: " << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    query.exec("CREATE TABLE IF NOT EXISTS users("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "username TEXT NOT NULL,"
               "email TEXT NOT NULL");
}
