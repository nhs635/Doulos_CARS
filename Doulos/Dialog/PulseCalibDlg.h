#ifndef FLIMCALIBDLG_H
#define FLIMCALIBDLG_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/Viewer/QScope.h>
#include <Doulos/Viewer/QImageView.h>

#include <Common/array.h>
#include <Common/callback.h>

#include <ipps.h>
#include <ippi.h>
#include <ippvm.h>

class QDeviceControlTab;
class DataProcess;


class PulseCalibDlg : public QDialog
{
	Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
	explicit PulseCalibDlg(QWidget *parent = nullptr);
	virtual ~PulseCalibDlg();

// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *e);

private:
	void createPulseView();
	void createCalibWidgets();

public:
    inline QImageView* getPulseImageView() const { return m_pImageView_PulseImage; }

public slots : // widgets
	void drawRoiPulse(DataProcess*, int);

	void showWindow(bool);
	void splineView(bool);
	
	void captureBackground();
	void captureBackground(const QString &);

	void resetChStart0(double);
	void resetChStart1(double);
	void resetChStart2(double);
	void resetChStart3(double);
    void resetChStart4(double);

signals:
	void plotRoiPulse(DataProcess*, int);

public:
	// Callbacks
	callback<const char*> SendStatusMessage;

	// Variables ////////////////////////////////////////////
private:
	Configuration* m_pConfig;
	QDeviceControlTab* m_pDeviceControlTab;
	DataProcess* m_pDataProc;

private:
	// Layout
	QVBoxLayout *m_pVBoxLayout;

	// Widgets for pulse view
    QImageView *m_pImageView_PulseImage;
	QScope *m_pScope_PulseView;

	QCheckBox *m_pCheckBox_ShowWindow;
	QCheckBox *m_pCheckBox_SplineView;

	// Widgets for pulse calibration widgets
	QPushButton *m_pPushButton_CaptureBackground;
	QLineEdit *m_pLineEdit_Background;

	QLabel *m_pLabel_ChStart;
    QLabel *m_pLabel_Ch[5];
    QMySpinBox *m_pSpinBox_ChStart[5];
	QLabel *m_pLabel_NanoSec;
};

#endif // FLIMCALIBDLG_H
