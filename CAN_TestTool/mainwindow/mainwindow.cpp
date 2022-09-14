#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QStyleFactory>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QMetaType>
#include <QPluginLoader>

#include "other/version.h"

#include <QDebug>

#if (IS_RELEASE_VERSION > 0)
#define VERSION_TIP "正式版"
#else
#define VERSION_TIP "测试版"
#endif

#define CANTESTTOOL_VERSION "CAN测试工具 V" PRODUCT_VERSION_STR " " VERSION_TIP
#define CANTESTTOOL_SETUP 	"CAN_TestTool-Setup"

#define CETCAN_TESTTOOL_DIR "C:/Users/Administrator/Desktop/CETCAN-TT-DIR"

#define SEND_FRAME_NUM "  发送帧数："
#define RECV_FRAME_NUM "  接收帧数："

#define TABLE_INDEX_WIDTH       70
#define TABLE_TIME_WIDTH        160
#define TABLE_DIRECTION_WIDTH   40 
#define TABLE_FRAMEID_WIDTH     80 
#define TABLE_FRAMETYPE_WIDTH   50
#define TABLE_FRAMEFORMAT_WIDTH 50 
#define TABLE_DATALEN_WIDTH     40 
#define TABLE_DATAHEX_WIDTH     160

#define TABLE_MAX_INDEX_ROWS 10000000

static const QString g_tim0[10]=
{
    "00",       // 1000Kbps
    "00",       // 800Kbps
    "00",       // 500Kbps
    "01",       // 250Kbps
    "03",       // 125Kbps
    "04",       // 100Kbps
    "09",       // 50Kbps
    "18",       // 20Kbps
    "31",       // 10Kbps
    "BF"        // 5Kbps
};
    
static const QString g_tim1[10]=
{
    "14",       // 1000Kbps
    "16",       // 800Kbps
    "1C",       // 500Kbps
    "1C",       // 250Kbps
    "1C",       // 125Kbps
    "1C",       // 100Kbps
    "1C",       // 50Kbps
    "1C",       // 20Kbps
    "1C",       // 10Kbps
    "FF"        // 5Kbps	
};

static QString timeStampToTimeStr(UINT TimeStamp)
{
    QString resultStr = "";

    int hour = TimeStamp / 36000000;
    int minute = (TimeStamp - hour * 36000000) / 600000;
    int second = (TimeStamp - hour * 36000000 - minute * 600000) / 10000;
    int ms = (TimeStamp - hour * 36000000 - minute * 600000 - second * 10000) / 10;
    int mms = (TimeStamp - hour * 36000000 - minute * 600000 - second * 10000 - ms * 10);

    resultStr = QString("%1:").arg(hour, 2, 10, QChar('0'));//时
    resultStr += QString("%1:").arg(minute, 2, 10, QChar('0'));//分
    resultStr += QString("%1:").arg(second, 2, 10, QChar('0'));//秒
    resultStr += QString("%1:").arg(ms, 3, 10, QChar('0'));//毫秒
    resultStr += QString::number(mms);//0.1ms

    return resultStr;
}

static bool isInRange(uint32_t candid, const uint32_t *fromLowHex, const uint32_t *toHighHex, int len)
{
    bool result = false;
    
    for (int i = 0; i < len; ++i) {
        if ((candid >= fromLowHex[i]) && (candid <= toHighHex[i])) {
            result = true;
            break;
        }
    }
    
    return result;
}

