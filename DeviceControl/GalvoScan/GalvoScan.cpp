
#include "GalvoScan.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


GalvoScan::GalvoScan() :
	_taskHandle(nullptr),
    //horizontal_size(512), //1024
	//nIter(1),
	pp_voltage(2.1), //2.0  //2.1 
	offset(0.0),
	//pp_voltage_slow(2.0),  //2.0
	//offset_slow(0.0),
	step(N_LINES),
    max_rate(400000), //40000
	data(nullptr),
	physicalChannel(NI_GALVO_CHANNEL),
    sourceTerminal(NI_GAVLO_SOURCE)
{
}


GalvoScan::~GalvoScan()
{
	if (data)
	{
		delete[] data;
		data = nullptr;
	}
	if (_taskHandle) 
		DAQmxClearTask(_taskHandle);
}


bool GalvoScan::initialize()
{
    SendStatusMessage("Initializing NI Analog Output for galvano mirror...", false);

	int res;
	int sample_mode = DAQmx_Val_ContSamps;
	
	// Bi-directional slow scan
	//data = new double[2 * step];	
	//for (int i = 0; i < step; i++)
	//{
	//	data[i] = (double)i * (pp_voltage * 2 / (step - 1)) - pp_voltage + offset;
	//	data[2 * step - 1 - i] = data[i];
	//}
	//step = 2 * step;

	// Uni-directional slow scan
	data = new double[step];
	for (int i = 0; i < step - GALVO_FLYING_BACK; i++)
		data[i] = (double)i * (pp_voltage * 2 / (step - GALVO_FLYING_BACK)) - pp_voltage + offset;
	
	for (int i = 0; i < GALVO_FLYING_BACK; i++)
		data[step - GALVO_FLYING_BACK + i] = (double)(GALVO_FLYING_BACK - i) * (pp_voltage * 2 / GALVO_FLYING_BACK) - pp_voltage + offset;

	/*********************************************/
	// Scan Part
	/*********************************************/
	if ((res = DAQmxCreateTask("",&_taskHandle)) !=0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner1: ");
		return false;
	}
    if ((res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", -3.0, 3.0, DAQmx_Val_Volts, NULL)) != 0)  //-2 2
	{
		dumpError(res, "ERROR: Failed to set galvoscanner2: ");
		return false;
	}
	if ((res = DAQmxCfgSampClkTiming(_taskHandle, sourceTerminal, max_rate, DAQmx_Val_Rising, sample_mode, step)) != 0)  //0.8v - 3920
	{
		dumpError(res, "ERROR: Failed to set galvoscanner3: ");
		return false;
	}
	if ((res = DAQmxWriteAnalogF64(_taskHandle, step, FALSE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByScanNumber, data, NULL, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner4: ");
		return false;
	}
	
    SendStatusMessage("NI Analog Output for galvano mirror is successfully initialized.", false);

	return true;
	
}


void GalvoScan::start()
{
	if (_taskHandle)
	{
        SendStatusMessage("Galvano mirror is scanning a sample...", false);
		DAQmxStartTask(_taskHandle);
	}
}


void GalvoScan::stop()
{
	if (_taskHandle)
	{
        SendStatusMessage("NI Analog Output is stopped.", false);		

		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);
	
		if (data)			
		{
			delete[] data;
			data = nullptr;
		}
		_taskHandle = nullptr;

		//if (!_taskHandle)
		//{
		//	double data[1] = { -pp_voltage + offset };

		//	int res = DAQmxCreateTask("", &_taskHandle);
		//	res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", 0.0, 1.0, DAQmx_Val_Volts, NULL);
		//	res = DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL);

		//	res = DAQmxStartTask(_taskHandle);
		//	res = DAQmxStopTask(_taskHandle);
		//	res = DAQmxClearTask(_taskHandle);

		//	_taskHandle = nullptr;
		//}
	}
}


void GalvoScan::dumpError(int res, const char* pPreamble)
{	
	char errBuff[2048];
	if (res < 0)
		DAQmxGetErrorString(res, errBuff, 2048);

	//QMessageBox::critical(nullptr, "Error", (QString)pPreamble + (QString)errBuff);
    SendStatusMessage(((QString)pPreamble + (QString)errBuff).toUtf8().data(), true);

	GalvoScan::stop();

	//if (_taskHandle)
	//{
	//	double data[1] = { -pp_voltage + offset };
	//	DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL);

	//	DAQmxStopTask(_taskHandle);
	//	DAQmxClearTask(_taskHandle);

	//	_taskHandle = nullptr;
	//}
}

