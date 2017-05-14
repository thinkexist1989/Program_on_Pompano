/**********************Driver Control Order**********************
﻿/**********************Driver Control Order**********************
 * fault   code: 2B 40 60 01 80 00 00 00
 * stop    code: 2B 40 60 01 0E 00 00 00
 * start   code: 2B 40 60 01 07 00 00 00
 * enable  code: 2B 40 60 01 0F 00 00 00
 *
 * control and return order:
 * positive run: 23 FF 60 02 PL PH 00 00  ===> where PL PH is the high(low) bytes of pwm(0~99.99)*10. for example: pwm 76.5% --> 76.5*100 = 7650(dec) ==> 0x1DE2, so PL = 0xE2, PH = 0x1D.
 * negtive  run: 23 FF 60 04 PL PH 00 00
 * check   info: 23 FF 60 06 00 00 00 00
 *
 * returns:
 * return  info: 23 FF 60 DR VL VH IL IH  ===> where DR is the direction of motor 03 ==> positive, 05 ==> negtive. VL VH is the high(low) bytes of rotation speed. IL IH is the high(low) bytes of driver's current.
 ***************************************************************/

#include "canctrl.h"
#include <iostream>
#include <math.h>
#include <QMutex>

#define FRAMELEN  8
#define MOTOR1    0x581 //return is 0x581 , send is 0x601
#define MOTOR9    0x589

extern QMutex canmutex;

union res4
{
    float f;
    int   i;
    unsigned char data[4];
};


union res2
{
    short  int   i;
    unsigned char data[2];
};



CanCtrl::CanCtrl():
    stopped(false),motorvec(8),bs(0),cabin_temp(0),wall_temp(0),bsCtrl(0)
{
}

void CanCtrl::run()
{
    cnt = 0;
    int nn = 0;
    motorstart();
    while(!stopped){
        get_motor_info(); //给8个电机发送查询指令并返回转速和电流，受到CAN总线限制，查询一轮250ms左右。巨TM慢，日了狗了
        //std::cout <<bs.to_ulong() <<std::endl;
        canmutex.lock();
        motorctrl();
        canmutex.unlock();
        if(nn%4 == 0){
            get_temp();
           // bTemp = false;
        }

    //    std::cout << "                                                                                          " <<std::dec << nn++ <<std::endl;
    }   
    motorstop();
}

int CanCtrl::InitCan()
{
    char *ifname = "can0";

    if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0){
        perror("error while opening socket");
        return CAN_INIT_FAIL;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(s,SIOCGIFINDEX,&ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("Error in socket bind");
        return CAN_INIT_FAIL;
    }

    int i = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, i | O_NONBLOCK);

    return CAN_INIT_OK;
}

void CanCtrl::motorctrl()
{
    for(int i = 0; i < 8 ; i++){
        if(bsCtrl[i] == 1){
            res2 val;
            send_frame.can_id = 0x601 + i;
            send_frame.can_dlc = 8;
            val.i = abs((int)(motorvec[i].pwm * 100));
            send_frame.data[0] = 0x23;
            send_frame.data[1] = 0xFF;
            send_frame.data[2] = 0x60;
            send_frame.data[3] = motorvec[i].pwm >=0? 0x02 : 0x04;
            send_frame.data[4] = val.data[0];
            send_frame.data[5] = val.data[1];
            send_frame.data[6] = 0x00;
            send_frame.data[7] = 0x00;

            int nbytes = write(s, &send_frame, sizeof(struct can_frame));
            usleep(20000);
        }
    }
}

void CanCtrl::check()
{
    //23 FF 60 06 00 00 00 00
    for(int i = 0; i < 9 ; i++){
        send_frame.can_id = 0x601 + i;
        send_frame.can_dlc = 8;
        send_frame.data[0] = 0x23;
        send_frame.data[1] = 0xFF;
        send_frame.data[2] = 0x60;
        send_frame.data[3] = 0x06;
        send_frame.data[4] = 0x00;
        send_frame.data[5] = 0x00;
        send_frame.data[6] = 0x00;
        send_frame.data[7] = 0x00;

        int nbytes = write(s, &send_frame, sizeof(struct can_frame));
        usleep(20000);
    }
}

