
#include "ZaberStage.h"
#include "../QSerialComm.h"

#include <Doulos/Configuration.h>

#include <chrono>


ZaberStage::ZaberStage() :
    m_pSerialComm(nullptr),
    port_name(ZABER_PORT),
    microstep_size(ZABER_MICRO_STEPSIZE),
    conversion_factor(ZABER_CONVERSION_FACTOR),
    _running(false),
    cur_dev(1),
    comm_num(1),
    is_moving(false)
{
	m_pSerialComm = new QSerialComm;
}


ZaberStage::~ZaberStage()
{
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool ZaberStage::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->m_bIsConnected)
	{
		if (m_pSerialComm->openSerialPort(port_name))
		{
            char msg[256];
            sprintf(msg, "ZABER: Success to connect to %s.", port_name);
            SendStatusMessage(msg, false);

            // Define callback lambda function
			m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			{
                static char msg[256];
                static int j = 0;

                for (int i = 0; i < (int)len; i++)
                {
                    msg[j++] = buffer[i];

                    if (buffer[i] == '\n')
                    {
                        msg[j] = '\0';
                        SendStatusMessage(msg, false);
                        if (is_moving) DidMovedRelative(msg);
                        j = 0;
                    }

                    if (j == 256) j = 0;
                }
            };

            // Define and execute current pos monitoring thread
            t_monitor = std::thread([&]() {
                _running = true;
                while (_running)
                {
                    if (!_running)
                        break;

                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    if (is_moving) DidMonitoring();
                }
            });
		}
		else
		{
            char msg[256];
            sprintf(msg, "ZABER: Success to disconnect to %s.", port_name);
            SendStatusMessage(msg, false);
			return false;
		}
	}
	else
        SendStatusMessage("ZABER: Already connected.", false);

	return true;
}


void ZaberStage::DisconnectDevice()
{
    if (t_monitor.joinable())
    {
        _running = false;
        t_monitor.join();
    }

	if (m_pSerialComm->m_bIsConnected)
    {
		m_pSerialComm->closeSerialPort();

        char msg[256];
        sprintf(msg, "ZABER: Success to disconnect to %s.", port_name);
        SendStatusMessage(msg, false);
	}
}


void ZaberStage::StopMonitorThread()
{
    if (t_monitor.joinable())
    {
        _running = false;
        t_monitor.join();
    }
}


void ZaberStage::Home(int _stageNumber)
{
    cur_dev = _stageNumber;

    char msg[256];

    char buff[100];
    sprintf_s(buff, sizeof(buff), "/%02d 0 %02d home\n", _stageNumber, comm_num);
    sprintf(msg, "ZABER: Send: %s", buff);
    SendStatusMessage(msg, false);

    comm_num++;
    if (comm_num == 96) comm_num = 1;

    m_pSerialComm->writeSerialPort(buff);
    is_moving = true;
}


void ZaberStage::Stop(int _stageNumber)
{
    cur_dev = _stageNumber;

    char msg[256];

    char buff[100];
    sprintf_s(buff, sizeof(buff), "/%02d 0 %02d stop\n", _stageNumber, comm_num);
    sprintf(msg, "ZABER: Send: %s", buff);
    SendStatusMessage(msg, false);

    comm_num++;
    if (comm_num == 96) comm_num = 1;

    m_pSerialComm->writeSerialPort(buff);
}


void ZaberStage::MoveAbsolute(int _stageNumber, double position)
{
    cur_dev = _stageNumber;

    int data = (int)round(position * 1000.0 / microstep_size);

    char msg[256];

    char buff[100];
    sprintf_s(buff, sizeof(buff), "/%02d 0 %02d move abs %d\n", _stageNumber, comm_num, data);
    sprintf(msg, "ZABER: Send: %s", buff);
    SendStatusMessage(msg, false);

    comm_num++;
    if (comm_num == 96) comm_num = 1;

    m_pSerialComm->writeSerialPort(buff);
    is_moving = true;
}


void ZaberStage::MoveRelative(int _stageNumber, double position)
{
    cur_dev = _stageNumber;

    int data = (int)round(position * 1000.0 / microstep_size);

    char msg[256];

    char buff[100];
    sprintf_s(buff, sizeof(buff), "/%02d 0 %02d move rel %d\n", _stageNumber, comm_num, data);
    sprintf(msg, "ZABER: Send: %s", buff);
    SendStatusMessage(msg, false);

    comm_num++;
    if (comm_num == 96) comm_num = 1;

    m_pSerialComm->writeSerialPort(buff);
    is_moving = true;
}


void ZaberStage::SetTargetSpeed(int _stageNumber, double speed)
{
    cur_dev = _stageNumber;

    int data = (int)round(speed * 1000.0 / microstep_size * conversion_factor);

    char msg[256];

    char buff[100];
    sprintf_s(buff, sizeof(buff), "/%02d 0 %02d set maxspeed %d\n", _stageNumber, comm_num, data);
    sprintf(msg, "ZABER: Send: %s", buff);
    SendStatusMessage(msg, false);

    comm_num++;
    if (comm_num == 96) comm_num = 1;

    m_pSerialComm->writeSerialPort(buff);
}


void ZaberStage::GetPos()
{
    char msg[256];

    char buff[100];
    sprintf_s(buff, sizeof(buff), "/%02d 0 %02d get pos\n", cur_dev, comm_num);
    sprintf(msg, "ZABER: Send: %s", buff);
    SendStatusMessage(msg, false);

    comm_num++;
    if (comm_num == 96) comm_num = 1;

    m_pSerialComm->writeSerialPort(buff);
}
