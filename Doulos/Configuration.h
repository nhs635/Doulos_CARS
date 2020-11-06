#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"2.1.0"

#define POWER_2(x)					(1 << x)
#define NEAR_2_POWER(x)				(int)(1 << (int)ceil(log2(x)))

////////////////////// Microcope Setup ///////////////////////
#define PX14_ADC_RATE               400 // MHz   400

#define PX14_VOLT_RANGE             1.2 // Vpp    1.2
#define PX14_BOOTBUF_IDX            0

#define N_SCANS						100 // 100
#define N_PIXELS					500 //500

#define N_SEGMENTS					65536 //65536
#define N_TIMES						4 //4

#define N_LINES 					512 //	

#define NI_PMT_GAIN_CHANNEL		    "Dev1/ao2"
#define NI_PMT_GAIN_MONITOR			"Dev2/ai0"

//////////////////////// Scan Setup /////////////////////////	
#define NI_RESONANT_CHANNEL			"Dev1/ao0"   // Dev1/ao0:1
#define RESONANT_DEFAULT_VOLTAGE	1.5 // 1.5 V;  resonant scanner default voltage  1.5v-400um 1.1v-300um 0.4v-100um

#define NI_GALVO_CHANNEL			"Dev1/ao1"  
#define NI_GAVLO_SOURCE				"/Dev1/PFI0"  // /Dev1/PFI12
// 2.3v Galvo scanner default voltage;   2.3v-400um 1.8v-300um 0.6v-100um 

#define GALVO_FLYING_BACK   		12 // flyingback pixel
#define GALVO_SHIFT                 2

#define ZABER_PORT					"COM5"
#define ZABER_MICRO_STEPSIZE		0.047625 // micro-meter ///
#define ZABER_CONVERSION_FACTOR		1.6384

//////////////// Thread & Buffer Processing /////////////////
#define RAW_PULSE_WRITE
#define PROCESSING_BUFFER_SIZE		50 //50
#ifdef RAW_PULSE_WRITE
#define WRITING_BUFFER_SIZE			5000
#endif
#define WRITING_IMAGE_SIZE          1000

///////////////////// Data Processing ///////////////////////
#define FLIM_SPLINE_FACTOR			1
#define INTENSITY_THRES				0.05f

/////////////////////// Visualization ///////////////////////
#define SHG_COLORTABLE			    ColorTable::blueo
#define TPFE_COLORTABLE			    ColorTable::greeno
#define CARS_COLORTABLE			    ColorTable::redo
#define RCM_COLORTABLE			    ColorTable::gray

//#define MED_FILT

#define RENEWAL_COUNT				20 //20


template <typename T>
struct Range
{
	T min = 0;
	T max = 0;
};

#include <QString>
#include <QSettings>
#include <QDateTime>

#include <Common/callback.h>


class Configuration
{
public:
    explicit Configuration() : imageAccumulationFrames(1), imageAveragingFrames(1), resonantScanVoltage(0), crsCompensation(false) {}
	~Configuration() {}

public:
    void getConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");

        // Image size
        nScans = settings.value("nScans").toInt();
        nPixels = settings.value("nPixels").toInt();
		nTimes = settings.value("nTimes").toInt();
		nCompPixels = settings.value("nCompPixels").toInt();
		nSegments = settings.value("nSegments").toInt();
		nLines = settings.value("nLines").toInt();

		bufferSize = nSegments * nTimes;
		imageSize = nPixels * nLines;
		
		// Image averaging & accumulation
		imageAccumulationFrames = settings.value("imageAccumulationFrames").toInt();
		imageAveragingFrames = settings.value("imageAveragingFrames").toInt();

        // Image stitching
        imageStichingXStep = settings.value("imageStichingXStep").toInt();
        imageStichingYStep = settings.value("imageStichingYStep").toInt();
        imageStichingMisSyncPos = settings.value("imageStichingMisSyncPos").toInt();
		
        // Data processing
        for (int i = 0; i < 4; i++)
            channelImageMode[i] = settings.value(QString("channelImageMode_%1").arg(i)).toInt();
		flimBg = settings.value("flimBg").toFloat();
		flimWidthFactor = settings.value("flimWidthFactor").toFloat();
        for (int i = 0; i < 5; i++)
			flimChStartInd[i] = settings.value(QString("flimChStartInd_%1").arg(i)).toInt();

        // Image contrast & processing
        for (int i = 0; i < 4; i++)
        {
            imageContrastRange[i].max = settings.value(QString("imageContrastRangeMax_%1").arg(i)).toFloat();
            imageContrastRange[i].min = settings.value(QString("imageContrastRangeMin_%1").arg(i)).toFloat();
        }
        crsCompensation = settings.value("crsCompensation").toBool();

