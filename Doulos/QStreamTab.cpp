
#include "QStreamTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Dialog/PulseCalibDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>

#include <DataAcquisition/DataProcess/DataProcess.h>

#include <DeviceControl/GalvoScan/GalvoScan.h>
#include <DeviceControl/ZaberStage/ZaberStage.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <iostream>
#include <mutex>
#include <condition_variable>


QStreamTab::QStreamTab(QWidget *parent) :
    QDialog(parent), m_nAcquiredFrames(0), m_bIsStageTransition(false), m_nImageCount(0)
{
	// Set main window objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
	m_pConfig = m_pMainWnd->m_pConfiguration;

	// Create message window
	m_pListWidget_MsgWnd = new QListWidget(this);
	m_pListWidget_MsgWnd->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	m_pListWidget_MsgWnd->setStyleSheet("font: 7pt");
	m_pConfig->msgHandle += [&](const char* msg) {
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, false);
	};
	connect(this, SIGNAL(sendStatusMessage(QString, bool)), this, SLOT(processMessage(QString, bool)));

	// Create streaming objects
	m_pOperationTab = new QOperationTab(this);
	m_pDeviceControlTab = new QDeviceControlTab(this);
	m_pVisualizationTab = new QVisualizationTab(this);

	// Create accumulation widgets
	m_pLabel_Accumulation = new QLabel(this);
	m_pLabel_Accumulation->setText("Accumulation Frame ");

	m_pLineEdit_Accumulation = new QLineEdit(this);
	m_pLineEdit_Accumulation->setFixedWidth(25);
	m_pLineEdit_Accumulation->setText(QString::number(m_pConfig->imageAccumulationFrames));
	m_pLineEdit_Accumulation->setAlignment(Qt::AlignCenter);
	m_pLineEdit_Accumulation->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// Create averaging widgets
	m_pLabel_Averaging = new QLabel(this);
	m_pLabel_Averaging->setText("   Averaging Frame ");

	m_pLineEdit_Averaging = new QLineEdit(this);
	m_pLineEdit_Averaging->setFixedWidth(25);
	m_pLineEdit_Averaging->setText(QString::number(m_pConfig->imageAveragingFrames));
	m_pLineEdit_Averaging->setAlignment(Qt::AlignCenter);
	m_pLineEdit_Averaging->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	m_pLabel_AcquisitionStatusMsg = new QLabel(this);
#ifndef RAW_PULSE_WRITE
    QString str; str.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
        0, m_pConfig->imageAccumulationFrames, 0, m_pConfig->imageAveragingFrames, 0, 1);
#else
    QString str; str.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
        0, m_pConfig->imageAccumulationFrames, 0, m_pConfig->imageAveragingFrames, 0);
#endif
	m_pLabel_AcquisitionStatusMsg->setText(str);
	//m_pLabel_AcquisitionStatusMsg->setFixedWidth(500);
	m_pLabel_AcquisitionStatusMsg->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	// Sync compensation
	m_pLabel_SyncComp = new QLabel(this);
	str.sprintf("Sync Comp : %3d", m_pConfig->nCompPixels);
	m_pLabel_SyncComp->setText(str);
	m_pLabel_SyncComp->setFixedWidth(80);

	m_pSlider_SyncComp = new QSlider(this);
	m_pSlider_SyncComp->setOrientation(Qt::Horizontal);
	m_pSlider_SyncComp->setRange(-40, 40);
	m_pSlider_SyncComp->setValue(m_pConfig->nCompPixels);
	m_pSlider_SyncComp->setFixedWidth(150);

    // CRS nonlinearity compensation
    m_pCheckBox_CRSNonlinearityComp = new QCheckBox(this);
    m_pCheckBox_CRSNonlinearityComp->setText("CRS Nonlinearity Compensation");

    m_pCRSCompIdx = np::FloatArray2(m_pConfig->nPixels, 2);
    for (int i = 0; i < m_pConfig->nPixels; i++)
    {
        m_pCRSCompIdx(i, 0) = i;
        m_pCRSCompIdx(i, 1) = 1;
    }

    m_pCheckBox_CRSNonlinearityComp->setChecked(m_pConfig->crsCompensation);
    if (m_pConfig->crsCompensation) changeCRSNonlinearityComp(true);

#ifndef RAW_PULSE_WRITE
    // Image stitching mode
    m_pCheckBox_StitchingMode = new QCheckBox(this);
    m_pCheckBox_StitchingMode->setText("Enable Stitching Mode");
    m_pCheckBox_StitchingMode->setDisabled(true);

    m_pLabel_XStep = new QLabel("X Step", this);
    m_pLabel_XStep->setDisabled(true);
    m_pLineEdit_XStep = new QLineEdit(this);
    m_pLineEdit_XStep->setFixedWidth(25);
    m_pLineEdit_XStep->setText(QString::number(m_pConfig->imageStichingXStep));
    m_pLineEdit_XStep->setAlignment(Qt::AlignCenter);
    m_pLineEdit_XStep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pLineEdit_XStep->setDisabled(true);

    m_pLabel_YStep = new QLabel("  Y Step", this);
    m_pLabel_YStep->setDisabled(true);
    m_pLineEdit_YStep = new QLineEdit(this);
    m_pLineEdit_YStep->setFixedWidth(25);
    m_pLineEdit_YStep->setText(QString::number(m_pConfig->imageStichingYStep));
    m_pLineEdit_YStep->setAlignment(Qt::AlignCenter);
    m_pLineEdit_YStep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pLineEdit_YStep->setDisabled(true);

