
#include "QDeviceControlTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Dialog/PulseCalibDlg.h>
#include <Doulos/Viewer/QImageView.h>

#include <DeviceControl/PmtGain/PmtGainControl.h>
#include <DeviceControl/PmtGain/PmtGainMonitor.h>
#include <DeviceControl/ResonantScan/ResonantScan.h>
#include <DeviceControl/GalvoScan/GalvoScan.h>
#include <DeviceControl/ZaberStage/ZaberStage.h>
#include <DeviceControl/QSerialComm.h>

#include <DataAcquisition/DataAcquisition.h>
#include <MemoryBuffer/MemoryBuffer.h>

#include <Common/Array.h>

#include <iostream>
#include <thread>


QDeviceControlTab::QDeviceControlTab(QWidget *parent) :
    QDialog(parent), m_pPmtGainControl(nullptr), m_pPmtGainMonitor(nullptr), m_pPulseCalibDlg(nullptr),
    m_pResonantScan(nullptr), m_pGalvoScan(nullptr), m_pZaberStage(nullptr)
{
	// Set main window objects
    m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(0);

	m_pVBoxLayout_PulseControl = new QVBoxLayout;
	m_pVBoxLayout_PulseControl->setSpacing(3);
    m_pGroupBox_PulseControl = new QGroupBox;

    createPmtGainControl();
    createPulseCalibControl();
	createResonantScanControl();
    createGalvoScanControl();
    createZaberStageControl();
	
    // Set layout
    setLayout(m_pVBoxLayout);
}

