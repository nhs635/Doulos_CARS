#ifndef FLIM_PROCESS_H
#define FLIM_PROCESS_H

#include <Doulos/Configuration.h>

#ifndef FLIM_SPLINE_FACTOR
#error("FLIM_SPLINE_FACTOR is not defined for FLIM processing.");
#endif
#ifndef INTENSITY_THRES
#error("INTENSITY_THRES is not defined for FLIM processing.");
#endif

#include <iostream>
#include <vector>
#include <utility>
#include <cmath>

#include <QString>
#include <QFile>

#include <ipps.h>
#include <ippi.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <mkl_df.h>

#include <Common/array.h>
#include <Common/callback.h>
using namespace np;


struct FLIM_PARAMS
{
    float bg;

    float samp_intv = 1.0f;
    float width_factor = 2.0f;

    int ch_start_ind[5] = { 0, };
};

struct OPERATOR
{
public:
	OPERATOR() : scoeff(nullptr), nx(-1), initiated(false)
    {
    }

    ~OPERATOR()
    {
        if (scoeff) delete[] scoeff;
    }

    void operator() (const Uint16Array2 &src, const FLIM_PARAMS &pParams)
    {
        // 0. Initialize        
        int _nx = src.size(0); //pParams.ch_start_ind[4] - pParams.ch_start_ind[0];
        if ((nx != _nx) || !initiated)
            initialize(pParams, _nx, FLIM_SPLINE_FACTOR, src.size(1));

        // 1. Convert data
		int offset;
		ippsConvert_16u32f(src.raw_ptr(), crop_src.raw_ptr(), crop_src.length());

        /// 2. Determine whether saturated
        ///ippsThreshold_32f(crop_src.raw_ptr(), sat_src.raw_ptr(), sat_src.length(), 65531, ippCmpLess);
        ///ippsSubC_32f_I(65531, sat_src.raw_ptr(), sat_src.length());
        ///int roi_len = (int)round(pulse_roi_length / ActualFactor);
        ///for (int i = 1; i < 4; i++)
        ///{
        ///    for (int j = 0; j < ny; j++)
        ///    {
        ///        saturated(j, i) = 0;
        ///        offset = pParams.ch_start_ind[i] - pParams.ch_start_ind[0];
        ///        ippsSum_32f(&sat_src(offset, j), roi_len, &saturated(j, i), ippAlgHintFast);
        ///    }
        ///}

        // 3. BG subtraction
        ippsSubC_32f_I(pParams.bg, crop_src.raw_ptr(), crop_src.length());

		// 4. Window-wise integral to obtain intensity data
		memcpy(crop_src0.raw_ptr(), crop_src.raw_ptr(), sizeof(float) * crop_src0.length());
        tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)ny),
            [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i)
            {
                ///DFTaskPtr task1 = nullptr;

                ///dfsNewTask1D(&task1, nx, x, DF_UNIFORM_PARTITION, 1, &crop_src(0, (int)i), DF_MATRIX_STORAGE_ROWS);
                ///dfsEditPPSpline1D(task1, DF_PP_CUBIC, DF_PP_NATURAL, DF_BC_NOT_A_KNOT, 0, DF_NO_IC, 0, scoeff + (int)i * (nx - 1) * DF_PP_CUBIC, DF_NO_HINT);
                ///dfsConstruct1D(task1, DF_PP_SPLINE, DF_METHOD_STD);
                ///dfsInterpolate1D(task1, DF_INTERP, DF_METHOD_PP, nsite, x, DF_UNIFORM_PARTITION, 1, &dorder,
                ///                 DF_NO_APRIORI_INFO, &ext_src(0, (int)i), DF_MATRIX_STORAGE_ROWS, NULL);
                ///dfDeleteTask(&task1);

				for (int j = 0; j < 4; j++)
				{
					intensity((int)i, j) = 0;
					if (saturated((int)i, j) < 1)
					{
						offset = ch_start_ind1[j] - ch_start_ind1[0];
                        ippsSum_32f(&crop_src(ch_start_ind1[j], (int)i), ch_start_ind1[j + 1] - ch_start_ind1[j], &intensity((int)i, j), ippAlgHintFast);
					}
				}
            }
        });

		ippsDivC_32f_I(65532.0f, intensity.raw_ptr(), intensity.length());
    }

    void initialize(const FLIM_PARAMS& pParams, int _nx, int _upSampleFactor, int _alines)
    {
        /* Parameters */
        nx = _nx; ny = _alines;
        upSampleFactor = _upSampleFactor;
        nsite = nx * upSampleFactor;
        ActualFactor = (float)(nx * upSampleFactor - 1) / (float)(nx - 1);
        x[0] = 0.0f; x[1] = (float)nx - 1.0f;
        srcSize = { (int)nx, (int)ny };
        dorder = 1;

        /* Find pulse roi length for mean delay calculation */
        for (int i = 0; i < 5; i++)
            ch_start_ind1[i] = (int)round((float)pParams.ch_start_ind[i] * ActualFactor);

        int diff_ind[4];
        for (int i = 0; i < 4; i++)
            diff_ind[i] = ch_start_ind1[i + 1] - ch_start_ind1[i];

        ippsMin_32s(diff_ind, 4, &pulse_roi_length);
//		char msg[256];
//		sprintf(msg, "Initializing... %d", pulse_roi_length);
//		SendStatusMessage(msg);
		
        /* Array of spline coefficients */
        if (scoeff) { delete[] scoeff; scoeff = nullptr; }
        scoeff = new float[ny * (nx - 1) * DF_PP_CUBIC];

        /* data buffer allocation */
        crop_src  = std::move(FloatArray2((int)nx, (int)ny));
		crop_src0 = std::move(FloatArray2((int)nx, (int)ny));
        sat_src   = std::move(FloatArray2((int)nx, (int)ny));
        ext_src   = std::move(FloatArray2((int)nsite, (int)ny));

        saturated = std::move(FloatArray2((int)ny, 4));
        memset(saturated, 0, sizeof(float) * saturated.length());
		
		/* intensity */
		intensity = std::move(FloatArray2((int)ny, 4));

        initiated = true;
    }

private:
    IppiSize srcSize;
    float* scoeff;
    float x[2];
    MKL_INT dorder;

public:
    bool initiated;

    MKL_INT nx, ny; // original data length, dimension
    MKL_INT nsite; // interpolated data length

    int ch_start_ind1[5];
    int upSampleFactor;
    Ipp32f ActualFactor;
    int pulse_roi_length;
	
    FloatArray2 saturated;

    FloatArray2 crop_src;
	FloatArray2 crop_src0;
    FloatArray2 sat_src;
    FloatArray2 ext_src;

	FloatArray2 intensity;

	callback<const char*> SendStatusMessage;
};


class DataProcess
{
// Methods
public: // Constructor & Destructor
    explicit DataProcess();
    ~DataProcess();

private: // Not to call copy constrcutor and copy assignment operator
    DataProcess(const DataProcess&);
    DataProcess& operator=(const DataProcess&);
	
public:
    // Generate fluorescence intensity & lifetime
    void operator()(FloatArray2& intensity, Uint16Array2& pulse);

    // For FLIM parameters setting
    void setParameters(Configuration* pConfig);
	
// Variables
public:
    FLIM_PARAMS _params;

    OPERATOR _operator; // resize objects

public:
	// Callbacks
	callback<const char*> SendStatusMessage;
};

#endif