//    m_pLabel_MisSyncPos = new QLabel("Mis-Sync Position", this);
//    m_pLabel_MisSyncPos->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//    m_pLabel_MisSyncPos->setDisabled(true);
//    m_pLineEdit_MisSyncPos = new QLineEdit(this);
//    m_pLineEdit_MisSyncPos->setFixedWidth(25);
//    m_pLineEdit_MisSyncPos->setText(QString::number(m_pConfig->imageStichingMisSyncPos));
//    m_pLineEdit_MisSyncPos->setAlignment(Qt::AlignCenter);
//    m_pLineEdit_MisSyncPos->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    m_pLineEdit_MisSyncPos->setDisabled(true);
#endif

    // Set timer for monitoring status
    m_pTimer_Monitoring = new QTimer(this);
    m_pTimer_Monitoring->start(500); // renew per 500 msec


	QGridLayout *pGridLayout_Averaging = new QGridLayout;
	pGridLayout_Averaging->setSpacing(2);

    pGridLayout_Averaging->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
    pGridLayout_Averaging->addWidget(m_pLabel_Accumulation, 0, 1);
    pGridLayout_Averaging->addWidget(m_pLineEdit_Accumulation, 0, 2);
    pGridLayout_Averaging->addWidget(m_pLabel_Averaging, 0, 3);
    pGridLayout_Averaging->addWidget(m_pLineEdit_Averaging, 0, 4);

    pGridLayout_Averaging->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_Averaging->addWidget(m_pLabel_AcquisitionStatusMsg, 1, 1, 1, 4);

	QHBoxLayout *pHBoxLayout_SyncComp = new QHBoxLayout;
	pHBoxLayout_SyncComp->setSpacing(2);

	pHBoxLayout_SyncComp->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_SyncComp->addWidget(m_pLabel_SyncComp);
	pHBoxLayout_SyncComp->addWidget(m_pSlider_SyncComp);

	pGridLayout_Averaging->addItem(pHBoxLayout_SyncComp, 2, 0, 1, 5);

    QHBoxLayout *pHBoxLayout_CRS = new QHBoxLayout;
    pHBoxLayout_CRS->setSpacing(2);

    pHBoxLayout_CRS->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_CRS->addWidget(m_pCheckBox_CRSNonlinearityComp);

    pGridLayout_Averaging->addItem(pHBoxLayout_CRS, 3, 0, 1, 5);

#ifndef RAW_PULSE_WRITE
    QGridLayout *pGridLayout_ImageStitching = new QGridLayout;
    pGridLayout_ImageStitching->setSpacing(2);

    pGridLayout_ImageStitching->addWidget(m_pCheckBox_StitchingMode, 0, 0);
    pGridLayout_ImageStitching->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
    pGridLayout_ImageStitching->addWidget(m_pLabel_XStep, 0, 2);
    pGridLayout_ImageStitching->addWidget(m_pLineEdit_XStep, 0, 3);
    pGridLayout_ImageStitching->addWidget(m_pLabel_YStep, 0, 4);
    pGridLayout_ImageStitching->addWidget(m_pLineEdit_YStep, 0, 5);

//    pGridLayout_ImageStitching->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0, 1, 2);
//    pGridLayout_ImageStitching->addWidget(m_pLabel_MisSyncPos, 1, 2, 1, 3);
//    pGridLayout_ImageStitching->addWidget(m_pLineEdit_MisSyncPos, 1, 5);

    pGridLayout_Averaging->addItem(pGridLayout_ImageStitching, 4, 0, 1, 5);
#endif

	m_pVisualizationTab->getAveragingBox()->setLayout(pGridLayout_Averaging);


	// Create group boxes for streaming objects
	m_pGroupBox_OperationTab = new QGroupBox();
	m_pGroupBox_OperationTab->setLayout(m_pOperationTab->getLayout());
	m_pGroupBox_OperationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_OperationTab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pGroupBox_OperationTab->setFixedWidth(332);

	m_pGroupBox_DeviceControlTab = new QGroupBox();
	m_pGroupBox_DeviceControlTab->setLayout(m_pDeviceControlTab->getLayout());
	m_pGroupBox_DeviceControlTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_DeviceControlTab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pGroupBox_DeviceControlTab->setFixedWidth(332);

	m_pGroupBox_VisualizationTab = new QGroupBox();
	m_pGroupBox_VisualizationTab->setLayout(m_pVisualizationTab->getLayout());
	m_pGroupBox_VisualizationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_VisualizationTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


	// Create thread managers for data processing
	m_pThreadDataProcess = new ThreadManager("Image process");
	m_pThreadVisualization = new ThreadManager("Visualization process");

	// Create buffers for threading operation
    m_pOperationTab->m_pMemoryBuffer->m_syncImageBuffer.allocate_queue_buffer(m_pConfig->nPixels /* width */ * m_pConfig->nLines * 4 /* height */, PROCESSING_BUFFER_SIZE);