QDeviceControlTab::~QDeviceControlTab()
{
    if (m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(false);
	if (m_pCheckBox_ResonantScannerControl->isChecked()) m_pCheckBox_ResonantScannerControl->setChecked(false);
	if (m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(false);
    if (m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(false);
}

void QDeviceControlTab::closeEvent(QCloseEvent* e)
{
    e->accept();
}

void QDeviceControlTab::setControlsStatus(bool enabled)
{
	if (!enabled)
	{
		if (m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(false);
		if (m_pCheckBox_ResonantScannerControl->isChecked()) m_pCheckBox_ResonantScannerControl->setChecked(false);
		if (m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(false);
        if (m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(false);
	}
	else
	{
		if (!m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(true);
		if (!m_pCheckBox_ResonantScannerControl->isChecked()) m_pCheckBox_ResonantScannerControl->setChecked(true);
		if (!m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(true);        
        if (!m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(true);
	}
}

void QDeviceControlTab::setRelevantWidgets(bool enabled)
{
    // Gain control
    m_pCheckBox_PmtGainControl->setEnabled(enabled);

    // PX14 Calibration
    m_pPushButton_PulseCalibration->setEnabled(enabled);

    // Resonant scanner control
    m_pCheckBox_ResonantScannerControl->setEnabled(enabled);
    m_pToggledButton_ResonantScanner->setEnabled(enabled);
    m_pDoubleSpinBox_ResonantScanner->setEnabled(enabled);
    m_pLabel_ResonantScanner->setEnabled(enabled);

    // Galvano mirror control
    m_pCheckBox_GalvoScanControl->setEnabled(enabled);

    // Zaber stage control
    m_pCheckBox_ZaberStageControl->setEnabled(enabled);
    m_pPushButton_SetStageNumber->setEnabled(enabled);
    m_pPushButton_SetTargetSpeed->setEnabled(enabled);
    m_pPushButton_MoveRelative->setEnabled(enabled);
    m_pPushButton_Home->setEnabled(enabled);
    m_pPushButton_Stop->setEnabled(enabled);
    m_pComboBox_StageNumber->setEnabled(enabled);
    m_pLineEdit_TargetSpeed->setEnabled(enabled);
    m_pLineEdit_TravelLength->setEnabled(enabled);
    m_pLabel_TargetSpeed->setEnabled(enabled);
    m_pLabel_TravelLength->setEnabled(enabled);

}

void QDeviceControlTab::createPmtGainControl()
{
    // Create widgets for PMT gain control
    QHBoxLayout *pHBoxLayout_PmtGainControl = new QHBoxLayout;
    pHBoxLayout_PmtGainControl->setSpacing(3);

    m_pCheckBox_PmtGainControl = new QCheckBox(m_pGroupBox_PulseControl);
    m_pCheckBox_PmtGainControl->setText("Apply and Monitor PMT Gain Voltage");
    m_pCheckBox_PmtGainControl->setFixedWidth(250);

    //m_pLineEdit_PmtGainVoltage = new QLineEdit(m_pGroupBox_PulseControl);
    //m_pLineEdit_PmtGainVoltage->setFixedWidth(35);
    //m_pLineEdit_PmtGainVoltage->setText(QString::number(m_pConfig->pmtGainVoltage, 'f', 2));
    //m_pLineEdit_PmtGainVoltage->setAlignment(Qt::AlignCenter);
    //m_pLineEdit_PmtGainVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    //m_pLineEdit_PmtGainVoltage->setEnabled(true);

    //m_pLabel_PmtGainVoltage = new QLabel("V", m_pGroupBox_PulseControl);
    //m_pLabel_PmtGainVoltage->setBuddy(m_pLineEdit_PmtGainVoltage);
    //m_pLabel_PmtGainVoltage->setEnabled(true);

    pHBoxLayout_PmtGainControl->addWidget(m_pCheckBox_PmtGainControl);
    pHBoxLayout_PmtGainControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    //pHBoxLayout_PmtGainControl->addWidget(m_pLineEdit_PmtGainVoltage);
    //pHBoxLayout_PmtGainControl->addWidget(m_pLabel_PmtGainVoltage);

    m_pVBoxLayout_PulseControl->addItem(pHBoxLayout_PmtGainControl);

    // Connect signal and slot
    connect(m_pCheckBox_PmtGainControl, SIGNAL(toggled(bool)), this, SLOT(applyPmtGainVoltage(bool)));
    //connect(m_pLineEdit_PmtGainVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changePmtGainVoltage(const QString &)));
}

void QDeviceControlTab::createPulseCalibControl()
{
    // Create widgets for Pulse Calibration
    QGridLayout *pGridLayout_FlimCalibration = new QGridLayout;
    pGridLayout_FlimCalibration->setSpacing(3);

    m_pCheckBox_PX14DigitizerControl = new QCheckBox(m_pGroupBox_PulseControl);
    m_pCheckBox_PX14DigitizerControl->setText("Connect to PX14 Digitizer");
    m_pCheckBox_PX14DigitizerControl->setDisabled(true);
	
	m_pLineEdit_PX14PreTrigger = new QLineEdit(m_pGroupBox_PulseControl);
	m_pLineEdit_PX14PreTrigger->setFixedWidth(35);
	m_pLineEdit_PX14PreTrigger->setText(QString::number(m_pConfig->px14PreTrigger));
	m_pLineEdit_PX14PreTrigger->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PX14PreTrigger->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_PX14PreTrigger->setDisabled(true);

	m_pLabel_PX14PreTrigger = new QLabel("PreTrigger", m_pGroupBox_PulseControl);
	m_pLabel_PX14PreTrigger->setBuddy(m_pLineEdit_PX14PreTrigger);
	m_pLabel_PX14PreTrigger->setDisabled(true);

	m_pLineEdit_PX14TriggerDelay = new QLineEdit(m_pGroupBox_PulseControl);
	m_pLineEdit_PX14TriggerDelay->setFixedWidth(35);
	m_pLineEdit_PX14TriggerDelay->setText(QString::number(m_pConfig->px14TriggerDelay));
	m_pLineEdit_PX14TriggerDelay->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PX14TriggerDelay->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_PX14TriggerDelay->setDisabled(true);

	m_pLabel_PX14TriggerDelay = new QLabel("  Trigger Delay", m_pGroupBox_PulseControl);
	m_pLabel_PX14TriggerDelay->setBuddy(m_pLineEdit_PX14TriggerDelay);
	m_pLabel_PX14TriggerDelay->setDisabled(true);
	
	m_pSlider_PX14DcOffset = new QSlider(m_pGroupBox_PulseControl);
	m_pSlider_PX14DcOffset->setOrientation(Qt::Horizontal);
	m_pSlider_PX14DcOffset->setRange(0, 4095);
	m_pSlider_PX14DcOffset->setValue(m_pConfig->px14DcOffset);
	m_pSlider_PX14DcOffset->setDisabled(true);

	m_pLabel_PX14DcOffset = new QLabel(QString("Set DC Offset (%1)").arg(m_pConfig->px14DcOffset, 4, 10), m_pGroupBox_PulseControl);
	m_pLabel_PX14DcOffset->setBuddy(m_pSlider_PX14DcOffset);
	m_pLabel_PX14DcOffset->setDisabled(true);

	m_pPushButton_PulseCalibration = new QPushButton(this);
	m_pPushButton_PulseCalibration->setText("Pulse View and Calibration...");
	m_pPushButton_PulseCalibration->setFixedWidth(200);
	m_pPushButton_PulseCalibration->setDisabled(true);
	
	QHBoxLayout *pHBoxLayout_TriggerSamples = new QHBoxLayout;
	pHBoxLayout_TriggerSamples->setSpacing(2);

	pHBoxLayout_TriggerSamples->addWidget(m_pLabel_PX14PreTrigger);
	pHBoxLayout_TriggerSamples->addWidget(m_pLineEdit_PX14PreTrigger);
	pHBoxLayout_TriggerSamples->addWidget(m_pLabel_PX14TriggerDelay);
	pHBoxLayout_TriggerSamples->addWidget(m_pLineEdit_PX14TriggerDelay);

    pGridLayout_FlimCalibration->addWidget(m_pCheckBox_PX14DigitizerControl, 0, 0, 1, 3);
	pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_FlimCalibration->addItem(pHBoxLayout_TriggerSamples, 1, 1, 1, 2);
    pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_FlimCalibration->addWidget(m_pLabel_PX14DcOffset, 2, 1);
    pGridLayout_FlimCalibration->addWidget(m_pSlider_PX14DcOffset, 2, 2);
    pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 0);
    pGridLayout_FlimCalibration->addWidget(m_pPushButton_PulseCalibration, 3, 1, 1, 2);

    m_pVBoxLayout_PulseControl->addItem(pGridLayout_FlimCalibration);

    m_pGroupBox_PulseControl->setLayout(m_pVBoxLayout_PulseControl);
    m_pGroupBox_PulseControl->resize(m_pVBoxLayout_PulseControl->minimumSize());
    m_pVBoxLayout->addWidget(m_pGroupBox_PulseControl);

    // Connect signal and slot
    connect(m_pCheckBox_PX14DigitizerControl, SIGNAL(toggled(bool)), this, SLOT(connectPX14Digitizer(bool)));
	connect(m_pLineEdit_PX14PreTrigger, SIGNAL(textChanged(const QString &)), this, SLOT(setPX14PreTrigger(const QString &)));
	connect(m_pLineEdit_PX14TriggerDelay, SIGNAL(textChanged(const QString &)), this, SLOT(setPX14TriggerDelay(const QString &)));
	connect(m_pSlider_PX14DcOffset, SIGNAL(valueChanged(int)), this, SLOT(setPX14DcOffset(int)));
    connect(m_pPushButton_PulseCalibration, SIGNAL(clicked(bool)), this, SLOT(createPulseCalibDlg()));
}

void QDeviceControlTab::createResonantScanControl()
{
	// Create widgets for resonant scanner control
	QGroupBox *pGroupBox_ResonantScannerControl = new QGroupBox;
	QGridLayout *pGridLayout_ResonantScannerControl = new QGridLayout;
	pGridLayout_ResonantScannerControl->setSpacing(3);
	
	m_pCheckBox_ResonantScannerControl = new QCheckBox(pGroupBox_ResonantScannerControl);
	m_pCheckBox_ResonantScannerControl->setText("Connect to Resonant Scanner Voltage Control");

	m_pToggledButton_ResonantScanner = new QPushButton(pGroupBox_ResonantScannerControl);
	m_pToggledButton_ResonantScanner->setCheckable(true);
	m_pToggledButton_ResonantScanner->setText("Voltage On");
	m_pToggledButton_ResonantScanner->setDisabled(true);

	m_pDoubleSpinBox_ResonantScanner = new QDoubleSpinBox(pGroupBox_ResonantScannerControl);
	m_pDoubleSpinBox_ResonantScanner->setFixedWidth(50);
	m_pDoubleSpinBox_ResonantScanner->setRange(0.0, 5.0);
	m_pDoubleSpinBox_ResonantScanner->setSingleStep(0.1);
	m_pDoubleSpinBox_ResonantScanner->setValue(0.0);
	m_pDoubleSpinBox_ResonantScanner->setDecimals(1);
	m_pDoubleSpinBox_ResonantScanner->setAlignment(Qt::AlignCenter);
	m_pDoubleSpinBox_ResonantScanner->setDisabled(true);

	m_pLabel_ResonantScanner = new QLabel("V", pGroupBox_ResonantScannerControl);
	m_pLabel_ResonantScanner->setDisabled(true);


	pGridLayout_ResonantScannerControl->addWidget(m_pCheckBox_ResonantScannerControl, 0, 0, 1, 4);

	pGridLayout_ResonantScannerControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_ResonantScannerControl->addWidget(m_pToggledButton_ResonantScanner, 1, 1);
	pGridLayout_ResonantScannerControl->addWidget(m_pDoubleSpinBox_ResonantScanner, 1, 2);
	pGridLayout_ResonantScannerControl->addWidget(m_pLabel_ResonantScanner, 1, 3);

	pGroupBox_ResonantScannerControl->setLayout(pGridLayout_ResonantScannerControl);
	pGroupBox_ResonantScannerControl->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pVBoxLayout->addWidget(pGroupBox_ResonantScannerControl);

	// Connect signal and slot
	connect(m_pCheckBox_ResonantScannerControl, SIGNAL(toggled(bool)), this, SLOT(connectResonantScanner(bool)));
	connect(m_pToggledButton_ResonantScanner, SIGNAL(toggled(bool)), this, SLOT(applyResonantScannerVoltage(bool)));
	connect(m_pDoubleSpinBox_ResonantScanner, SIGNAL(valueChanged(double)), this, SLOT(setResonantScannerVoltage(double)));
}

void QDeviceControlTab::createGalvoScanControl()
{
    // Create widgets for galvano mirror control
    QGroupBox *pGroupBox_GalvanoMirrorControl = new QGroupBox;
    QGridLayout *pGridLayout_GalvanoMirrorControl = new QGridLayout;
    pGridLayout_GalvanoMirrorControl->setSpacing(3);

    m_pCheckBox_GalvoScanControl = new QCheckBox(pGroupBox_GalvanoMirrorControl);
    m_pCheckBox_GalvoScanControl->setText("Connect to Galvano Mirror");

    m_pLineEdit_PeakToPeakVoltage = new QLineEdit(pGroupBox_GalvanoMirrorControl);
    m_pLineEdit_PeakToPeakVoltage->setFixedWidth(30);
    m_pLineEdit_PeakToPeakVoltage->setText(QString::number(m_pConfig->galvoScanVoltage, 'f', 1));
    m_pLineEdit_PeakToPeakVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_PeakToPeakVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLineEdit_OffsetVoltage = new QLineEdit(pGroupBox_GalvanoMirrorControl);
	m_pLineEdit_OffsetVoltage->setFixedWidth(30);
	m_pLineEdit_OffsetVoltage->setText(QString::number(m_pConfig->galvoScanVoltageOffset, 'f', 1));
	m_pLineEdit_OffsetVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_OffsetVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLabel_ScanVoltage = new QLabel("Scan Voltage ", pGroupBox_GalvanoMirrorControl);
    m_pLabel_ScanPlusMinus = new QLabel("+", pGroupBox_GalvanoMirrorControl);
    m_pLabel_ScanPlusMinus->setBuddy(m_pLineEdit_PeakToPeakVoltage);
    m_pLabel_GalvanoVoltage = new QLabel("V", pGroupBox_GalvanoMirrorControl);
    m_pLabel_GalvanoVoltage->setBuddy(m_pLineEdit_OffsetVoltage);
		

    pGridLayout_GalvanoMirrorControl->addWidget(m_pCheckBox_GalvoScanControl, 0, 0, 1, 6);

    pGridLayout_GalvanoMirrorControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_ScanVoltage, 2, 1);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_PeakToPeakVoltage, 2, 2);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_ScanPlusMinus, 2, 3);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_OffsetVoltage, 2, 4);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_GalvanoVoltage, 2, 5);

    pGroupBox_GalvanoMirrorControl->setLayout(pGridLayout_GalvanoMirrorControl);
    m_pVBoxLayout->addWidget(pGroupBox_GalvanoMirrorControl);

    // Connect signal and slot
    connect(m_pCheckBox_GalvoScanControl, SIGNAL(toggled(bool)), this, SLOT(connectGalvanoMirror(bool)));
    connect(m_pLineEdit_PeakToPeakVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoScanVoltage(const QString &)));
    connect(m_pLineEdit_OffsetVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoScanVoltageOffset(const QString &)));
}

void QDeviceControlTab::createZaberStageControl()
{
    // Create widgets for Zaber stage control
    QGroupBox *pGroupBox_ZaberStageControl = new QGroupBox;
    QGridLayout *pGridLayout_ZaberStageControl = new QGridLayout;
    pGridLayout_ZaberStageControl->setSpacing(3);

    m_pCheckBox_ZaberStageControl = new QCheckBox(pGroupBox_ZaberStageControl);
    m_pCheckBox_ZaberStageControl->setText("Connect to Zaber Stage");

    m_pPushButton_SetStageNumber = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_SetStageNumber->setText("Stage Number");
    m_pPushButton_SetStageNumber->setDisabled(true);
    m_pPushButton_SetTargetSpeed = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_SetTargetSpeed->setText("Target Speed");
    m_pPushButton_SetTargetSpeed->setDisabled(true);
    m_pPushButton_MoveRelative = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_MoveRelative->setText("Move Relative");
    m_pPushButton_MoveRelative->setDisabled(true);
    m_pPushButton_Home = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Home->setText("Home");
    m_pPushButton_Home->setFixedWidth(60);
    m_pPushButton_Home->setDisabled(true);
    m_pPushButton_Stop = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Stop->setText("Stop");
    m_pPushButton_Stop->setFixedWidth(60);
    m_pPushButton_Stop->setDisabled(true);

    m_pComboBox_StageNumber = new QComboBox(pGroupBox_ZaberStageControl);
    m_pComboBox_StageNumber->addItem("Y-axis");
    m_pComboBox_StageNumber->addItem("X-axis");
    m_pComboBox_StageNumber->setCurrentIndex(1);
    m_pComboBox_StageNumber->setDisabled(true);
    m_pLineEdit_TargetSpeed = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TargetSpeed->setFixedWidth(35);
    m_pLineEdit_TargetSpeed->setText(QString::number(m_pConfig->zaberPullbackSpeed, 'f', 2));
    m_pLineEdit_TargetSpeed->setAlignment(Qt::AlignCenter);
    m_pLineEdit_TargetSpeed->setDisabled(true);
    m_pLineEdit_TravelLength = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TravelLength->setFixedWidth(35);
    m_pLineEdit_TravelLength->setText(QString::number(m_pConfig->zaberPullbackLength, 'f', 2));
    m_pLineEdit_TravelLength->setAlignment(Qt::AlignCenter);
    m_pLineEdit_TravelLength->setDisabled(true);

    m_pLabel_TargetSpeed = new QLabel("mm/s", pGroupBox_ZaberStageControl);
    m_pLabel_TargetSpeed->setBuddy(m_pLineEdit_TargetSpeed);
    m_pLabel_TargetSpeed->setDisabled(true);
    m_pLabel_TravelLength = new QLabel("mm", pGroupBox_ZaberStageControl);
    m_pLabel_TravelLength->setBuddy(m_pLineEdit_TravelLength);
    m_pLabel_TravelLength->setDisabled(true);

//    m_pLabel_ScanningStatus = new QLabel("scanning", pGroupBox_ZaberStageControl);
//    m_pLabel_ScanningStatus->setStyleSheet("font: 12pt; color: red;");
//    m_pLabel_ScanningStatus->setAlignment(Qt::AlignCenter);
//    m_pLabel_ScanningStatus->setDisabled(true);


    pGridLayout_ZaberStageControl->addWidget(m_pCheckBox_ZaberStageControl, 0, 0, 1, 5);

    pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_SetStageNumber, 1, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pComboBox_StageNumber, 1, 3, 1, 2);

    //pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_SetTargetSpeed, 2, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TargetSpeed, 2, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TargetSpeed, 2, 4);

    //pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_MoveRelative, 3, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TravelLength, 3, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TravelLength, 3, 4);

    //pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 4, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Home, 4, 1);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Stop, 4, 2);
//    pGridLayout_ZaberStageControl->addWidget(m_pLabel_ScanningStatus, 2, 0, 3, 1);


    pGroupBox_ZaberStageControl->setLayout(pGridLayout_ZaberStageControl);
    m_pVBoxLayout->addWidget(pGroupBox_ZaberStageControl);

    // Connect signal and slot
    connect(m_pCheckBox_ZaberStageControl, SIGNAL(toggled(bool)), this, SLOT(connectZaberStage(bool)));
    connect(m_pPushButton_MoveRelative, SIGNAL(clicked(bool)), this, SLOT(moveRelative()));
    connect(m_pLineEdit_TargetSpeed, SIGNAL(textChanged(const QString &)), this, SLOT(setTargetSpeed(const QString &)));
    connect(m_pLineEdit_TravelLength, SIGNAL(textChanged(const QString &)), this, SLOT(changeZaberPullbackLength(const QString &)));
    connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
    connect(m_pPushButton_Stop, SIGNAL(clicked(bool)), this, SLOT(stop()));
}