static bool isTrigger(QMap<QString, QList<SEND_DATA_T>> &trigmap, 
        const VCI_CAN_OBJ *info, QList<SEND_DATA_T> &send)
{
    bool result = false;

    if (trigmap.count() > 0) {
        QString frameid = QString("%1").arg(info->ID, 8, 16, QLatin1Char('0'));
        QString frametype = QString("%1").arg(info->ExternFlag);
        QString frameformat = QString("%1").arg(info->RemoteFlag);
        QString framelen = QString("%1").arg(info->DataLen, 2, 16, QLatin1Char('0'));
        QString framecontent;
        for (int i = 0; i < info->DataLen; ++i) {
            QString str = QString("%1").arg(info->Data[i], 2, 16, QLatin1Char('0'));
            framecontent += str;
        }

        frameid = frameid.toLower();
        framecontent = framecontent.toLower();
        
        QString allkey;
        allkey.append(frameid);
        allkey.append(":");
        allkey.append(framelen);
        allkey.append(":");
        allkey.append(frametype);
        allkey.append(":");
        allkey.append(frameformat);
        allkey.append(":");
        allkey.append(framecontent);
        
        QString idkey;
        idkey.append(frameid);
        idkey.append(":");
        idkey.append("00");
        idkey.append(":");
        idkey.append(frametype);
        idkey.append(":");
        idkey.append(frameformat);
        
        QString contentkey;
        contentkey.append(framelen);
        contentkey.append(":");
        contentkey.append(frametype);
        contentkey.append(":");
        contentkey.append(frameformat);
        contentkey.append(":");
        contentkey.append(framecontent);

        QString typeformatkey;
        typeformatkey.append("00");
        typeformatkey.append(":");
        typeformatkey.append(frametype);
        typeformatkey.append(":");
        typeformatkey.append(frameformat);

        qDebug() << "[isTrigger] allkey:" << allkey << "idkey:" << idkey
                 << "contentkey:" << contentkey << "typeformatkey:" << typeformatkey;
        QMap<QString, QList<SEND_DATA_T>>::iterator beit;
        for (beit = trigmap.begin(); beit != trigmap.end(); ++beit) {
            QString itkey = beit.key();
            if (!allkey.compare(itkey, Qt::CaseInsensitive)
                || !idkey.compare(itkey, Qt::CaseInsensitive)
                || !contentkey.compare(itkey, Qt::CaseInsensitive)
                || !typeformatkey.compare(itkey, Qt::CaseInsensitive)) {
                QMap<QString, QList<SEND_DATA_T>>::iterator it = trigmap.find(itkey);
                send << it.value();
                qDebug() << "[isTrigger] itkey:" << itkey << "send.count():" << send.count();
                result = true;
            }
        }
    }
    
    return result;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_cetLicIface(nullptr),
    m_cetUpIface(nullptr),
    m_indexCount(0),
    m_recvFrameCount(0),
    m_sendFrameCount(0),
    m_filtermode(0)
{
    ui->setupUi(this);
    ui->DLCLineEdit->setEnabled(false);
    
    this->setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    this->setWindowTitle(tr(CANTESTTOOL_VERSION));

    recvThread = new RecvThread();
    sendThread = new SendThread();
    filterDlg  = new FilterDialog(this);
    triggerDlg = new TriggerDialog(this);

    sendNumLabel = new QLabel(tr("%1%2").arg(SEND_FRAME_NUM).arg(m_sendFrameCount), this);
    recvNumLabel = new QLabel(tr("%1%2").arg(RECV_FRAME_NUM).arg(m_recvFrameCount), this);
    QPushButton *clearCountButton = new QPushButton(tr("清空计数"), this);
    QStatusBar *sBar = statusBar();
    sBar->addWidget(sendNumLabel, 1);
    sBar->addWidget(recvNumLabel, 1);
    sBar->addWidget(clearCountButton);

    QAction *action = NULL;
    QActionGroup *styleActions = new QActionGroup(this);
    QStringList styleList = QStyleFactory::keys();
    for (QString stylename : styleList) {
        action = styleActions->addAction(stylename);
        action->setCheckable(true);
        action->setChecked(true);
    }
    
    slotStyleActions(action);
    
    ui->timer0LineEdit->setEnabled(false);
    ui->timer1LineEdit->setEnabled(false);
    ui->styleMenu->addActions(styleActions->actions());
    ui->centralWidget->layout()->setMargin(10);
    ui->centralWidget->layout()->setSpacing(10);

    m_model = new QStandardItemModel(this);
    setInfoViewHeader();
    
#if 0
    m_mapDevtype["VCI_PCI5121"]     = VCI_PCI5121;
    m_mapDevtype["VCI_PCI9840"]     = VCI_PCI9840;
    m_mapDevtype["VCI_PCIE_9220"]   = VCI_PCIE_9220;
    m_mapDevtype["VCI_PCIe9140I"]   = VCI_PCIe9140;
#elif 1
    m_mapDevtype["VCI_USBCAN1(I+)"] = VCI_USBCAN1;
    m_mapDevtype["VCI_USBCAN2(II+)"]= VCI_USBCAN2;
    m_mapDevtype["VCI_USBCAN2A"]    = VCI_USBCAN2A;
    m_mapDevtype["VCI_USBCAN_E_U"]  = VCI_USBCAN_E_U;
    m_mapDevtype["VCI_USBCAN_2E_U"] = VCI_USBCAN_2E_U;
#elif 0
    m_mapDevtype["VCI_ISA5420"]     = VCI_ISA5420;
    m_mapDevtype["VCI_ISA9620"]     = VCI_ISA9620;
    m_mapDevtype["VCI_PC104CAN2"]   = VCI_PC104CAN2;
    m_mapDevtype["VCI_PCI9820I"]    = VCI_PCI9820I;
    m_mapDevtype["VCI_PEC9920"]     = VCI_PEC9920;
    m_mapDevtype["VCI_DNP9810"]     = VCI_DNP9810;
    m_mapDevtype["VCI_CANLITE"]     = VCI_CANLITE;
#endif
    for(QMap<QString, int>::ConstIterator ite = m_mapDevtype.constBegin(); 
        ite != m_mapDevtype.constEnd(); ++ite) {
        ui->typeComboBox->addItem(ite.key());
    }

    qRegisterMetaType<VCI_CAN_OBJ>("VCI_CAN_OBJ");

    connect(ui->activateAction, &QAction::triggered, this, &MainWindow::slotActivateDialog);
        
    connect(recvThread, &RecvThread::signalRecvData, this, &MainWindow::slotRecvDataHandle);
    connect(sendThread, &SendThread::signalSendData, this, &MainWindow::slotShowSendFrame);
    connect(sendThread, &SendThread::signalMessage, this, &MainWindow::slotThreadMessage);
    
    connect(styleActions, &QActionGroup::triggered, this, &MainWindow::slotStyleActions);
    connect(clearCountButton, &QPushButton::clicked, this, &MainWindow::slotClearCountButton);

    connect(ui->sendDataLineEdit, &QLineEdit::textChanged, this, &MainWindow::slotSendTextChanged);
    connect(ui->saveLogButton, &QPushButton::clicked, this, &MainWindow::slotSaveLogButton);
    connect(ui->filterButton, &QPushButton::clicked, this, &MainWindow::slotFilterDialog);
    connect(ui->triggerButton, &QPushButton::clicked, this, &MainWindow::slotTriggerDialog);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::slotConnectButton);
    connect(ui->startCANButton, &QPushButton::clicked, this, &MainWindow::slotStartCANButton);
    connect(ui->resetCANButton, &QPushButton::clicked, this, &MainWindow::slotResetCANButton);
    connect(ui->clearMsgButton, &QPushButton::clicked, this, &MainWindow::slotClearInfoButton);
    connect(ui->sendDataButton, &QPushButton::clicked, this, &MainWindow::slotSendDataButton);
    connect(ui->baudrateComboBox, SIGNAL(activated(int)), this, SLOT(slotBaudrateIndex(int)));

    initDllPlugin();
    //ui->licenseMenu->menuAction()->setVisible(false);
}

