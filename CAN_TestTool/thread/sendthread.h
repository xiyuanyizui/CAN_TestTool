#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include "ControlCAN.h"
#include <QThread>
#include <QList>
#include <QMutex>

struct SEND_DATA_T {
    VCI_CAN_OBJ can;
    ULONG sendnum;
    int intervalms;
};

class SendThread : public QThread
{
    Q_OBJECT

public:
    SendThread();
    ~SendThread();
    void setDevParam(DWORD devtype, DWORD device, DWORD chanel);
    void setThrowSendSignal(bool throwsendsignal);
    void addSendData(const SEND_DATA_T *send);
    

protected:
    void run();

private:
    QMutex mutex;
    volatile bool m_throwsendsignal;

    DWORD m_devtype;
    DWORD m_device;
    DWORD m_chanel;
    
    QList<SEND_DATA_T> m_sendlist;
        
signals:
    void signalSendData(const VCI_CAN_OBJ &info);
    void signalMessage(const QString &message);
};


#endif // SENDTHREAD_H
