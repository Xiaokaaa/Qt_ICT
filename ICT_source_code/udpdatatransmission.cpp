#include "udpdatatransmission.h"

//qint64 QUdpSocket::writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port)
//QNetworkDatagram QUdpSocket::receiveDatagram(qint64 maxSize = -1)
//UDP数据包的数据帧内容长度最好不要超过512字节，否则底层协议有可能会将数据报分片发送

UdpDataTransmission::UdpDataTransmission(QString &tarIPv4Address,quint16 tarPort,
                                         QString &souIPv4Address,quint16 souPort,
                                         QFileInfo *sFileInfo):
                                    targetPort(tarPort),sourcePort(souPort),targetIPv4Address(tarIPv4Address),
                                    sourceIPv4Address(souIPv4Address),sendFileInfo(sFileInfo)
{
    udpSocket = new QUdpSocket(this);
    sendFileSize = sendFileInfo->size();
    udpWorker = new UdpWorker(targetPort,sourcePort,&targetIPv4Address,
                                        &sourceIPv4Address,udpSocket,&byteArrayList);
    //udpWoker->moveToThread(&udpWorkerThread);
    //udpWoker->timer->moveToThread(&udpWorkerThread);
    //connect(&udpWorkerThread, &QThread::finished, udpWoker, &QObject::deleteLater);
    //udpWorkerThread.start();
}

UdpDataTransmission::~UdpDataTransmission()
{
    if(sendFileInfo != nullptr)
    {
        delete sendFileInfo;
        sendFileInfo = nullptr;
    }
   // udpWorkerThread.quit();
   // udpWorkerThread.wait();

//    if(udpSocket != nullptr)
//    {
//        delete udpSocket;
//        udpSocket = nullptr;
//    }
//    if(byteArrayList != nullptr)
//    {
//        delete byteArrayList;
//        byteArrayList = nullptr;
//    }
//    if(totalFileArray != nullptr)
//    {
//        delete totalFileArray;
//        totalFileArray = nullptr;
//    }
}

bool UdpDataTransmission::udpBind()
{
    udpSocket->abort();  //终止连接
    return udpSocket->bind(sourceIPv4Address,sourcePort);

}

quint8 UdpDataTransmission::setDatagramNum(const quint32 &value)
{
    datagramNum = value;
    udpWorker->setDatagramNum(value);
    return UdpDataTransmission::cutArray();
}

void UdpDataTransmission::shakeHands()
{
    connect(this,SIGNAL(startSharkHands()),udpWorker,SLOT(sharkHands()),Qt::UniqueConnection);
    emit startSharkHands();
}

void UdpDataTransmission::udpSocketAbort()
{
    udpSocket->abort();  //终止连接
}

QList<QByteArray> UdpDataTransmission::getByteArrayList() const
{
    return byteArrayList;
}

/**
 * @brief 打开文件，将文件以二进制保存进队列中，再将队列进行分割保存在QList<QByteArray>，
 * 每个QByteArray的前面有32位的序号，序号从0开始。
 * @return 0表示成功，1表示文件打开失败，2表示文件读取失败
 */
quint8 UdpDataTransmission::cutArray()
{
    quint8 mes = 0;
    file = new QFile(sendFileInfo->filePath());
    if(!file->open(QFile::ReadOnly)||file == nullptr)
    {
        return mes = 1;
    }
    totalFileArray = file->readAll();
    if(totalFileArray.isEmpty())
    {
        file->close();
        return mes = 2;
    }
    file->close();
    //emit newInfomation("正在分割文件...");
    quint16 DatagramSize;
    DatagramSize = totalFileArray.size()/datagramNum;
    if(totalFileArray.size()%datagramNum != 0)
    {
        datagramNum +=1;
        udpWorker->setDatagramNum(datagramNum);
    }
    char ch[12] = {0}; //datagramNum占4byte，时间戳占8byte
    for(quint32 i=0;i<datagramNum;i++)//数据报序号从0开始
    {
        //*ch = {0};
        for(quint8 j=0;j<4;j++) //循环将4byte序号保存进字符数组中，序号的低位对应数组的低位
        {
            ch[j] = i>>(j*8);
        }
        QByteArray num(ch,12);
        byteArrayList << totalFileArray.mid(i*DatagramSize,DatagramSize).prepend(num);
    }
    //emit newInfomation("分割完成！");

    return mes;
}
