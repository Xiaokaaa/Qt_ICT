#include "transmissionwindow.h"
#include "ui_transmissionwindow.h"


TransmissionWindow::TransmissionWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TransmissionWindow)
{
    model = -1;  //未选择模式的情况下model=-1
    ui->setupUi(this);
    UiInit();
}

TransmissionWindow::~TransmissionWindow()
{
    delete ui;
    if(sendFileInfo != nullptr)
    {
        delete sendFileInfo;
        sendFileInfo = nullptr;
    }
    if(udpDataTran != nullptr)
    {
        delete udpDataTran;
        udpDataTran = nullptr;
    }
    if(udpClient != nullptr)
    {
        delete udpClient;
        udpClient = nullptr;
    }
}

/**
 * @brief 初始化应用界面，连接相应的信号与槽
 */
void TransmissionWindow::UiInit()
{
    ui->modelGroupBox->setEnabled(false);
    ui->sendGroupBox->setEnabled(false);
    ui->receiveGroupBox->setEnabled(false);
    datagramNum = 0;
    udpDataTran = nullptr;
    isShakeHand = false;
    isRead = false;
    isReceiveDatagram = false;
    udpClient = nullptr;

    if(refreshAdapterDomboBox())
    {
        //当在下拉组件点击选择一个成员的时候，会发射此信号
        connect(ui->adapterDomboBox, QOverload<int>::of(&QComboBox::activated),
             [=](int index){
            if(index == -1) return;   //单清空NetInterface时，index为-1。
            for (int i = 0; i < interfaceList.at(index).addressEntries().size(); ++i)
            {
                if (interfaceList.at(index).addressEntries().at(i).ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    //将IP显示出来
                    ui->hostIpLabel->setText("Host IP:"+interfaceList.at(index).addressEntries().at(i).ip().toString());
                    hostIPv4 =  ui->hostIpLabel->text().remove("Host IP:");  //将地址保存到变量
                    ui->logOutputTextBrowser->append("Successfully selected network adapter!");
                    break;
                }
            }
            if(!ui->modelGroupBox->isEnabled()&&(getModel() == -1))
            {
                ui->modelGroupBox->setEnabled(true);
                connect(ui->sendModelPushButton,SIGNAL(clicked()),this,SLOT(setServerModel()),Qt::UniqueConnection);  //当用户点击发送端按钮，设置服务器模式
                connect(ui->ReceiveModelPushButton,SIGNAL(clicked()),this,SLOT(setClientModel()),Qt::UniqueConnection);
                ui->logOutputTextBrowser->append("Please select a mode ...");
            }
        });
        ui->logOutputTextBrowser->append("Please click the drop-down window to select a network adapter...");
    }else
    {
       return;
    }
    QString rule = "(?:[0-9]?[0-9]?[0-9])";
    ui->targetIpLineEdit->setValidator(new QRegExpValidator(QRegExp("^" + rule + "\\." + rule + "\\." + rule + "\\." + rule + "$"), this));
    ui->targetPortLineEdit->setValidator(new QIntValidator(0, 65535, this));
    ui->hostPortLineEdit->setValidator(new QIntValidator(0, 65535, this));
    ui->SourcePortLineEdit->setValidator(new QIntValidator(0, 65535, this));
    ui->numOfDatagramLineEdit->setValidator(new QIntValidator(0, 2097152, this));
}

/**
 * @brief 刷新网络适配器下拉组件
 * @return bool:获取网络适配器信息是否成功
 */
bool TransmissionWindow::refreshAdapterDomboBox()
{
    bool isSuccess;

    interfaceList = QNetworkInterface::allInterfaces();    //获取网络适配器，如果获取失败返回0成员的列表
    if(interfaceList.isEmpty())
    {
        ui->logOutputTextBrowser->setTextColor(QColor(255, 0, 0, 255));  //红色字体
        ui->logOutputTextBrowser->append("Network adapter information acquisition failed!");
        ui->logOutputTextBrowser->setTextColor(QColor(0, 0, 0, 255));   //黑色字体
        QMessageBox::information(this,"Error","Unable to get network adapter information!");
        isSuccess = false;
    }else
    {
        for (int j = 0; j < interfaceList.size(); ++j)
        {
            ui->adapterDomboBox->addItem(interfaceList.at(j).humanReadableName());
        }
        ui->logOutputTextBrowser->append("Get network adapter information successfully!");
        isSuccess = true;
    }
    return isSuccess;
}

