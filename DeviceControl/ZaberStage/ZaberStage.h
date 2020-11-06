#ifndef ZABER_STAGE_H_
#define ZABER_STAGE_H_

#include <Common/callback.h>

#include <iostream>
#include <thread>


class QSerialComm;

class ZaberStage
{
public:
    ZaberStage();
    ~ZaberStage();

public:
	bool ConnectDevice();
    void DisconnectDevice();
    void StopMonitorThread();

public:
    inline bool getIsMoving() { return is_moving; }
    inline void setIsMoving(bool status) { is_moving = status; }

public:
    void Home(int _stageNumber);
    void Stop(int _stageNumber);
    void MoveAbsolute(int _stageNumber, double position);
    void MoveRelative(int _stageNumber, double position);
    void SetTargetSpeed(int _stageNumber, double speed);
    void GetPos();

public:
    callback<const char*> DidMovedRelative;
    callback<void> DidMonitoring;
    callback2<const char*, bool> SendStatusMessage;

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;

    std::thread t_monitor;
    bool _running;

    double microstep_size;
    double conversion_factor;

    int cur_dev;
    int comm_num;
    bool is_moving;
};

#endif
