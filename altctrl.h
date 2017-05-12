#ifndef ALTCTRL_H
#define ALTCTRL_H

#include <QThread>


#define ALT0018 0
#define ALT0020 1

class AltCtrl : public QThread
{
public:
   // AltCtrl();
    AltCtrl(int fd = 0);
protected:
    void run();

private:
    volatile bool stopped; // thread stop signal
    int 	m_hPort;			// Handle of port
public:
    bool  isnew;
    float m_distance[2];
    float m_energy[2];
    float m_correlation[2];
    float m_temperature[2];

    float m_watertemp;

    int OpenCommPort(char *devname);
    void GetData(int ID, float& distance, float& energy, float& correlation, float& temperature);

    inline float* get_distance() {return m_distance;}
    inline float* get_energy() {return m_energy;}
    inline float* get_correlation() {return m_correlation;}
    inline float* get_temperature() {return m_temperature;}
};

#endif // ALTCTRL_H