#ifdef RAW_PULSE_WRITE
	m_pOperationTab->m_pMemoryBuffer->m_syncBuffering.allocate_queue_buffer(m_pConfig->nSegments /* width */ * m_pConfig->nTimes /* height */, PROCESSING_BUFFER_SIZE);
#endif
	m_syncDataProcessing.allocate_queue_buffer(m_pConfig->nSegments /* width */ * m_pConfig->nTimes /* height */, PROCESSING_BUFFER_SIZE);
	m_syncDataVisualization.allocate_queue_buffer((m_pConfig->nSegments * m_pConfig->nTimes) / 2 /* raw pulse */ 
												   + (m_pConfig->nPixels * m_pConfig->nTimes /* width */ * 4 /* height */), PROCESSING_BUFFER_SIZE);

	// Set signal object
	setDataAcquisitionCallback();
	setDataProcessingCallback();
	setVisualizationCallback();

	emit m_pVisualizationTab->drawImage();

	// Create layout
	QGridLayout* pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(2);

	// Set layout
	pGridLayout->addWidget(m_pGroupBox_VisualizationTab, 0, 0, 4, 1);
	pGridLayout->addWidget(m_pGroupBox_OperationTab, 0, 1);
	pGridLayout->addWidget(m_pVisualizationTab->getVisualizationWidgetsBox(), 1, 1);
	pGridLayout->addWidget(m_pGroupBox_DeviceControlTab, 2, 1);
	pGridLayout->addWidget(m_pListWidget_MsgWnd, 3, 1);

	this->setLayout(pGridLayout);


	// Connect signal and slot
    connect(m_pTimer_Monitoring, SIGNAL(timeout()), this, SLOT(onTimerMonitoring()));
	connect(m_pLineEdit_Accumulation, SIGNAL(textChanged(const QString &)), this, SLOT(changeAccumulationFrame(const QString &)));
	connect(m_pLineEdit_Averaging, SIGNAL(textChanged(const QString &)), this, SLOT(changeAveragingFrame(const QString &)));
	connect(m_pSlider_SyncComp, SIGNAL(valueChanged(int)), this, SLOT(setSyncComp(int)));
    connect(m_pCheckBox_CRSNonlinearityComp, SIGNAL(toggled(bool)), this, SLOT(changeCRSNonlinearityComp(bool)));
#ifndef RAW_PULSE_WRITE
    connect(m_pCheckBox_StitchingMode, SIGNAL(toggled(bool)), this, SLOT(enableStitchingMode(bool)));
    connect(m_pLineEdit_XStep, SIGNAL(textChanged(const QString &)), this, SLOT(changeStitchingXStep(const QString &)));
    connect(m_pLineEdit_YStep, SIGNAL(textChanged(const QString &)), this, SLOT(changeStitchingYStep(const QString &)));
//    connect(m_pLineEdit_MisSyncPos, SIGNAL(textChanged(const QString &)), this, SLOT(changeStitchingMisSyncPos(const QString &)));
#endif
}

QStreamTab::~QStreamTab()
{    
    m_pTimer_Monitoring->stop();

	if (m_pThreadVisualization) delete m_pThreadVisualization;
	if (m_pThreadDataProcess) delete m_pThreadDataProcess;
}

void QStreamTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void QStreamTab::setWidgetsText()
{
}

void QStreamTab::setAveragingWidgets(bool enabled)
{
	m_pLabel_Accumulation->setEnabled(enabled);
	m_pLineEdit_Accumulation->setEnabled(enabled);
	m_pLabel_Averaging->setEnabled(enabled);
	m_pLineEdit_Averaging->setEnabled(enabled);
	m_pLabel_SyncComp->setEnabled(enabled);
	m_pSlider_SyncComp->setEnabled(enabled);
    m_pCheckBox_CRSNonlinearityComp->setEnabled(enabled);

#ifndef RAW_PULSE_WRITE
    if (getDeviceControlTab()->getZaberStageControl()->isChecked())
    {
        m_pCheckBox_StitchingMode->setEnabled(enabled);
        if (m_pCheckBox_StitchingMode->isChecked())
        {
            m_pLabel_XStep->setEnabled(enabled);
            m_pLineEdit_XStep->setEnabled(enabled);
            m_pLabel_YStep->setEnabled(enabled);
            m_pLineEdit_YStep->setEnabled(enabled);
        }
    }

    if (!enabled)
    {
        m_bIsStageTransition = false;
        m_nImageCount = 0;
    }
#endif
}

void QStreamTab::resetImagingMode()
{
}

void QStreamTab::stageMoving()
{
}


