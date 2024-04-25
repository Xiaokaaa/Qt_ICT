#include "udpworker.h"

UdpWorker::UdpWorker(quint16 tarPort, quint16 souPort, QHostAddress *tarIPv4Address, QHostAddress *souIPv4Address,
                     QUdpSocket *udpSoc, QList<QByteArray> *list)
{
    targetPort = tarPort;
    sourcePort = souPort;
    targetIPv4Address = tarIPv4Address;
    sourceIPv4Address = souIPv4Address;
    udpSocket = udpSoc;
    byteArrayList = list;
    readOrNot = false;
    newT2T3 = false;
    isSharkHands = false;
    isTimeSynchronize = false;
    isSendT = false;
    totalDelay = 0;
}

UdpWorker::~UdpWorker()
{
    if(udpSocket != nullptr)
    {
        delete udpSocket;
        udpSocket = nullptr;
    }
    if(targetIPv4Address != nullptr)
    {
        delete targetIPv4Address;
        targetIPv4Address = nullptr;
    }
    if(sourceIPv4Address != nullptr)
    {
        delete sourceIPv4Address;
        sourceIPv4Address = nullptr;
    }
    if(timer != nullptr)
    {
        delete timer;
        timer = nullptr;
    }
    byteArrayList->clear();
}

void UdpWorker::sharkHands()
{
    QString str = QString("DatagramNum:");//.arg(datagramNum);
    QByteArray ba =  str.toLatin1(),quintArray;
    quintArray.resize(5);
    memcpy(quintArray.data(),&datagramNum,4);
    ba.append(quintArray);
    udpSocket->writeDatagram(ba.data(),ba.size(),*targetIPv4Address,targetPort);

    readOrNot = true;
    connect(timer,&QTimer::timeout,this,&UdpWorker::tryAgain,Qt::UniqueConnection);
    timer->start(4000);
    connect(udpSocket,&QUdpSocket::readyRead,this,&UdpWorker::readPendingDatagrams,Qt::UniqueConnection);
    connect(this,SIGNAL(timeSynchronizeReady()),this,SLOT(timeSynchronize()),Qt::UniqueConnection);
    connect(this,SIGNAL(timeSynchronizeSuccess()),this,SLOT(sendClientT()),Qt::UniqueConnection);
    tryNum = 0;
    numOfSendAveT = 0;
    nextSendNum = 0;
}

void UdpWorker::tryAgain()
{
    readOrNot = false;  //停止接收新数据
    timer->stop();
    disconnect(timer,SIGNAL(timeout()),nullptr,nullptr);

    QString str = QString("DatagramNum:");//.arg(datagramNum);
    QByteArray ba =  str.toLatin1(),quintArray;
    quintArray.resize(5);
    memcpy(quintArray.data(),&datagramNum,4);
    ba.append(quintArray);
    udpSocket->writeDatagram(ba.data(),ba.size(),*targetIPv4Address,targetPort);

    readOrNot = true;
    connect(timer,&QTimer::timeout,[=](){
        timer->stop();
        readOrNot = false;  //停止接收新数据
        disconnect(timer,SIGNAL(timeout()),nullptr,nullptr);
        emit noClientRespond();
    });
    timer->start(4000);
}

/**
 * @brief 事件会触发10次该槽函数，尝试10次获取服务器与客户端的时间戳差，将成功获取的时间差求和再求平均值。
 * 如果成功获取的时间戳差数量大于等于4，则会去掉最大值和最小值再求平均。
 *
 */
