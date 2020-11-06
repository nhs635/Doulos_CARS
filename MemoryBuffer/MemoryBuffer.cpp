
#include "MemoryBuffer.h"

#include <Doulos/QOperationTab.h>
#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Viewer/QImageView.h>

#include <Common/ImageObject.h>
#include <Common/medfilt.h>

#include <iostream>
#include <deque>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>

static int color_table_index[4] = { SHG_COLORTABLE, TPFE_COLORTABLE, CARS_COLORTABLE, RCM_COLORTABLE };
static const char* mode_name[4] = { "SHG", "TPFE", "CARS", "RCM" };


MemoryBuffer::MemoryBuffer(QObject *parent) :
    QObject(parent),
    m_bIsAllocatedWritingBuffer(false),
#ifdef RAW_PULSE_WRITE
    m_bIsImageStartPoint(false), m_bIsRecordingPulse(false), m_nRecordedFrames(0),
#endif
    m_bIsFirstRecImage(false), m_bIsRecordingImage(false), m_nRecordedImages(0),
    m_bIsSaved(false)
{
    m_pOperationTab = (QOperationTab*)parent;
    m_pConfig = m_pOperationTab->getStreamTab()->getMainWnd()->m_pConfiguration;
    m_pDeviceControlTab = m_pOperationTab->getStreamTab()->getDeviceControlTab();
}

MemoryBuffer::~MemoryBuffer()
{
    deallocateWritingBuffer();
}


void MemoryBuffer::allocateWritingBuffer()
{
    if (!m_bIsAllocatedWritingBuffer)
    {
        for (int i = 0; i < WRITING_IMAGE_SIZE; i++)
        {
            float* pImage = new float[4 * m_pConfig->imageSize];
            memset(pImage, 0, sizeof(float) * 4 * m_pConfig->imageSize);
            m_queueWritingImage.push(pImage);
        }

        char msg[256];
        sprintf(msg, "Writing images are successfully allocated. [Total buffer size: %zd MBytes]",
            WRITING_IMAGE_SIZE * 4 * m_pConfig->imageSize * sizeof(float) / 1024 / 1024);
        SendStatusMessage(msg, false);

#ifdef RAW_PULSE_WRITE
        for (int i = 0; i < WRITING_BUFFER_SIZE; i++)
        {
            uint16_t *writingBuffer = new uint16_t[m_pConfig->bufferSize];
            memset(writingBuffer, 0, m_pConfig->bufferSize * sizeof(uint16_t));
            m_queueWritingBuffer.push(writingBuffer);
        }

        sprintf(msg, "Writing buffers are successfully allocated. [Total buffer size: %zd MBytes]",
            WRITING_BUFFER_SIZE * m_pConfig->bufferSize * sizeof(uint16_t) / 1024 / 1024);
        SendStatusMessage(msg, false);
#endif
        SendStatusMessage("Now, recording process is available!", false);

        m_bIsAllocatedWritingBuffer = true;
    }

    emit finishedBufferAllocation();
}

void MemoryBuffer::deallocateWritingBuffer()
{
    if (!m_queueWritingImage.empty())
    {
        float* image = m_queueWritingImage.front();
        if (image)
        {
            m_queueWritingImage.pop();
            delete[] image;
        }
    }

#ifdef RAW_PULSE_WRITE
    if (!m_queueWritingBuffer.empty())
    {
        uint16_t* buffer = m_queueWritingBuffer.front();
        if (buffer)
        {
            m_queueWritingBuffer.pop();
            delete[] buffer;
        }
    }
#endif

    SendStatusMessage("Writing buffers are successfully disallocated.", false);
}