MainWindow::~MainWindow()
{
    delete recvThread;
    delete sendThread;
    delete filterDlg;
    delete triggerDlg;
    delete m_cetLicIface;
	delete m_cetUpIface;
    delete sendNumLabel;
    delete recvNumLabel;
    delete ui;
}

QObject *MainWindow::loadDllPlugin(const QString &dllname)
{
    QObject *plugin = nullptr;
    QDir pluginsDir("./plugins");

    foreach (const QString &fileName, pluginsDir.entryList(QDir::Files)) {
        //qDebug() << "fileName" << fileName << "absoluteFilePath(fileName)" << pluginsDir.absoluteFilePath(fileName);
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        plugin = pluginLoader.instance();
        if (plugin) {
            plugin->setParent(this);
            if (0 == dllname.compare(fileName, Qt::CaseInsensitive)) {
                break;
            }
        }
    }

    return plugin;
}

void MainWindow::initDllPlugin()
{
    QFileInfo fileInfo("./change.log");
    if (fileInfo.size() < 64) {
        QFile file("./change.log");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QString details;
        details.append("/***********************************************************************************************\n");
        details.append(tr("* 项目名称：%1\n").arg(QString(CANTESTTOOL_SETUP).split('-').first()));
        details.append("* 项目简介：CAN测试应用程序\n");
        details.append("* 创建日期：2022.05.11\n");
        details.append("* 创建人员：郑建宇(CetXiyuan)\n");
        details.append("* 版权说明：本程序仅为个人兴趣开发，有需要者可以免费使用，但是请不要在网络上进行恶意传播\n");
        details.append("***********************************************************************************************/\n\n");

        file.seek(0);
        file.write(details.toUtf8());
        file.close();
    }

    m_cetLicIface = qobject_cast<CetLicenseInterface *>(loadDllPlugin("CetLicensePlugin.dll"));
    if (!m_cetLicIface) {
        QMessageBox::warning(this, tr("警告"), tr("缺少 CetLicensePlugin.dll 库\t"));
    }

    m_cetUpIface = qobject_cast<CetUpdateInterface *>(loadDllPlugin("CetUpdatePlugin.dll"));
    if (!m_cetUpIface) {
        QMessageBox::warning(this, tr("警告"), tr("缺少 CetUpdatePlugin.dll 库\t"));
    }

    if (!m_cetLicIface || CetLicenseInterface::ACTIVATE_OK != m_cetLicIface->activate(PRODUCT_NAME)) {
        this->centralWidget()->setEnabled(false);
        this->setWindowTitle(windowTitle().append(tr(" [已到期]")));
    }
    
    if (m_cetUpIface) {
        m_cetUpIface->checkUpdate(CANTESTTOOL_SETUP, PRODUCT_VERSION_STR, QCoreApplication::quit);
    }
}