		// Device control
        pmtGainVoltage = settings.value("pmtGainVoltage").toFloat();
		px14PreTrigger = settings.value("px14PreTrigger").toInt();
		px14TriggerDelay = settings.value("px14TriggerDelay").toInt();
		px14DcOffset = settings.value("px14DcOffset").toInt();
        galvoScanVoltage = settings.value("galvoScanVoltage").toFloat();
        galvoScanVoltageOffset = settings.value("galvoScanVoltageOffset").toFloat();
        zaberPullbackSpeed = settings.value("zaberPullbackSpeed").toFloat();
        zaberPullbackLength = settings.value("zaberPullbackLength").toFloat();
        
		settings.endGroup();
	}

	void setConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");
		
        // Image size
		settings.setValue("nScans", nScans);
		settings.setValue("nPixels", nPixels);
		settings.setValue("nTimes", nTimes);
		settings.setValue("nCompPixels", nCompPixels);
		settings.setValue("nSegments", nSegments);
		settings.setValue("nLines", nLines);
		
		settings.setValue("bufferSize", bufferSize);
        settings.setValue("imageSize", imageSize);

		// Image averaging & accumulation
		settings.setValue("imageAccumulationFrames", imageAccumulationFrames);
		settings.setValue("imageAveragingFrames", imageAveragingFrames);

        // Image stitching
        settings.setValue("imageStichingXStep", imageStichingXStep);
        settings.setValue("imageStichingYStep", imageStichingYStep);
        settings.setValue("imageStichingMisSyncPos", imageStichingMisSyncPos);

        // Data processing
        for (int i = 0; i < 4; i++)
            settings.setValue(QString("channelImageMode_%1").arg(i), channelImageMode[i]);
		settings.setValue("flimBg", QString::number(flimBg, 'f', 2));
		settings.setValue("flimWidthFactor", QString::number(flimWidthFactor, 'f', 2)); 
        for (int i = 0; i < 5; i++)
			settings.setValue(QString("flimChStartInd_%1").arg(i), flimChStartInd[i]);

        // Image contrast & processing
        for (int i = 0; i < 4; i++)
        {
            settings.setValue(QString("imageContrastRangeMax_%1").arg(i), QString::number(imageContrastRange[i].max, 'f', 1));
            settings.setValue(QString("imageContrastRangeMin_%1").arg(i), QString::number(imageContrastRange[i].min, 'f', 1));
        }
        settings.setValue("crsCompensation", crsCompensation);

		// Device control
        settings.setValue("pmtGainVoltage", QString::number(pmtGainVoltage, 'f', 2));
		settings.setValue("px14PreTrigger", QString::number(px14PreTrigger));
		settings.setValue("px14TriggerDelay", QString::number(px14TriggerDelay));
		settings.setValue("px14DcOffset", QString::number(px14DcOffset));
        settings.setValue("galvoScanVoltage", QString::number(galvoScanVoltage, 'f', 1));
        settings.setValue("galvoScanVoltageOffset", QString::number(galvoScanVoltageOffset, 'f', 1));
		settings.setValue("resonantScanVoltage", QString::number(resonantScanVoltage, 'f', 1));        
        settings.setValue("zaberPullbackSpeed", QString::number(zaberPullbackSpeed, 'f', 2));
        settings.setValue("zaberPullbackLength", QString::number(zaberPullbackLength, 'f', 2));

		// Current Time
		QDate date = QDate::currentDate();
		QTime time = QTime::currentTime();
		settings.setValue("time", QString("%1-%2-%3 %4-%5-%6")
			.arg(date.year()).arg(date.month(), 2, 10, (QChar)'0').arg(date.day(), 2, 10, (QChar)'0')
			.arg(time.hour(), 2, 10, (QChar)'0').arg(time.minute(), 2, 10, (QChar)'0').arg(time.second(), 2, 10, (QChar)'0'));

		settings.endGroup();
	}
	
public:
    // Image size
    int nScans, nPixels, nTimes;
	int nCompPixels;
	int nSegments;
	int nLines;
	int bufferSize;
    int imageSize;

	// Image averaging & accumulation
	int imageAccumulationFrames;
	int imageAveragingFrames;

    // Image stitching
    int imageStichingXStep;
    int imageStichingYStep;
    int imageStichingMisSyncPos;

    // Data processing
    int channelImageMode[4];
	float flimBg;
	float flimWidthFactor;
    int flimChStartInd[5];

    // Image contrast & processing
    Range<float> imageContrastRange[4];
    bool crsCompensation;

	// Device control
    float pmtGainVoltage; 
	int px14PreTrigger;
	int px14TriggerDelay;
	int px14DcOffset;

	float resonantScanVoltage;
    float galvoScanVoltage;
    float galvoScanVoltageOffset;

    float zaberPullbackSpeed;
    float zaberPullbackLength;
	
	// Message callback
	callback<const char*> msgHandle;
};


#endif // CONFIGURATION_H
