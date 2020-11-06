
#include "PulseCalibDlg.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QOperationTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/DataProcess/DataProcess.h>

#include <iostream>
#include <thread>


PulseCalibDlg::PulseCalibDlg(QWidget *parent) : QDialog(parent)
{
	// Set default size & frame
    setFixedSize(600, 550);
	setWindowFlags(Qt::Tool);
	setWindowTitle("Pulse Calibration");

	// Set main window objects
	m_pDeviceControlTab = dynamic_cast<QDeviceControlTab*>(parent);
	m_pConfig = m_pDeviceControlTab->getStreamTab()->getMainWnd()->m_pConfiguration;
	m_pDataProc = m_pDeviceControlTab->getStreamTab()->getOperationTab()->getDataAcq()->getDataProc();


	// Create layout
	m_pVBoxLayout = new QVBoxLayout;
	m_pVBoxLayout->setSpacing(3);

	// Create widgets for pulse view and FLIM calibration
	createPulseView();
	createCalibWidgets();

	// Set layout
	this->setLayout(m_pVBoxLayout);

	drawRoiPulse(m_pDataProc, 0);
}

PulseCalibDlg::~PulseCalibDlg()
{
}

void PulseCalibDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void PulseCalibDlg::createPulseView()
{
	// Create widgets for FLIM pulse view layout
	QGridLayout *pGridLayout_PulseView = new QGridLayout;
	pGridLayout_PulseView->setSpacing(2);

	// Create widgets for FLIM pulse view
    m_pImageView_PulseImage = new QImageView(ColorTable::colortable::parula, m_pConfig->nScans, m_pConfig->nPixels);
    m_pImageView_PulseImage->setFixedWidth(531);
    m_pImageView_PulseImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pImageView_PulseImage->setHorizontalLine(1, 0);

	m_pScope_PulseView = new QScope({ 0, (double)m_pDataProc->_operator.crop_src.size(0) }, { 0, POWER_2(16) },
		2, 2, m_pDataProc->_params.samp_intv, PX14_VOLT_RANGE / (double)POWER_2(16), 0, 0, " nsec", " V", true);
	m_pScope_PulseView->setMinimumHeight(180);
	m_pScope_PulseView->getRender()->setGrid(8, 32, 1, true);
	m_pScope_PulseView->setDcLine(m_pDataProc->_params.bg);	

    QLabel *pLabel_null = new QLabel(this);
    pLabel_null->setFixedWidth(37);

	m_pCheckBox_ShowWindow = new QCheckBox(this);
	m_pCheckBox_ShowWindow->setText("Show Window");
	m_pCheckBox_SplineView = new QCheckBox(this);
	m_pCheckBox_SplineView->setText("Spline View");
	m_pCheckBox_SplineView->setDisabled(true);
	

	// Set layout
    QHBoxLayout *pHBoxLayout_ImageView = new QHBoxLayout;
    pHBoxLayout_ImageView->setSpacing(2);
    pHBoxLayout_ImageView->addWidget(pLabel_null);
    pHBoxLayout_ImageView->addWidget(m_pImageView_PulseImage);
    pHBoxLayout_ImageView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

    pGridLayout_PulseView->addItem(pHBoxLayout_ImageView, 0, 0, 1, 9);
    pGridLayout_PulseView->addWidget(m_pScope_PulseView, 1, 0, 1, 9);
		
	m_pVBoxLayout->addItem(pGridLayout_PulseView);

	// Connect
	connect(this, SIGNAL(plotRoiPulse(DataProcess*, int)), this, SLOT(drawRoiPulse(DataProcess*, int)));

	connect(m_pCheckBox_ShowWindow, SIGNAL(toggled(bool)), this, SLOT(showWindow(bool)));
	connect(m_pCheckBox_SplineView, SIGNAL(toggled(bool)), this, SLOT(splineView(bool)));
}