void MainWindow::insertInfoViewCanInfo(bool isSend, const VCI_CAN_OBJ *info)
{
    QString index = "";
    QString time = "";
    QString frameid = "";
    QString datalen = "";
    QString datahex = "";

    m_indexCount = m_sendFrameCount + m_recvFrameCount;
    index.sprintf("%07u", m_indexCount);
    frameid.sprintf("0x%08x", info->ID);
    datalen.sprintf("%d", info->DataLen);
    
    if (ui->timeComboBox->currentIndex()) {
        time = timeStampToTimeStr(info->TimeStamp);
    } else {
        time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
    
    for (int i = 0; i < info->DataLen; ++i) {
        QString str = QString("%1 ").arg(info->Data[i], 2, 16, QLatin1Char('0'));
        datahex += str;
    }
    
    if (ui->caseHexComboBox->currentIndex()) {
        frameid = frameid.toUpper();
        datahex = datahex.toUpper();
    }

    QStandardItem *indexItem = new QStandardItem(index);
    QStandardItem *timeItem = new QStandardItem(time);
    QStandardItem *directionItem = new QStandardItem((isSend? "发送" : "接收"));
    QStandardItem *frameIdItem = new QStandardItem(frameid);
    QStandardItem *frameTypeItem = new QStandardItem((info->ExternFlag? "扩展帧" : "标准帧"));
    QStandardItem *frameFormatItem = new QStandardItem((info->RemoteFlag? "远程帧" : "数据帧"));
    QStandardItem *dataLenItem = new QStandardItem(datalen);
    QStandardItem *dataHexItem = new QStandardItem(datahex);

    indexItem->setTextAlignment(Qt::AlignCenter);
    timeItem->setTextAlignment(Qt::AlignCenter);
    directionItem->setTextAlignment(Qt::AlignCenter);
    frameIdItem->setTextAlignment(Qt::AlignCenter);
    frameTypeItem->setTextAlignment(Qt::AlignCenter);
    frameFormatItem->setTextAlignment(Qt::AlignCenter);
    dataLenItem->setTextAlignment(Qt::AlignCenter);

    int column = 0;
    int endrow = m_model->rowCount();
    m_model->setRowCount(endrow + 1);
    m_model->setItem(endrow, column++, indexItem);
    m_model->setItem(endrow, column++, timeItem);
    m_model->setItem(endrow, column++, directionItem);
    m_model->setItem(endrow, column++, frameIdItem);
    m_model->setItem(endrow, column++, frameTypeItem);
    m_model->setItem(endrow, column++, frameFormatItem);
    m_model->setItem(endrow, column++, dataLenItem);
    m_model->setItem(endrow, column++, dataHexItem);
    ui->infoTableView->setCurrentIndex(m_model->index(endrow, column-1));
    
    sendNumLabel->setText(tr("%1%2").arg(SEND_FRAME_NUM).arg(m_sendFrameCount));
    recvNumLabel->setText(tr("%1%2").arg(RECV_FRAME_NUM).arg(m_recvFrameCount));
}

void MainWindow::setInfoViewHeader()
{
    int section = 0;

    m_model->setColumnCount(8);
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("序号"));
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("时间"));
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("方向"));
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("帧ID"));
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("帧类型"));
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("帧格式"));    
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("长度"));
    m_model->setHeaderData(section++, Qt::Horizontal, QStringLiteral("数据(十六进制)"));
    
    ui->infoTableView->setModel(m_model);
    ui->infoTableView->setSortingEnabled(true); // 可以按列来排序
    ui->infoTableView->verticalHeader()->setVisible(false); // 隐藏列头
    ui->infoTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
    ui->infoTableView->setSelectionBehavior(QAbstractItemView::SelectRows); //整行选中
    ui->infoTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);// 表格单元格为只读 

    /* 设置各列的宽度 */
    section = 0;
    ui->infoTableView->setColumnWidth(section++, TABLE_INDEX_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_TIME_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_DIRECTION_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_FRAMEID_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_FRAMETYPE_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_FRAMEFORMAT_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_DATALEN_WIDTH);
    ui->infoTableView->setColumnWidth(section++, TABLE_DATAHEX_WIDTH);
}