bool MemoryBuffer::startRecording()
{
    // Check if the previous recorded data is saved
#ifndef RAW_PULSE_WRITE
    if (!m_bIsSaved && (m_nRecordedImages != 0))
#else
    if (!m_bIsSaved && (m_nRecordedFrames != 0) && (m_nRecordedImages != 0))
#endif
    {
        QMessageBox MsgBox;
        MsgBox.setWindowTitle("Question");
        MsgBox.setIcon(QMessageBox::Question);
        MsgBox.setText("The previous recorded data is not saved.\nWhat would you like to do with this data?");
        MsgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Discard | QMessageBox::Cancel);
        MsgBox.setDefaultButton(QMessageBox::Cancel);

        int resp = MsgBox.exec();
        switch (resp)
        {
        case QMessageBox::Ignore:
            break;
        case QMessageBox::Discard:
            m_bIsSaved = true;
#ifndef RAW_PULSE_WRITE
            m_nRecordedImages = 0;
#else
            m_nRecordedFrames = 0;
#endif
            return false;
        case QMessageBox::Cancel:
            return false;
        default:
            break;
        }
    }

    // Start Recording
    SendStatusMessage("Data recording is started.", false);
	m_bIsRecordingImage = true;
#ifndef RAW_PULSE_WRITE
    m_nRecordedImages = 0;
#else
	m_bIsRecordingPulse = true;
    m_nRecordedFrames = 0;
	m_bIsFirstRecImage = true;
#endif
    m_bIsSaved = false;

    // Thread for buffering transfered image (memcpy)
    std::thread thread_buffering_image = std::thread([&]() {
        SendStatusMessage("Image buffering thread is started.", false);
        while (1)
        {
            // Get the buffer from the buffering sync Queue
            float* image_ptr = m_syncImageBuffer.Queue_sync.pop();
            if (image_ptr != nullptr)
            {
                // Body
                if (m_nRecordedImages < WRITING_IMAGE_SIZE)
                {
                    float* buffer = m_queueWritingImage.front();
                    m_queueWritingImage.pop();
                    memcpy(buffer, image_ptr, sizeof(float) * 4 * m_pConfig->imageSize);
                    m_queueWritingImage.push(buffer);

                    m_nRecordedImages++;

                    // Return (push) the buffer to the buffering threading queue
                    {
                        std::unique_lock<std::mutex> lock(m_syncImageBuffer.mtx);
                        m_syncImageBuffer.queue_buffer.push(image_ptr);
                    }
                }
            }
            else
                break;
        }
        SendStatusMessage("Image copying thread is finished.", false);
    });
    thread_buffering_image.detach();

#ifdef RAW_PULSE_WRITE
    // Thread for buffering transfered data (memcpy)
    std::thread thread_buffering_data = std::thread([&]() {
        SendStatusMessage("Data buffering thread is started.", false);
        while (1)
        {
            // Get the buffer from the buffering sync Queue
            uint16_t* pulse_ptr = m_syncBuffering.Queue_sync.pop();
            if (pulse_ptr != nullptr)
            {
                // Body
                if (m_nRecordedFrames < WRITING_BUFFER_SIZE)
                {
                    uint16_t* buffer = m_queueWritingBuffer.front();
                    m_queueWritingBuffer.pop();
                    memcpy(buffer, pulse_ptr, sizeof(uint16_t) * m_pConfig->bufferSize);
                    m_queueWritingBuffer.push(buffer);

                    m_nRecordedFrames++;

                    // Return (push) the buffer to the buffering threading queue
                    {
                        std::unique_lock<std::mutex> lock(m_syncBuffering.mtx);
                        m_syncBuffering.queue_buffer.push(pulse_ptr);
                    }
                }
            }
            else
                break;
        }
        SendStatusMessage("Data copying thread is finished.", false);
    });
    thread_buffering_data.detach();
#endif

    return true;
}