void PulseCalibDlg::createCalibWidgets()
{
	// Create widgets for FLIM calibration layout
	QGridLayout *pGridLayout_PulseView = new QGridLayout;
	pGridLayout_PulseView->setSpacing(2);

	// Create widgets for FLIM calibration
	m_pPushButton_CaptureBackground = new QPushButton(this);
	m_pPushButton_CaptureBackground->setText("Capture Background");
	m_pLineEdit_Background = new QLineEdit(this);
	m_pLineEdit_Background->setText(QString::number(m_pDataProc->_params.bg, 'f', 2));
	m_pLineEdit_Background->setFixedWidth(60);
	m_pLineEdit_Background->setAlignment(Qt::AlignCenter);

    m_pLabel_ChStart = new QLabel("Channel Start  ", this);

    m_pLabel_Ch[0] = new QLabel("Ch 0", this);
	m_pLabel_Ch[0]->setFixedWidth(70);  //70
	m_pLabel_Ch[0]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[1] = new QLabel("Ch 1", this);
	m_pLabel_Ch[1]->setFixedWidth(70); //70
	m_pLabel_Ch[1]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[2] = new QLabel("Ch 2", this);
	m_pLabel_Ch[2]->setFixedWidth(70); //70
	m_pLabel_Ch[2]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[3] = new QLabel("Ch 3", this);
	m_pLabel_Ch[3]->setFixedWidth(70); //70
	m_pLabel_Ch[3]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[4] = new QLabel("Ch 4", this);
    m_pLabel_Ch[4]->setFixedWidth(70); //70
    m_pLabel_Ch[4]->setAlignment(Qt::AlignCenter);

    for (int i = 0; i < 5; i++)
	{
		m_pSpinBox_ChStart[i] = new QMySpinBox(this);
		m_pSpinBox_ChStart[i]->setFixedWidth(70);
        m_pSpinBox_ChStart[i]->setRange(0.0, m_pConfig->nScans * m_pDataProc->_params.samp_intv);
		m_pSpinBox_ChStart[i]->setSingleStep(m_pDataProc->_params.samp_intv);
		m_pSpinBox_ChStart[i]->setValue((float)m_pDataProc->_params.ch_start_ind[i] * m_pDataProc->_params.samp_intv);
		m_pSpinBox_ChStart[i]->setDecimals(2);
		m_pSpinBox_ChStart[i]->setAlignment(Qt::AlignCenter);
	}
	resetChStart0((double)m_pDataProc->_params.ch_start_ind[0] * (double)m_pDataProc->_params.samp_intv);
	resetChStart1((double)m_pDataProc->_params.ch_start_ind[1] * (double)m_pDataProc->_params.samp_intv);
	resetChStart2((double)m_pDataProc->_params.ch_start_ind[2] * (double)m_pDataProc->_params.samp_intv);
	resetChStart3((double)m_pDataProc->_params.ch_start_ind[3] * (double)m_pDataProc->_params.samp_intv);
    resetChStart4((double)m_pDataProc->_params.ch_start_ind[4] * (double)m_pDataProc->_params.samp_intv);

	m_pLabel_NanoSec = new QLabel("nsec", this);
	m_pLabel_NanoSec->setFixedWidth(25);

	// Set layout
	QHBoxLayout *pHBoxLayout_Background = new QHBoxLayout;
	pHBoxLayout_Background->setSpacing(2);

	pHBoxLayout_Background->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_Background->addWidget(m_pCheckBox_ShowWindow);
	pHBoxLayout_Background->addWidget(m_pCheckBox_SplineView);
	pHBoxLayout_Background->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_Background->addWidget(m_pPushButton_CaptureBackground);
	pHBoxLayout_Background->addWidget(m_pLineEdit_Background);

    pGridLayout_PulseView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0, 1, 4);
    pGridLayout_PulseView->addItem(pHBoxLayout_Background, 0, 4, 1, 4);
	
	QGridLayout *pGridLayout_ChStart = new QGridLayout;
	pGridLayout_ChStart->setSpacing(2);

	pGridLayout_ChStart->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_ChStart->addWidget(m_pLabel_ChStart, 1, 1);

    for (int i = 0; i < 5; i++)
	{
		pGridLayout_ChStart->addWidget(m_pLabel_Ch[i], 0, i + 2);
		pGridLayout_ChStart->addWidget(m_pSpinBox_ChStart[i], 1, i + 2);
	}
    pGridLayout_ChStart->addWidget(m_pLabel_NanoSec, 1, 7);

    pGridLayout_PulseView->addItem(pGridLayout_ChStart, 1, 0, 1, 9);

	m_pVBoxLayout->addItem(pGridLayout_PulseView);

	// Connect
	connect(m_pPushButton_CaptureBackground, SIGNAL(clicked(bool)), this, SLOT(captureBackground()));
	connect(m_pLineEdit_Background, SIGNAL(textChanged(const QString &)), this, SLOT(captureBackground(const QString &)));
	connect(m_pSpinBox_ChStart[0], SIGNAL(valueChanged(double)), this, SLOT(resetChStart0(double)));
	connect(m_pSpinBox_ChStart[1], SIGNAL(valueChanged(double)), this, SLOT(resetChStart1(double)));
	connect(m_pSpinBox_ChStart[2], SIGNAL(valueChanged(double)), this, SLOT(resetChStart2(double)));
	connect(m_pSpinBox_ChStart[3], SIGNAL(valueChanged(double)), this, SLOT(resetChStart3(double)));
    connect(m_pSpinBox_ChStart[4], SIGNAL(valueChanged(double)), this, SLOT(resetChStart4(double)));
}
			