void QStreamTab::setDataAcquisitionCallback()
{
	DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
	MemoryBuffer* pMemBuff = m_pOperationTab->getMemBuff();
	pDataAcq->ConnectDaqAcquiredFlimData([&, pMemBuff](int frame_count, const np::Array<uint16_t, 2>& frame) {

		// Data transfer for FLIm processing
		const uint16_t* frame_ptr = frame.raw_ptr();

		// Get buffer from threading queue
		uint16_t* pulse_ptr = nullptr;
		{
			std::unique_lock<std::mutex> lock(m_syncDataProcessing.mtx);

			if (!m_syncDataProcessing.queue_buffer.empty())
			{
				pulse_ptr = m_syncDataProcessing.queue_buffer.front();
				m_syncDataProcessing.queue_buffer.pop();
			}
		}

		if (pulse_ptr != nullptr)
		{
			// Body
			ippsSubCRev_16u_ISfs(65532, (uint16_t*)frame_ptr, frame.length(), 0);			
			memcpy(pulse_ptr, frame_ptr, sizeof(uint16_t) * m_pConfig->bufferSize);

			// Push the buffer to sync Queue
			m_syncDataProcessing.Queue_sync.push(pulse_ptr);
		}

		(void)frame_count;
	});
	pDataAcq->ConnectDaqStopFlimData([&]() {
		m_syncDataProcessing.Queue_sync.push(nullptr);
	});

	pDataAcq->ConnectDaqSendStatusMessage([&](const char * msg, bool is_error) {
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
	});
}

void QStreamTab::setDataProcessingCallback()
{
	// FLIm Process Signal Objects /////////////////////////////////////////////////////////////////////////////////////////
	DataProcess *pDataProc = m_pOperationTab->getDataAcq()->getDataProc();
	m_pThreadDataProcess->DidAcquireData += [&, pDataProc](int frame_count) {

		// Get the buffer from the previous sync Queue
		uint16_t* pulse_data = m_syncDataProcessing.Queue_sync.pop();
		if (pulse_data != nullptr)
		{
			// Get buffers from threading queues
			float* data_ptr = nullptr;
			{
				std::unique_lock<std::mutex> lock(m_syncDataVisualization.mtx);

				if (!m_syncDataVisualization.queue_buffer.empty())
				{
					data_ptr = m_syncDataVisualization.queue_buffer.front();
					m_syncDataVisualization.queue_buffer.pop();
				}
			}

			if (data_ptr != nullptr)
			{
				// Body
				memcpy((uint16_t*)data_ptr, pulse_data, sizeof(uint16_t) * m_pConfig->bufferSize);

				np::Uint16Array2 pulse0(pulse_data, m_pConfig->nSegments, m_pConfig->nTimes);
				np::Uint16Array2 pulse1(m_pConfig->nScans, m_pConfig->nPixels * m_pConfig->nTimes);

				int m = m_pConfig->nCompPixels; ///(int)(N_PIXELS / m_pConfig->nCompPixels);
				tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)pulse1.size(1)),
					[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i = r.begin(); i != r.end(); ++i)
					{
						int y = (int)(i / m_pConfig->nPixels);
						int x = (int)(i % m_pConfig->nPixels);
						x = x * m_pConfig->nScans + ((m != 0) ? (x / m) : 0);

						uint16_t* temp_pulse = &pulse0(x, y);
						memcpy(&pulse1(0, (int)i), temp_pulse, sizeof(uint16_t) * pulse1.size(0));
					}
				});
				
				float* image_ptr = data_ptr + m_pConfig->bufferSize / 2;
				np::FloatArray2 intensity(image_ptr, m_pConfig->nPixels * m_pConfig->nTimes, 4);
				(*pDataProc)(intensity, pulse1);

				//// Pulse Data
				//QFile file("pulse.data");
				//if (file.open(QIODevice::WriteOnly))
				//{	
				//	file.write(reinterpret_cast<char*>(pulse1.raw_ptr()), sizeof(uint16_t) * pulse1.length());
				//	file.close();
				//}

				// Transfer to FLIm calibration dlg
				if (m_pDeviceControlTab->getPulseCalibDlg())
				{
					int x, y;
					m_pVisualizationTab->getPixelPos(&x, &y);
                    m_pDeviceControlTab->getPulseCalibDlg()->getPulseImageView()->setHorizontalLine(1, x);

					if ((frame_count % (m_pConfig->nLines / m_pConfig->nTimes)) == (y / m_pConfig->nTimes))
						emit m_pDeviceControlTab->getPulseCalibDlg()->plotRoiPulse(pDataProc, (y % m_pConfig->nTimes) * m_pConfig->nPixels + x);
				}

				// Push the buffers to sync Queues
				m_syncDataVisualization.Queue_sync.push(data_ptr);

				// Return (push) the buffer to the previous threading queue
				{
					std::unique_lock<std::mutex> lock(m_syncDataProcessing.mtx);
					m_syncDataProcessing.queue_buffer.push(pulse_data);
				}
			}
		}
		else
			m_pThreadDataProcess->_running = false;

		(void)frame_count;
	};

	m_pThreadDataProcess->DidStopData += [&]() {
		m_syncDataVisualization.Queue_sync.push(nullptr);
	};

	m_pThreadDataProcess->SendStatusMessage += [&](const char* msg, bool is_error) {
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
	};
}

