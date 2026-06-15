#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QObject *parent = nullptr);

signals:
};

#endif // CLIENTHANDLER_H
