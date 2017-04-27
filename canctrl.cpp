#include "canctrl.h"
#include <iostream>

CanCtrl::CanCtrl():
    stopped(false)
{
}

void CanCtrl::run()
{
    int nbytes = 0, cnt = 0;
    while(!stopped){
        frame.can_id = 0x601;
        frame.can_dlc = 8;
        frame.data[0] = 0x01;
        frame.data[1] = 0xff;
        frame.data[2] = 0x60;
        frame.data[3] = 0x01;
        frame.data[4] = 0x00;
        frame.data[5] = 0x00;
        frame.data[6] = 0x00;
        frame.data[7] = 0x00;

        nbytes = write(s, &frame, sizeof(struct can_frame));

        nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes > 0){
           // printf("Get frame: <%04x> ", frame.can_id);
            std::cout<<std::dec<< cnt++ <<" :  Get frame: "<<std::hex<<frame.can_id<<'\t';
            std::cout<<"frame dlc: "<<(int)frame.can_dlc<<'\t';

           // for (i = 0; i < frame.can_dlc && i < sizeof(frame.data); i++){
            for (int i = 0; i < frame.can_dlc && i < sizeof(frame.data); i++){
                std::cout<<std::hex<<(int)frame.data[i]<<' ';
                // printf("%02x ", frame.data[i]);
            }
            std::cout<<std::endl;
              //  printf("\n");
        }
        else
            printf("Get frame failed!\n");
    }
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

    return CAN_INIT_OK;
}
