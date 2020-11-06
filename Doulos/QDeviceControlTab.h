#ifndef QDEVICECONTROLTAB_H
#define QDEVICECONTROLTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>

#include <iostream>
#include <mutex>
#include <condition_variable>


class QStreamTab;

class PmtGainControl;
class PmtGainMonitor;
class PulseCalibDlg;

class ResonantScan;
class GalvoScan;
class ZaberStage;

class QImageView;

class QMySpinBox : public QDoubleSpinBox
{
public:
    explicit QMySpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent)
    {
        lineEdit()->setReadOnly(true);
    }
    virtual ~QMySpinBox() {}
};


class QDeviceControlTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QDeviceControlTab(QWidget *parent = nullptr);
    virtual ~QDeviceControlTab();

// Methods //////////////////////////////////////////////
protected:
	void closeEvent(QCloseEvent*);

public:
	void setControlsStatus(bool enabled);
    void setRelevantWidgets(bool enabled);

public: ////////////////////////////////////////////////////////////////////////////////////////////////
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline PulseCalibDlg* getPulseCalibDlg() const { return m_pPulseCalibDlg; }
    inline QCheckBox* getPX14DigitizerControl() const { return m_pCheckBox_PX14DigitizerControl; }
	inline QCheckBox* getPmtGainControl() const { return m_pCheckBox_PmtGainControl; }
    inline QCheckBox* getGalvoScanControl() const { return m_pCheckBox_GalvoScanControl; }	
	inline QCheckBox* getResonantScanControl() const { return m_pCheckBox_ResonantScannerControl; }
	inline QPushButton* getResonantScanVoltageControl() const { return m_pToggledButton_ResonantScanner; }
	inline QDoubleSpinBox* getResonantScanVoltageSpinBox() const { return m_pDoubleSpinBox_ResonantScanner; }
    inline QCheckBox* getZaberStageControl() const { return m_pCheckBox_ZaberStageControl; }
    inline GalvoScan* getGalvoScan() const { return m_pGalvoScan; }
    inline ZaberStage* getZaberStage() const { return m_pZaberStage; }

private: ////////////////////////////////////////////////////////////////////////////////////////////////
    void createPmtGainControl();
    void createPulseCalibControl();
	void createResonantScanControl();
    void createGalvoScanControl();
    void createZaberStageControl();

private slots: /////////////////////////////////////////////////////////////////////////////////////////
    // FLIm PMT Gain Control
    void applyPmtGainVoltage(bool);
    void changePmtGainVoltage(const QString &);
		
    // FLIm Calibration Control
    void connectPX14Digitizer(bool);
	void setPX14PreTrigger(const QString &);
	void setPX14TriggerDelay(const QString &);
	void setPX14DcOffset(int);
    void createPulseCalibDlg();
    void deletePulseCalibDlg();

	// Resonant Scanner
	void connectResonantScanner(bool);
	void applyResonantScannerVoltage(bool);
	void setResonantScannerVoltage(double);

    // Galvano Mirror
    void connectGalvanoMirror(bool);
    void changeGalvoScanVoltage(const QString &);
    void changeGalvoScanVoltageOffset(const QString &);

    // Zaber Stage Control
    void connectZaberStage(bool);
    void moveRelative();
    void setTargetSpeed(const QString &);
    void changeZaberPullbackLength(const QString &);
    void home();
    void stop();
    void stageScan(int, double);
    void getCurrentPosition();

signals: ////////////////////////////////////////////////////////////////////////////////////////////////
    void startStageScan(int, double);
    void monitoring();
	
// Variables ////////////////////////////////////////////
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    // PMT Gain Control & Monitor
    PmtGainControl* m_pPmtGainControl;
	PmtGainMonitor* m_pPmtGainMonitor;
	
	// CRS Scanner Control
	ResonantScan* m_pResonantScan;

    // Galvo Scanner Control
    GalvoScan* m_pGalvoScan;

    // Zaber Stage Control;
    ZaberStage* m_pZaberStage;
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

    // Layout
    QVBoxLayout *m_pVBoxLayout;

	// Pulse View Layout
	QVBoxLayout *m_pVBoxLayout_PulseControl;
    QGroupBox *m_pGroupBox_PulseControl;

    // Widgets for Gain control
    QCheckBox *m_pCheckBox_PmtGainControl;
    QLineEdit *m_pLineEdit_PmtGainVoltage;
    QLabel *m_pLabel_PmtGainVoltage;
		
    // Widgets for PX14 Calibration
    QCheckBox *m_pCheckBox_PX14DigitizerControl;
	QLabel *m_pLabel_PX14PreTrigger;
	QLineEdit *m_pLineEdit_PX14PreTrigger;
	QLabel *m_pLabel_PX14TriggerDelay;
	QLineEdit *m_pLineEdit_PX14TriggerDelay;
	QLabel *m_pLabel_PX14DcOffset;
	QSlider *m_pSlider_PX14DcOffset;
    QPushButton *m_pPushButton_PulseCalibration;
    PulseCalibDlg *m_pPulseCalibDlg;

	// Widgets for resonant scanner control
	QCheckBox *m_pCheckBox_ResonantScannerControl;
	QPushButton *m_pToggledButton_ResonantScanner;
	QDoubleSpinBox *m_pDoubleSpinBox_ResonantScanner;
	QLabel *m_pLabel_ResonantScanner;

    // Widgets for galvano mirror control
    QCheckBox *m_pCheckBox_GalvoScanControl;
    QLineEdit *m_pLineEdit_PeakToPeakVoltage;
    QLineEdit *m_pLineEdit_OffsetVoltage;
    QLabel *m_pLabel_ScanVoltage;
    QLabel *m_pLabel_ScanPlusMinus;
    QLabel *m_pLabel_GalvanoVoltage;

    // Widgets for Zaber stage control
    QCheckBox *m_pCheckBox_ZaberStageControl;
    QPushButton *m_pPushButton_SetStageNumber;
    QPushButton *m_pPushButton_SetTargetSpeed;
    QPushButton *m_pPushButton_MoveRelative;
    QPushButton *m_pPushButton_Home;
    QPushButton *m_pPushButton_Stop;
    QComboBox *m_pComboBox_StageNumber;
    QLineEdit *m_pLineEdit_TargetSpeed;
    QLineEdit *m_pLineEdit_TravelLength;
    QLabel *m_pLabel_TargetSpeed;
    QLabel *m_pLabel_TravelLength;
};

#endif // QDEVICECONTROLTAB_H
