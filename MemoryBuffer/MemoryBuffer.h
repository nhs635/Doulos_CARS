#ifndef MEMORYBUFFER_H
#define MEMORYBUFFER_H

#include <QObject>

#include <Doulos/Configuration.h>

#include <iostream>
#include <thread>
#include <queue>

#include <Common/SyncObject.h>
#include <Common/callback.h>

class MainWindow;
class Configuration;
class QOperationTab;
class QDeviceControlTab;

class MemoryBuffer : public QObject
{
	Q_OBJECT

public:
    explicit MemoryBuffer(QObject *parent = nullptr);
    virtual ~MemoryBuffer();

public:
	// Memory allocation function (buffer for writing)
    void allocateWritingBuffer();
	void deallocateWritingBuffer();

    // Data recording (transfer streaming data to writing buffer)
    bool startRecording();
    void stopRecording();

    // Data saving (save wrote data to hard disk)
    bool startSaving();
	
private: // writing threading operation
	void write();

signals:
	void wroteSingleFrame(int);
	void finishedBufferAllocation();
	void finishedWritingThread(bool);

private:
	Configuration* m_pConfig;
	QOperationTab* m_pOperationTab;
	QDeviceControlTab* m_pDeviceControlTab;

public:
	bool m_bIsAllocatedWritingBuffer;
#ifdef RAW_PULSE_WRITE
    bool m_bIsImageStartPoint;
	bool m_bIsRecordingPulse;
    int m_nRecordedFrames;
#endif
    bool m_bIsFirstRecImage;
	bool m_bIsRecordingImage;
    int m_nRecordedImages;
    bool m_bIsSaved;

public:
	callback2<const char*, bool> SendStatusMessage;
	
public:
#ifdef RAW_PULSE_WRITE
	SyncObject<uint16_t> m_syncBuffering;
#endif
    SyncObject<float> m_syncImageBuffer;

private:
#ifdef RAW_PULSE_WRITE
    std::queue<uint16_t*> m_queueWritingBuffer; // writing buffer
#endif
    std::queue<float*> m_queueWritingImage;
	QString m_fileName;
};

#endif // MEMORYBUFFER_H
