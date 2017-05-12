#ifndef LIGHTCTRL_H
#define LIGHTCTRL_H

#include <QThread>

class LightCtrl : public QThread
{
public:
    LightCtrl(int fd = 0);
protected:
    void run();

private:
    volatile bool stopped; // thread stop signal
    int 	m_hPort;			// Handle of port

public:
    char isnew;

    short int brightness[2]; //current brightness value from 0 to 76==> 0x00 ~ 0x4c
    short int turn_off_temp[2];
    short int turn_on_temp[2];
    short int current_temp[2];

    int OpenCommPort(char *devname);

    void add_checksum_and_tail(char* data);
    int  set_brightness(int id,int b);
    int  set_turn_on_temp(int id, int b);
    int  set_turn_off_temp(int id, int b);
    int  get_current_temp(int id);




};

#endif // LIGHTCTRL_H