/**
 * @brief 设置应用为服务器模式，关闭和启用相关界面
 */
void TransmissionWindow::setServerModel()
{
    model = 0;
    ui->modelGroupBox->setEnabled(false);  //使模式设置按钮失效，防止用户再次点击
    ui->sendGroupBox->setEnabled(true);    //使能发送模块
    ui->logOutputTextBrowser->append("\nCurrent mode:\n"
                                     "----------------------------------\n"
                                     "             Sending mode\n"
                                     "----------------------------------\n");
    ui->startSendPushButton->setEnabled(false);
    ui->stopSendPushButton->setEnabled(false);
    ui->shakeHandsPushButton->setEnabled(false);
    ui->numOfDatagramLineEdit->setEnabled(false);
    connect(ui->fileLocationPushButton,SIGNAL(clicked()),this,SLOT(selectSendingFile()),Qt::UniqueConnection);
    connect(ui->downloadDirectorPushButton,SIGNAL(clicked()),this,SLOT(selectLogFileDirectory()),Qt::UniqueConnection);
    connect(ui->checkSendPushButton,SIGNAL(clicked()),this,SLOT(checkSendParameter()),Qt::UniqueConnection);
}

/**
 * @brief 设置应用为客户端模式，关闭和启用相关界面
 */
void TransmissionWindow::setClientModel()
{
    model = 1;
    ui->modelGroupBox->setEnabled(false);
    ui->receiveGroupBox->setEnabled(true);
    ui->logOutputTextBrowser->append("\nCurrent mode:\n"
                                     "----------------------------------\n"
                                     "             Receiving mode\n"
                                     "----------------------------------\n");
    //ui->startReceivePushButton->setEnabled(false);
    ui->stopReceivePushButton->setEnabled(false);
    connect(ui->startReceivePushButton,SIGNAL(clicked()),this,SLOT(readyReceive()),Qt::UniqueConnection);
    connect(ui->stopReceivePushButton,SIGNAL(clicked()),this,SLOT(stopReceive()),Qt::UniqueConnection);
}

/**
 * @brief 打开文件选择窗口，将选择的文件路径显示在文本行中
 */
void TransmissionWindow::selectSendingFile()
{
    sendingFileName = QFileDialog::getOpenFileName(this,"Select the file to send",".","All files(*.*)"); //"."表示文件选择窗口的默认路径是工程目录
    if(!sendingFileName.isEmpty())
    {
        ui->fileLocationLineEdit->setText(sendingFileName);
    }
}

/**
 * @brief 打开文件夹选择窗口，将选择的文件夹路径显示在文本行中
 */
void TransmissionWindow::selectLogFileDirectory()
{
    logFileDirectory = QFileDialog::getExistingDirectory(this,"Select the location where the log file is saved",".");
    if(!logFileDirectory.isEmpty())
    {
        ui->downloadDirectoryLineEdit->setText(logFileDirectory);
    }
}

/**
 * @brief 检查文件是否存在，文件路径是否存在，判断文件的大小，显示UDP数据报的数量范围，绑定UDP。
 */
