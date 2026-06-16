#include "clienthandler.h"
#include <QThread>
#include <QSqlError>
#include <QSqlQuery>

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket)
{
}

ClientHandler::~ClientHandler()
{
    QString connName = m_db.connectionName();
    if (m_db.isOpen()) m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connName);
    qDebug() << "Thread cleaned. Connection " << connName << " deleted.";
}

void ClientHandler::startProcessing()
{
    connect(m_socket, &QTcpSocket::disconnected, &QTcpSocket::deleteLater);
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);

    QString connName = "db_conn_" + QString::number((quintptr)QThread::currentThreadId());
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName("users.db");
    initDatabase();
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    while(m_buffer.contains('\n'))
    {
        int separatorIndex = m_buffer.indexOf('\n');
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
    json res;
    std::string username = j.value("username", "");
    std::string email = j.value("email", "");

    // возможно добавить проверку на пустые строки.

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, email) VALUES (:name, :email)");
    query.bindValue(":name", QString::fromStdString(username));
    query.bindValue(":email", QString::fromStdString(username));

    if(query.exec())
    {
        res["status"] = "success";
        res["message"] = "User added successfully";
    }
    else
    {
        res["status"] = "error";
        res["message"] = query.lastError().text().toStdString();
    }
    sendResponse(res);
}

void ClientHandler::handleGetAllUsers()
{
    json res;
    QSqlQuery query(m_db);

    if (query.exec("SELECT id, username, email FROM users"))
    {
        res["status"] = "success";
        res["users"] = json::array();
        while (query.next()) {
            json user;
            user["id"] = query.value(0).toInt();
            user["username"] = query.value(1).toString().toStdString();
            user["email"] = query.value(2).toString().toStdString();
            res["users"].push_back(user);
        }
    }
    else
    {
        res["status"] = "error";
        res["message"] = query.lastError().text().toStdString();
    }
    sendResponse(res);
}

void ClientHandler::sendResponse(const json &j)
{
    if (m_socket && m_socket->isOpen())
    {
        std::string responseStr = j.dump() + "\n";
        m_socket->write(responseStr.c_str(), responseStr.length());
        m_socket->flush();
    }
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
               "email TEXT NOT NULL UNIQUE");
}