void PulseCalibDlg::drawRoiPulse(DataProcess* pFLIm, int aline)
{
	// Reset pulse view (if necessary)
//	static int roi_width = 0;
	np::FloatArray2 data0((!m_pCheckBox_SplineView->isChecked()) ? pFLIm->_operator.crop_src0 : pFLIm->_operator.ext_src);
	np::FloatArray2 data(data0.size(0), data0.size(1));
	memcpy(data.raw_ptr(), data0.raw_ptr(), sizeof(float) * data0.length());

//	if (roi_width != data.size(0))
//	{
//		float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : pFLIm->_operator.upSampleFactor;

//		//m_pScope_PulseView->getRender()->m_bMaskUse = true;
//		m_pScope_PulseView->resetAxis({ 0, (double)data.size(0) }, { 0, POWER_2(16) },
//			pFLIm->_params.samp_intv / factor, PX14_VOLT_RANGE / (double)POWER_2(16), 0, 0, " nsec", " V");
//		roi_width = data.size(0);
//	}

    // ROI image
    np::Uint8Array2 pulse_image(data.size(0), data.size(1));
    ippiScale_32f8u_C1R(data.raw_ptr(), sizeof(float) * data.size(0),
                        pulse_image.raw_ptr(), sizeof(uint8_t) * pulse_image.size(0),
                        {data.size(0), data.size(1)}, 0, 65535);
    m_pImageView_PulseImage->drawImage(pulse_image.raw_ptr());

	// ROI pulse
	ippsAddC_32f_I(pFLIm->_params.bg, data.raw_ptr(), data.length());
	m_pScope_PulseView->drawData(&data(0, aline));
}