void TransmissionWindow::checkSendParameter()
{
    sendingFileName = ui->fileLocationLineEdit->text();
    sendFileInfo = new QFileInfo(sendingFileName);

    if (sendingFileName.isEmpty() || !sendFileInfo->isFile())
    {
       QMessageBox::information(this,"error","要发送的文件不存在，请重新选择文件！");
       return;
    }
    if (!sendFileInfo->isReadable()) //判断文件是否可读
    {
       QMessageBox::information(this,"error","要发送的文件没有读权限，请重新选择文件！");
       return;
    }
    logFileDirectory = ui->downloadDirectoryLineEdit->text();
    ui->logOutputTextBrowser->append("File selection succeeded!");

    minDatagramNum = (sendFileInfo->size()/UdpDataTransmission::MAX_DATAGRAM_SIZE)+1;
    ui->numScaleLabel->setText(QString("range:%1-%2").arg(minDatagramNum).arg(sendFileInfo->size()));

    if (logFileDirectory.isEmpty() || !QFileInfo(logFileDirectory).isDir())
    {
        if (QMessageBox::question(this, "error","选择的保存日志文件的路径不存在，是否使用默认路径？",
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::Yes) == QMessageBox::Yes)
        {
            logFileDirectory = QDir::currentPath();
            ui->downloadDirectoryLineEdit->setText(logFileDirectory);
        }
        else
            return;
    }
    ui->logOutputTextBrowser->append("File path setting succeeded!");
    serverModelTargetIPv4 = ui->targetIpLineEdit->text();
    serverModelTargetPort = ui->targetPortLineEdit->text().toInt();
    serverModelHostPort =  ui->hostPortLineEdit->text().toInt();

    if(udpDataTran != nullptr)  //新建UdpDataTransmission对象对象前先将udpSocket解绑
    {
        udpDataTran->udpSocketAbort();
    }
    udpDataTran = new UdpDataTransmission(serverModelTargetIPv4,serverModelTargetPort,  //每次点击check按钮都会重新创建UdpDataTransmission对象
                                          hostIPv4,serverModelHostPort,
                                          sendFileInfo);
    //udpDataTran->setDatagramNum(datagramNum);
    if(!udpDataTran->udpBind())
    {
        QMessageBox::information(this,"error","UDP绑定失败！");
        ui->logOutputTextBrowser->setTextColor(QColor(255, 0, 0, 255));  //红色字体
        ui->logOutputTextBrowser->append("UDP binding failed!");
        ui->logOutputTextBrowser->setTextColor(QColor(0, 0, 0, 255));
        return;
    }

    ui->shakeHandsPushButton->setEnabled(true);
    //ui->checkSendPushButton->setEnabled(false);
    ui->logOutputTextBrowser->append("UDP binding succeeded!");
    ui->logOutputTextBrowser->append("Please enter the number of UDP datagrams to send...");
    ui->numOfDatagramLineEdit->setEnabled(true);
    connect(ui->shakeHandsPushButton,SIGNAL(clicked()),this,SLOT(shakeHands()),Qt::UniqueConnection);
}
/**
 * @brief 与接收端握手。握手动作包括询问、告知数据报数量、时间同步。
 */
