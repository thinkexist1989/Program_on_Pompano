///////////////////////////////////////////////////////////////////////////////////////////////////
// kellerctrl.cpp

#include "kellerctrl.h"

#include <stdio.h>   /*标准输入输出定义*///

#include <stdlib.h>  /*标准函数库定义*/
#include <unistd.h>  /*UNIX标准函数定义*/
#include <sys/types.h>  /*基本系统数据类型*/
#include <sys/stat.h>   /*unix/linux系统定义文件状态*/
#include <fcntl.h>   /*文件控制定义*/
#include <termios.h>  /*PPSIX 终端控制定义*/
#include <errno.h>    /*错误号定义*/
#include <string.h>

#include <iostream>
#include <QMutex>

extern QMutex mutex; // defined in main.cpp


KellerCtrl::KellerCtrl(int fd): m_hPort(fd),pressval(0),tempval(0),m_nDevice(250),stopped(false), isnew(false) {}

//KellerCtrl::KellerCtrl() : m_hPort(0),pressval(0),tempval(0),m_nDevice(250),stopped(false), isnew(false) {}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// PORT-FUNCTIONS


void KellerCtrl::InitCommunication()
{	
	// Set handle of open port to default value
    m_hPort = 0;
    _bEcho = false;
    m_bRepeatIfError = false;
}

int KellerCtrl::OpenCommPort(char* devname)
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

    ttyopt.c_cc[VTIME] = 1;   //设置超时100 ms
    ttyopt.c_cc[VMIN] = 50;    //define the minimum bytes data to be readed
                   // opt.c_cc[VMIN]=0:Update the options and do it NOW  !!!!!!

    tcsetattr(ttyd, TCSANOW, &ttyopt);



    m_hPort = ttyd;
    return ttyd;

}

