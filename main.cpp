#include <QCoreApplication>
#include <kellerctrl.h>
#include <xsensctrl.h>
#include <canctrl.h>
#include <iostream>
#include <iomanip>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    KellerCtrl keller;
    keller.InitCommunication();
    int fd485 = keller.OpenCommPort("/dev/ttyO2");
    XsensCtrl xsens;
    int fd232 = xsens.OpenCommPort("/dev/ttyO1");
    keller.start();
    xsens.start();


    while(1){
        std::cout << std::fixed << std::setprecision(2)<<"roll: " << xsens.m_roll <<"   pitch: "<< xsens.m_pitch <<"   yaw: "<<xsens.m_yaw;
        std::cout << std::setprecision(6)<<"   pressure: "<<keller.pressval << "   tempreture: "<<keller.tempval <<std::endl;
        usleep(50000);
    }
    return a.exec();
}
