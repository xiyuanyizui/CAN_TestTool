#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ControlCAN.h"
#include "recvthread.h"
#include "sendthread.h"
#include "filterdialog.h"
#include "triggerdialog.h"
#include "cetlicenseinterface.h"
#include "cetupdateinterface.h"

#include <QMainWindow>
#include <QLabel>
#include <QStandardItemModel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	void initDllPlugin();
    void insertInfoViewCanInfo(bool isSend, const VCI_CAN_OBJ *info);
    void setInfoViewHeader();
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent * event);

public slots:
    void slotActivateDialog();
    void slotStyleActions(QAction *action);
    void slotConnectButton();
    void slotStartCANButton();
    void slotResetCANButton();
    void slotClearInfoButton();
    void slotSendDataButton();
    void slotFilterDialog();
    void slotTriggerDialog();
    void slotClearCountButton();
    void slotSaveLogButton();
    void slotRecvDataHandle(const VCI_CAN_OBJ *info, int recvlen);
    void slotShowSendFrame(const VCI_CAN_OBJ &info);
    void slotBaudrateIndex(int index);
    void slotThreadMessage(const QString &message);
    void slotSendTextChanged(const QString &text);

private:
    QObject *loadDllPlugin(const QString &dllname);

signals:
    void signalActivate();

private:
    Ui::MainWindow *ui;
    RecvThread *recvThread;
    SendThread *sendThread;
    FilterDialog *filterDlg;
    TriggerDialog *triggerDlg;
    CetLicenseInterface *m_cetLicIface;
	CetUpdateInterface *m_cetUpIface;

    QStandardItemModel *m_model;

    QMap<QString, int> m_mapDevtype;
    DWORD m_devtype;
    DWORD m_device;
    DWORD m_chanel;

    QLabel *sendNumLabel;
    QLabel *recvNumLabel;

    uint m_indexCount;
    uint m_recvFrameCount;
    uint m_sendFrameCount;

    /* 过滤参数 */
    int m_hexlen;
    int m_filtermode;
    uint32_t m_fromlowhex[200];
    uint32_t m_tohighhex[200];

    /* 触发参数 */
    int m_triggermode;
    QMap<QString, QList<SEND_DATA_T>> m_trigmap;
};

#endif // MAINWINDOW_H