void PulseCalibDlg::showWindow(bool checked)
{
	if (checked)
	{
		int* ch_ind = (!m_pCheckBox_SplineView->isChecked()) ? m_pDataProc->_params.ch_start_ind : m_pDataProc->_operator.ch_start_ind1;

        m_pImageView_PulseImage->setVerticalLine(5, ch_ind[0], ch_ind[1], ch_ind[2], ch_ind[3], ch_ind[4]);
        m_pScope_PulseView->setWindowLine(5, ch_ind[0], ch_ind[1], ch_ind[2], ch_ind[3], ch_ind[4]);
	}
	else
	{
        m_pImageView_PulseImage->setVerticalLine(0);
		m_pScope_PulseView->setWindowLine(0);
	}

    m_pImageView_PulseImage->getRender()->update();
	m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::splineView(bool checked)
{
	if (m_pCheckBox_ShowWindow->isChecked())
	{
		int* ch_ind = (!checked) ? m_pDataProc->_params.ch_start_ind : m_pDataProc->_operator.ch_start_ind1;

        m_pImageView_PulseImage->setVerticalLine(5, ch_ind[0], ch_ind[1], ch_ind[2], ch_ind[3], ch_ind[4]);
        m_pScope_PulseView->setWindowLine(5, ch_ind[0], ch_ind[1], ch_ind[2], ch_ind[3], ch_ind[4]);
	}
	else
	{
        m_pImageView_PulseImage->setVerticalLine(0);
		m_pScope_PulseView->setWindowLine(0);
	}

    m_pImageView_PulseImage->getRender()->update();
	drawRoiPulse(m_pDataProc, 0);
}


void PulseCalibDlg::captureBackground()
{
	Ipp32f bg;
	ippsMean_32f(m_pScope_PulseView->getRender()->m_pData, m_pScope_PulseView->getRender()->m_sizeGraph.width(), &bg, ippAlgHintFast);

	m_pLineEdit_Background->setText(QString::number(bg, 'f', 2));

	m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::captureBackground(const QString &str)
{
	float bg = str.toFloat();

	m_pDataProc->_params.bg = bg;
	m_pConfig->flimBg = bg;

	m_pScope_PulseView->setDcLine(bg);

	m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::resetChStart0(double start)
{
	int ch_ind = (int)round(start / m_pDataProc->_params.samp_intv);

	m_pDataProc->_params.ch_start_ind[0] = ch_ind;
	m_pConfig->flimChStartInd[0] = ch_ind;

    m_pDataProc->_operator.initiated = false;

    m_pSpinBox_ChStart[1]->setMinimum((double)(ch_ind + 10) * (double)m_pDataProc->_params.samp_intv);

	if (m_pCheckBox_ShowWindow->isChecked())
	{
		float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pDataProc->_operator.ActualFactor;
        m_pImageView_PulseImage->getRender()->m_pVLineInd[0] = (int)round((float)(ch_ind) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[0] = (int)round((float)(ch_ind) * factor);
	}

    m_pImageView_PulseImage->getRender()->update();
	m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::resetChStart1(double start)
{
	int ch_ind = (int)round(start / m_pDataProc->_params.samp_intv);

	m_pDataProc->_params.ch_start_ind[1] = ch_ind;
    m_pConfig->flimChStartInd[1] = ch_ind;

    m_pDataProc->_operator.initiated = false;

	m_pSpinBox_ChStart[0]->setMaximum((double)(ch_ind - 10) * (double)m_pDataProc->_params.samp_intv);
	m_pSpinBox_ChStart[2]->setMinimum((double)(ch_ind + 10) * (double)m_pDataProc->_params.samp_intv);

	if (m_pCheckBox_ShowWindow->isChecked())
	{
		float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pDataProc->_operator.ActualFactor;
        m_pImageView_PulseImage->getRender()->m_pVLineInd[1] = (int)round((float)(ch_ind) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[1] = (int)round((float)(ch_ind) * factor);
	}

    m_pImageView_PulseImage->getRender()->update();
	m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::resetChStart2(double start)
{
	int ch_ind = (int)round(start / m_pDataProc->_params.samp_intv);

	m_pDataProc->_params.ch_start_ind[2] = ch_ind;
	m_pConfig->flimChStartInd[2] = ch_ind;

    m_pDataProc->_operator.initiated = false;

	m_pSpinBox_ChStart[1]->setMaximum((double)(ch_ind - 10) * (double)m_pDataProc->_params.samp_intv);
	m_pSpinBox_ChStart[3]->setMinimum((double)(ch_ind + 10) * (double)m_pDataProc->_params.samp_intv);

	if (m_pCheckBox_ShowWindow->isChecked())
	{
		float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pDataProc->_operator.ActualFactor;
        m_pImageView_PulseImage->getRender()->m_pVLineInd[2] = (int)round((float)(ch_ind) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[2] = (int)round((float)(ch_ind) * factor);
	}

    m_pImageView_PulseImage->getRender()->update();
	m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::resetChStart3(double start)
{
    int ch_ind = (int)round(start / m_pDataProc->_params.samp_intv);

    m_pDataProc->_params.ch_start_ind[3] = ch_ind;
    m_pConfig->flimChStartInd[3] = ch_ind;

    m_pDataProc->_operator.initiated = false;

    m_pSpinBox_ChStart[2]->setMaximum((double)(ch_ind - 10) * (double)m_pDataProc->_params.samp_intv);
    m_pSpinBox_ChStart[4]->setMinimum((double)(ch_ind + 10) * (double)m_pDataProc->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pDataProc->_operator.ActualFactor;
        m_pImageView_PulseImage->getRender()->m_pVLineInd[3] = (int)round((float)(ch_ind) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[3] = (int)round((float)(ch_ind) * factor);
    }

    m_pImageView_PulseImage->getRender()->update();
    m_pScope_PulseView->getRender()->update();
}

void PulseCalibDlg::resetChStart4(double start)
{
    int ch_ind = (int)round(start / m_pDataProc->_params.samp_intv);

    m_pDataProc->_params.ch_start_ind[4] = ch_ind;
    m_pConfig->flimChStartInd[4] = ch_ind;

    m_pDataProc->_operator.initiated = false;

    m_pSpinBox_ChStart[3]->setMaximum((double)(ch_ind - 10) * (double)m_pDataProc->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
        float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pDataProc->_operator.ActualFactor;
        m_pImageView_PulseImage->getRender()->m_pVLineInd[4] = (int)round((float)(ch_ind) * factor);
        m_pScope_PulseView->getRender()->m_pWinLineInd[4] = (int)round((float)(ch_ind) * factor);
    }

    m_pImageView_PulseImage->getRender()->update();
    m_pScope_PulseView->getRender()->update();
}

