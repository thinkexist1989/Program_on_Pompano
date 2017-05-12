#ifndef PLATFORMCTRL_H
#define PLATFORMCTRL_H

#include <QThread>

class PlatformCtrl : public QThread
{
public:
 //   PlatformCtrl();
    PlatformCtrl(int fd = 0);

    int OpenCommPort(char *devname);
private:
    volatile bool stopped; // thread stop signal
    int 	m_hPort;			// Handle of port
protected:
    void run();
};

#endif // PLATFORMCTRL_H
