#include "xsensctrl.h"

#include <stdio.h>   /*标准输入输出定义*/
#include <stdlib.h>  /*标准函数库定义*/
#include <unistd.h>  /*UNIX标准函数定义*/
#include <sys/types.h>  /*基本系统数据类型*/
#include <sys/stat.h>   /*unix/linux系统定义文件状态*/
#include <fcntl.h>   /*文件控制定义*/
#include <termios.h>  /*PPSIX 终端控制定义*/
#include <errno.h>    /*错误号定义*/
#include <string.h>

#include <iostream>

XsensCtrl::XsensCtrl() : m_hPort(0),stopped(false) {}


XsensCtrl::XsensCtrl(int fd) : m_hPort(fd),stopped(false) {}

unsigned char XsensCtrl::CalcCrc8(unsigned char *Data, int datalen)
{
    int crc = 0x00;
    // FRAME = Preamle + BID + MID + DATALEN + DATA + CS
    for(int i = 1; i< datalen + 4 ; i++){  //excluding preamble
        crc += Data[i];
    }
    if(crc >= 0xFF){
        crc = (~crc + 1) & 0xFF;  //取补码
    }
    return crc;
}

//send and recieve   nTX nRX ==>DATA LENGTH
int  XsensCtrl::TransferDataDirect(int Tx_datalen , int Rx_datalen)
{
    int n;
    unsigned char cs; //check-sum
    int nTX = Tx_datalen + 4; //length of frame

    cs = CalcCrc8(m_TxBuffer, Tx_datalen);
    m_TxBuffer[nTX++] = cs;
    n = write(m_hPort, m_TxBuffer, nTX);

    if(n != nTX){
        return  FRAME_NO_SEND;
    }

    int nRX = Rx_datalen + 5;
    n = read(m_hPort, m_RxBuffer, nRX);
    if(n != nRX)
        return FRAME_NO_RESPONSE;
    //CHECK-SUM
    cs = CalcCrc8(m_RxBuffer, Rx_datalen);
    //CHECK-SUM OK?
    if(cs != m_RxBuffer[nRX - 1])
        return FRAME_BAD_CS;



    return m_RxBuffer[2];
}

int XsensCtrl::TransferData(int Tx_datalen, int Rx_datalen)
{
    return TransferDataDirect(Tx_datalen, Rx_datalen);
}

void XsensCtrl::run()
{
    GoToConfig();
    usleep(1000);

    unsigned char ID[4];
    ReqDID(ID);
    unsigned long num;
    num = (ID[0]<<24)+(ID[1]<<16)+(ID[2]<<8)+ID[3];

    std::cout<< "Device ID: " << std::hex << num <<std::endl;

    if(SetOutputConfig())
        std::cout << "set output config success"<<std::endl;

    if(SetSyncSettings())
        std::cout << "set sync success"<<std::endl;

    if(GoToMeasurement())
        std::cout << "begin measuring:"<<std::endl;
    usleep(100000); // must wait some time
    while(!stopped){
        usleep(50000);
        ReqData();
//        std::cout << m_roll << ' '<< m_pitch << ' ' <<m_yaw <<std::endl;
    }

}

int XsensCtrl::OpenCommPort(char *devname)
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

    //fcntl(ttyd, F_SETFL, FNDELAY); //设置非阻塞串口模式

   // tcgetattr(ttyd, &ttyopt);
    memset(&ttyopt, 0, sizeof ttyopt);  //在一段内存块中填充某个给定的值，它对较大的结构体或数组进行清零操作的一种最快方法
                                  //void *memset(void *s,  int c, size_t n)
                                  //把一个char a[20]清零, 是 memset(a, 0, 20)

    /* Set baudrate 115200bps */
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
    ttyopt.c_cc[VMIN] = 30;    //define the minimum bytes data to be readed
                   // opt.c_cc[VMIN]=0:Update the options and do it NOW  !!!!!!

    tcsetattr(ttyd, TCSANOW, &ttyopt);

    m_hPort = ttyd;
    return ttyd;

}

bool XsensCtrl::GoToConfig()
{
    m_TxBuffer[0] = PRMBL; //Preamble
    m_TxBuffer[1] = BID; //BID
    m_TxBuffer[2] = GOTOCONFIG;
    m_TxBuffer[3] = 0;

    if(TransferData(0,0) == GOTOCONFIGACK)
        return true;

}

bool XsensCtrl::GoToMeasurement()
{
    m_TxBuffer[0] = PRMBL; //Preamble
    m_TxBuffer[1] = BID; //BID
    m_TxBuffer[2] = GOTOMEASUREMENT;
    m_TxBuffer[3] = 0;

    if(TransferData(0,0) == GOTOMEASUREMENTACK)
        return true;

    return false;
}

bool XsensCtrl::ReqDID(unsigned char *ID)
{
    m_TxBuffer[0] = PRMBL; //Preamble
    m_TxBuffer[1] = BID; //BID
    m_TxBuffer[2] = REQDID;
    m_TxBuffer[3] = 0;

    if(TransferData(0,4) == DEVICEID){
        for(int i = 0; i < 4; i++){
            ID[i] = m_RxBuffer[i + 4];
        }
        return true;
    }

    return false;
}

bool XsensCtrl::SetOutputConfig()
{
    m_TxBuffer[0] = PRMBL; //Preamble
    m_TxBuffer[1] = BID; //BID
    m_TxBuffer[2] = SETOUTPUTCONFIG;
    m_TxBuffer[3] = 4;
    m_TxBuffer[4] = 0x20;
    m_TxBuffer[5] = 0x30;
    m_TxBuffer[6] = 0x00;
    m_TxBuffer[7] = 0x00;

    if(TransferData(4,4) == SETOUTPUTCONFIGACK)
        return true;

    return false;
}

bool XsensCtrl::SetSyncSettings()
{
    memset(m_TxBuffer,0,12);
    m_TxBuffer[0] = PRMBL; //Preamble
    m_TxBuffer[1] = BID; //BID
    m_TxBuffer[2] = SETSYNCSETTINGS;
    m_TxBuffer[3] = 12;
    m_TxBuffer[4] = 0x08;
    m_TxBuffer[5] = 0x06;
    m_TxBuffer[6] = 0x01;

    if(TransferData(12,0) == SETSYNCSETTINGSACK)
        return true;

    return false;
}

bool XsensCtrl::ReqData()
{
    m_TxBuffer[0] = PRMBL; //Preamble
    m_TxBuffer[1] = BID; //BID
    m_TxBuffer[2] = REQDATA;
    m_TxBuffer[3] = 0;

    if(TransferData(0,15) == MTDATA2){
        unsigned char bteArr[4];

        bteArr[0] = m_RxBuffer[10]; bteArr[1] = m_RxBuffer[9]; bteArr[2] = m_RxBuffer[8]; bteArr[3] = m_RxBuffer[7];
        m_roll = *(float*)(&bteArr[0]);

        bteArr[0] = m_RxBuffer[14]; bteArr[1] = m_RxBuffer[13]; bteArr[2] = m_RxBuffer[12]; bteArr[3] = m_RxBuffer[11];
        m_pitch = *(float*)(&bteArr[0]);

        bteArr[0] = m_RxBuffer[18]; bteArr[1] = m_RxBuffer[17]; bteArr[2] = m_RxBuffer[16]; bteArr[3] = m_RxBuffer[15];
        m_yaw= *(float*)(&bteArr[0]);

        return true;
    }

    return false;
}