void UdpWorker::timeSynchronize()
{
    if(newT2T3)
    {
        newT2T3 = true;
        t4 = QDateTime::currentMSecsSinceEpoch();
        T = ((t2 - t1) + (t3 - t4)) / 2;  //T = Client - Server
        num++;
        sum += T;
        if(num==1)
        {
            max = T;
            min = T;
        }else
        {
            if(T > max) max = T;
            if(T < min) min = T;
        }
    }
    if(tryNum == 0)
    {
        isSharkHands = true;
        disconnect(this,SIGNAL(timeSynchronizeReady()),this,SLOT(timeSynchronize()));
        connect(this,SIGNAL(newClientT2T3()),this,SLOT(timeSynchronize()),Qt::UniqueConnection);
        connect(this,SIGNAL(getOneTimeDifFail()),this,SLOT(timeSynchronize()),Qt::UniqueConnection);
    }
    if(tryNum < 10)
    {
        t1 = QDateTime::currentMSecsSinceEpoch();
        udpSocket->writeDatagram("Get timestamp.",14,*targetIPv4Address,targetPort);
        readOrNot = true;
        timer->start(4000);
        connect(timer,&QTimer::timeout,[=](){
            timer->stop();
            disconnect(timer,SIGNAL(timeout()),nullptr,nullptr);
            readOrNot = false;  //停止接收新数据
            emit getOneTimeDifFail();
        });
    }else if(tryNum == 10)
    {
        disconnect(this,SIGNAL(newClientT2T3()),this,SLOT(timeSynchronize()));
        disconnect(this,SIGNAL(getOneTimeDifFail()),this,SLOT(timeSynchronize()));
        tryNum = 0;
        if(num == 0)
        {
            emit timeSynchronizeFail();
            return ;
        }else
        {
            if(num >= 4)
                ave = (sum - min - max) / (num - 2);  //去掉一个最大值一个最小值
            else
                ave = sum / num;
            aveT = ave;
            emit timeSynchronizeSuccess();
            qDebug("Time synchronize success! aveT = %lld",aveT);
            isTimeSynchronize = true;
            return;
        }
    }
    tryNum++;
}
/**
 * @brief 在readOrNot为true的情况下，这个函数会处理接收到的UDP数据报。
 */
void UdpWorker::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
             QNetworkDatagram datagram = udpSocket->receiveDatagram(512);
             qDebug("----------------New message-------------------");
             if(readOrNot == true && !datagram.isNull())
             {
                 if(isSharkHands == false && strcmp(datagram.data().constData(),"Client response.") == 0)
                 {
                     timer->stop();
                     disconnect(timer,SIGNAL(timeout()),nullptr,nullptr);
                     readOrNot = false;  //停止接收新数据
                     qDebug("Client response.");
                     emit timeSynchronizeReady();
                     return;
                 }
                 if(isTimeSynchronize == false && strncmp(datagram.data().constData(),"Time difference response:",25) == 0)
                 {
                     timer->stop();
                     disconnect(timer,SIGNAL(timeout()),nullptr,nullptr);
                     memcpy(&t2,&datagram.data().data()[28],8);
                     memcpy(&t3,&datagram.data().data()[39],8);
                     qDebug("Time difference response:t2:%lldt3:%lld",t2,t3);
                     readOrNot = false;
                     newT2T3 = true;
                     emit newClientT2T3();
                     return;
                 }
                 if(isSendT == false &&strncmp(datagram.data().constData(),"Client response: Time difference has been obtained.",51) == 0)
                 {
                     timer->stop();
                     disconnect(timer,&QTimer::timeout,this,&UdpWorker::sendClientT);
                     readOrNot = false;  //停止接收新数据
                     isSendT = true;
                     qDebug("Client response: Time difference has been obtained.");
                     emit readyToSendData();
                     return;
                 }
                 if(strncmp(datagram.data().constData(),"New datagram had received:",26) == 0)
                 {
                     //timer->stop();
//                     disconnect(timer,&QTimer::timeout,this,&UdpWorker::sendClientT);
//                     readOrNot = false;  //停止接收新数据
//                     isSendT = true;
//                     qDebug("Client response: Time difference has been obtained.");
                     quint32 num;
                     memcpy(&num,&datagram.data().data()[26],4);
                     datagramNumList << num;
                     qint64 deltaT;
                     memcpy(&deltaT,&datagram.data().data()[30],8);
                     totalDelay += deltaT;
                     qDebug("New datagram had received: %u,deltaT = %lld",num,deltaT);
                     emit datagramReachClient(num,deltaT);
                     return;
                 }
             }
    }
}