void QStreamTab::setVisualizationCallback()
{
    // Visualization Signal Objects ///////////////////////////////////////////////////////////////////////////////////////////
    m_pThreadVisualization->DidAcquireData += [&](int frame_count) {

		MemoryBuffer *pMemBuff = m_pOperationTab->m_pMemoryBuffer;
		
		static int updateFrames = 0;
		static int writtenSamples = 0;
		static ULONG dwTickStart = GetTickCount();
		static ULONG dwTickLastUpdate;
		int accumulationCount, averageCount;
		
		if (frame_count == 0)
		{
			m_nAcquiredFrames = 0;
			updateFrames = 0;
			writtenSamples = 0;
			dwTickStart = GetTickCount();
			dwTickLastUpdate = GetTickCount();
		}
		
		// Get the buffers from the previous sync Queues
		float* data_ptr = m_syncDataVisualization.Queue_sync.pop();
		if (data_ptr != nullptr)
		{
			// Body
			if (m_pOperationTab->isAcquisitionButtonToggled()) // Only valid if acquisition is running 
			{
				// Averaging buffer
				if ((m_nAcquiredFrames == 0) && (writtenSamples == 0))
				{
					m_pTempImage = np::FloatArray2(m_pConfig->nPixels, 4 * m_pConfig->nLines);
					memset(m_pTempImage, 0, sizeof(float) * m_pTempImage.length());
				}

				// Data copy (SHG / TPFE / CARS / RCM)
				float* image_ptr = data_ptr + m_pConfig->bufferSize / 2;
				np::FloatArray2 data(image_ptr, m_pConfig->nPixels * m_pConfig->nTimes, 4);
				for (int i = 0; i < 4; i++)
					ippsAdd_32f_I(&data(0, i), &m_pTempImage(0, i * m_pConfig->nLines) + writtenSamples, m_pConfig->nPixels * m_pConfig->nTimes);
				writtenSamples += m_pConfig->nPixels * m_pConfig->nTimes;
				
#ifdef RAW_PULSE_WRITE
				// Buffering (When recording)
                if (pMemBuff->m_bIsRecordingPulse)
				{
					if ((!(frame_count % (m_pConfig->nLines / m_pConfig->nTimes))) && (m_nAcquiredFrames == 0))
						pMemBuff->m_bIsImageStartPoint = true;

					if (pMemBuff->m_bIsImageStartPoint)
					{
                        int single_frames = m_pConfig->nLines / m_pConfig->nTimes;
                        int total_frames = single_frames * m_pConfig->imageAveragingFrames * m_pConfig->imageAccumulationFrames;

                        if (pMemBuff->m_nRecordedFrames < total_frames)
						{
							// Get buffer from writing queue
							uint16_t* frame_ptr = nullptr;
							{
								std::unique_lock<std::mutex> lock(pMemBuff->m_syncBuffering.mtx);

								if (!pMemBuff->m_syncBuffering.queue_buffer.empty())
								{
									frame_ptr = pMemBuff->m_syncBuffering.queue_buffer.front();
									pMemBuff->m_syncBuffering.queue_buffer.pop();
								}
							}

							if (frame_ptr != nullptr)
							{
								// Body (Copying the frame data)
								memcpy(frame_ptr, (uint16_t*)data_ptr, sizeof(uint16_t) * m_pConfig->bufferSize);

								// Push to the copy queue for copying transfered data in copy thread
								pMemBuff->m_syncBuffering.Queue_sync.push(frame_ptr);
							}
						}
						else
						{                            
                            // Finish recording when the buffer is full
                            pMemBuff->m_bIsImageStartPoint = false;
                            pMemBuff->m_bIsRecordingPulse = false;
                            m_pOperationTab->setRecordingButton(false);
                        }
					}
				}
#endif

				// Image formation
				if (writtenSamples == m_pConfig->imageSize)
				{
					// Update Status
					accumulationCount = (m_nAcquiredFrames % m_pConfig->imageAccumulationFrames) + 1;
					averageCount = (m_nAcquiredFrames / m_pConfig->imageAccumulationFrames) + 1;
					m_nAcquiredFrames++;

#ifdef RAW_PULSE_WRITE
					QString str; str.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
						accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames, pMemBuff->m_nRecordedFrames);
#else
                    QString str; str.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
                        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                                             pMemBuff->m_nRecordedImages,
                                             m_pCheckBox_StitchingMode->isChecked() ? m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep : 1);
#endif
					m_pLabel_AcquisitionStatusMsg->setText(str);

					// Visualization
					if (m_nAcquiredFrames == (m_pConfig->imageAveragingFrames * m_pConfig->imageAccumulationFrames))
                    {
						for (int i = 0; i < 4; i++)
                        {
                            // Averaging
                            ippsDivC_32f(&m_pTempImage(0, i * m_pConfig->nLines), m_pConfig->imageAveragingFrames,
                                         m_pVisualizationTab->m_vecVisImage.at(i).raw_ptr(), m_pConfig->imageSize);

                            // CRS nonlinear scanning compensation                            
                            np::FloatArray2 scanArray0(m_pVisualizationTab->m_vecVisImage.at(i).raw_ptr(), m_pConfig->nPixels, m_pConfig->nLines);
                            np::FloatArray2 scanArray1(m_pConfig->nPixels, m_pConfig->nLines);
                            memcpy(scanArray1, scanArray0, sizeof(float) * scanArray1.length());

                            if (m_pCheckBox_CRSNonlinearityComp->isChecked())
                            {
                                np::FloatArray comp_index(&m_pCRSCompIdx(0, 0), m_pConfig->nPixels);
                                np::FloatArray comp_weight(&m_pCRSCompIdx(0, 1), m_pConfig->nPixels);

                                tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)m_pConfig->nLines),
                                    [&](const tbb::blocked_range<size_t>& r) {
                                    for (size_t j = r.begin(); j != r.end(); ++j)
                                    {
                                        float* fast_scan = &scanArray0(0, (int)j);
                                        float* comp_scan = &scanArray1(0, (int)j);

                                        for (int k = 0; k < m_pConfig->nPixels; k++)
                                            comp_scan[k] = comp_weight[k] * fast_scan[(int)comp_index[k]]
                                                    + (1 - comp_weight[k]) * fast_scan[(int)comp_index[k] + 1];
                                    }
                                });

                                memcpy(scanArray0.raw_ptr(), scanArray1.raw_ptr(), sizeof(float) * scanArray1.length());
                            }

                            ippiCopy_32f_C1R(&scanArray1(0, GALVO_SHIFT), sizeof(float) * m_pConfig->nPixels,
                                             &scanArray0(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels, m_pConfig->nLines - GALVO_SHIFT });
                            ippiCopy_32f_C1R(&scanArray1(0, 0), sizeof(float) * m_pConfig->nPixels,
                                             &scanArray0(0, m_pConfig->nLines - GALVO_SHIFT), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels, GALVO_SHIFT });
                        }

						// Draw Images
						updateFrames++;
						emit m_pVisualizationTab->drawImage();

                        // Buffering (When recording)
                        if (pMemBuff->m_bIsRecordingImage && !m_bIsStageTransition)
                        {
                            if (pMemBuff->m_bIsFirstRecImage)
                            {
#ifndef RAW_PULSE_WRITE
                                int total_images = m_pCheckBox_StitchingMode->isChecked() ? m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep : 1;
#else
                                int total_images = 1;
#endif
                                {
                                    // Get buffer from writing queue
                                    float* image_ptr = nullptr;
                                    {
                                        std::unique_lock<std::mutex> lock(pMemBuff->m_syncImageBuffer.mtx);

                                        if (!pMemBuff->m_syncImageBuffer.queue_buffer.empty())
                                        {
                                            image_ptr = pMemBuff->m_syncImageBuffer.queue_buffer.front();
                                            pMemBuff->m_syncImageBuffer.queue_buffer.pop();
                                        }
                                    }

                                    if (image_ptr != nullptr)
                                    {
                                        // Body (Copying the frame data)
                                        for (int i = 0; i < 4; i++)
                                            memcpy(image_ptr + i * m_pConfig->imageSize,
                                                   m_pVisualizationTab->m_vecVisImage.at(i).raw_ptr(), sizeof(float) * m_pConfig->imageSize);

                                        // Push to the copy queue for copying transfered data in copy thread
                                        pMemBuff->m_syncImageBuffer.Queue_sync.push(image_ptr);
                                    }
                                }

                                pMemBuff->m_bIsFirstRecImage = false;

#ifdef RAW_PULSE_WRITE
                                // Finish recording when the buffer is full
                                pMemBuff->m_bIsRecordingImage = false;
#else
                                // Stitiching - stage scanning
                                if (++m_nImageCount < total_images)
                                {
                                    m_bIsStageTransition = true;
                                    if (m_nImageCount % m_pConfig->imageStichingXStep != 0) // x move
                                    {
                                        double step = (double)((m_nImageCount / m_pConfig->imageStichingXStep + 1) % 2 ? +1 : -1) * (double)m_pConfig->zaberPullbackLength;
                                        emit getDeviceControlTab()->startStageScan(2, step);
                                        printf("x-axis transition: %.2f : %d (%d, %d)\n", step, m_nImageCount,
                                               m_nImageCount % m_pConfig->imageStichingXStep, m_nImageCount / m_pConfig->imageStichingXStep);
                                    }
                                    else // y move
                                    {
                                        emit getDeviceControlTab()->startStageScan(1, m_pConfig->zaberPullbackLength);
                                        printf("y-axis transition: %.2f : %d (%d, %d)\n", m_pConfig->zaberPullbackLength, m_nImageCount,
                                               m_nImageCount % m_pConfig->imageStichingXStep, m_nImageCount / m_pConfig->imageStichingXStep);
                                    }
                                }
                                else
                                {
                                    m_nImageCount = 0;
                                    pMemBuff->m_bIsRecordingImage = false;
                                    m_pOperationTab->setRecordingButton(false);

                                    if (m_pCheckBox_StitchingMode->isChecked())
                                    {
                                        emit getDeviceControlTab()->startStageScan(2, 0);
                                        emit getDeviceControlTab()->startStageScan(1, 0);
                                    }
                                }
#endif
                            }
                        }

                        // Temporal infomation
						ULONG dwTickNow = GetTickCount();
						if (dwTickNow - dwTickLastUpdate > 5000)
						{
//							ULONG dwElapsedUpdate = dwTickNow - dwTickLastUpdate;
//							dwTickLastUpdate = dwTickNow;

//							if (dwElapsedUpdate)
//							{
//								double fps = (double)updateFrames / (double)dwElapsedUpdate * 1000.0;
//								printf("[Frames Updated: %d / Elapsed Time: %d msec] => %.3f fps (%.3f fps; if no acc avg)\n", updateFrames, dwElapsedUpdate,
//									fps, fps * (m_pConfig->imageAveragingFrames * m_pConfig->imageAccumulationFrames));
//							}

							updateFrames = 0;
						}						

                        // Re-initializing
						m_nAcquiredFrames = 0;
                        if (pMemBuff->m_bIsRecordingImage && !m_bIsStageTransition) pMemBuff->m_bIsFirstRecImage = true;
					}

					// Re-initializing
					writtenSamples = 0;
				}
			}

			// Return (push) the buffer to the previous threading queue
			{
				std::unique_lock<std::mutex> lock(m_syncDataVisualization.mtx);
				m_syncDataVisualization.queue_buffer.push(data_ptr);
			}
		}
		else
			m_pThreadVisualization->_running = false;
	};

	m_pThreadVisualization->DidStopData += [&]() {
		// None
	};

	m_pThreadVisualization->SendStatusMessage += [&](const char* msg, bool is_error) {
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
	};
}