void MainWindow::slotActivateDialog()
{
    if (!m_cetLicIface)
        return;
    
    m_cetLicIface->exec();
    int result = m_cetLicIface->result();
    if (CetLicenseInterface::ACTIVATE_OK == result) {
        this->centralWidget()->setEnabled(true);
        this->setWindowTitle(tr(CANTESTTOOL_VERSION));
    }

    if (CetLicenseInterface::ACTIVATE_NONE != result) {
        QMessageBox::information(this, tr("通知"), m_cetLicIface->message() + "\t", QMessageBox::Ok);
    }
}

void MainWindow::slotStyleActions(QAction *action)
{
    QApplication::setStyle(QStyleFactory::create(action->text()));
    //QApplication::setPalette(QApplication::style()->standardPalette());
}

void MainWindow::slotConnectButton()
{
    if (tr("连接") == ui->connectButton->text()) {
        m_devtype = m_mapDevtype.value(ui->typeComboBox->currentText());
        m_device  = ui->indexNoComboBox->currentIndex();
        m_chanel  = ui->CANIndexComboBox->currentIndex();
        /* 连接设备 */
        if (STATUS_OK != VCI_OpenDevice(m_devtype, m_device, m_chanel)) {
            ui->connectButton->setChecked(false);
            QMessageBox::critical(this, tr("error"), tr("连接失败 !"), QMessageBox::Ok);
            return ;
        }

        /* 初始化CAN设备 */
        QString accessCode = ui->accessCodeLineEdit->text();
        QString maskCode = ui->maskCodeLineEdit->text();
        int filterTypeIndex = ui->filterTypeComboBox->currentIndex();
        int modeIndex = ui->modeComboBox->currentIndex();
        int baudrateIndex = ui->baudrateComboBox->currentIndex();
        
        bool ok = false;
        VCI_INIT_CONFIG config;
        config.AccCode = accessCode.toUInt(&ok, 16);
        config.AccMask = maskCode.toUInt(&ok, 16);
        config.Filter  = filterTypeIndex;
        config.Mode    = modeIndex;
        config.Timing0 = g_tim0[baudrateIndex].toInt(&ok, 16);
        config.Timing1 = g_tim1[baudrateIndex].toInt(&ok, 16);

        qDebug("m_devtype:%lu, m_device:%lu, m_chanel:%lu", m_devtype, m_device, m_chanel);    
        qDebug("AccCode:0x%08X, AccMask:0x%08X, Filter:%u, Mode:%u, Timing0:0x%02X, Timing1:0x%02X",
            (uint)config.AccCode, (uint)config.AccMask, config.Filter, 
            config.Mode, config.Timing0, config.Timing1);
        
        if (STATUS_OK != VCI_InitCAN(m_devtype, m_device, m_chanel, &config)) {
            QMessageBox::critical(this, tr("error"), tr("CAN初始化失败 !"), QMessageBox::Ok);
            return ;
        }
        
        ui->devParamGroupBox->setEnabled(false);
        ui->initCANParamGroupBox->setEnabled(false);
        ui->connectButton->setText(tr("断开"));
        ui->startCANButton->setChecked(true);
        
        slotStartCANButton(); 
    } else {   
        /* 关闭设备 */
        if (STATUS_OK != VCI_CloseDevice(m_devtype, m_device)) {
            QMessageBox::critical(this, tr("error"), tr("断开失败 !"), QMessageBox::Ok);
            return ;
        }
        
        recvThread->quit();
        sendThread->quit();
        
        ui->startCANButton->setText(tr("启动CAN"));
        ui->startCANButton->setChecked(false);
        ui->devParamGroupBox->setEnabled(true);
        ui->initCANParamGroupBox->setEnabled(true);
        ui->connectButton->setText(tr("连接"));
    }
}

