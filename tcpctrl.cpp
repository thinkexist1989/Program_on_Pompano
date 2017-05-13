#include "tcpctrl.h"
#include <iostream>

#define SERVER_PORT   6666
#define SERVER_IP     "192.168.0.100"
#define TCP_FRAME_LEN 255



TcpCtrl::TcpCtrl(AltCtrl &alt, CanCtrl &can, KellerCtrl &keller, XsensCtrl &xsens)
    : alt(alt), can(can), keller(keller), xsens(xsens)
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

void TcpCtrl::Send()
{
    if(sock_connect_fd == -1){
        perror("No Connecting!!");
        return;
    }
    else{
        char sendbuf[] = "I gotcha!!";
        int n = send(sock_connect_fd, sendbuf, TCP_FRAME_LEN, 0);
    }

}


void TcpCtrl::run()
{
   while(!stopped){
       socklen_t sin_size = sizeof(struct sockaddr_in);
       sock_connect_fd = accept(sock_server_fd, (struct sockaddr*)&client_addr, &sin_size);
       if(sock_connect_fd < 0){
           perror("Error Accepting");
           return;
       }
       std::cout << "Get Connection from: " << inet_ntoa(client_addr.sin_addr) <<std::endl;

       while(!stopped){
           char recvbuf[1024];
           int n = recv(sock_connect_fd, recvbuf, TCP_FRAME_LEN, 0);
           if(n < 0){
               perror("Error Recieving data!!");
           }

           std::cout << recvbuf <<std::endl;
       }
   }
}