void QStreamTab::onTimerMonitoring()
{
    if (getOperationTab()->isAcquisitionButtonToggled())
    {
        m_pMainWnd->m_pStatusLabel_Acquisition->setText("Acquistion O");
        m_pMainWnd->m_pStatusLabel_Acquisition->setStyleSheet("color: green;");
    }
    else
    {
        m_pMainWnd->m_pStatusLabel_Acquisition->setText("Acquistion X");
        m_pMainWnd->m_pStatusLabel_Acquisition->setStyleSheet("color: red;");
    }

    if (getOperationTab()->m_pMemoryBuffer->m_bIsRecordingImage && !m_bIsStageTransition && getOperationTab()->m_pMemoryBuffer->m_bIsFirstRecImage)
    {
        m_pMainWnd->m_pStatusLabel_Recording->setText("Recording O");
        m_pMainWnd->m_pStatusLabel_Recording->setStyleSheet("color: green;");
    }
    else
    {
        m_pMainWnd->m_pStatusLabel_Recording->setText("Recording X");
        m_pMainWnd->m_pStatusLabel_Recording->setStyleSheet("color: red;");
    }

    if (getDeviceControlTab()->getZaberStage() && getDeviceControlTab()->getZaberStage()->getIsMoving())
    {
        m_pMainWnd->m_pStatusLabel_StageMoving->setText("Stage Moving O");
        m_pMainWnd->m_pStatusLabel_StageMoving->setStyleSheet("color: green;");
    }
    else
    {
        m_pMainWnd->m_pStatusLabel_StageMoving->setText("Stage Moving X");
        m_pMainWnd->m_pStatusLabel_StageMoving->setStyleSheet("color: red;");
    }
}