/**
 * @brief 发送T到客户端
 */
void UdpWorker::sendClientT()
{
    if(numOfSendAveT == 0)
    {
        disconnect(this,SIGNAL(timeSynchronizeSuccess()),this,SLOT(sendClientT()));
        timer->start(2000);
        connect(timer,&QTimer::timeout,this,&UdpWorker::sendClientT);
    }
    if(numOfSendAveT < 3)
    {
        QString str = QString("Time difference between Server and Client:");//42byte
        QByteArray ba =  str.toLatin1(),aveTArray;
        aveTArray.resize(8);
        memcpy(aveTArray.data(),&aveT,8);
        ba.append(aveTArray);
        udpSocket->writeDatagram(ba.data(),ba.size(),*targetIPv4Address,targetPort);
        readOrNot = true; //接收数据
    }else if(numOfSendAveT == 3)
    {
        numOfSendAveT = 0;
        disconnect(timer,&QTimer::timeout,this,&UdpWorker::sendClientT);
        readOrNot = false;
        emit clientUnreachable();
    }
    numOfSendAveT++;
}
/**
 * @brief 发送数据槽函数，接收到messageReachClient()信号或者用户点击事件时触发。
 * n表示上一个已经确认接收到的数据报序号,n==-10表示第一次发送或者有数据报未送达
 */
void UdpWorker::sendData(qint64 n = -10)
{
    qDebug("Invoking sendData(qint64 n)");
    qDebug("nextSendNum = %u,n = %lld, datagramNum = %u",nextSendNum,n,datagramNum);
    if(nextSendNum == datagramNum) //下一个要发送的数据报序号等于数据报总数，则代表数据发送完了。
    {
       qDebug("nextSendNum == datagramNum");
       disconnect(this,&UdpWorker::datagramReachClient,this,&UdpWorker::sendData);
       timer->stop();
       disconnect(timer,&QTimer::timeout,nullptr,nullptr);
       nextSendNum = 0;
       QString str = QString("Datagram transmission completed.");
       QByteArray ba = str.toLatin1();
       udpSocket->writeDatagram(ba.data(),ba.size(),*targetIPv4Address,targetPort);
       emit datagramSendCompleted();
    }else
    {
        if(nextSendNum == 0)
        {
            connect(this,&UdpWorker::datagramReachClient,this,&UdpWorker::sendData,Qt::UniqueConnection);
            readOrNot = true;
            connect(timer,&QTimer::timeout,[=](){
                if(nextSendNum == datagramNum)
                {
                    disconnect(this,&UdpWorker::datagramReachClient,this,&UdpWorker::sendData);
                    timer->stop();
                    disconnect(timer,&QTimer::timeout,nullptr,nullptr);
                    datagramFailedList << nextSendNum-1;
                    nextSendNum = 0;
                    emit datagramSendCompleted();
                }else
                {
                    timer->start(10000);
                    datagramFailedList << nextSendNum-1;
                    emit datagramdReachOutTime();
                    sendData(-10);
                }
            });
        }
        if(n == nextSendNum-1 || n == -10)
        {
            timer->stop();
            qint64 t = QDateTime::currentMSecsSinceEpoch();
            QByteArray sendArray = byteArrayList->at(nextSendNum);
            memcpy(&sendArray.data()[4],&t,8);                  //插入时间戳
            udpSocket->writeDatagram(sendArray.data(),sendArray.size(),*targetIPv4Address,targetPort);
            timer->start(10000);
            nextSendNum++;
        }else if(n < nextSendNum-1 && n != -10) //数据超时到达
        {
            datagramFailedList.removeOne(n);
            //totalDelay += delT;
        }
    }
}

quint32 UdpWorker::getDatagramNum() const
{
    return datagramNum;
}

qint64 UdpWorker::getAveT() const
{
    return aveT;
}

void UdpWorker::setDatagramNum(const quint32 &value)
{
    datagramNum = value;
}
