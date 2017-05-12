#include "altctrl.h"
#include <stdio.h>   /*标准输入输出定义*/
#include <stdlib.h>  /*标准函数库定义*/
#include <unistd.h>  /*UNIX标准函数定义*/
#include <sys/types.h>  /*基本系统数据类型*/
#include <sys/stat.h>   /*unix/linux系统定义文件状态*/
#include <fcntl.h>   /*文件控制定义*/
#include <termios.h>  /*PPSIX 终端控制定义*/
#include <errno.h>    /*错误号定义*/
#include <string.h>

#include <QDebug>
#include <QMutex>

#include <iostream>

extern QMutex mutex;
extern bool isusing;

//AltCtrl::AltCtrl() : m_hPort(0),stopped(false),isnew(false), m_distance({0}),m_energy({0}),m_correlation({0}), m_temperature({0}),m_watertemp(0) {}
AltCtrl::AltCtrl(int fd) : m_hPort(fd),stopped(false),isnew(false),m_distance({0}),m_energy({0}),m_correlation({0}), m_temperature({0}) ,m_watertemp(0) {}

void AltCtrl::run()
{
    int a = 0;
    while(!stopped){
        usleep(200000);
        GetData(ALT0018,m_distance[ALT0018],m_energy[ALT0018],m_correlation[ALT0018],m_temperature[ALT0018]);
        GetData(ALT0020,m_distance[ALT0020],m_energy[ALT0020],m_correlation[ALT0020],m_temperature[ALT0020]);
        isnew = true;
        m_watertemp = (m_temperature[0]+m_temperature[1])/2.0;
      //  std::cout <<"recieved altimeters data!!" << std::dec<< a++ <<std::endl;
    }
}

int AltCtrl::OpenCommPort(char *devname)
{
    int ttyd;
    struct termios ttyopt;

    if (devname == NULL)
    {
        perror("invalid devname!");
        return -EINVAL;
    }

    ttyd = open(devname, O_RDWR | O_NOCTTY);

   // fcntl(ttyd,F_SETFL,0);
   // ttyd = open(devname, O_RDWR | O_NOCTTY);

    if (ttyd < 0)
    {
        perror("cannot open serial port!");
        return -EIO;
    }

    //fcntl(ttyd, F_SETFL, FNDELAY);

   // tcgetattr(ttyd, &ttyopt);
    memset(&ttyopt, 0, sizeof ttyopt);  //在一段内存块中填充某个给定的值，它对较大的结构体或数组进行清零操作的一种最快方法
                                  //void *memset(void *s,  int c, size_t n)
                                  //把一个char a[20]清零, 是 memset(a, 0, 20)

    /* Set baudrate 9600bps */
    cfsetispeed(&ttyopt, B115200);
    cfsetospeed(&ttyopt, B115200);


    /* Set parity none, 1 stop bit, 8 data bits */
    ttyopt.c_cflag &= ~PARENB;
    ttyopt.c_cflag &= ~CSTOPB;
    ttyopt.c_cflag &= ~CSIZE;
    ttyopt.c_cflag |= CS8;
    ttyopt.c_iflag |= INPCK;
    ttyopt.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL);

    /* Enable reciever and set local mode */
    ttyopt.c_cflag |= (CLOCAL | CREAD);

    /* Ignore control input */
    ttyopt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* Disable flow control */
    ttyopt.c_iflag &= ~(IXON | IXOFF | IXANY);
    ttyopt.c_oflag &= ~(OPOST | ONLCR | OCRNL);

    ttyopt.c_cc[VTIME] = 1;   //设置超时0 seconds
    ttyopt.c_cc[VMIN] = 50;    //define the minimum bytes data to be readed
                   // opt.c_cc[VMIN]=0:Update the options and do it NOW  !!!!!!

    tcsetattr(ttyd, TCSANOW, &ttyopt);

    m_hPort = ttyd;
    return ttyd;

}

void AltCtrl::GetData(int ID, float &distance, float &energy, float &correlation, float &temperature)
{


    int dw;

    char sendbuf[3];
    strncpy(sendbuf,"#p\r",3);
    if (ID == ALT0018) sendbuf[1] = 'p';
    else if(ID == ALT0020)  sendbuf[1] = 'q';

    dw = write(m_hPort,sendbuf,3);
    if(dw != 3){
        qDebug()<<"Send to altimeter failed\n";
        return;
    }
    usleep(200);
    char recvbuf[42] = {0};
    dw = read(m_hPort,recvbuf,42);

    if(dw != 42){
        qDebug()<<"Recieve from altimeter failed\n";
        return;
    }
    char premble[6];
    memcpy(premble,recvbuf,6);
//    int a = strcmp(premble,"$ISADI");
    if(premble[0] == '$'){
        char temp[7];
        float dis,ener,corr;
        strncpy(temp,&recvbuf[7],7);
        dis = atof(temp);

        memset(temp,0,sizeof(temp));
        memcpy(temp,&recvbuf[17],6);
        ener = atof(temp);   //energy = 0.707 is perfect;

        memcpy(temp,&recvbuf[24],6);
        corr = atof(temp);

        memset(temp,0,sizeof(temp));
        memcpy(temp,&recvbuf[31],4);
        temperature = atof(temp);

        distance = dis;
        energy = ener;
        correlation = corr;
    }

}
