#ifndef TRANSMISSIONWINDOW_H
#define TRANSMISSIONWINDOW_H

#include <QMainWindow>
#include <QNetworkInterface>
#include <QtWidgets>
#include <QFileDialog>
#include <QCloseEvent>
#include <QRegularExpressionValidator>

#include "udpdatatransmission.h"
//#include "udpworker.h"
#include "udpclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TransmissionWindow; }
QT_END_NAMESPACE

class TransmissionWindow : public QMainWindow
{
    Q_OBJECT

public:
    TransmissionWindow(QWidget *parent = nullptr);
    ~TransmissionWindow();

    bool refreshAdapterDomboBox();

    qint8 getModel() const;

private slots:
    void setServerModel();
    void setClientModel();
    void selectSendingFile();
    void selectLogFileDirectory();
    void checkSendParameter();
    void shakeHands();
    void readyReceive();
    void stopReceive();
    void closeEvent(QCloseEvent * event) ;
    void startToSendData();

signals:
   void readyToRead();

private:
    void UiInit();
    int deleteLastLine(QString &str);

    QList<QNetworkInterface> interfaceList;  // 保存网络接口列表
    qint8 model;
    Ui::TransmissionWindow *ui;
    QString sendingFileName;
    QString logFileDirectory;
    quint16 serverModelTargetPort;
    QString serverModelTargetIPv4;
    QString hostIPv4;
    quint16 serverModelHostPort;
    quint16 clientModelHostPort;
    QFileInfo *sendFileInfo;
    quint32 datagramNum;
    quint32 minDatagramNum;

    UdpDataTransmission *udpDataTran;
    UdpClient *udpClient;
    bool isShakeHand;
    bool isRead;
    bool isReceiveDatagram;
    qint64 currentTime;
};
#endif // TRANSMISSIONWINDOW_H
