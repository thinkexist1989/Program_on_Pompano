#ifndef CANCTRL_H
#define CANCTRL_H

#include <QThread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define CAN_INIT_OK     0
#define CAN_INIT_FAIL   -1

class CanCtrl : public QThread
{
public:
    CanCtrl();
protected:
    void run();
private:
    volatile bool stopped; //线程停止标志
public:
    struct sockaddr_can addr;
    struct can_frame frame;
    struct ifreq ifr;
    int s;
    int InitCan();
};

#endif // CANCTRL_H