void QDeviceControlTab::applyPmtGainVoltage(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_PmtGainControl->setText("Stop Applying and Monitoring PMT Gain Voltage");

        // Set enabled false for PMT gain control widgets
        //m_pLineEdit_PmtGainVoltage->setEnabled(false);
        //m_pLabel_PmtGainVoltage->setEnabled(false);

        // Create PMT gain control objects
        if (!m_pPmtGainControl)
        {
            m_pPmtGainControl = new PmtGainControl;
            m_pPmtGainControl->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

        //m_pPmtGainControl->voltage = m_pLineEdit_PmtGainVoltage->text().toDouble();
        //if (m_pPmtGainControl->voltage > 1.0)
        //{
        //    m_pPmtGainControl->SendStatusMessage(">1.0V Gain cannot be assigned!", true);
        //    m_pCheckBox_PmtGainControl->setChecked(false);
        //    return;
        //}

		// Create PMT gain monitor objects
		if (!m_pPmtGainMonitor)
		{
			m_pPmtGainMonitor = new PmtGainMonitor;

			m_pPmtGainMonitor->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};

			m_pPmtGainMonitor->acquiredData += [&](double voltage) {
				m_pStreamTab->getMainWnd()->m_pStatusLabel_PmtGain->setText(QString("PMT Gain Voltage: %1 V").arg(voltage, 4, 'f', 3));
				if (voltage > 1.5)
				{
					m_pPmtGainMonitor->SendStatusMessage("PMT Gain Voltage SATURATION!!", true);
					m_pCheckBox_PmtGainControl->setChecked(false);
				}
			};
		}

        // Initializing
        if (!m_pPmtGainControl->initialize() || !m_pPmtGainMonitor->initialize())
        {
            m_pCheckBox_PmtGainControl->setChecked(false);
            return;
        }

        // Generate & monitor PMT gain voltage
        m_pPmtGainControl->start();
		m_pPmtGainMonitor->start();
    }
    else
	{
		// Delete PMT gain monitor objects
		m_pStreamTab->getMainWnd()->m_pStatusLabel_PmtGain->setText(QString("PMT Gain Voltage: %1 V").arg(0.0, 4, 'f', 3));

		if (m_pPmtGainMonitor)
		{
			m_pPmtGainMonitor->stop();
			delete m_pPmtGainMonitor;
			m_pPmtGainMonitor = nullptr;
		}

        // Delete PMT gain control objects
        if (m_pPmtGainControl)
        {
            m_pPmtGainControl->stop();
            delete m_pPmtGainControl;
            m_pPmtGainControl = nullptr;
        }

        // Set enabled true for PMT gain control widgets
        //m_pLineEdit_PmtGainVoltage->setEnabled(true);
        //m_pLabel_PmtGainVoltage->setEnabled(true);

        // Set text
        m_pCheckBox_PmtGainControl->setText("Apply and Monitor PMT Gain Voltage");
    }
}

