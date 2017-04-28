#ifndef ALTCTRL_H
#define ALTCTRL_H

#include <QThread>
#include <QMutex>

#define ALT0018 0
#define ALT0020 1

class AltCtrl : public QThread
{
public:
    AltCtrl();
    AltCtrl(int fd);
protected:
    void run();

private:
    volatile bool stopped; // thread stop signal
    int 	m_hPort;			// Handle of port
public:
    float m_distance[2];
    float m_energy[2];
    float m_correlation[2];
    float m_temperature[2];
    QMutex  mutex;

    int OpenCommPort(char *devname);
    void GetData(int ID, float& distance, float& energy, float& correlation, float& temperature);
};

#endif // ALTCTRL_H
