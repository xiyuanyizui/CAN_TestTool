#include "recvthread.h"

/* 该值已调试好 不需要改 改大会出 SID命令没有按顺序打印 */
#define THREAD_SLEEP_US 20

#define RECV_MAX_NUM 500

RecvThread::RecvThread() : QThread()
{

}

RecvThread::~RecvThread()
{

}

void RecvThread::setThrowRecvSignal(bool throwrecvsignal)
{
    m_throwrecvsignal = throwrecvsignal;
}

void RecvThread::setDevParam(DWORD devtype, DWORD device, DWORD chanel)
{
    m_devtype = devtype;
    m_device  = device;
    m_chanel  = chanel;
}

void RecvThread::run()
{
    VCI_ERR_INFO errInfo;
    VCI_CAN_OBJ frameinfo[RECV_MAX_NUM];
    ULONG recvNum = 0;
    int rcvLength = 0;
    while (true) {
        usleep(THREAD_SLEEP_US);
        // ----- RX-timeout test ----------------------------------------------
        recvNum = VCI_GetReceiveNum(m_devtype, m_device, m_chanel);
        if (recvNum > 0) {
            rcvLength = VCI_Receive(m_devtype, m_device, m_chanel, frameinfo, recvNum, 50);
            if (rcvLength <= 0) {
                VCI_ReadErrInfo(m_devtype, m_device, m_chanel, &errInfo);
            } else {
                #if 0
                for (int i = 0; i < rcvLength; ++i) {
                    qWarning("rcvLength:%d, i:%d, id:0x%08x, Data:%02x %02x %02x %02x", \
                        rcvLength, i, frameinfo[i].ID, frameinfo[i].Data[0], frameinfo[i].Data[1],
                        frameinfo[i].Data[2], frameinfo[i].Data[3]);
                }
                #endif
                if (m_throwrecvsignal) {
                    /* 处理接收的数据 */
                    emit signalRecvData(frameinfo, rcvLength);
                }
            }
        }
    }
}