void QDeviceControlTab::changePmtGainVoltage(const QString &vol)
{
    m_pConfig->pmtGainVoltage = vol.toFloat();
}


void QDeviceControlTab::connectPX14Digitizer(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_PX14DigitizerControl->setText("Disconnect from PX14 Digitizer");

        // Connect the digitizer

        // Set enabled true for PX14 digitizer widgets
		m_pPushButton_PulseCalibration->setEnabled(true);
    }
    else
    {
        // Set disabled true for PX14 digitizer widgets
		m_pPushButton_PulseCalibration->setDisabled(true);

        // Close FLIm calibration window
        if (m_pPulseCalibDlg) m_pPulseCalibDlg->close();

        // Set text
        m_pCheckBox_PX14DigitizerControl->setText("Connect to PX14 Digitizer");
    }
}

void QDeviceControlTab::setPX14PreTrigger(const QString &str)
{
	int preTrigger = str.toInt();
	m_pConfig->px14PreTrigger = preTrigger;

	DataAcquisition *pDataAcq = m_pStreamTab->getOperationTab()->m_pDataAcquisition;
	pDataAcq->SetPreTrigger(preTrigger);
}

void QDeviceControlTab::setPX14TriggerDelay(const QString &str)
{
	int triggerDelay = str.toInt();
	m_pConfig->px14TriggerDelay = triggerDelay;
	
	DataAcquisition *pDataAcq = m_pStreamTab->getOperationTab()->m_pDataAcquisition;
	pDataAcq->SetTriggerDelay(triggerDelay);
}

