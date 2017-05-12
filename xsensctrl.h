#ifndef XSENSCTRL_H
#define XSENSCTRL_H

#include <QThread>


// Preamble indicator of start of packet -->250(0xFA)
#define PRMBL 0xFA

// BID or Address(master device)
#define BID 0xFF

//MID message identifier

#define WakeUp          0x3E    //To HOST

#define GOTOCONFIG      0x30    //TO MT
#define GOTOCONFIGACK   0x31    //TO HOST
#define GOTOMEASUREMENT 0x10    //TO MT
#define GOTOMEASUREMENTACK  0x11    //TO HOST
#define RESET           0x40    //TO MT
#define RESETACK        0x41    //TO HOST
#define REQDID          0x00    //TO MT
#define DEVICEID        0x01    //TO HOST
#define INITMT          0x02    //TO MT
#define INITMTRESULT   0x03    //TO HOST
#define SETOUTPUTCONFIG 0xC0    //TO MT
#define SETOUTPUTCONFIGACK 0xC1    //TO HOST
#define SETSYNCSETTINGS 0x2C    //TO MT
#define SETSYNCSETTINGSACK 0x2D //TO HOST

#define REQDATA         0x34    //TO MT
#define MTDATA2         0x36    //TO HOST


#define TX_MAX          20
#define RX_MAX          200

#define FRAME_NO_SEND      -1
#define FRAME_OK            0
#define FRAME_NO_RESPONSE  -2
#define FRAME_BAD_CS       -3
#define FRAME_RX_ERROR     -4

class XsensCtrl : public QThread
{
public:
  //  XsensCtrl();
    XsensCtrl(int fd = 0);
private:
    int m_hPort;   //Handle of port
    unsigned char   m_TxBuffer[TX_MAX];     //Send-buffer
    unsigned char   m_RxBuffer[RX_MAX];     //Send-buffer

    unsigned char   CalcCrc8(unsigned char* Data, int datalen); // check-sum
    int TransferDataDirect(int Tx_datalen , int Rx_datalen);
    int TransferData(int Tx_datalen , int Rx_datalen);
protected:
    void run();
private:
    volatile bool stopped; //thread stop signal
public:
    int OpenCommPort(char *devname);
    void CloseCommPort();// Close port
    bool GoToConfig(); //Change to Configuration Mode
    bool GoToMeasurement(); // Change to Measurment Mode
    bool ReqDID(unsigned char* ID); //get Xsens Device ID
    bool SetOutputConfig();
    bool SetSyncSettings();
    bool ReqData();

    float m_roll;
    float m_pitch;
    float m_yaw;

    bool isnew;

};

#endif // XSENSCTRL_H