void TransmissionWindow::shakeHands()
{
    ui->shakeHandsPushButton->setEnabled(false);
    datagramNum = ui->numOfDatagramLineEdit->text().toInt();
    if(datagramNum < minDatagramNum||datagramNum > sendFileInfo->size())
    {
        QMessageBox::information(this,"error","UDP数据报数量不在范围内！请重新输入。");
        ui->shakeHandsPushButton->setEnabled(true);
        datagramNum = 0;
        return;
    }
    switch(udpDataTran->setDatagramNum(datagramNum))  //setDatagramNum函数内调用了cutArray()，根据cutArray()返回来的数据判断分割是否成功
    {
    case 0: ui->logOutputTextBrowser->append("The number of datagrams is set successfully!");break;
    case 1: QMessageBox::information(this,"error","文件打开失败！");ui->shakeHandsPushButton->setEnabled(true);
        return;break;
    case 2: QMessageBox::information(this,"error","文件读取失败！");ui->shakeHandsPushButton->setEnabled(true);
        return;break;
    default: ;
    }
    ui->logOutputTextBrowser->append(QString("Number of datagrams: %1").arg(udpDataTran->udpWorker->getDatagramNum()));
    //test
//    {
//         QList<QByteArray> byteArrayList = udpDataTran->getByteArrayList();
//         quint32 n;
//         ui->logOutputTextBrowser->append(QString("Size:%1").arg(byteArrayList.size()));
//         for(qint32 i=0;i<byteArrayList.size();i++)
//         {
//             memcpy(&n, byteArrayList.at(i).data(), 4);//byteArrayList.at(i).left(4).toUInt();
//             ui->logOutputTextBrowser->append(QString("No.%1: %2").arg(i).arg(n));
//         }
//    }
    udpDataTran->sharkHands();
    if(true)
    {
        isShakeHand = true;
        connect(udpDataTran->udpWorker,&UdpWorker::noClientRespond,[=](){
            ui->logOutputTextBrowser->setTextColor(QColor(255, 0, 0, 255));  //红色字体
            ui->logOutputTextBrowser->append("Client did not respond!");
            ui->logOutputTextBrowser->setTextColor(QColor(0, 0, 0, 255));
            QMessageBox::information(this,"error","客户端没有回应！请检查网络配置是否正确，客户端是否已经打开。");
            ui->shakeHandsPushButton->setEnabled(true);
            //disconnect(udpDataTran->udpWorker,&UdpWorker::noClientRespond,nullptr,nullptr);
        });
        connect(udpDataTran->udpWorker,&UdpWorker::timeSynchronizeReady,[=](){
            ui->logOutputTextBrowser->append("Start time synchronization...");
            //disconnect(udpDataTran->udpWorker,&UdpWorker::timeSynchronizeReady,nullptr,nullptr);
        });
        connect(udpDataTran->udpWorker,&UdpWorker::timeSynchronizeSuccess,[=](){
            ui->logOutputTextBrowser->append("Get time difference successfully!");
        });
        connect(udpDataTran->udpWorker,&UdpWorker::timeSynchronizeFail,[=](){
            ui->logOutputTextBrowser->setTextColor(QColor(255, 0, 0, 255));
            ui->logOutputTextBrowser->append("Failed to get time difference!");
            ui->logOutputTextBrowser->setTextColor(QColor(0, 0, 0, 255));
            ui->shakeHandsPushButton->setEnabled(true);
        });
        //开始发送数据报
        connect(udpDataTran->udpWorker,&UdpWorker::readyToSendData,[=](){
            ui->logOutputTextBrowser->append(QString("Time synchronization completed!\n"
                                                     "Time difference: Client - Server = %1 ms").arg(udpDataTran->udpWorker->getAveT()));
            ui->startSendPushButton->setEnabled(true);
           connect(ui->startSendPushButton,SIGNAL(clicked()),this,SLOT(startToSendData()),Qt::UniqueConnection);
        });
        connect(udpDataTran->udpWorker,&UdpWorker::clientUnreachable,[=](){
            ui->logOutputTextBrowser->setTextColor(QColor(255, 0, 0, 255));  //红色字体
            ui->logOutputTextBrowser->append("Client did not respond!");
            ui->logOutputTextBrowser->setTextColor(QColor(0, 0, 0, 255));
            QMessageBox::information(this,"error","客户端没有回应！请检查网络配置是否正确，客户端是否已经打开。");
            ui->shakeHandsPushButton->setEnabled(true);
        });
    }
}
/**
 * @brief 绑定端口，准备好接收数据。
 */
