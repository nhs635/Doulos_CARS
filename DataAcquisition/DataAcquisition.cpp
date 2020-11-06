
#include "DataAcquisition.h"

#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#include <DataAcquisition/DataProcess/DataProcess.h>


DataAcquisition::DataAcquisition(Configuration* pConfig)
    : m_pDaq(nullptr), m_pDataProc(nullptr)
{
    m_pConfig = pConfig;

    // Create SignatecDAQ object
    m_pDaq = new SignatecDAQ;
    m_pDaq->DidStopData += [&]() { m_pDaq->_running = false; };

    // Create data process object
    m_pDataProc = new DataProcess;
	m_pDataProc->SendStatusMessage += [&](const char* msg) { m_pConfig->msgHandle(msg); };
	m_pDataProc->_operator.SendStatusMessage += [&](const char* msg) { m_pConfig->msgHandle(msg); };
    m_pDataProc->setParameters(m_pConfig);
    m_pDataProc->_operator(np::Uint16Array2(m_pConfig->nScans, m_pConfig->nPixels * m_pConfig->nTimes), m_pDataProc->_params);
}

DataAcquisition::~DataAcquisition()
{
    if (m_pDaq) delete m_pDaq;
    if (m_pDataProc) delete m_pDataProc;
}


bool DataAcquisition::InitializeAcquistion()
{
    // Set boot-time buffer
    SetBootTimeBufCfg(PX14_BOOTBUF_IDX, m_pConfig->nSegments * m_pConfig->nTimes);
	 
    // Parameter settings for DAQ & Axsun Capture
	m_pDaq->nSegments = m_pConfig->nSegments;
	m_pDaq->nTimes = m_pConfig->nTimes;
	m_pDaq->PreTrigger = m_pConfig->px14PreTrigger;
	m_pDaq->TriggerDelay = m_pConfig->px14TriggerDelay;
	m_pDaq->DcOffset = m_pConfig->px14DcOffset;
    m_pDaq->BootTimeBufIdx = PX14_BOOTBUF_IDX;

    // Initialization for DAQ
    if (!(m_pDaq->set_init()))
    {
        StopAcquisition();
        return false;
    }

    return true;
}

bool DataAcquisition::StartAcquisition()
{
	// Parameter settings for DAQ
	m_pDaq->PreTrigger = m_pConfig->px14PreTrigger;
	m_pDaq->TriggerDelay = m_pConfig->px14TriggerDelay;
	m_pDaq->DcOffset = m_pConfig->px14DcOffset;

    // Start acquisition
    if (!(m_pDaq->startAcquisition()))
    {
        StopAcquisition();
        return false;
    }

    return true;
}

void DataAcquisition::StopAcquisition()
{
    // Stop thread
    m_pDaq->stopAcquisition();
}


void DataAcquisition::GetBootTimeBufCfg(int idx, int& buffer_size)
{
    buffer_size = m_pDaq->getBootTimeBuffer(idx);
}

void DataAcquisition::SetBootTimeBufCfg(int idx, int buffer_size)
{
    m_pDaq->setBootTimeBuffer(idx, buffer_size);
}

void DataAcquisition::SetPreTrigger(int pre_trigger)
{
	m_pDaq->setPreTrigger(pre_trigger);
}

void DataAcquisition::SetTriggerDelay(int trigger_delay)
{
	m_pDaq->setTriggerDelay(trigger_delay);
}

void DataAcquisition::SetDcOffset(int offset)
{
	m_pDaq->setDcOffset(offset);
}


void DataAcquisition::ConnectDaqAcquiredFlimData(const std::function<void(int, const np::Array<uint16_t, 2>&)> &slot)
{
    m_pDaq->DidAcquireData += slot;
}

void DataAcquisition::ConnectDaqStopFlimData(const std::function<void(void)> &slot)
{
    m_pDaq->DidStopData += slot;
}

void DataAcquisition::ConnectDaqSendStatusMessage(const std::function<void(const char*, bool)> &slot)
{
    m_pDaq->SendStatusMessage += slot;
}
