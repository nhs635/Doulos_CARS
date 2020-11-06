#ifndef QVISUALIZATIONTAB_H
#define QVISUALIZATIONTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>

#include <Common/medfilt.h>
#include <Common/ImageObject.h>

#include <iostream>
#include <vector>

class QStreamTab;
class QImageView;


class QVisualizationTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QVisualizationTab(QWidget *parent = nullptr);
    virtual ~QVisualizationTab();

// Methods //////////////////////////////////////////////
public:
    inline QGridLayout* getLayout() const { return m_pGridLayout; }
	inline QGroupBox* getVisualizationWidgetsBox() const { return m_pGroupBox_VisualizationWidgets; }
	//inline QGroupBox* getDataVisualizationBox() const { return m_pGroupBox_DataVisualization; }
	inline QGroupBox* getAveragingBox() const { return m_pGroupBox_Averaging; }
	inline QImageView* getImageView(int mode) const { return m_pImageView_Image[mode]; }
	inline medfilt* getMedfilt() const { return m_pMedfilt; }

private:
    void createDataVisualizationOptionTab();

public:
	void setImgViewVisPixelPos(bool);
	void getPixelPos(int* x, int* y);
    void setRelevantWidgets(bool enabled);

public slots:
    void visualizeImage();

private slots:
    void setSingleModeVisualization(bool);
	void changeImageMode(int);
    void setRGBImageVisualization(bool);
    void setCh1ImageMode(int);
    void setCh2ImageMode(int);
    void setCh3ImageMode(int);
    void setCh4ImageMode(int);
    void adjustImageContrast();

signals:
    void drawImage();
    void plotCh1Image(uint8_t*);
    void plotCh2Image(uint8_t*);
    void plotCh3Image(uint8_t*);
    void plotCh4Image(uint8_t*);
    void plotRGBImage(uint8_t*);

// Variables ////////////////////////////////////////////
private:
	QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

public:
    // Visualization buffers
    std::vector<np::FloatArray2> m_vecVisImage;

    // CRS nonlinearity compensation idx
    np::FloatArray2 m_pCRSCompIdx;

private:
    // Image visualization buffers
    ImageObject *m_pImgObj[5];

	medfilt* m_pMedfilt;

private:
    // Layout
    QGridLayout *m_pGridLayout;

    // Image viewer widgets
    QImageView *m_pImageView_Image[5];

	// Visualization widgets groupbox
	QGroupBox *m_pGroupBox_VisualizationWidgets;
	QGroupBox *m_pGroupBox_Averaging;

	// Data visualization option widgets
	QGroupBox *m_pGroupBox_DataVisualization;

	QCheckBox *m_pCheckBox_SingleModeVisualization;
	QComboBox *m_pComboBox_SingleModeVisualization;

    QCheckBox *m_pCheckBox_RGBImage;

    QComboBox *m_pComboBox_ModeName[4];
    QLineEdit *m_pLineEdit_ContrastMax[4];
    QLineEdit *m_pLineEdit_ContrastMin[4];
    QImageView *m_pImageView_ModeColorbar[4];
};

#endif // QVISUALIZATIONTAB_H
