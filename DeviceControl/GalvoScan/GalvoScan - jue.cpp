
#include "GalvoScan.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


GalvoScan::GalvoScan() :
	_taskHandle(nullptr),
    //horizontal_size(512), //1024
	nIter(1),
	pp_voltage(2.1), //2.0
	offset(0.0),
	//pp_voltage_slow(2.0),  //2.0
	//offset_slow(0.0),
	step(N_LINES),
    max_rate(40000), //40000
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
	/*
    int sample_mode = DAQmx_Val_ContSamps;

	/
    nIter = int(ceil(pp_voltage_slow / step_slow));
	if (step_slow == 0)
		nIter = 0;
	
    if (nIter == 0) nIter = 1;
    int N = horizontal_size * nIter;

	data = new double[2 * N];
	for (int i = 0; i < N; i++)
	{
        double x = (double)i / (double)horizontal_size;
        if (nIter != 1)
            data[2 * i + 0] = -pp_voltage_slow * (floor(x)) / double(nIter - 1) + pp_voltage_slow / 2 + offset_slow;
        else
            data[2 * i + 0] = offset_slow;
		data[2 * i + 1] = pp_voltage_fast * (x - floor(x)) - pp_voltage_fast / 2 + offset_fast;
    }
	*/

	//double max_galvano = 2.1;
	double Galvo_decrease_step = 12;
	//double data[512];
	data = new double[step];
	

	for (int i = 0; i < (step - Galvo_decrease_step); i++)
	{
		//data[i] = (double)i * (max_galvano * 2 / Galvo_step) - max_galvano;
		data[i] = (double)i * (pp_voltage * 2 / (step - Galvo_decrease_step)) - pp_voltage + offset;
	}

	//double decrease_step = 5;


	for (int i = (Galvo_step - Galvo_decrease_step); i<Galvo_step; i++)
	{
		data[i] = max_galvano + (Galvo_step - double(i) - Galvo_decrease_step)*(max_galvano*2 / Galvo_decrease_step);
	}



	/*   6월 6일 이전 사용 갈바노 스캔 코드
	double Galvo_step = 512;
	double max_galvano = 3;

	double data[512];


	for (int i = 0; i < Galvo_step; i++)
	{
		//data[i] = (double)i * (max_galvano * 2 / Galvo_step) - max_galvano;
		data[i] = (double)i * (max_galvano * 2 / Galvo_step) - max_galvano;
	}

	//double decrease_step = 5;

	
	for (int i = Galvo_step - decrease_step; i, Galvo_step; i++)
	{
		data[i] = ((double)i - Galvo_step + 3) *((max_galvano-0.4) * 2 / decrease_step);
	}
	
	
	data[501] = 2.5;
	data[502] = 2.0;
	data[503] = 1.5;
	data[504] = 1.0;
	data[505] = 0.5;
	data[506] = 0;
	data[507] = -0.5;
	data[508] = -1.0;
	data[509] = -1.5;
	data[510] = -2.0;
	data[511] = -2.5;
	
	//data[512] = -1.0;
	
*/
	
	/*********************************************/
	// Scan Part
	/*********************************************/

	if ((res = DAQmxCreateTask("",&_taskHandle)) !=0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner1: ");
		return false;
	}
	if ((res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", -4, 4, DAQmx_Val_Volts, NULL)) != 0)  //-2 2
	{
		dumpError(res, "ERROR: Failed to set galvoscanner2: ");
		return false;
	}
	if ((res = DAQmxCfgSampClkTiming(_taskHandle,"/Dev1/PFI0", 40000, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, Galvo_step)) != 0)  //0.8v - 3920
	{
		dumpError(res, "ERROR: Failed to set galvoscanner3: ");
		return false;
	}

	/*
		if ((res = DAQmxCfgSampClkTiming(_taskHandle, "", max_rate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, Galvo_step)) != 0)  //0.8v - 3920
	{
		dumpError(res, "ERROR: Failed to set galvoscanner3: ");
		return false;
	}*/

	if ((res = DAQmxCfgDigEdgeStartTrig(_taskHandle, "/Dev1/PFI0", DAQmx_Val_Rising)) != 0)
	{
		dumpError(res, "ERROR: Failed to set resonant input: ");
		return false;
	}
		
	if ((res = DAQmxSetStartTrigRetriggerable(_taskHandle, 1) != 0))  //1
	{
		dumpError(res, "ERROR: Failed to set retrigger: ");
		return false;
	}
	
	/*
	if ((res = DAQmxWriteAnalogF64(_taskHandle, Galvo_step,-2.0, 2.0, DAQmx_Val_GroupByChannel, data, NULL, NULL)) != 0)   // 0 2.0
	{
		dumpError(res, "ERROR: Failed to Writeanalog: ");
		return false;
	}
	*/

	if ((res = DAQmxWriteAnalogF64(_taskHandle, Galvo_step, FALSE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByScanNumber, data, NULL, NULL)) != 0)  //511 -> Galvo_step 
	{
		dumpError(res, "ERROR: Failed to Writeanalog: ");
		return false;
	}

		/*
	if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner1: ");
		return false;
	}
	if ((res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", -10.0, 10.0, DAQmx_Val_Volts, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner2: ");
		return false;
	}
	if ((res = DAQmxCfgSampClkTiming(_taskHandle, sourceTerminal, max_rate, DAQmx_Val_Rising, sample_mode, N)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner3: ");
		return false;
	}
    if ((res = DAQmxWriteAnalogF64(_taskHandle, N, FALSE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByScanNumber, data, NULL, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner4: ");
		return false;
    }
	
	*/


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
	}
}


void GalvoScan::dumpError(int res, const char* pPreamble)
{	
	char errBuff[2048];
	if (res < 0)
		DAQmxGetErrorString(res, errBuff, 2048);

	//QMessageBox::critical(nullptr, "Error", (QString)pPreamble + (QString)errBuff);
    SendStatusMessage(((QString)pPreamble + (QString)errBuff).toUtf8().data(), true);

	if (_taskHandle)
	{
		double data[1] = { 0.0 };
		DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL);

		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);

		_taskHandle = nullptr;
	}
}