void QDeviceControlTab::setPX14DcOffset(int offset)
{
	DataAcquisition *pDataAcq = m_pStreamTab->getOperationTab()->m_pDataAcquisition;
	pDataAcq->SetDcOffset(offset);

	m_pConfig->px14DcOffset = offset;
	m_pLabel_PX14DcOffset->setText(QString("Set DC Offset (%1)").arg(m_pConfig->px14DcOffset, 4, 10));
}

void QDeviceControlTab::createPulseCalibDlg()
{
    if (m_pPulseCalibDlg == nullptr)
    {
        m_pPulseCalibDlg = new PulseCalibDlg(this);
        connect(m_pPulseCalibDlg, SIGNAL(finished(int)), this, SLOT(deletePulseCalibDlg()));
        m_pPulseCalibDlg->show();
//        emit m_pPulseCalibDlg->plotRoiPulse(m_pDataProc, m_pSlider_SelectAline->value() / 4);
    }
    m_pPulseCalibDlg->raise();
    m_pPulseCalibDlg->activateWindow();
	
	m_pLabel_PX14PreTrigger->setEnabled(true);
	m_pLineEdit_PX14PreTrigger->setEnabled(true);
	m_pLabel_PX14TriggerDelay->setEnabled(true);
	m_pLineEdit_PX14TriggerDelay->setEnabled(true);
	m_pLabel_PX14DcOffset->setEnabled(true);
	m_pSlider_PX14DcOffset->setEnabled(true);

	m_pStreamTab->getVisualizationTab()->setImgViewVisPixelPos(true);
}

