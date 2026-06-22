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
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);


    m_heartbeatCount = 0;
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        m_heartbeatCount++;
        if (m_heartbeatCount >= 3)
        {
            qWarning() << "Timeout Heartbeat. Disconnect client.";
            m_socket->abort();
        }
    });
    m_heartbeatTimer->start(5000);


    QString connName = "db_conn_" + QString::number((quintptr)QThread::currentThreadId());
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName("users.db");
    if(!initDatabase())
    {
        json res;
        res["status"] = "error";
        res["message"] = "database unavailable";
        sendResponse(res);
        m_socket->disconnectFromHost();
        return;
    }
}

void ClientHandler::onReadyRead()
{
    m_heartbeatCount = 0;
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
                json errRes;
                errRes["status"] = "error";
                errRes["message"] = "Invalid JSON structure";
                sendResponse(errRes);
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
    else if(action == "delete_user")
    {
        handleDeleteUser(j);
    }
    else
    {
        json res;
        res["status"] = "error";
        res["message"] = "Unknown network command";
        sendResponse(res);
    }

}

void ClientHandler::handleAddUser(const json &j)
{
    json res;
    std::string username = j.value("username", "");
    std::string email = j.value("email", "");

    if(username.empty() || email.empty())
    {
        res["status"] = "error";
        res["message"] = "Fields cannot be empty!";
        sendResponse(res);
        return;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, email) VALUES (:username, :email)");
    query.bindValue(":username", QString::fromStdString(username));
    query.bindValue(":email", QString::fromStdString(email));

    if(query.exec())
    {
        res["status"] = "success";
        res["message"] = "User added successfully";
    }
    else
    {
        res["status"] = "error";
        if (query.lastError().text().contains("UNIQUE constraint failed"))
            res["message"] = "Пользователь с таким email уже существует";
        else
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

void ClientHandler::handleDeleteUser(const json &j)
{
    json res;
    std::string username = j.value("username", "");
    std::string email = j.value("email", "");

    if(username.empty() || email.empty())
    {
        res["status"] = "error";
        res["message"] = "Fields cannot be empty!";
        sendResponse(res);
        return;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM users WHERE username = :username AND email = :email");
    query.bindValue(":username", QString::fromStdString(username));
    query.bindValue(":email", QString::fromStdString(email));

    if (query.exec()) {
        if (query.numRowsAffected() > 0)
        {
            res["status"] = "success";
            res["message"] = "Пользователь успешно удален из базы данных";
        }
        else
        {
            res["status"] = "error";
            res["message"] = "Пользователь с таким именем и email не найден!";
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

bool ClientHandler::initDatabase()
{
    if(!m_db.open())
    {
        qDebug() << "Connection error for database: " << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    if(!query.exec("CREATE TABLE IF NOT EXISTS users("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "username TEXT NOT NULL,"
                   "email TEXT NOT NULL UNIQUE)"))
    {
        qCritical() << "Error creating table: " << query.lastError().text();
        return false;
    }
    return true;
}