void QStreamTab::changeAccumulationFrame(const QString &str)
{
	int acc_frame = str.toInt();
	if (acc_frame < 1)
	{
		m_pLineEdit_Accumulation->setText(QString::number(1));
		return;
	}

	m_pConfig->imageAccumulationFrames = acc_frame;

	m_nAcquiredFrames = 0;

#ifdef RAW_PULSE_WRITE
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
        0, m_pConfig->imageAccumulationFrames, 0, m_pConfig->imageAveragingFrames, 0);
#else
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
        0, m_pConfig->imageAccumulationFrames, 0, m_pConfig->imageAveragingFrames, 0, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
#endif
    m_pLabel_AcquisitionStatusMsg->setText(str1);
}

void QStreamTab::changeAveragingFrame(const QString &str)
{
	int avg_frame = str.toInt();
	if (avg_frame < 1)
	{
		m_pLineEdit_Averaging->setText(QString::number(1));
		return;
	}

	m_pConfig->imageAveragingFrames = avg_frame;

	m_nAcquiredFrames = 0;

#ifdef RAW_PULSE_WRITE
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
        0, m_pConfig->imageAccumulationFrames, 0, m_pConfig->imageAveragingFrames, 0);
#else
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
        0, m_pConfig->imageAccumulationFrames, 0, m_pConfig->imageAveragingFrames, 0, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
#endif
    m_pLabel_AcquisitionStatusMsg->setText(str1);
}

void QStreamTab::processMessage(QString qmsg, bool is_error)
{
	m_pListWidget_MsgWnd->addItem(qmsg);
	m_pListWidget_MsgWnd->setCurrentRow(m_pListWidget_MsgWnd->count() - 1);

	if (is_error)
	{
		QMessageBox MsgBox(QMessageBox::Critical, "Error", qmsg);
		MsgBox.exec();
	}
}

void QStreamTab::setSyncComp(int comp)
{
	m_pConfig->nCompPixels = comp;

	QString str; str.sprintf("Sync Comp : %3d", m_pConfig->nCompPixels);
	m_pLabel_SyncComp->setText(str);
}

