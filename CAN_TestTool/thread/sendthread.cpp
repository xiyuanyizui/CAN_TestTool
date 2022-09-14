#include "sendthread.h"

#include <QDebug>

SendThread::SendThread() : QThread()
{

}

SendThread::~SendThread()
{

}

void SendThread::setThrowSendSignal(bool throwsendsignal)
{
    m_throwsendsignal = throwsendsignal;
}

void SendThread::setDevParam(DWORD devtype, DWORD device, DWORD chanel)
{
    m_devtype = devtype;
    m_device  = device;
    m_chanel  = chanel;
}

void SendThread::addSendData(const SEND_DATA_T *send)
{
    if (send->sendnum > 0) {
        mutex.lock();
        m_sendlist << *send;
        #if 1
        qDebug("[Send] sendnum:%lu, intervalms:%dms, SendType:%d, ID:0x%08x, "
            "TimeStamp:%u, TimeFlag:%u, ExternFlag:0x%02x, RemoteFlag:0x%02x, DataLen:%d "
            "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", send->sendnum, 
            send->intervalms, send->can.SendType, send->can.ID, send->can.TimeStamp, 
            send->can.TimeFlag, send->can.ExternFlag, send->can.RemoteFlag, 
            send->can.DataLen, send->can.Data[0], send->can.Data[1], send->can.Data[2], 
            send->can.Data[3], send->can.Data[4], send->can.Data[5], send->can.Data[6], 
            send->can.Data[7]);
        #endif
        mutex.unlock();
    }
}

void SendThread::run()
{
    int intervalus = 10;

    while (true) {
        usleep(20);

        mutex.lock();
        for (int i = 0; i < m_sendlist.count(); ++i) {
            SEND_DATA_T senddata = m_sendlist[i];
            while (senddata.sendnum) {
                senddata.sendnum--;
                if (m_throwsendsignal) {
                    emit signalSendData(senddata.can);
                }

                if (1 != VCI_Transmit(m_devtype, m_device, m_chanel, &senddata.can, 1)) {
                    emit signalMessage(tr("发送失败!"));
                    continue;
                }

                if (senddata.intervalms) {
                    intervalus = senddata.intervalms * 1000;
                    if (senddata.intervalms > 4) {
                        intervalus -= 2000;
                    } else if (senddata.intervalms > 1) {
                        intervalus -= 1600;
                    } else {
                        intervalus -= 200;
                    }
                }
                usleep(intervalus);
            }
        } 
        m_sendlist.clear();
        mutex.unlock();
    }
}