void QDeviceControlTab::deletePulseCalibDlg()
{
	m_pStreamTab->getVisualizationTab()->setImgViewVisPixelPos(false);

    m_pPulseCalibDlg->showWindow(false);

    m_pPulseCalibDlg->deleteLater();
    m_pPulseCalibDlg = nullptr;

	m_pLabel_PX14PreTrigger->setDisabled(true);
	m_pLineEdit_PX14PreTrigger->setDisabled(true);
	m_pLabel_PX14TriggerDelay->setDisabled(true);
	m_pLineEdit_PX14TriggerDelay->setDisabled(true);
	m_pLabel_PX14DcOffset->setDisabled(true);
	m_pSlider_PX14DcOffset->setDisabled(true);
}


void QDeviceControlTab::connectResonantScanner(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_ResonantScannerControl->setText("Disconnect from Resonant Scanner Voltage Control");

		// Set enable true for voltage control widgets
		m_pToggledButton_ResonantScanner->setEnabled(true);
	}
	else
	{
		// Voltage control off
		m_pToggledButton_ResonantScanner->setChecked(false);

		// Set enable false for voltage control widgets
		m_pToggledButton_ResonantScanner->setEnabled(false);
		m_pDoubleSpinBox_ResonantScanner->setEnabled(false);
		m_pLabel_ResonantScanner->setEnabled(false);

		// Set text
		m_pCheckBox_ResonantScannerControl->setText("Connect to Resonant Scanner Voltage Control");
	}
}

