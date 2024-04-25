#include "udpclient.h"

UdpClient::UdpClient(QString &souIPv4Address, quint16 souPort):
    sourceIPv4Address(souIPv4Address),sourcePort(souPort)
{
    udpSocket = new QUdpSocket(this);
    isTimeSynchronize = false;
    isReceiving = false;
    receivedDatagramNumber = 0;
    readOrNot = false;
    totalDelay = 0;
}

UdpClient::~UdpClient()
{
    if(udpSocket != nullptr)
    {
        delete udpSocket;
        udpSocket = nullptr;
    }
    byteArrayList.clear();
}

bool UdpClient::udpBind()
{
    udpSocket->abort();  //终止连接
    return udpSocket->bind(sourceIPv4Address,sourcePort);
}

void UdpClient::readyToRead()
{
    connect(udpSocket,&QUdpSocket::readyRead,this,&UdpClient::clientResponse,Qt::UniqueConnection);
//    QEventLoop loop;
//    QTimer::singleShot(100, &loop, SLOT(quit()));
    //    loop.exec();
}

void UdpClient::udpSocketAbort()
{
    udpSocket->abort();
}

void UdpClient::clientResponse()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram(512);
        if(readOrNot && !datagram.isNull())
        {
            qDebug("----------New message---------");
            if(strncmp(datagram.data().constData(),"DatagramNum",11) == 0)
            {
                qDebug("Server message:");
                QByteArray array;
                array.resize(12);
                strncpy(array.data(),datagram.data().constData(),12);
                memcpy(&datagramNum,&datagram.data().constData()[12],4);
                array.append("\0");
                qDebug("%s%u",array.constData(),datagramNum);
                targetIPv4Address = datagram.senderAddress();
                targetPort = 4040;//datagram.senderPort();
                qDebug() <<"targetIPv4Address:"<<targetIPv4Address.toString()<<"targetPort:"<<targetPort;

                QString str = QString("Client response.");
                QByteArray ba =  str.toLatin1();
                udpSocket->writeDatagram(ba.data(),ba.size(),targetIPv4Address,targetPort);
                emit severAsk();
                isTimeSynchronize = false;
                isReceiving = false;

                byteArrayList.clear();
                datagramNumList.clear();
                receivedDatagramNumber = 0;
                totalDelay = 0;

                return;
            }
            if(!isTimeSynchronize && strncmp(datagram.data().constData(),"Get timestamp.",14) == 0)
            {
                qDebug("Receive message:Get timestamp.");
                qint64 t2 = QDateTime::currentMSecsSinceEpoch();
                QString str = QString("Time difference response:");
                QByteArray ba = str.toLatin1(),t2Array,t3Array;
                t2Array.resize(8);
                memcpy(t2Array.data(),&t2,8);
                ba.append("t2:");
                ba.append(t2Array);

                ba.append("t3:");
                t3Array.resize(8);
                qint64 t3 = QDateTime::currentMSecsSinceEpoch();
                qDebug("Send t2:%lld t3:%lld",t2,t3);
                memcpy(t3Array.data(),&t3,8);
                ba.append(t3Array);
                udpSocket->writeDatagram(ba.data(),ba.size(),targetIPv4Address,targetPort);

                qDebug("Send t2t3");
                emit severGetTimestamp();
                return;
            }
            if(!isTimeSynchronize && strncmp(datagram.data().constData(),"Time difference between Server and Client:",42) == 0)//Client response: Time difference has been obtained.51byte
            {
                memcpy(&aveT,&datagram.data().constData()[42],8);
                qDebug("Get timer difference! aveT = %lld,datagram.data().size = %d",aveT,datagram.data().size());
                QString str = QString("Client response: Time difference has been obtained.");
                QByteArray ba = str.toLatin1();
                udpSocket->writeDatagram(ba.data(),ba.size(),targetIPv4Address,targetPort);
                emit getTimerDifference();
                isTimeSynchronize = true;
                isReceiving = true;
                return;
            }
            if(isReceiving)
            {
                if(strncmp(datagram.data().constData(),"Datagram transmission completed.",32) == 0)
                {
                    qDebug("Datagram transmission completed.");
                    isReceiving = false;
                    emit datagramSendCompleted();
                    return;
                }else
                {
                    qint64 datagramTime;
                    memcpy(&datagramTime,&datagram.data().constData()[4],8);
                    if(datagramTime/1600000000000 == 1)
                    {
                        byteArrayList<<datagram.data().mid(12);
                        receivedDatagramNumber++;

                        qint64 delay;
                        qint64 now = QDateTime::currentMSecsSinceEpoch();
                        delay = now - datagramTime - aveT;
                        totalDelay += delay;

                        quint32 num;
                        datagramNumList<<num;
                        memcpy(&num,datagram.data().constData(),4);
                        QString str = QString("New datagram had received:");
                        QByteArray ba = str.toLatin1(),array;
                        array.resize(12);
                        memcpy(array.data(),&num,4);
                        memcpy(&array.data()[4],&delay,8);
                        ba.append(array);
                        udpSocket->writeDatagram(ba.data(),ba.size(),targetIPv4Address,targetPort);
                        qDebug("Datagram: num=%u, delay=%lld ms",num,delay);
                        emit receiveDatagram(num,delay);
                    }
                    return;
                }
            }
//            if(strncmp(datagram.data().constData(),"Datagram transmission completed.",32) == 0)
//            {
//                qDebug("Datagram transmission completed.");
//                isReceiving = false;
//                emit datagramSendCompleted();
//                return;
//            }
        }
    }
}

void UdpClient::setIsReceiving(bool value)
{
    isReceiving = value;
}

qint64 UdpClient::getTotalDelay() const
{
    return totalDelay;
}

QList<quint32> UdpClient::getDatagramNumList() const
{
    return datagramNumList;
}

quint16 UdpClient::getTargetPort() const
{
    return targetPort;
}

QHostAddress UdpClient::getTargetIPv4Address() const
{
    return targetIPv4Address;
}

void UdpClient::setReadOrNot(bool value)
{
    readOrNot = value;
}

quint32 UdpClient::getReceivedDatagramNumber() const
{
    return receivedDatagramNumber;
}

qint64 UdpClient::getAveT() const
{
    return aveT;
}

quint32 UdpClient::getDatagramNum() const
{
    return datagramNum;
}

