#ifndef GAIN_MONITOR_H_
#define GAIN_MONITOR_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class PmtGainMonitor
{
public:
	PmtGainMonitor();
	~PmtGainMonitor();
			
	bool initialize();
	void start();
	void stop();

public:
	callback<double> acquiredData;
	callback2<const char*, bool> SendStatusMessage;

	void dumpError(int res, const char* pPreamble);
			
//public:
//	std::deque<double> deque_ecg;
//	std::deque<bool> deque_is_peak;
//	std::deque<double> deque_record_ecg;
//	std::deque<int> deque_period;
//	int prev_peak_pos;
//	double heart_interval;
//	bool isRecording;

private:
	const char* physicalChannel;
	const char* sourceTerminal;

	TaskHandle _taskHandle;
};

#endif // GAIN_MONITOR_H_