void QDeviceControlTab::applyResonantScannerVoltage(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pToggledButton_ResonantScanner->setText("Voltage Off");

		// Create resonant scanner voltage control objects
		if (!m_pResonantScan)
		{
			m_pResonantScan = new ResonantScan;
			m_pResonantScan->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Initializing
		if (!m_pResonantScan->initialize())
		{
			m_pToggledButton_ResonantScanner->setChecked(false);
			return;
		}

		// Apply zero voltage
		m_pResonantScan->apply(0);
		m_pConfig->resonantScanVoltage = 0;

		// Set enable true for resonant scanner voltage control
		m_pDoubleSpinBox_ResonantScanner->setEnabled(true);
		m_pLabel_ResonantScanner->setEnabled(true);
	}
	else
	{
		// Set enable false for resonant scanner voltage control
		m_pDoubleSpinBox_ResonantScanner->setValue(0.0);
		m_pDoubleSpinBox_ResonantScanner->setEnabled(false);
		m_pLabel_ResonantScanner->setEnabled(false);

		// Delete resonant scanner voltage control objects
		if (m_pResonantScan)
		{
			m_pResonantScan->apply(0);
			m_pConfig->resonantScanVoltage = 0;

			delete m_pResonantScan;
			m_pResonantScan = nullptr;
		}

		// Set text
		m_pToggledButton_ResonantScanner->setText("Voltage On");
	}
}

void QDeviceControlTab::setResonantScannerVoltage(double voltage)
{
	// Generate voltage for resonant scanner control
	m_pResonantScan->apply(voltage);
	m_pConfig->resonantScanVoltage = voltage;
}


void QDeviceControlTab::connectGalvanoMirror(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_GalvoScanControl->setText("Disconnect from Galvano Mirror");

        // Set enabled false for Galvano mirror control widgets
        m_pLabel_ScanVoltage->setEnabled(false);
        m_pLineEdit_PeakToPeakVoltage->setEnabled(false);
        m_pLabel_ScanPlusMinus->setEnabled(false);
        m_pLineEdit_OffsetVoltage->setEnabled(false);
        m_pLabel_GalvanoVoltage->setEnabled(false);
		
        // Create Galvano mirror control objects
        if (!m_pGalvoScan)
        {
            m_pGalvoScan = new GalvoScan;
            m_pGalvoScan->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

        //m_pGalvoScan->horizontal_size = m_pConfig->imageSize;
        m_pGalvoScan->pp_voltage = m_pLineEdit_PeakToPeakVoltage->text().toDouble();
        m_pGalvoScan->offset = m_pLineEdit_OffsetVoltage->text().toDouble();
		m_pGalvoScan->step = m_pConfig->nLines; // m_pGalvoScan->pp_voltage_slow / (double)m_pConfig->imageSize;

        // Initializing
		if (!m_pGalvoScan->initialize())
		{
			m_pCheckBox_GalvoScanControl->setChecked(false);
			return;
		}
		else
			m_pGalvoScan->start();
    }
    else
    {
        // Delete Galvano mirror control objects
        if (m_pGalvoScan)
        {
            m_pGalvoScan->stop();
            delete m_pGalvoScan;
            m_pGalvoScan = nullptr;
        }

        // Set enabled false for Galvano mirror control widgets
        m_pLabel_ScanVoltage->setEnabled(true);
        m_pLineEdit_PeakToPeakVoltage->setEnabled(true);
        m_pLabel_ScanPlusMinus->setEnabled(true);
        m_pLineEdit_OffsetVoltage->setEnabled(true);
        m_pLabel_GalvanoVoltage->setEnabled(true);
		
        // Set text
        m_pCheckBox_GalvoScanControl->setText("Connect to Galvano Mirror");
    }
}

void QDeviceControlTab::changeGalvoScanVoltage(const QString &vol)
{
    m_pConfig->galvoScanVoltage = vol.toFloat();
}

void QDeviceControlTab::changeGalvoScanVoltageOffset(const QString &vol)
{
    m_pConfig->galvoScanVoltageOffset = vol.toFloat();
}