void TransmissionWindow::readyReceive()
{
    QString str = ui->SourcePortLineEdit->text();
    if(str.isEmpty())
    {
        QMessageBox::information(this,"error","请输入端口号！");
        return;
    }else
    {
        clientModelHostPort = str.toInt();
        //ui->SourcePortLineEdit->setEnabled(false);
    }
    if(udpClient != nullptr)  //新建UdpClient对象对象前先将udpSocket解绑
    {
        udpClient->udpSocketAbort();
    }
    udpClient = new UdpClient(hostIPv4,clientModelHostPort);
    if(true)
    {
        connect(udpClient,&UdpClient::severAsk,[=](){
            ui->logOutputTextBrowser->append(QString("The client sends a request!\n"
                                                     "Datagram number = %1").arg(udpClient->getDatagramNum()));
        });
        connect(udpClient,&UdpClient::severGetTimestamp,[=](){
            ui->logOutputTextBrowser->append("Server requests timestamp...");
        });
        connect(udpClient,&UdpClient::getTimerDifference,[=](){
            ui->logOutputTextBrowser->append(QString("Time synchronization completed!\n"
                                                     "Time difference: Client - Server = %1 ms").arg(udpClient->getAveT()));
        });
        //接收到新的数据报
        connect(udpClient,&UdpClient::receiveDatagram,[=](quint32 num,qint64 delay){
            if(!isReceiveDatagram)    //开始接收
            {
                ui->logOutputTextBrowser->append(QString("Receiving from %1:%2 :\n"
                                                         "num:%3, delay:%4 ms").arg(udpClient->getTargetIPv4Address().toString())
                                                 .arg(udpClient->getTargetPort()).arg(num).arg(delay));
                isReceiveDatagram = true;
                currentTime = QDateTime::currentMSecsSinceEpoch();
            }else
            {
                QString str = ui->logOutputTextBrowser->toPlainText();
                 deleteLastLine(str);
                 str.append(QString("num:%1, delay:%2 ms").arg(num).arg(delay));
                 ui->logOutputTextBrowser->setText(str);
            }
        });
        //完成接收
        connect(udpClient,&UdpClient::datagramSendCompleted,[=](){
            qDebug("signal:datagramSendCompleted()");
            isReceiveDatagram = false;
            int comNum = udpClient->getDatagramNumList().size();
            quint32 num = udpClient->getDatagramNum();
            float tcr = 100*(comNum/num);
            qint32 t = QDateTime::currentMSecsSinceEpoch() - currentTime;
            ui->logOutputTextBrowser->append(QString("The received datagram number: %1 /%2 TCR: %%3\n"
                                                     "The total delays of all datagrams: %4 ms\n"
                                                     "Average delay of all datagrams: %5 ms\n"
                                                     "Time taken to receive: %6 ms")
                                             .arg(comNum).arg(num).arg(tcr)
                                             .arg(udpClient->getTotalDelay())
                                             .arg(udpClient->getTotalDelay()/comNum)
                                             .arg(t));
        });
        isRead = true;
    }

    if(!udpClient->udpBind())
    {
        QMessageBox::information(this,"error","UDP绑定失败！");
        ui->logOutputTextBrowser->setTextColor(QColor(255, 0, 0, 255));  //红色字体
        ui->logOutputTextBrowser->append("UDP binding failed!");
        ui->logOutputTextBrowser->setTextColor(QColor(0, 0, 0, 255));
        ui->SourcePortLineEdit->setEnabled(true);
        return;
    }
    ui->logOutputTextBrowser->append("UDP binding succeeded!");
    ui->startReceivePushButton->setEnabled(false);
    ui->stopReceivePushButton->setEnabled(true);
    connect(this,&TransmissionWindow::readyToRead,udpClient,&UdpClient::readyToRead,Qt::UniqueConnection);
    emit readyToRead();
    ui->logOutputTextBrowser->append("Start receiving...");
    udpClient->setReadOrNot(true);
}

void TransmissionWindow::stopReceive()
{
    ui->logOutputTextBrowser->append("Terminate receive.");
    udpClient->setReadOrNot(false);
    ui->SourcePortLineEdit->setEnabled(true);
    ui->startReceivePushButton->setEnabled(true);
    ui->stopReceivePushButton->setEnabled(false);
}

qint8 TransmissionWindow::getModel() const
{
    return model;
}

void TransmissionWindow::closeEvent( QCloseEvent * event )
{
switch( QMessageBox::information( this, tr("exit tip"), tr("Do you really want exit?"), tr("Yes"), tr("No"), 0, 1 ) )
   {
     case 0:
          event->accept();
          break;
     case 1:
     default:
         event->ignore();
         break;
}
}

void TransmissionWindow::startToSendData()
{
    //connect(ui->startSendPushButton,SIGNAL(clicked()),udpDataTran->udpWorker,SLOT(sendData()),Qt::UniqueConnection);setIsReceiving

    udpDataTran->udpWorker->sendData(-10); //开始发送
    ui->logOutputTextBrowser->append("Start to send data...");
    ui->startSendPushButton->setEnabled(false);
}
/**
 * @brief 删除输入的字符串str中的最后一行，如果str中没有换行符则不进行任何操作返回-1
 * @param 要删除最后一行的字符串
 * @return 返回换行符的索引，-1代表字符串中没有换行符
 */
int TransmissionWindow::deleteLastLine(QString &str)
{
    int result = str.lastIndexOf('\n');
    if(result == -1)
    {
        return result;
    }else
    {
        str.remove(result+1,1024);
        return result;
    }
}
