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

#include <vector>
#include <bitset>

#define CAN_INIT_OK     0
#define CAN_INIT_FAIL   -1

struct Motor
{
    int   status; // motor status : fault==>-1, stop==>0, start==>2, enable==>1
    float pwm; //-99.99~99.99 %
    int   speed; //-9999~9999 r/min
    int   current; //0 ~ 9999mA
};





class CanCtrl : public QThread
{
public:
    CanCtrl();
protected:
    void run();
private:
    volatile bool stopped; //线程停止标志
public:
    std::bitset<8> bs; //bits = 00000001  then motor1's data is new
    struct sockaddr_can addr;
    struct can_frame frame;
    struct can_frame send_frame;
    struct ifreq ifr;
    int s;
    int InitCan();

    std::vector<Motor> motorvec;

    float cabin_temp;
    float wall_temp;

    inline std::vector<Motor>& get_motorvec() {return motorvec;}
    inline float get_cabin_temp() {return cabin_temp;}
    inline float get_wall_temp() {return wall_temp;}
    inline int sgn(float a) {return a >= 0?1:-1;}

    void motorctrl(); //run motorctrl can send the Motor.pwm to motors
};


#endif // CANCTRL_H
