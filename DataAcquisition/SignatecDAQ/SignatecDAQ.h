#ifndef SIGNATEC_DAQ_H
#define SIGNATEC_DAQ_H

#include <Doulos/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>

#include <iostream>
#include <thread>

#define MAX_MSG_LENGTH 2000


typedef struct _px14hs_* HPX14;

class SignatecDAQ
{
public:
	explicit SignatecDAQ();
	virtual ~SignatecDAQ();

	// callbacks
	callback2<int, const np::Uint16Array2 &> DidAcquireData;
	callback<void> DidStopData;
	callback2<const char*, bool> SendStatusMessage;

public:
	bool initialize();
	bool set_init();
	int getBootTimeBuffer(int idx);
	bool setBootTimeBuffer(int idx, int buffer_samples);
	bool setDcOffset(int offset);
	bool setPreTrigger(int pre_trigger);
	bool setTriggerDelay(int trigger_delay);

	bool startAcquisition();
	void stopAcquisition();

public:
	int nChannels, nSegments, nTimes;
	unsigned int GainLevel;
	unsigned int AcqRate;
	unsigned int PreTrigger, TriggerDelay;
	unsigned int DcOffset;
	unsigned short BootTimeBufIdx;
	bool UseVirtualDevice, UseInternalTrigger;

	bool _running;

private:
	bool _dirty;

	// thread
	std::thread _thread;
	void run();

private:
	// PX14400 board driver handle
	HPX14 _board;

	// DMA buffer pointer to acquire data
	//std::array<unsigned short*, 8> dma_bufp;
	unsigned short *dma_bufp;

	// Data buffer size (2 channel, half size. merge two buffers to build a frame) 
	int getDataBufferSize() { return sizeof(uint16_t) * nSegments * nTimes / 8; }

	// Dump a PX14400 library error
	void dumpError(int res, const char* pPreamble);
	void dumpErrorSystem(int res, const char* pPreamble);
};

#endif // SIGNATEC_DAQ_H_