void CanCtrl::frameanalysis()
{
    if (frame.can_dlc == FRAMELEN){
        //<info display>
        std::cout<<std::dec<< cnt++ <<" :  Get frame: "<<std::hex<<frame.can_id<<'\t';
        std::cout<<"frame dlc: "<<(int)frame.can_dlc<<'\t';

        for (int i = 0; i < frame.can_dlc && i < sizeof(frame.data); i++){
            std::cout<<std::hex<<(int)frame.data[i]<<' ';
        }
        std::cout<<std::endl;
        //</info display>

        if(frame.can_id == MOTOR9){  //temperature module
            int a[2] = {frame.data[4], frame.data[5]};
            int   b  = (a[1]<<8) + a[0];
            cabin_temp = ((float)(((b>>12)&0xf)*1000+((b>>8)&0xf)*100+((b>>4)&0xf)*10+(b&0xf)))/100.0;

            int aa[2] = {frame.data[6], frame.data[7]};
                  b  = (aa[1]<<8) + aa[0];
            wall_temp  = ((float)(((b>>12)&0xf)*1000+((b>>8)&0xf)*100+((b>>4)&0xf)*10+(b&0xf)))/100.0;
        }
        else{
            int no = frame.can_id - MOTOR1; //No. of Motor
            char premble = frame.data[0];
            switch (premble) {
            case 0x2B:
                switch (frame.data[4]) {
                case 0x80:
                    motorvec[no].status = -1;
                    break;
                case 0x0E:
                    motorvec[no].status =  0;
                    break;
                case 0x07:
                    motorvec[no].status =  2;
                    break;
                case 0x0F:
                    motorvec[no].status =  1;
                    break;
                default:
                    break;
                }
                break;
            case 0x23:
                switch (frame.data[3]) {
                case 0x00:
                    bs.set(no);
                    break;
                case 0x02:

                    break;
                case 0x03:{
                    memcpy(&motorvec[no].speed,&frame.data[4],2);
                    memcpy(&motorvec[no].current,&frame.data[6],2);
                    bs.set(no); //set flag bit
                    break;
                }
                case 0x04:

                    break;
                case 0x05:{
                    memcpy(&motorvec[no].speed,&frame.data[4],2);
                    motorvec[no].speed = -motorvec[no].speed;

                    memcpy(&motorvec[no].current,&frame.data[6],2);
                    bs.set(no); // set flag bit
                    break;
                }
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
    else
        std::cout << "Get frame failed!" << std::endl;
}

void CanCtrl::motorstart()
{
    for(int i = 0; i < 8 ; i++){

        send_frame.can_id = 0x601 + i;
        send_frame.can_dlc = 8;
        send_frame.data[0] = 0x2b;
        send_frame.data[1] = 0x40;
        send_frame.data[2] = 0x60;
        send_frame.data[3] = 0x01;
        send_frame.data[4] = 0x07;
        send_frame.data[5] = 0x00;
        send_frame.data[6] = 0x00;
        send_frame.data[7] = 0x00;

        int nbytes = write(s, &send_frame, sizeof(struct can_frame));
     //   nbytes = read(s, &frame, sizeof(struct can_frame));

        frameanalysis();

        usleep(20000);
    }
}

void CanCtrl::motorstop()
{
    for(int i = 0; i < 8 ; i++){
        //2B 40 60 01 0E 00 00 00
        send_frame.can_id = 0x601 + i;
        send_frame.can_dlc = 8;
        send_frame.data[0] = 0x2b;
        send_frame.data[1] = 0x40;
        send_frame.data[2] = 0x60;
        send_frame.data[3] = 0x01;
        send_frame.data[4] = 0x0e;
        send_frame.data[5] = 0x00;
        send_frame.data[6] = 0x00;
        send_frame.data[7] = 0x00;

        int nbytes = write(s, &send_frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        frameanalysis();

        usleep(20000);
    }
}

void CanCtrl::get_motor_info()
{
    for(int i = 0; i < 8 ; i++){
        send_frame.can_id = 0x601 + i;
        send_frame.can_dlc = 8;
        send_frame.data[0] = 0x23;
        send_frame.data[1] = 0xFF;
        send_frame.data[2] = 0x60;
        send_frame.data[3] = 0x06;
        send_frame.data[4] = 0x00;
        send_frame.data[5] = 0x00;
        send_frame.data[6] = 0x00;
        send_frame.data[7] = 0x00;

        int nbytes = write(s, &send_frame, sizeof(struct can_frame));
        nbytes = read(s, &frame, sizeof(struct can_frame));
        frameanalysis();

        usleep(20000);
    }
}

void CanCtrl::get_temp()
{
    usleep(20000);
    send_frame.can_id = 0x609;
    send_frame.can_dlc = 8;
    send_frame.data[0] = 0x23;
    send_frame.data[1] = 0x00;
    send_frame.data[2] = 0x60;
    send_frame.data[3] = 0x06;
    send_frame.data[4] = 0x00;
    send_frame.data[5] = 0x33;
    send_frame.data[6] = 0x11;
    send_frame.data[7] = 0x22;

    int nbytes = write(s, &send_frame, sizeof(struct can_frame));
    nbytes = read(s, &frame, sizeof(struct can_frame));
    frameanalysis();
    usleep(20000);
}