void MainWindow::slotStartCANButton()
{
    if (tr("连接") == ui->connectButton->text()) {
        ui->startCANButton->setChecked(false);
        QMessageBox::warning(this, tr("warning"), tr("请先连接设备 !"), QMessageBox::Ok);
        return ;
    }
    
    if (tr("启动CAN") == ui->startCANButton->text()) {
        if (STATUS_OK != VCI_StartCAN(m_devtype, m_device, m_chanel)) {
            QMessageBox::critical(this, tr("error"), tr("CAN启动失败 !"), QMessageBox::Ok);
            return ;
        }
        
        /* 启动接收线程 */
        recvThread->setDevParam(m_devtype, m_device, m_chanel);
        recvThread->setThrowRecvSignal(true);
        recvThread->start(QThread::HighestPriority);

        /* 启动发送线程 */
        sendThread->setDevParam(m_devtype, m_device, m_chanel);
        sendThread->setThrowSendSignal(true);
        sendThread->start(QThread::NormalPriority);
        
        ui->startCANButton->setText(tr("停止CAN"));
    } else {        
        recvThread->setThrowRecvSignal(false);
        ui->startCANButton->setText(tr("启动CAN"));
    }
}

void MainWindow::slotResetCANButton()
{
    if (STATUS_OK != VCI_ResetCAN(m_devtype, m_device, m_chanel)) {
        QMessageBox::critical(this, tr("error"), tr("CAN复位失败 !"), QMessageBox::Ok);
        return ;
    }
    
    if (STATUS_OK != VCI_StartCAN(m_devtype, m_device, m_chanel)) {
        QMessageBox::critical(this, tr("error"), tr("CAN启动失败 !"), QMessageBox::Ok);
        return ;
    }    
}

