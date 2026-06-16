#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <json.hpp>

using json = nlohmann::json;

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientHandler();
public slots:
    void startProcessing();
    void onReadyRead();
    void onDisconnected();
signals:
    void finished();
private:
    QTcpSocket *m_socket;
    QSqlDatabase m_db;
    QByteArray m_buffer;

    void processJson(const json& j);
    void handleAddUser(const json& j);
    void handleGetAllUsers();
    void sendResponse(const json& j);
    void initDatabase();
};

#endif // CLIENTHANDLER_H