void QStreamTab::changeCRSNonlinearityComp(bool toggled)
{
    m_pConfig->crsCompensation = toggled;
    if (toggled)
    {
        QFile file("crs_comp_idx.txt");
        if (false != file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);

            int i = 0;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                QStringList comp_idx = line.split('\t');

                m_pCRSCompIdx(i, 0) = comp_idx[0].toFloat();
                m_pCRSCompIdx(i, 1) = comp_idx[1].toFloat();
                i++;
            }

            file.close();
        }
        else
        {
            m_pCheckBox_CRSNonlinearityComp->setChecked(false);
            processMessage("No CRS nonlinearity compensation index file! (crs_comp_idx.txt)", true);
        }
    }
    else
    {
        for (int i = 0; i < m_pConfig->nPixels; i++)
        {
            m_pCRSCompIdx(i, 0) = i;
            m_pCRSCompIdx(i, 1) = 1;
        }
    }
}

#ifndef RAW_PULSE_WRITE
void QStreamTab::enableStitchingMode(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_StitchingMode->setText("Disable Stitching Mode");

        // Set enabled true for image stitching widgets
//        if (!m_pOperationTab->isAcquisitionButtonToggled())
//        {
            m_pLabel_XStep->setEnabled(true);
            m_pLineEdit_XStep->setEnabled(true);
            m_pLabel_YStep->setEnabled(true);
            m_pLineEdit_YStep->setEnabled(true);
//        }

//        m_pLabel_MisSyncPos->setEnabled(true);
//        m_pLineEdit_MisSyncPos->setEnabled(true);

        //m_pVisualizationTab->getImageView()->setHorizontalLine(1, m_pConfig->imageStichingMisSyncPos);
        //m_pVisualizationTab->visualizeImage();
    }
    else
    {
        // Set enabled false for image stitching widgets
        m_pLabel_XStep->setEnabled(false);
        m_pLineEdit_XStep->setEnabled(false);
        m_pLabel_YStep->setEnabled(false);
        m_pLineEdit_YStep->setEnabled(false);
//        m_pLabel_MisSyncPos->setEnabled(false);
//        m_pLineEdit_MisSyncPos->setEnabled(false);

        //m_pVisualizationTab->getImageView()->setHorizontalLine(0);
        //m_pVisualizationTab->visualizeImage();

        // Set text
        m_pCheckBox_StitchingMode->setText("Enable Stitching Mode");
    }

    int accumulationCount = (m_nAcquiredFrames % m_pConfig->imageAccumulationFrames) + 1;
    int averageCount = (m_nAcquiredFrames / m_pConfig->imageAccumulationFrames) + 1;

#ifdef RAW_PULSE_WRITE
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                               getOperationTab()->m_pMemoryBuffer->m_nRecordedFrames);
#else
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                               getOperationTab()->m_pMemoryBuffer->m_nRecordedImages,
                               toggled ? m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep : 1);
#endif
    m_pLabel_AcquisitionStatusMsg->setText(str1);
}

void QStreamTab::changeStitchingXStep(const QString &str)
{
    m_pConfig->imageStichingXStep = str.toInt();
    if (m_pConfig->imageStichingXStep < 1)
        m_pLineEdit_XStep->setText(QString::number(1));

    int accumulationCount = (m_nAcquiredFrames % m_pConfig->imageAccumulationFrames) + 1;
    int averageCount = (m_nAcquiredFrames / m_pConfig->imageAccumulationFrames) + 1;

#ifdef RAW_PULSE_WRITE
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                               getOperationTab()->m_pMemoryBuffer->m_nRecordedFrames);
#else
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                               getOperationTab()->m_pMemoryBuffer->m_nRecordedImages, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
#endif
    m_pLabel_AcquisitionStatusMsg->setText(str1);
}

void QStreamTab::changeStitchingYStep(const QString &str)
{
    m_pConfig->imageStichingYStep = str.toInt();
    if (m_pConfig->imageStichingYStep < 1)
        m_pLineEdit_YStep->setText(QString::number(1));

    int accumulationCount = (m_nAcquiredFrames % m_pConfig->imageAccumulationFrames) + 1;
    int averageCount = (m_nAcquiredFrames / m_pConfig->imageAccumulationFrames) + 1;

#ifdef RAW_PULSE_WRITE
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d",
        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                               getOperationTab()->m_pMemoryBuffer->m_nRecordedFrames);
#else
    QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
        accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
                               getOperationTab()->m_pMemoryBuffer->m_nRecordedImages, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
#endif
    m_pLabel_AcquisitionStatusMsg->setText(str1);
}

//void QStreamTab::changeStitchingMisSyncPos(const QString &str)
//{
//    m_pConfig->imageStichingMisSyncPos = str.toInt();
//    if (m_pConfig->imageStichingMisSyncPos >= m_pConfig->imageSize)
//    {
//        m_pConfig->imageStichingMisSyncPos = m_pConfig->imageSize - 1;
//        m_pLineEdit_MisSyncPos->setText(QString("%1").arg(m_pConfig->imageStichingMisSyncPos));
//    }
//    if (m_pConfig->imageStichingMisSyncPos < 0)
//    {
//        m_pConfig->imageStichingMisSyncPos = 0;
//        m_pLineEdit_MisSyncPos->setText(QString("%1").arg(m_pConfig->imageStichingMisSyncPos));
//    }

//    m_pVisualizationTab->getImageView()->setHorizontalLine(1, m_pConfig->imageStichingMisSyncPos);
 //   m_pVisualizationTab->visualizeImage();
//}
#endif
