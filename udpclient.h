#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QByteArray>
#include <QDebug>

#include "udpworker.h"

class UdpClient : public QObject
{
    Q_OBJECT

public:
    UdpClient(QString &souIPv4Address,quint16 souPort);
    virtual ~UdpClient();

    //UdpWorker *udpWoker;
    bool udpBind();

    quint32 getDatagramNum() const;

    qint64 getAveT() const;

    quint32 getReceivedDatagramNumber() const;

    void setReadOrNot(bool value);

    QHostAddress getTargetIPv4Address() const;

    quint16 getTargetPort() const;

    QList<quint32> getDatagramNumList() const;

    qint64 getTotalDelay() const;

    void setIsReceiving(bool value);

public slots:
    void readyToRead();
    void udpSocketAbort();

private slots:
    void clientResponse();

signals:
    void severAsk();
    void severGetTimestamp();
    void getTimerDifference();
    void receiveDatagram(quint32,qint64);
    void datagramSendCompleted();

private:
    QHostAddress targetIPv4Address;
    QHostAddress sourceIPv4Address;
    quint16 targetPort;
    quint16 sourcePort;
    QUdpSocket *udpSocket;
    quint32 datagramNum;
    QList<QByteArray> byteArrayList;
    qint64 aveT;
    bool readOrNot;
    bool isTimeSynchronize;
    bool isReceiving;
    quint32 receivedDatagramNumber;
    QList<quint32> datagramNumList;
    qint64 totalDelay;

};

#endif // UDPCLIENT_H