void KellerCtrl::CloseCommPort()
{
    usleep(500);
    if(m_hPort > 0) {
        close(m_hPort);
        m_hPort = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// CRC16-CALCULATION

_u16 KellerCtrl::CalcCrc16(_u8* Data,_u16 nCnt)
{
    bool	b;
	_u16	crc= 0xFFFF;

	for(int i=0;i<nCnt;i++)	{
		crc^= *Data++;
		for(int n=0;n<8;n++) {
			if(crc%2==1)
                b= true;
			else
                b= false;
			crc/= 2;
			if(b)
				crc^= 0xA001;
		}
	}
	return crc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// SEND AND RECEIVE

int KellerCtrl::TransferData(_u16 nTX,_u16 nRX)
{
    bool	bRepeat;

	do {
        bRepeat = false;

        nResult = TransferDataDirect(nTX,nRX);
		
		switch(nResult) {
		case COMM_NO_RESPONSE:
		case COMM_BAD_RESPONSE:
		case COMM_BAD_LENGTH:
		case COMM_BAD_CRC:
		case COMM_BAD_ADDR: 
		case COMM_BAD_EXCEPTION:
		case COMM_RX_ERROR:
			if(m_bRepeatIfError) {
                usleep(400);
                bRepeat= false;
			}
			break;
		}
	} while(bRepeat);

    return nResult;
}


int KellerCtrl::TransferDataDirect(_u16 nTX,_u16 nRX)
{
     int  		dw;
	_u16		n,nCrc;

	// 1. Reset port
	
	// 2. Flash buffer

	// 3. Add CRC16-checksum
    nCrc = CalcCrc16(&m_TxBuffer[0],nTX);
	m_TxBuffer[nTX++]= HIBYTE(nCrc);
	m_TxBuffer[nTX++]= LOBYTE(nCrc);

	// 4. Send
    dw = write(m_hPort,m_TxBuffer,nTX);
    n = _u16(dw);

    if(n!= nTX)
		return COMM_NOT_NTX;
																				
    if(_bEcho) {
		// mit Echo Empfangen
        dw = read(m_hPort,m_RxBuffer,nTX+nRX);
		n= _u16(dw);	
	}
	else {
		// ohne Echo Empfangen
        dw = read(m_hPort,&m_RxBuffer[nTX],nRX);
		n= _u16(dw+nTX);
	}
	
	// 6. Echo empfangen ?
	if(_bEcho) {
		if(n < nTX+nRX)
			return COMM_NO_RESPONSE;
	}
	else{
		if(n < nRX)
			return COMM_NO_RESPONSE;
	}

	// 7. Check echo
	if(_bEcho) {	
		for(int i=0;i<nTX;i++) 
			if(m_TxBuffer[i] != m_RxBuffer[i]) 
				return COMM_BAD_RESPONSE;
	}

	// 8. Check length of answer
	if(n!=nTX+nRX)	
		return COMM_BAD_LENGTH;

	// 9. Check CRC16
	nCrc= CalcCrc16(&m_RxBuffer[nTX],n-nTX-2);

	if((HIBYTE(nCrc)!=m_RxBuffer[n-2]) || (LOBYTE(nCrc)!=m_RxBuffer[n-1])) 
		return COMM_BAD_CRC;

	// 10. Check deviceaddress
	if(m_TxBuffer[0]!=m_RxBuffer[nTX])	
		return COMM_BAD_ADDR;

	// 11. Function ok?
	if(m_TxBuffer[1]==m_RxBuffer[nTX+1]) 
		return COMM_OK;

	// 12. Exception ?
	if(m_TxBuffer[1]==m_RxBuffer[nTX+1]+128) 
		switch(m_RxBuffer[nTX+2]) {
		case 1:
			return COMM_EXCEPTION_1;
		case 2:
			return COMM_EXCEPTION_2;
		case 3:
			return COMM_EXCEPTION_3;
		case 32:
			return COMM_EXCEPTION_32;
		default:
			return COMM_BAD_EXCEPTION;
		}

	// 13. General error
	return COMM_RX_ERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// BUS-FUNCTIONS

// Initialise the device
int KellerCtrl::F48(_u8* Class,_u8* Group,_u8* Year,_u8* Week,_u8* Buffer,_u8* State)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 48;
	
	if(TransferData(2,10) == COMM_OK) {
		*Class= m_RxBuffer[6];
		*Group= m_RxBuffer[7];
		*Year= m_RxBuffer[8];
		*Week= m_RxBuffer[9];
		*Buffer= m_RxBuffer[10];
		*State= m_RxBuffer[11];
	}

	return nResult;
}


// Read the serialnumber
int KellerCtrl::F69(_u32* SN)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 69;
	
	if(TransferData(2,8) == COMM_OK) {
		*SN= 256*65536*(_u32)m_RxBuffer[6]+65536*(_u32)m_RxBuffer[7]+256*(_u32)m_RxBuffer[8]+(_u32)m_RxBuffer[9];
	}

	return nResult;
}

// Read the  actual value of "Channel"
int KellerCtrl::F73(_u8 Channel, float* fValue)
{
	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 73;
	m_TxBuffer[2]= Channel;
	

	if (TransferData(3,9) == COMM_OK) {

		_u8	bteArr[4];

		bteArr[0] = m_RxBuffer[10];
		bteArr[1] = m_RxBuffer[9];
		bteArr[2] = m_RxBuffer[8];
		bteArr[3] = m_RxBuffer[7];
		
		*fValue= *(float*)(&bteArr[0]);
	}	
	return nResult;
}

// Set zero
int KellerCtrl::F95(_u8 Command)
{

	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 95;
	m_TxBuffer[2]= Command;
	
	TransferData(3,5);
	
	return nResult;
}

// Set to value
int KellerCtrl::F95_Val(_u8 Command, float fValue)
{

	FloatByte fb;

	fb.f= 0;
	for(int i=0;i<4;i++)
		fb.b[i]= 0;

	fb.f= fValue;

	m_TxBuffer[0]= m_nDevice;
	m_TxBuffer[1]= 95;
	m_TxBuffer[2]= Command;
	m_TxBuffer[3]= fb.b[3];
	m_TxBuffer[4]= fb.b[2];
	m_TxBuffer[5]= fb.b[1];
	m_TxBuffer[6]= fb.b[0];
	
	TransferData(7,5);
	
	return nResult;
}

void KellerCtrl::run()  //thread running function
{
    _u8 nClass;
    _u8 nGroup;
    _u8 nYear;
    _u8 nWeek;
    _u8 nBuffer;
    _u8 nState;
    _u32 nSN;

    // First F48 call should wake-up the device
    F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);
    usleep(200);
    int nRes = F48(&nClass, &nGroup, &nYear, &nWeek, &nBuffer, &nState);

    if(nRes != COMM_OK) {
        std::cout <<"Initialisation not possible !" << std::endl;
        return;
    }
    std::cout << "Keller ID: " << (int)nClass <<"." << (int)nGroup << "Version: " <<(int)nYear << (int)nWeek <<std::endl;

    nRes = F69(&nSN);

    // Check the answer
    if(nRes!= COMM_OK){
        std::cout <<"Reading of SN not possible !" << std::endl;
        return;
    }
    std::cout <<"Keller SN: " << nSN << std::endl;


    int a = 0;
   // usleep(100000);
    while(!stopped){

       // usleep(200);
        nRes = F73(PRESSURE,&pressval);
        if(nRes!= COMM_OK){
            std::cout << "Reading of pressure value not possible !" << std::endl;
            return;
        }
        isnew = true; // Recieved New Data!!
     //   std::cout << a++ << "Pressure: " << pressval;
     //   nRes = F73(TEMPRETURE,&tempval);

     //   if(nRes!= COMM_OK){
     //       std::cout << "Reading of pressure value not possible !" << std::endl;
    //        return;
     //   }
      //  std::cout <<"reicieved keller data!" << std::dec << a++<<std::endl;
        usleep(100000); //50ms
     //   std::cout << "       Tempreture: " << tempval << std::endl;

    }

}

