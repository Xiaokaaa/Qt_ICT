#ifndef UDPDATATRANSMISSION_H
#define UDPDATATRANSMISSION_H

#include <QObject>
#include <QUdpSocket>
#include <QByteArray>
#include <QFileInfo>
#include <QList>
#include <QFile>
#include <QDebug>
//#include <QThread>

#include "udpworker.h"
//#include "transmissionwindow.h"

class UdpDataTransmission : public QObject
{
    Q_OBJECT
public:
    UdpDataTransmission(QString &targetIPv4Address,quint16 targetPor,
                        QString &sourceIPv4Address,quint16 sourcePort,
                        QFileInfo *sendFileInfo);
    virtual ~UdpDataTransmission();

    static const quint16 MAX_DATAGRAM_SIZE = 500; //一次可以发送UDP数据报的最大大小，512-4-8=500byte，4byte保存序号，8byte保存时间戳
    UdpWorker *udpWorker;
    QList<QByteArray> getByteArrayList() const;

    bool udpBind();
    quint8 setDatagramNum(const quint32 &value);
    void shakeHands();

public slots:
    //void readPendingDatagrams();
    //void handleResults();
    void udpSocketAbort();
signals:
    //void newInfomation(QString);
    //void readyToSend();
    void startSharkHands();

private:
    quint16 targetPort;
    quint16 sourcePort;
    QHostAddress targetIPv4Address;
    QHostAddress sourceIPv4Address;
    QFileInfo *sendFileInfo;
    QUdpSocket *udpSocket;
    quint64 sendFileSize;
    QFile *file;
    QList<QByteArray> byteArrayList;
    QByteArray totalFileArray;
    quint32 datagramNum;
    //QThread udpWorkerThread;

    quint8 cutArray();
};

#endif // UDPDATATRANSMISSION_H
