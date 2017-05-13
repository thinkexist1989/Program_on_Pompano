#include <QCoreApplication>

#include <iostream>
#include <iomanip>
#include <QtNetwork>
#include <QMutex>

//Device Class Headers
#include <kellerctrl.h>     // keller pressure sumitter
#include <xsensctrl.h>      // xsens mti AHRS
#include <canctrl.h>        // canbus for motors
#include <altctrl.h>        // ISA500 altimeters
#include <lightctrl.h>      // underwater light
#include <platformctrl.h>   // three-axises plantform

#include <tcpctrl.h>    // tcpip communications with upper PC



QMutex canmutex;
bool   isusing = false;


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    std::cout << "Please wait for a while........." << std::endl;
    usleep(200000);
    std::cout << "Initializing all sensors...." << std::endl;

    KellerCtrl keller; // object of Keller
    keller.InitCommunication();
   // int fd485 = keller.OpenCommPort("/dev/ttyUSB0");  // 485 port
   // keller.start();

  //  AltCtrl altimeter(fd485); //object of ISA500 altimeter
    AltCtrl altimeter;
    altimeter.OpenCommPort("/dev/ttyO2");
    altimeter.start();

    XsensCtrl xsens; //object of Xsens
    int fd232 = xsens.OpenCommPort("/dev/ttyO1"); // 232 port
    //xsens.start();

    CanCtrl can;  //object of Canbus
    can.InitCan();
    can.start();

    LightCtrl light; //object of underwater light
   // light.OpenCommPort("/dev/ttyUSB0");
   // light.start();

    PlatformCtrl plat; //object of platform


    TcpCtrl tcp(altimeter,can,keller,xsens,light,plat); //object of tcp
    tcp.StartServer();

   // can.check();
    while(1){
        tcp.Send(); //send data to upper PC
        usleep(100000);
    }
    return a.exec();
}
