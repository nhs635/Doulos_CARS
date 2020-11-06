
#include "DataProcess.h"


DataProcess::DataProcess()
{
}

DataProcess::~DataProcess()
{
}


void DataProcess::operator() (FloatArray2& intensity, Uint16Array2& pulse)
{
    // 1. Crop and resize pulse data
    _operator(pulse, _params);

    // 2. Get intensity
    memcpy(intensity, _operator.intensity, sizeof(float) * _operator.intensity.length());
}



void DataProcess::setParameters(Configuration* pConfig)
{
    _params.bg = pConfig->flimBg;
//    _params.pre_trig = pConfig->preTrigSamps;

    _params.samp_intv = 1000.0f / (float)PX14_ADC_RATE; //1000
    _params.width_factor = 2.0f;

    for (int i = 0; i < 5; i++)
		_params.ch_start_ind[i] = pConfig->flimChStartInd[i];
//    _params.ch_start_ind[5] = _params.ch_start_ind[3] + FLIM_CH_START_5;
}