void MainWindow::slotClearInfoButton()
{
    slotClearCountButton();

    m_model->clear();

    setInfoViewHeader();
}

void MainWindow::slotSendDataButton()
{
    int count = 0;
    bool ok = false;
    QString frameID = ui->frameIDLineEdit->text();
    QString sendFrameNum = ui->sendFrameNumLineEdit->text();
    QString sendData = ui->sendDataLineEdit->text();
    int sendTypeIndex = ui->sendTypeComboBox->currentIndex();
    int frameTypeIndex = ui->frameTypeComboBox->currentIndex();
    int frameFormatIndex = ui->frameFormatComboBox->currentIndex();
    int intervalms = ui->sendIntervalMsLineEdit->text().toInt(&ok, 10);
    ULONG sendnum = sendFrameNum.toInt(&ok, 10);

    UINT canid = frameID.toUInt(&ok, 16);
    if (0 == frameTypeIndex && canid > 0xFFFF) {
        QMessageBox::critical(this, tr("error"), tr("帧类型错误 !"), QMessageBox::Ok);
        return ;
    }
    
    SEND_DATA_T send;
    memset(&send, 0, sizeof(SEND_DATA_T));
    /* 0：正常发送 1：单次发送 2：自发自收 3：单次自发自收 */
    send.can.SendType   = sendTypeIndex;
    send.can.ID         = canid;
    send.can.ExternFlag = frameTypeIndex;
    send.can.RemoteFlag = frameFormatIndex;

    sendData.replace(QLatin1String(" "), QLatin1String(""));
    for (int i = 0; i < sendData.length(); i += 2) {
        QString byte = sendData.mid(i, 2);        
        send.can.Data[count++] = byte.toInt(&ok, 16);
    }
    send.can.DataLen = count;
    send.sendnum = sendnum;
    send.intervalms = intervalms;

    sendThread->addSendData(&send);
}

void MainWindow::slotClearCountButton()
{
    m_recvFrameCount = 0;
    m_sendFrameCount = 0;
    
    sendNumLabel->setText(tr("%1%2").arg(SEND_FRAME_NUM).arg(m_sendFrameCount));
    recvNumLabel->setText(tr("%1%2").arg(RECV_FRAME_NUM).arg(m_recvFrameCount));
}

void MainWindow::slotRecvDataHandle(const VCI_CAN_OBJ *info, int recvlen)
{
    for (int i = 0; i < recvlen; ++i) {
        if (info[i].DataLen > 0) {
            /* 过滤处理 */
            if (1 == m_filtermode) { // 不接受IDs
                if (isInRange(info[i].ID, m_fromlowhex, m_tohighhex, m_hexlen)) {
                    continue;
                }
            } else if (2 == m_filtermode) { // 仅接收IDs
                if (!isInRange(info[i].ID, m_fromlowhex, m_tohighhex, m_hexlen)) {
                    continue;
                }
            }

            m_recvFrameCount++;
            insertInfoViewCanInfo(false, &info[i]);
            /* 触发处理 */
            if (m_triggermode > 0) {
                QList<SEND_DATA_T> send;
                if (isTrigger(m_trigmap, &info[i], send)) {
                    for (int i = 0; i < send.count(); ++i) {
                        sendThread->addSendData(&send[i]);
                    }
                }
            }
            if (m_indexCount > TABLE_MAX_INDEX_ROWS) {
                slotClearInfoButton();
            }
            
        }
    }
}