void MemoryBuffer::stopRecording()
{
    // Stop recording
#ifdef RAW_PULSE_WRITE
    m_bIsRecordingPulse = false;
#endif
    m_bIsRecordingImage = false;
    m_bIsFirstRecImage = false;

    if (m_nRecordedImages != 0) // Not allowed when 'discard'
    {
        // Push nullptr to Buffering Queue
        m_syncImageBuffer.Queue_sync.push(nullptr);

        // Status update
        uint64_t total_size = (uint64_t)(m_nRecordedImages * 4 * m_pConfig->imageSize * sizeof(float));

        char msg[256];
        sprintf(msg, "Image recording is finished normally. (Recorded images: %d (%.2f MB)", m_nRecordedImages, (double)total_size / 1024.0 / 1024.0);
        SendStatusMessage(msg, false);
    }

#ifdef RAW_PULSE_WRITE
    if (m_nRecordedFrames != 0) // Not allowed when 'discard'
    {
        // Push nullptr to Buffering Queue
        m_syncBuffering.Queue_sync.push(nullptr);

        // Status update
        uint64_t total_size = (uint64_t)(m_nRecordedFrames * m_pConfig->bufferSize * sizeof(uint16_t));

        char msg[256];
        sprintf(msg, "Data recording is finished normally.\n(Recorded frames: %d frames (%.2f MB)", m_nRecordedFrames, (double)total_size / 1024.0 / 1024.0);
        SendStatusMessage(msg, false);
    }
#endif
}

bool MemoryBuffer::startSaving()
{
    // Get path to write
    m_fileName = QFileDialog::getSaveFileName(nullptr, "Save As", "", "Raw data (*.data)");
    if (m_fileName == "") return false;

    // Start writing thread
    std::thread _thread = std::thread(&MemoryBuffer::write, this);
    _thread.detach();

    return true;
}


