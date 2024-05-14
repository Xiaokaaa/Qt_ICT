#ifndef UDPWORKER_H
#define UDPWORKER_H

#include <QObject>
#include <QUdpSocket>
#include <QByteArray>
#include <QList>
#include <QDebug>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDateTime>
#include <QEventLoop>

class UdpWorker:public QObject
{
    Q_OBJECT
public:
    UdpWorker(quint16 targetPort,quint16 sourcePort,QHostAddress *targetIPv4Address,QHostAddress *sourceIPv4Address,
              QUdpSocket *udpSocket,QList<QByteArray> *byteArrayList);
    virtual ~UdpWorker();

    QTimer *timer = new QTimer();

    void setDatagramNum(const quint32 &value);

    qint64 getAveT() const;

    quint32 getDatagramNum() const;

public slots:
    void sharkHands();
    void sendData(qint64);
    //void send();
private slots:
    void tryAgain();
    void timeSynchronize();
    //bool getOneTimeDifference();
    void readPendingDatagrams();
    void sendClientT();

signals:
    void readyToSendData();
    void noClientRespond();
    void timeSynchronizeReady();
    void getOneTimeDifFail();
    void newClientT2T3();
    void timeSynchronizeSuccess();
    void timeSynchronizeFail();
    void clientUnreachable();
    void aveTUnreachable();
    void datagramReachClient(quint32,qint64);
    void datagramdReachOutTime();
    void datagramSendCompleted();

private:
    quint16 targetPort;
    quint16 sourcePort;
    QHostAddress *targetIPv4Address;
    QHostAddress *sourceIPv4Address;
    //QFileInfo *sendFileInfo;
    QUdpSocket *udpSocket;
    //quint64 sendFileSize;
    //QFile *file;
    QList<QByteArray> *byteArrayList;
    //QByteArray totalFileArray;
    quint32 datagramNum;

    bool readOrNot;
    bool isSharkHands;
    bool isTimeSynchronize;

    //时间同步用到的参数
    qint8 num = 0,tryNum = 0;
    qlonglong sum =0 ;
    qint64 ave = 0,max = 0, min = 0;
    qint64 t1,t4;
    bool newT2T3;
    qint64 t2,t3,T,aveT;
    bool isSendT;
    quint8 numOfSendAveT;

    quint32 nextSendNum;  //下一个要发送的数据报序号，序号从0开始
    qint64 totalDelay;
     QList<quint32> datagramNumList,datagramFailedList;
};

#endif // UDPWORKER_H
