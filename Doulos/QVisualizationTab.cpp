
#include "QVisualizationTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>

#include <Doulos/Dialog/PulseCalibDlg.h>
#include <Doulos/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
//#include <DataAcquisition/DataProcess/DataProcess.h>

//#include <Common/basic_functions.h>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#define SHG   0
#define TPFE  1
#define CARS  2
#define RCM   3

static int color_table_index[4] = { SHG_COLORTABLE, TPFE_COLORTABLE, CARS_COLORTABLE, RCM_COLORTABLE };
static const char* mode_name[4] = { "SHG", "TPFE", "CARS", "RCM" };


QVisualizationTab::QVisualizationTab(QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pMedfilt(nullptr)
{
	// Set image objects as nullptr
	for (int i = 0; i < 4; i++) m_pImgObj[i] = nullptr;

    // Set configuration objects	
	m_pStreamTab = (QStreamTab*)parent;
	m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
	
    // Create image view
	for (int i = 0; i < 4; i++)
        m_pImageView_Image[i] = new QImageView(ColorTable::colortable(color_table_index[m_pConfig->channelImageMode[i]]), m_pConfig->nPixels, m_pConfig->nLines);
    m_pImageView_Image[4] = new QImageView(ColorTable::gray, m_pConfig->nPixels, m_pConfig->nLines, true);

    for (int i = 0; i < 5; i++)
	{
		m_pImageView_Image[i]->setMinimumSize(460, 460);
		m_pImageView_Image[i]->setSquare(true);
		m_pImageView_Image[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		m_pImageView_Image[i]->setHorizontalLine(1, m_pConfig->nLines - GALVO_FLYING_BACK);
		m_pImageView_Image[i]->setClickedMouseCallback([&](int x, int y) {
			for (int j = 0; j < 4; j++)
			{
				m_pImageView_Image[j]->getRender()->m_pixelPos[0] = x;
				m_pImageView_Image[j]->getRender()->m_pixelPos[1] = y;
				m_pImageView_Image[j]->getRender()->update();
			}
		});			
		m_pImageView_Image[i]->setMovedMouseCallback([&, i](QPoint& p) { 
			m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("[%1] (%2, %3) | (%4)")
                .arg(mode_name[m_pConfig->channelImageMode[i]]).arg(p.x(), 4).arg(p.y(), 4)
                .arg(m_vecVisImage.at(m_pConfig->channelImageMode[i]).at(p.x(), p.y()), 4, 'f', 3)); });
	} 
    m_pImageView_Image[4]->hide();
	
	// Create visualization buffers
	for (int i = 0; i < 4; i++)
	{
		np::FloatArray2 image = np::FloatArray2(m_pConfig->nPixels, m_pConfig->nLines);
		memset(image.raw_ptr(), 0, sizeof(float) * image.length());
		m_vecVisImage.push_back(image);
	}

	// Create image visualization buffers
	ColorTable temp_ctable;
	for (int i = 0; i < 4; i++)
        m_pImgObj[i] = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(color_table_index[m_pConfig->channelImageMode[i]]));
    m_pImgObj[4] = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(0));

	// Create med filt
	m_pMedfilt = new medfilt(m_pConfig->nPixels, m_pConfig->nLines, 3, 3);

    // Create data visualization option tab
    createDataVisualizationOptionTab();

    // Set layout
	m_pGroupBox_VisualizationWidgets = new QGroupBox;
	m_pGroupBox_VisualizationWidgets->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pGroupBox_VisualizationWidgets->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_VisualizationWidgets->setFixedWidth(332);

	QVBoxLayout *pVBoxLayout = new QVBoxLayout;
	pVBoxLayout->setSpacing(1);

	pVBoxLayout->addWidget(m_pGroupBox_DataVisualization);
	
	m_pGroupBox_Averaging = new QGroupBox;
	m_pGroupBox_Averaging->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pGroupBox_Averaging->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pVBoxLayout->addWidget(m_pGroupBox_Averaging);

	m_pGroupBox_VisualizationWidgets->setLayout(pVBoxLayout);
	
	// Create layout
	m_pGridLayout = new QGridLayout;
	m_pGridLayout->setSpacing(1);

	//m_pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
	m_pGridLayout->addWidget(m_pImageView_Image[0], 0, 1);
	m_pGridLayout->addWidget(m_pImageView_Image[1], 0, 2);
	//m_pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 3);

	//m_pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	m_pGridLayout->addWidget(m_pImageView_Image[2], 1, 1);
	m_pGridLayout->addWidget(m_pImageView_Image[3], 1, 2);
	//m_pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 3);

    m_pGridLayout->addWidget(m_pImageView_Image[4], 2, 0);

	setLayout(m_pGridLayout);


    // Connect signal and slot
    connect(this, SIGNAL(drawImage()), this, SLOT(visualizeImage()));
    connect(this, SIGNAL(plotCh1Image(uint8_t*)), m_pImageView_Image[0], SLOT(drawImage(uint8_t*)));
    connect(this, SIGNAL(plotCh2Image(uint8_t*)), m_pImageView_Image[1], SLOT(drawImage(uint8_t*)));
    connect(this, SIGNAL(plotCh3Image(uint8_t*)), m_pImageView_Image[2], SLOT(drawImage(uint8_t*)));
    connect(this, SIGNAL(plotCh4Image(uint8_t*)), m_pImageView_Image[3], SLOT(drawImage(uint8_t*)));
    connect(this, SIGNAL(plotRGBImage(uint8_t*)), m_pImageView_Image[4], SLOT(drawRgbImage(uint8_t*)));
}