void MemoryBuffer::write()
{
    qint64 res, samplesToWrite;

    if (QFile::exists(m_fileName))
    {
        SendStatusMessage("Doulos does not overwrite a recorded data.", false);
        emit finishedWritingThread(true);
        return;
    }

    // Move to start point
#ifdef RAW_PULSE_WRITE
    uint16_t* buffer = nullptr;
    for (int i = 0; i < WRITING_BUFFER_SIZE - m_nRecordedFrames; i++)
    {
        buffer = m_queueWritingBuffer.front();
        m_queueWritingBuffer.pop();
        m_queueWritingBuffer.push(buffer);
    }
#endif

    float* image = nullptr;
    for (int i = 0; i < WRITING_IMAGE_SIZE - m_nRecordedImages; i++)
    {
        image = m_queueWritingImage.front();
        m_queueWritingImage.pop();
        m_queueWritingImage.push(image);
    }

    // Get file title and path
    QString fileTitle, filePath;
    for (int i = 0; i < m_fileName.length(); i++)
    {
        if (m_fileName.at(i) == QChar('.')) fileTitle = m_fileName.left(i);
        if (m_fileName.at(i) == QChar('/')) filePath = m_fileName.left(i);
    }
#ifdef RAW_PULSE_WRITE
    QString pulseName = fileTitle + ".pulse";
#endif

    // Raw + scaled image writing
    QFile file(m_fileName);
    samplesToWrite = 4 * m_pConfig->imageSize;

    QString path = filePath + "/scaled_image/";
    QDir().mkpath(path);

    QString path0[4];
    if (m_nRecordedImages > 1)
    {
        for (int i = 0; i < 4; i++)
        {
            path0[i] = filePath + "/scaled_image/" + mode_name[m_pConfig->channelImageMode[i]] + "/";
            QDir().mkpath(path0[i]);
        }
    }

    ColorTable temp_ctable;
    IppiSize roi_image = { m_pConfig->nPixels, m_pConfig->nLines };

    ImageObject imgObj_Ch1(roi_image.width, roi_image.height, temp_ctable.m_colorTableVector.at(color_table_index[m_pConfig->channelImageMode[0]]));
    ImageObject imgObj_Ch2(roi_image.width, roi_image.height, temp_ctable.m_colorTableVector.at(color_table_index[m_pConfig->channelImageMode[1]]));
    ImageObject imgObj_Ch3(roi_image.width, roi_image.height, temp_ctable.m_colorTableVector.at(color_table_index[m_pConfig->channelImageMode[2]]));
    ImageObject imgObj_Ch4(roi_image.width, roi_image.height, temp_ctable.m_colorTableVector.at(color_table_index[m_pConfig->channelImageMode[3]]));

    // Writing images
    if (file.open(QIODevice::WriteOnly))
    {
        for (int i = 0; i < m_nRecordedImages; i++)
        {
            image = m_queueWritingImage.front();
            m_queueWritingImage.pop();

            // Write raw float images
            res = file.write(reinterpret_cast<char*>(image), sizeof(float) * samplesToWrite);
            if (!(res == sizeof(float) * samplesToWrite))
            {
                SendStatusMessage("Error occurred while writing...", true);
                emit finishedWritingThread(true);
                return;
            }

            // Write scaled images
            float* scan_ch1 = image + 0 * m_pConfig->imageSize;
            float* scan_ch2 = image + 1 * m_pConfig->imageSize;
            float* scan_ch3 = image + 2 * m_pConfig->imageSize;
            float* scan_ch4 = image + 3 * m_pConfig->imageSize;

            ippiScale_32f8u_C1R(scan_ch1, sizeof(float) * roi_image.width, imgObj_Ch1.arr.raw_ptr(), sizeof(uint8_t) * roi_image.width,
                roi_image, m_pConfig->imageContrastRange[0].min, m_pConfig->imageContrastRange[0].max);
            ippiScale_32f8u_C1R(scan_ch2, sizeof(float) * roi_image.width, imgObj_Ch2.arr.raw_ptr(), sizeof(uint8_t) * roi_image.width,
                roi_image, m_pConfig->imageContrastRange[1].min, m_pConfig->imageContrastRange[1].max);
            ippiScale_32f8u_C1R(scan_ch3, sizeof(float) * roi_image.width, imgObj_Ch3.arr.raw_ptr(), sizeof(uint8_t) * roi_image.width,
                roi_image, m_pConfig->imageContrastRange[2].min, m_pConfig->imageContrastRange[2].max);
            ippiScale_32f8u_C1R(scan_ch4, sizeof(float) * roi_image.width, imgObj_Ch4.arr.raw_ptr(), sizeof(uint8_t) * roi_image.width,
                roi_image, m_pConfig->imageContrastRange[3].min, m_pConfig->imageContrastRange[3].max);

#ifdef MED_FILT
            (*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObj_Ch1.arr);
            (*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObj_Ch2.arr);
            (*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObj_Ch3.arr);
            (*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObj_Ch4.arr);
#endif

            int x = i % m_pConfig->imageStichingXStep;
            int y = i / m_pConfig->imageStichingXStep;
            if (y % 2 == 1)
                x = m_pConfig->imageStichingXStep - x - 1;
            int ii = x + y * m_pConfig->imageStichingXStep;

            QString write_path[4];
            for (int j = 0; j < 4; j++)
                write_path[j] = (m_nRecordedImages > 1) ? path0[j] : path;

            imgObj_Ch1.qindeximg.copy(0, 0, m_pConfig->nPixels, m_pConfig->nLines - GALVO_FLYING_BACK)
                .save(write_path[0] + QString("%1_image_acc_%2_avg_%3_[%4 %5]_%6.bmp").arg(mode_name[m_pConfig->channelImageMode[0]])
                    .arg(m_pConfig->imageAccumulationFrames).arg(m_pConfig->imageAveragingFrames)
                    .arg(m_pConfig->imageContrastRange[0].min, 2, 'f', 1).arg(m_pConfig->imageContrastRange[0].max, 2, 'f', 1).arg(ii + 1, 3, 10, (QChar)'0'), "bmp");
            imgObj_Ch2.qindeximg.copy(0, 0, m_pConfig->nPixels, m_pConfig->nLines - GALVO_FLYING_BACK)
                .save(write_path[1] + QString("%1_image_acc_%2_avg_%3_[%4 %5]_%6.bmp").arg(mode_name[m_pConfig->channelImageMode[1]])
                    .arg(m_pConfig->imageAccumulationFrames).arg(m_pConfig->imageAveragingFrames)
                    .arg(m_pConfig->imageContrastRange[1].min, 2, 'f', 1).arg(m_pConfig->imageContrastRange[1].max, 2, 'f', 1).arg(ii + 1, 3, 10, (QChar)'0'), "bmp");
            imgObj_Ch3.qindeximg.copy(0, 0, m_pConfig->nPixels, m_pConfig->nLines - GALVO_FLYING_BACK)
                .save(write_path[2] + QString("%1_image_acc_%2_avg_%3_[%4 %5]_%6.bmp").arg(mode_name[m_pConfig->channelImageMode[2]])
                    .arg(m_pConfig->imageAccumulationFrames).arg(m_pConfig->imageAveragingFrames)
                    .arg(m_pConfig->imageContrastRange[2].min, 2, 'f', 1).arg(m_pConfig->imageContrastRange[2].max, 2, 'f', 1).arg(ii + 1, 3, 10, (QChar)'0'), "bmp");
            imgObj_Ch4.qindeximg.copy(0, 0, m_pConfig->nPixels, m_pConfig->nLines - GALVO_FLYING_BACK)
                .save(write_path[3] + QString("%1_image_acc_%2_avg_%3_[%4 %5]_%6.bmp").arg(mode_name[m_pConfig->channelImageMode[3]])
                    .arg(m_pConfig->imageAccumulationFrames).arg(m_pConfig->imageAveragingFrames)
                    .arg(m_pConfig->imageContrastRange[3].min, 2, 'f', 1).arg(m_pConfig->imageContrastRange[3].max, 2, 'f', 1).arg(ii + 1, 3, 10, (QChar)'0'), "bmp");

            // Push back to the original queue
            emit wroteSingleFrame(i);

            m_queueWritingImage.push(image);
        }
        file.close();
    }
    else
    {
        SendStatusMessage("Error occurred during writing process.", true);
        return;
    }

#ifdef RAW_PULSE_WRITE
    // Writing raw pulse
    QFile filep(pulseName);
    samplesToWrite = m_pConfig->bufferSize;
    if (filep.open(QIODevice::WriteOnly))
    {
        for (int i = 0; i < m_nRecordedFrames; i++)
        {
            buffer = m_queueWritingBuffer.front();
            m_queueWritingBuffer.pop();

            res = filep.write(reinterpret_cast<char*>(buffer), sizeof(uint16_t) * samplesToWrite);
            if (!(res == sizeof(uint16_t) * samplesToWrite))
            {
                printf("Error occurred while writing...\n");
                emit finishedWritingThread(true);
                return;
            }
            emit wroteSingleFrame(i);

            m_queueWritingBuffer.push(buffer);
        }
        filep.close();
    }
    else
    {
        printf("Error occurred during writing process.\n");
        return;
    }
#endif

    m_bIsSaved = true;

    // Move files
    m_pConfig->setConfigFile("Doulos.ini");
    if (false == QFile::copy("Doulos.ini", fileTitle + ".ini"))
        SendStatusMessage("Error occurred while copying configuration data.", true);

    if (false == QFile::copy("Doulos.m", fileTitle + ".m"))
        SendStatusMessage("Error occurred while copying MATLAB processing data.", true);

    // Send a signal to notify this thread is finished
    emit finishedWritingThread(false);

    // Status update
    char msg[256];
#ifdef RAW_PULSE_WRITE
    sprintf(msg, "Data saving thread is finished normally. (Saved frames: %d frames)", m_nRecordedFrames);
    SendStatusMessage(msg, false);
#else
    sprintf(msg, "Data saving thread is finished normally. (Saved images: %d)", m_nRecordedImages);
    SendStatusMessage(msg, false);
#endif

    QByteArray temp = m_fileName.toLocal8Bit();
    char* filename = temp.data();
    sprintf(msg, "[%s]", filename);
    SendStatusMessage(msg, false);
}
