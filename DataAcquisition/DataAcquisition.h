#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Doulos/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>

class SignatecDAQ;
class DataProcess;


class DataAcquisition : public QObject
{
	Q_OBJECT

public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

public:
    inline DataProcess* getDataProc() const { return m_pDataProc; }

public:
    bool InitializeAcquistion();
    bool StartAcquisition();
    void StopAcquisition();

public:
    void GetBootTimeBufCfg(int idx, int& buffer_size);
    void SetBootTimeBufCfg(int idx, int buffer_size);
	void SetPreTrigger(int pre_trigger);
	void SetTriggerDelay(int trigger_delay);
	void SetDcOffset(int offset);
	
public:
    void ConnectDaqAcquiredFlimData(const std::function<void(int, const np::Array<uint16_t, 2>&)> &slot);
    void ConnectDaqStopFlimData(const std::function<void(void)> &slot);
    void ConnectDaqSendStatusMessage(const std::function<void(const char*, bool)> &slot);

private:
	Configuration* m_pConfig;

    SignatecDAQ* m_pDaq;
    DataProcess* m_pDataProc;
};

#endif // DATAACQUISITION_H