QVisualizationTab::~QVisualizationTab()
{
	for (int i = 0; i < 4; i++)
	    if (m_pImgObj[i]) delete m_pImgObj[i];
	
    if (m_pMedfilt) delete m_pMedfilt;
}


void QVisualizationTab::createDataVisualizationOptionTab()
{
    // Create widgets for data visualization option tab
    m_pGroupBox_DataVisualization = new QGroupBox;
	m_pGroupBox_DataVisualization->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pGroupBox_DataVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    QGridLayout *pGridLayout_DataVisualization = new QGridLayout;
	pGridLayout_DataVisualization->setSpacing(3);

    // Create widgets for single mode visualization
	m_pCheckBox_SingleModeVisualization = new QCheckBox(this);
	m_pCheckBox_SingleModeVisualization->setText("Single Mode Visualization   ");
	
	m_pComboBox_SingleModeVisualization = new QComboBox(this);
	for (int i = 0; i < 4; i++)
        m_pComboBox_SingleModeVisualization->addItem(QString("Ch %1").arg(i + 1));
	m_pComboBox_SingleModeVisualization->setDisabled(true);

    m_pCheckBox_RGBImage = new QCheckBox(this);
    m_pCheckBox_RGBImage->setText("RGB Image   ");
    m_pCheckBox_RGBImage->setDisabled(true);
	
    // Create line edit widgets for image contrast adjustment
	for (int i = 0; i < 4; i++)
	{
		m_pLineEdit_ContrastMax[i] = new QLineEdit(this);
		m_pLineEdit_ContrastMax[i]->setFixedWidth(35);
        m_pLineEdit_ContrastMax[i]->setText(QString::number(m_pConfig->imageContrastRange[i].max, 'f', 1));
		m_pLineEdit_ContrastMax[i]->setAlignment(Qt::AlignCenter);
		m_pLineEdit_ContrastMin[i] = new QLineEdit(this);
		m_pLineEdit_ContrastMin[i]->setFixedWidth(35);
        m_pLineEdit_ContrastMin[i]->setText(QString::number(m_pConfig->imageContrastRange[i].min, 'f', 1));
		m_pLineEdit_ContrastMin[i]->setAlignment(Qt::AlignCenter);
	}

    // Create color bar for data visualization
    uint8_t color[256];
    for (int i = 0; i < 256; i++)
        color[i] = i;

	for (int i = 0; i < 4; i++)
	{
        m_pImageView_ModeColorbar[i] = new QImageView(ColorTable::colortable(color_table_index[m_pConfig->channelImageMode[i]]), 256, 1);
		m_pImageView_ModeColorbar[i]->setFixedHeight(15);
		m_pImageView_ModeColorbar[i]->setMinimumWidth(150);
		m_pImageView_ModeColorbar[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_pImageView_ModeColorbar[i]->drawImage(color);
        m_pComboBox_ModeName[i] = new QComboBox(this); //mode_name[i], this);
        for (int j = 0; j < 4; j++)
            m_pComboBox_ModeName[i]->addItem(mode_name[j]);
        m_pComboBox_ModeName[i]->setCurrentIndex(m_pConfig->channelImageMode[i]);
        m_pComboBox_ModeName[i]->setFixedWidth(60);
	}

    // Set layout
    QHBoxLayout *pHBoxLayout_SingleModeVisualization = new QHBoxLayout;
    pHBoxLayout_SingleModeVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_SingleModeVisualization->addWidget(m_pCheckBox_RGBImage);
	pHBoxLayout_SingleModeVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_SingleModeVisualization->addWidget(m_pCheckBox_SingleModeVisualization);
    pHBoxLayout_SingleModeVisualization->addWidget(m_pComboBox_SingleModeVisualization);

	
    QGridLayout *pGridLayout_ContrastAdjustment = new QGridLayout;
	for (int i = 0; i < 4; i++)
	{
        pGridLayout_ContrastAdjustment->addWidget(m_pComboBox_ModeName[i], i, 0);
		pGridLayout_ContrastAdjustment->addWidget(m_pLineEdit_ContrastMin[i], i, 1);
		pGridLayout_ContrastAdjustment->addWidget(m_pImageView_ModeColorbar[i], i, 2);
		pGridLayout_ContrastAdjustment->addWidget(m_pLineEdit_ContrastMax[i], i, 3);
	}

	pGridLayout_DataVisualization->addItem(pHBoxLayout_SingleModeVisualization, 0, 0);
	pGridLayout_DataVisualization->addItem(pGridLayout_ContrastAdjustment, 1, 0);

    m_pGroupBox_DataVisualization->setLayout(pGridLayout_DataVisualization);

    // Connect signal and slot
    connect(m_pCheckBox_SingleModeVisualization, SIGNAL(toggled(bool)), this, SLOT(setSingleModeVisualization(bool)));
    connect(m_pComboBox_SingleModeVisualization, SIGNAL(currentIndexChanged(int)), this, SLOT(changeImageMode(int)));    
    connect(m_pCheckBox_RGBImage, SIGNAL(toggled(bool)), this, SLOT(setRGBImageVisualization(bool)));
    connect(m_pComboBox_ModeName[0], SIGNAL(currentIndexChanged(int)), this, SLOT(setCh1ImageMode(int)));
    connect(m_pComboBox_ModeName[1], SIGNAL(currentIndexChanged(int)), this, SLOT(setCh2ImageMode(int)));
    connect(m_pComboBox_ModeName[2], SIGNAL(currentIndexChanged(int)), this, SLOT(setCh3ImageMode(int)));
    connect(m_pComboBox_ModeName[3], SIGNAL(currentIndexChanged(int)), this, SLOT(setCh4ImageMode(int)));
	for (int i = 0; i < 4; i++)
    {
		connect(m_pLineEdit_ContrastMax[i], SIGNAL(textEdited(const QString &)), this, SLOT(adjustImageContrast()));
		connect(m_pLineEdit_ContrastMin[i], SIGNAL(textEdited(const QString &)), this, SLOT(adjustImageContrast()));
	}
}

void QVisualizationTab::setImgViewVisPixelPos(bool vis)
{
	for (int i = 0; i < 4; i++)
		m_pImageView_Image[i]->getRender()->setVisPixelPos(vis);
}

void QVisualizationTab::getPixelPos(int* x, int* y)
{
	*x = m_pImageView_Image[0]->getRender()->m_pixelPos[0];
	*y = m_pImageView_Image[0]->getRender()->m_pixelPos[1];
}

void QVisualizationTab::setRelevantWidgets(bool enabled)
{
    m_pCheckBox_SingleModeVisualization->setEnabled(enabled);
    m_pComboBox_SingleModeVisualization->setEnabled(enabled);
    m_pCheckBox_RGBImage->setEnabled(enabled);
    if (!m_pCheckBox_SingleModeVisualization->isChecked()) m_pCheckBox_RGBImage->setEnabled(false);
    if (m_pCheckBox_RGBImage->isChecked()) m_pComboBox_SingleModeVisualization->setEnabled(false);

    for (int i = 0; i < 4; i++)
    {
        m_pComboBox_ModeName[i]->setEnabled(enabled);
        m_pLineEdit_ContrastMax[i]->setEnabled(enabled);
        m_pLineEdit_ContrastMin[i]->setEnabled(enabled);
        m_pImageView_ModeColorbar[i]->setEnabled(enabled);
    }
}


void QVisualizationTab::visualizeImage()
{
    IppiSize roi_image = { m_pConfig->nPixels, m_pConfig->nLines };

    // Image Scaling
	for (int i = 0; i < 4; i++)
	{
        float* scanImage = m_vecVisImage.at(i).raw_ptr();

        // Scaling
        ippiScale_32f8u_C1R(scanImage, sizeof(float) * roi_image.width, m_pImgObj[i]->arr.raw_ptr(), sizeof(uint8_t) * roi_image.width,
            roi_image, m_pConfig->imageContrastRange[i].min, m_pConfig->imageContrastRange[i].max);
#ifdef MED_FILT
		(*m_pMedfilt)(m_pImgObj[i]->arr.raw_ptr());
#endif
	}

    // Visualization signal emit
	if (!m_pCheckBox_SingleModeVisualization->isChecked())
	{
        emit plotCh1Image(m_pImgObj[0]->qindeximg.bits());
        emit plotCh2Image(m_pImgObj[1]->qindeximg.bits());
        emit plotCh3Image(m_pImgObj[2]->qindeximg.bits());
        emit plotCh4Image(m_pImgObj[3]->qindeximg.bits());
	}
	else
    {
        if (!m_pCheckBox_RGBImage->isChecked())
        {
            int mode = m_pComboBox_SingleModeVisualization->currentIndex();

            switch (mode)
            {
            case 0:
                emit plotCh1Image(m_pImgObj[0]->qindeximg.bits());
                break;
            case 1:
                emit plotCh2Image(m_pImgObj[1]->qindeximg.bits());
                break;
            case 2:
                emit plotCh3Image(m_pImgObj[2]->qindeximg.bits());
                break;
            case 3:
                emit plotCh4Image(m_pImgObj[3]->qindeximg.bits());
                break;
            default:
                break;
            }
        }
        else
        {
            m_pImgObj[4]->convertRgb();

            IppiSize img_size = { m_pImgObj[4]->arr.size(0), m_pImgObj[4]->arr.size(1) };

            int cars = 0, tpfe = 0, shg = 0;
            for (int i = 0; i < 4; i++)
            {
                if (m_pConfig->channelImageMode[i] == CARS)
                    cars = i;
                if (m_pConfig->channelImageMode[i] == TPFE)
                    tpfe = i;
                if (m_pConfig->channelImageMode[i] == SHG)
                    shg = i;
            }

            ippiCopy_8u_C1C3R(m_pImgObj[cars]->arr, img_size.width,
                              m_pImgObj[4]->qrgbimg.bits() + 0, 3 * img_size.width, img_size);
            ippiCopy_8u_C1C3R(m_pImgObj[tpfe]->arr, img_size.width,
                              m_pImgObj[4]->qrgbimg.bits() + 1, 3 * img_size.width, img_size);
            ippiCopy_8u_C1C3R(m_pImgObj[shg]->arr, img_size.width,
                              m_pImgObj[4]->qrgbimg.bits() + 2, 3 * img_size.width, img_size);

            emit plotRGBImage(m_pImgObj[4]->qrgbimg.bits());
        }
	}
}


void QVisualizationTab::setSingleModeVisualization(bool toggled)
{
	if (toggled)
	{
		int mode = m_pComboBox_SingleModeVisualization->currentIndex();

		m_pImageView_Image[0]->hide();
		m_pImageView_Image[1]->hide();
		m_pImageView_Image[2]->hide();
		m_pImageView_Image[3]->hide();
        m_pImageView_Image[4]->hide();

        if (!m_pCheckBox_RGBImage->isChecked())
            m_pImageView_Image[mode]->show();
        else
            m_pImageView_Image[4]->show();
		
        m_pComboBox_SingleModeVisualization->setEnabled(!m_pCheckBox_RGBImage->isChecked());
        m_pCheckBox_RGBImage->setEnabled(true);
	}
	else
    {
		m_pImageView_Image[0]->show();
		m_pImageView_Image[1]->show();
		m_pImageView_Image[2]->show();
		m_pImageView_Image[3]->show();
        m_pImageView_Image[4]->hide();

        m_pComboBox_SingleModeVisualization->setDisabled(true);
        m_pCheckBox_RGBImage->setDisabled(true);
	}

	emit drawImage();
}

void QVisualizationTab::changeImageMode(int mode)
{
	m_pImageView_Image[0]->hide();
	m_pImageView_Image[1]->hide();
	m_pImageView_Image[2]->hide();
	m_pImageView_Image[3]->hide();

	m_pImageView_Image[mode]->show();

	emit drawImage();
}

void QVisualizationTab::setRGBImageVisualization(bool toggled)
{
    if (toggled)
    {
        m_pImageView_Image[0]->hide();
        m_pImageView_Image[1]->hide();
        m_pImageView_Image[2]->hide();
        m_pImageView_Image[3]->hide();
        m_pImageView_Image[4]->hide();

        m_pImageView_Image[4]->show();

        m_pComboBox_SingleModeVisualization->setDisabled(true);
    }
    else
    {
        int mode = m_pComboBox_SingleModeVisualization->currentIndex();

        m_pImageView_Image[0]->hide();
        m_pImageView_Image[1]->hide();
        m_pImageView_Image[2]->hide();
        m_pImageView_Image[3]->hide();
        m_pImageView_Image[4]->hide();

        m_pImageView_Image[mode]->show();

        m_pComboBox_SingleModeVisualization->setEnabled(true);
    }

    emit drawImage();
}

void QVisualizationTab::setCh1ImageMode(int mode)
{
    m_pConfig->channelImageMode[0] = mode;

    // Colormap reset
    m_pImageView_Image[0]->resetColormap(ColorTable::colortable(color_table_index[mode]));
    m_pImageView_ModeColorbar[0]->resetColormap(ColorTable::colortable(color_table_index[mode]));

    ColorTable temp_ctable;
    delete m_pImgObj[0];
    m_pImgObj[0] = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(color_table_index[mode]));

    // Contrast reset
    m_pLineEdit_ContrastMax[0]->setText(QString::number(m_pConfig->imageContrastRange[mode].max, 'f', 1));
    m_pLineEdit_ContrastMin[0]->setText(QString::number(m_pConfig->imageContrastRange[mode].min, 'f', 1));

    // Update
    m_pImageView_Image[0]->getRender()->update();
}

