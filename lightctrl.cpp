#include "lightctrl.h"
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

#define ID1 0
#define ID2 1



LightCtrl::LightCtrl(int fd): m_hPort(fd),isnew(0) {}

void LightCtrl::run()
{
    if(isnew == 0x01) // 0 0 0 1   id = 1
        set_brightness(ID1, brightness[ID1]);
    else if(isnew == 0x02)
        set_brightness(ID2, brightness[ID2]);
    else if(isnew == 0x03){
        set_brightness(ID1, brightness[ID1]);
        set_brightness(ID2, brightness[ID2]);
    }
}

int LightCtrl::OpenCommPort(char *devname)
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

   //fcntl(ttyd, F_SETFL, FNDELAY); //设置为非阻塞

   // tcgetattr(ttyd, &ttyopt);
    memset(&ttyopt, 0, sizeof ttyopt);  //在一段内存块中填充某个给定的值，它对较大的结构体或数组进行清零操作的一种最快方法
                                  //void *memset(void *s,  int c, size_t n)
                                  //把一个char a[20]清零, 是 memset(a, 0, 20)

    /* Set baudrate 9600bps */
    cfsetispeed(&ttyopt, B9600);
    cfsetospeed(&ttyopt, B9600);


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

    ttyopt.c_cc[VTIME] = 20;   //设置超时0 seconds
    ttyopt.c_cc[VMIN] = 100;    //define the minimum bytes data to be readed
                   // opt.c_cc[VMIN]=0:Update the options and do it NOW  !!!!!!

    tcflush(ttyd, TCIOFLUSH);
    tcsetattr(ttyd, TCSANOW, &ttyopt);

    m_hPort = ttyd;
    return ttyd;
}

void LightCtrl::add_checksum_and_tail(char *data)
{
    int cnt = data[1];
    data[cnt-3] = 0; //checksum
    for(int i = 1;i < (cnt - 3); i++){
        data[cnt - 3] += data[i];
    }
    data[cnt - 2] = 0x0D;
    data[cnt - 1] = 0x0A;
}

int LightCtrl::set_brightness(int id, int b)
{
    char sendbuf[8];
    sendbuf[0] = 0x02;
    sendbuf[1] = 0x08;
    sendbuf[2] = id;
    sendbuf[3] = 0xb1;
    sendbuf[4] = b;

    add_checksum_and_tail(sendbuf);

    int dw = write(m_hPort,sendbuf,8);
    if(dw != 8){
        qDebug()<<"Send to Light Failed\n";
        return 0;
    }

    usleep(200);

    char recvbuf[100] = {0};
    dw = read(m_hPort,recvbuf,100);

    if(strstr(recvbuf,"Error!") != NULL){
        brightness[id] = 76;
        std::cout <<"Light: ID:" << id << "Out of Range! Maximum of brightness is 76!! current brightness: 76!!"<<std::endl;
    }
    else if(strstr(recvbuf,"Current brightness") != NULL){
        char val[3];
        brightness[id] = b;
        std::cout <<"Light: ID:" << id << "   Current brightness: " << brightness[id] <<std::endl;
    }
    else{
        std::cout << "Light: Set brightness Failed!" <<std::endl;
        return 0;
    }
    return 1;
}

int LightCtrl::get_current_temp(int id)
{
    char sendbuf[7];
    sendbuf[0] = 0x02;
    sendbuf[1] = 0x07;
    sendbuf[2] = id;
    sendbuf[3] = 0x93;

    add_checksum_and_tail(sendbuf);

    int dw = write(m_hPort,sendbuf,8);
    if(dw != 8){
        qDebug()<<"Send to Light Failed\n";
        return 0;
    }

    usleep(200);

    char recvbuf[100] = {0};
    dw = read(m_hPort,recvbuf,100);

    if(strstr(recvbuf,"Current temperature") != NULL){
        char val[2];
        memcpy(&val,&recvbuf[dw-2],2);
        current_temp[id] = atoi(val);
        printf("Light: ID:%d  Current temperature : %d",id, current_temp[id]);
    }
    else{
        std::cout << "Light: Set brightness Failed!" <<std::endl;
        return 0;
    }

    return 1;
}
