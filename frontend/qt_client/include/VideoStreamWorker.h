#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QSize>

#include "RecognitionTypes.h"

class QTimer;

class VideoStreamWorker final : public QObject
{
    Q_OBJECT

public:
    explicit VideoStreamWorker(QObject* parent = nullptr);

public slots:
    void startStream(const CameraDevice& device);
    void stopStream();

signals:
    void frameReady(const QImage& frame, double fps, const QString& sourceId);
    void streamStateChanged(bool running, const QString& message);

private slots:
    void produceFrame();

private:
    QTimer* timer_ = nullptr;
    CameraDevice activeDevice_;
    QElapsedTimer fpsTimer_;
    QSize frameSize_ = QSize(1280, 720);
    quint64 frameCounter_ = 0;
    bool running_ = false;
};
