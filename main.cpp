#include <QCoreApplication>
#include <kellerctrl.h>
#include <xsensctrl.h>
#include <canctrl.h>
#include <altctrl.h>
#include <iostream>
#include <iomanip>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    KellerCtrl keller;
    keller.InitCommunication();
    int fd485 = keller.OpenCommPort("/dev/ttyO2");

    AltCtrl altimeter(fd485);
    XsensCtrl xsens;
    int fd232 = xsens.OpenCommPort("/dev/ttyO1");
   // keller.start();
   // xsens.start();
    altimeter.start();

    while(1){
     //   std::cout << std::fixed << std::setprecision(2)<<"roll: " << xsens.m_roll <<"   pitch: "<< xsens.m_pitch <<"   yaw: "<<xsens.m_yaw;
     //   std::cout << std::setprecision(6)<<"   pressure: "<<keller.pressval << "   tempreture: "<<keller.tempval <<std::endl;
        std::cout << std::fixed << std::setprecision(4) << "ALT0018--->dis: " << altimeter.m_distance[ALT0018] <<"  temp: "<< altimeter.m_temperature[ALT0018]<<"   ALT0020--->dis: " << altimeter.m_distance[ALT0020] <<"  temp: "<< altimeter.m_temperature[ALT0020] <<std::endl;
     //   usleep(50000);
        usleep(250000);
    }
    return a.exec();
}
