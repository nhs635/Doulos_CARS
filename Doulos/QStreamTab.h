#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>

#include <Common/array.h>
#include <Common/SyncObject.h>

#include <iostream>
#include <thread>


class MainWindow;
class QOperationTab;
class QDeviceControlTab;
class QVisualizationTab;

class ThreadManager;
class DataProcess;


class QStreamTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QStreamTab(QWidget *parent = nullptr);
	virtual ~QStreamTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline QOperationTab* getOperationTab() const { return m_pOperationTab; }
    inline QDeviceControlTab* getDeviceControlTab() const { return m_pDeviceControlTab; }
	inline QVisualizationTab* getVisualizationTab() const { return m_pVisualizationTab; }
    inline QCheckBox* getCRSNonlinComp() const { return m_pCheckBox_CRSNonlinearityComp; }
#ifndef RAW_PULSE_WRITE
    inline QCheckBox* getImageStitchingCheckBox() const { return m_pCheckBox_StitchingMode; }
#endif
	
public:
    void setWidgetsText();
	void setAveragingWidgets(bool enabled);
	void resetImagingMode();
    void stageMoving();

private:		
// Set thread callback objects
    void setDataAcquisitionCallback();
    void setDataProcessingCallback();
    void setVisualizationCallback();

private slots:
    void onTimerMonitoring();
	void changeAccumulationFrame(const QString &);
	void changeAveragingFrame(const QString &);
    void processMessage(QString, bool);
	void setSyncComp(int);
    void changeCRSNonlinearityComp(bool);    
#ifndef RAW_PULSE_WRITE
    void enableStitchingMode(bool);
    void changeStitchingXStep(const QString &);
    void changeStitchingYStep(const QString &);
//    void changeStitchingMisSyncPos(const QString &);
#endif

signals:
    void sendStatusMessage(QString, bool);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;

    QOperationTab* m_pOperationTab;
    QDeviceControlTab* m_pDeviceControlTab;
    QVisualizationTab* m_pVisualizationTab;

	// Message Window
	QListWidget *m_pListWidget_MsgWnd;

public:
	// Image buffer
	np::FloatArray2 m_pTempImage0;
	np::FloatArray2 m_pTempImage; 

	// Image acquisition
    int m_nAcquiredFrames;

    // CRS nonlinearity compensation idx
    np::FloatArray2 m_pCRSCompIdx;

    // Stitching flag
    bool m_bIsStageTransition;
    int m_nImageCount;

public:
    // Thread manager objects
    ThreadManager* m_pThreadDataProcess;
    ThreadManager* m_pThreadVisualization;

private:
    // Thread synchronization objects
    SyncObject<uint16_t> m_syncDataProcessing;
    SyncObject<float> m_syncDataVisualization;

    // Monitoring timer
    QTimer *m_pTimer_Monitoring;

private:
    // Layout
    QHBoxLayout *m_pHBoxLayout;

    // Group Boxes
    QGroupBox *m_pGroupBox_OperationTab;
	QGroupBox *m_pGroupBox_VisualizationTab;
    QGroupBox *m_pGroupBox_DeviceControlTab;
	
	// Image averaging & accumulation mode 
	QLabel *m_pLabel_Averaging;
	QLineEdit *m_pLineEdit_Averaging;
	QLabel *m_pLabel_Accumulation;
	QLineEdit *m_pLineEdit_Accumulation;
	
	QLabel *m_pLabel_AcquisitionStatusMsg;
	
	// Sync compensation
	QLabel *m_pLabel_SyncComp;
	QSlider *m_pSlider_SyncComp;

    // CRS nonlinearity compensation
    QCheckBox *m_pCheckBox_CRSNonlinearityComp;

#ifndef RAW_PULSE_WRITE
    // Stitching mode
    QCheckBox *m_pCheckBox_StitchingMode;
    QLineEdit *m_pLineEdit_XStep;
    QLineEdit *m_pLineEdit_YStep;
    QLabel *m_pLabel_XStep;
    QLabel *m_pLabel_YStep;
//    QLineEdit *m_pLineEdit_MisSyncPos;
//    QLabel *m_pLabel_MisSyncPos;
#endif
};

#endif // QSTREAMTAB_H