void MainWindow::slotShowSendFrame(const VCI_CAN_OBJ &info)
{
    m_sendFrameCount++;
    if (ui->isHideSendFrameComboBox->currentIndex()) {
        insertInfoViewCanInfo(true, &info);
        if (m_indexCount > TABLE_MAX_INDEX_ROWS) {
            slotClearInfoButton();
        }
    }
}

void MainWindow::slotBaudrateIndex(int index)
{
    if (index < 0 || index >= 10) {
        return ;
    }

    ui->timer0LineEdit->setText(g_tim0[index]);
    ui->timer1LineEdit->setText(g_tim1[index]);
}

void MainWindow::slotThreadMessage(const QString &message)
{
    QMessageBox::critical(this, tr("thread"), message, QMessageBox::Ok);
}

void MainWindow::slotFilterDialog()
{    
    if (QDialog::Accepted == filterDlg->exec()) {
        m_filtermode = filterDlg->getFilterMode();
        m_hexlen = filterDlg->getFilterParam(m_fromlowhex, m_tohighhex);
    }
}

void MainWindow::slotTriggerDialog()
{    
    if (QDialog::Accepted == triggerDlg->exec()) {
        m_triggermode = triggerDlg->getTriggerMode();
        triggerDlg->getTriggerParam(m_trigmap);
    }
}

void MainWindow::slotSendTextChanged(const QString &text)
{
    QString hexstr = text;

    hexstr.replace(QLatin1String(" "), QLatin1String(""));
    int bytes = hexstr.length() / 2;
    ui->DLCLineEdit->setText(QString("%1").arg(bytes, 2, 16, QLatin1Char('0')));
}

void MainWindow::slotSaveLogButton()
{
    QString strtime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString filePath = QFileDialog::getSaveFileName(this, tr("另存为"), 
        tr("%1/CAN-LOG-%2.txt").arg(CETCAN_TESTTOOL_DIR, strtime), 
        "文本文件 (*.txt);;所有文件 (*.*)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("error"), tr("%1 文件保存失败 !").arg(filePath), QMessageBox::Ok);
            return ;
        }

        int count = 0;
        QTextStream out(&file);
        for (int i = 0; i < m_model->columnCount(); ++i) { // 表头打印
            QStandardItem *item = m_model->horizontalHeaderItem(i);
            if (!item) {
                continue;
            }
            if (0 == count) {
                out << "  ";
            }
            out << item->text() << "\t";
            if (0 == count) {
                out << "\t\t";
            } else if (1 == count) {
                out << "\t\t";
            } else if (2 == count) {
                out << "\t";
            }
            count++;
        }
        out << "\n";
        
        for (int i = 0 ; i < m_model->rowCount(); ++i) { // 表内容打印
            for (int j = 0; j < m_model->columnCount(); ++j) {
                QStandardItem *item = m_model->item(i, j);
                if (!item) {
                    continue;
                }
                out << item->text() << "\t";
            }
            out << "\n";
            out.flush();
        }
        file.close();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    /* 多行复制处理 */
    if(event->matches(QKeySequence::Copy)) {
        QString copied_text;
        QModelIndexList selected_indexs = ui->infoTableView->selectionModel()->selectedIndexes();
        int current_row = selected_indexs.at(0).row();
        for(int i = 0; i < selected_indexs.count(); ++i) {
            if (current_row != selected_indexs.at(i).row()) {
                current_row = selected_indexs.at(i).row();
                copied_text.append("\n");
            } else if (0 != i) {
                copied_text.append("\t");
            }
            copied_text.append(selected_indexs.at(i).data().toString());
        }
        copied_text.append("\n");
        QApplication::clipboard()->setText(copied_text);
        event->accept();
        return ;
    }
    
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    if (QMessageBox::question(this, tr("退出确认"), tr("您确定要退出该应用 ？\t"),
         QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        event->accept(); // 不会将事件传递给组件的父组件
    } else {
        event->ignore();
    }
}

