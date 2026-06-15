#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>

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
};

#endif // CLIENTHANDLER_H
