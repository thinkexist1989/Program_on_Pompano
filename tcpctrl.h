#ifndef TCPCTRL_H
#define TCPCTRL_H

#include <QThread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#include <altctrl.h>
#include <canctrl.h>
#include <kellerctrl.h>
#include <xsensctrl.h>

union res
{
    float f;
    int   i;
    unsigned char data[4];
};



class TcpCtrl : public QThread
{
public:
   TcpCtrl(AltCtrl& alt, CanCtrl& can, KellerCtrl& keller, XsensCtrl& xsens);
protected:
   void run();
private:
   volatile bool stopped; // thread stop signal
public:
   int sock_server_fd;
   int sock_connect_fd;
   struct sockaddr_in server_addr;
   struct sockaddr_in client_addr;

   AltCtrl& alt;
   CanCtrl& can;
   KellerCtrl& keller;
   XsensCtrl& xsens;

   void StartServer();
   void Send();
};

#endif // TCPCTRL_H
