#include "tcpctrl.h"
#include <iostream>
#include <bitset>
#include <QMutex>

#define SERVER_PORT   6666
#define SERVER_IP     "192.168.0.100"
#define TCP_SEND_LEN 108
#define TCP_RECIEVE_LEN 56

extern QMutex canmutex;

TcpCtrl::TcpCtrl(AltCtrl &alt, CanCtrl &can, KellerCtrl &keller, XsensCtrl &xsens, LightCtrl& light, PlatformCtrl& plat)
    : alt(alt), can(can), keller(keller), xsens(xsens),light(light), plat(plat), sock_server_fd(-1),sock_connect_fd(-1),stopped(false)
{
}


void TcpCtrl::StartServer()
{
    sock_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_server_fd < 0){
        perror("Error Opening Socket!!");
        return;
    }
    std::cout << "open socket sccess!!" <<std::endl;

    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if(bind(sock_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Error Binding!!");
        return;
    }

    if(listen(sock_server_fd, 5) < 0) {
        perror("Error Listening!!");
        return;
    }

    std::cout << "listening success!!"<< std::endl;

    this->start();

}

void TcpCtrl::Send()  //send data to upper PC ==> DON'T forget clear the isnew of every sensor.
{
    if(sock_connect_fd == -1){
     //   perror("No Connecting!!");
        return;
    }

    char sendbuf[TCP_SEND_LEN] = {0};
    sendbuf[0] = 0xFF; sendbuf[1] = 0xFE;  sendbuf[2] = 0x80;

    sendbuf[3] = can.bs.to_ulong();

    for(int i = 0; i<8; i++){
        memcpy(&sendbuf[4+i*4],&can.motorvec[i].speed,2); //speed of motors
        memcpy(&sendbuf[6+i*4],&can.motorvec[i].current,2); // current of motors
    }

    sendbuf[36] = xsens.isnew;

    memcpy(&sendbuf[37],&xsens.m_roll,4); // xsens data
    memcpy(&sendbuf[41],&xsens.m_pitch,4);
    memcpy(&sendbuf[45],&xsens.m_yaw,4);

    sendbuf[49] = keller.isnew;

    memcpy(&sendbuf[50],&keller.pressval,4);

    sendbuf[54] = alt.isnew;

    for(int i = 0; i <2 ;i++){
        memcpy(&sendbuf[55+i*12],&alt.m_distance[i],4);
        memcpy(&sendbuf[59+i*12],&alt.m_energy[i],4);
        memcpy(&sendbuf[63+i*12],&alt.m_correlation[i],4);
    }

    memcpy(&sendbuf[79],&can.cabin_temp,4);
    memcpy(&sendbuf[83],&can.wall_temp,4);
    memcpy(&sendbuf[87],&alt.m_watertemp,4);

    memset(&sendbuf[91],0,17);

    int n = send(sock_connect_fd, sendbuf, TCP_SEND_LEN, 0);

    can.bs.reset();  //clear the flag bit.
    xsens.isnew = false;
    keller.isnew = false;
    alt.isnew = false;
}


void TcpCtrl::run()
{
   while(!stopped){
       socklen_t sin_size = sizeof(struct sockaddr_in);
       sock_connect_fd = accept(sock_server_fd, (struct sockaddr*)&client_addr, &sin_size);
       if(sock_connect_fd < 0){
       //    perror("Error Accepting");
           return;
       }
       std::cout << "Get Connection from: " << inet_ntoa(client_addr.sin_addr) <<std::endl;

       while(!stopped){
           char recvbuf[TCP_RECIEVE_LEN];
           int n = recv(sock_connect_fd, recvbuf, TCP_RECIEVE_LEN, 0);
           if(n < 0){
               perror("Error Recieving data!!");
           }

           if(n == TCP_RECIEVE_LEN){
               if(recvbuf[3] != 0){ //Recieved New Motor Order
                   canmutex.lock();
                   can.bsCtrl = recvbuf[3];
                   for(int i = 0; i<8 ;i++){
                        memcpy(&can.motorvec[i].pwm,&recvbuf[4+i*4],4);
                    }
                   canmutex.unlock();

                   std::cout <<"Recieved New Motor Order" <<std::endl;
               }
               if(recvbuf[36] == 0x01){ //Recieved new three-axises platform Oder
                   for(int i = 0; i < 4 ;i++){
                       //TODO:
                   }

                   std::cout <<"Recieved New Platform Order" <<std::endl;
               }
               if(recvbuf[53] != 0){
                   //TODO:SET LIGHT
                   light.brightness[0] = recvbuf[54];
                   light.brightness[1] = recvbuf[55];
                   light.start(); //thread light is not a while loop

                   std::cout <<"Recieved New Light Order" <<std::endl;
               }
           }
       }
   }
}