void QVisualizationTab::setCh2ImageMode(int mode)
{
    m_pConfig->channelImageMode[1] = mode;

    // Colormap reset
    m_pImageView_Image[1]->resetColormap(ColorTable::colortable(color_table_index[mode]));
    m_pImageView_ModeColorbar[1]->resetColormap(ColorTable::colortable(color_table_index[mode]));

    ColorTable temp_ctable;
    delete m_pImgObj[1];
    m_pImgObj[1] = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(color_table_index[mode]));

    // Contrast reset
    m_pLineEdit_ContrastMax[1]->setText(QString::number(m_pConfig->imageContrastRange[mode].max, 'f', 1));
    m_pLineEdit_ContrastMin[1]->setText(QString::number(m_pConfig->imageContrastRange[mode].min, 'f', 1));

    // Update
    m_pImageView_Image[1]->getRender()->update();
}

void QVisualizationTab::setCh3ImageMode(int mode)
{
    m_pConfig->channelImageMode[2] = mode;

    // Colormap reset
    m_pImageView_Image[2]->resetColormap(ColorTable::colortable(color_table_index[mode]));
    m_pImageView_ModeColorbar[2]->resetColormap(ColorTable::colortable(color_table_index[mode]));

    ColorTable temp_ctable;
    delete m_pImgObj[2];
    m_pImgObj[2] = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(color_table_index[mode]));

    // Contrast reset
    m_pLineEdit_ContrastMax[2]->setText(QString::number(m_pConfig->imageContrastRange[mode].max, 'f', 1));
    m_pLineEdit_ContrastMin[2]->setText(QString::number(m_pConfig->imageContrastRange[mode].min, 'f', 1));

    // Update
    m_pImageView_Image[2]->getRender()->update();
}

void QVisualizationTab::setCh4ImageMode(int mode)
{
    m_pConfig->channelImageMode[3] = mode;

    // Colormap reset
    m_pImageView_Image[3]->resetColormap(ColorTable::colortable(color_table_index[mode]));
    m_pImageView_ModeColorbar[3]->resetColormap(ColorTable::colortable(color_table_index[mode]));

    ColorTable temp_ctable;
    delete m_pImgObj[3];
    m_pImgObj[3] = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(color_table_index[mode]));

    // Contrast reset
    m_pLineEdit_ContrastMax[3]->setText(QString::number(m_pConfig->imageContrastRange[mode].max, 'f', 1));
    m_pLineEdit_ContrastMin[3]->setText(QString::number(m_pConfig->imageContrastRange[mode].min, 'f', 1));

    // Update
    m_pImageView_Image[3]->getRender()->update();
}

void QVisualizationTab::adjustImageContrast()
{
	for (int i = 0; i < 4; i++)
	{
        m_pConfig->imageContrastRange[i].min = m_pLineEdit_ContrastMin[i]->text().toFloat();
        m_pConfig->imageContrastRange[i].max = m_pLineEdit_ContrastMax[i]->text().toFloat();
	}

    emit drawImage();
}
