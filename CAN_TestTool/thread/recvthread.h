#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include "ControlCAN.h"
#include <QThread>

class RecvThread : public QThread
{
    Q_OBJECT

public:
    RecvThread();
    ~RecvThread();
    void setDevParam(DWORD devtype, DWORD device, DWORD chanel);
    void setThrowRecvSignal(bool throwrecvsignal);    
    void setFilterParam(int mode, const UINT *fromLowHex, const UINT *toHighHex, int len);

protected:
    void run();

private:
    volatile bool m_throwrecvsignal;
        
    DWORD m_devtype;
    DWORD m_device;
    DWORD m_chanel;
    
signals:
    void signalRecvData(const VCI_CAN_OBJ *info, int recvlen);
};

#endif // RECVTHREAD_H