void QDeviceControlTab::connectZaberStage(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_ZaberStageControl->setText("Disconnect from Zaber Stage");

        // Create Zaber stage control objects
        if (!m_pZaberStage)
        {
            m_pZaberStage = new ZaberStage;
            m_pZaberStage->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                qmsg.replace('\n', ' ');
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

        // Connect stage
        if (!(m_pZaberStage->ConnectDevice()))
        {
            m_pCheckBox_ZaberStageControl->setChecked(false);
            return;
        }

        // Set callback function
        m_pZaberStage->DidMovedRelative += [&](const char* msg) {
            QString qmsg = QString::fromUtf8(msg);
            bool is_idle = qmsg.contains("IDLE", Qt::CaseInsensitive);;
            if (is_idle)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                m_pZaberStage->setIsMoving(false);
#ifndef RAW_PULSE_WRITE
                if (m_pStreamTab->getOperationTab()->m_pMemoryBuffer->m_bIsRecordingImage)
                    if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
                        getStreamTab()->m_bIsStageTransition = false;
#endif
            }
        };

        m_pZaberStage->DidMonitoring += [&]() {
            emit monitoring();
        };

        // Set target speed first
        for (int i = 1; i <= 2; i++)
        {
            m_pZaberStage->SetTargetSpeed(i, m_pLineEdit_TargetSpeed->text().toDouble());
            // m_pZaberStage->Home(i);
        }

        // Set enable true for Zaber stage control widgets
        m_pPushButton_SetStageNumber->setEnabled(true);
        m_pPushButton_MoveRelative->setEnabled(true);
        m_pPushButton_SetTargetSpeed->setEnabled(true);
        m_pPushButton_Home->setEnabled(true);
        m_pPushButton_Stop->setEnabled(true);
        m_pComboBox_StageNumber->setEnabled(true);
        m_pLineEdit_TravelLength->setEnabled(true);
        m_pLineEdit_TargetSpeed->setEnabled(true);
        m_pLabel_TravelLength->setEnabled(true);
        m_pLabel_TargetSpeed->setEnabled(true);

#ifndef RAW_PULSE_WRITE
        m_pStreamTab->getImageStitchingCheckBox()->setEnabled(true);
#endif

        // signal & slot connection
        connect(this, SIGNAL(startStageScan(int, double)), this, SLOT(stageScan(int, double)));
        connect(this, SIGNAL(monitoring()), this, SLOT(getCurrentPosition()));
    }
    else
    {
        // signal & slot disconnection
        disconnect(this, SIGNAL(startStageScan(int, double)), 0, 0);
        disconnect(this, SIGNAL(monitoring()), 0, 0);

        // Set enable false for Zaber stage control widgets
#ifndef RAW_PULSE_WRITE
        if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
            m_pStreamTab->getImageStitchingCheckBox()->setChecked(false);
        m_pStreamTab->getImageStitchingCheckBox()->setDisabled(true);
#endif

        m_pPushButton_SetStageNumber->setEnabled(false);
        m_pPushButton_MoveRelative->setEnabled(false);
        m_pPushButton_SetTargetSpeed->setEnabled(false);
        m_pPushButton_Home->setEnabled(false);
        m_pPushButton_Stop->setEnabled(false);
        m_pComboBox_StageNumber->setEnabled(false);
        m_pLineEdit_TravelLength->setEnabled(false);
        m_pLineEdit_TargetSpeed->setEnabled(false);
        m_pLabel_TravelLength->setEnabled(false);
        m_pLabel_TargetSpeed->setEnabled(false);

        if (m_pZaberStage)
        {
            // Disconnect the Stage
            m_pZaberStage->DisconnectDevice();

            // Delete Zaber stage control objects
            delete m_pZaberStage;
            m_pZaberStage = nullptr;
        }

        // Set text
        m_pCheckBox_ZaberStageControl->setText("Connect to Zaber Stage");
    }
}

void QDeviceControlTab::moveRelative()
{
    m_pZaberStage->MoveRelative(m_pComboBox_StageNumber->currentIndex() + 1, m_pLineEdit_TravelLength->text().toDouble());
}

void QDeviceControlTab::setTargetSpeed(const QString & str)
{
    m_pZaberStage->SetTargetSpeed(1, str.toDouble());
    m_pZaberStage->SetTargetSpeed(2, str.toDouble());
    m_pConfig->zaberPullbackSpeed = str.toFloat();
}

void QDeviceControlTab::changeZaberPullbackLength(const QString &str)
{
    m_pConfig->zaberPullbackLength = str.toFloat();
}

void QDeviceControlTab::home()
{
    m_pZaberStage->Home(m_pComboBox_StageNumber->currentIndex() + 1);
}

void QDeviceControlTab::stop()
{
    m_pZaberStage->Stop(1);
    m_pZaberStage->Stop(2);
}

void QDeviceControlTab::stageScan(int dev_num, double length)
{
    if (length != 0)
        m_pZaberStage->MoveRelative(dev_num, length);
    else
        m_pZaberStage->Home(dev_num);
}

void QDeviceControlTab::getCurrentPosition()
{
    m_pZaberStage->GetPos();
}
