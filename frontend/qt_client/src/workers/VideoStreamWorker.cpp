#include "workers/VideoStreamWorker.h"

#include <QDateTime>
#include <QFont>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QTimer>

VideoStreamWorker::VideoStreamWorker(QObject* parent) : QObject(parent) {}

void VideoStreamWorker::startStream(const CameraDevice& device)
{
    activeDevice_ = device;
    frameCounter_ = 0;
    running_ = true;
    fpsTimer_.restart();

    if (timer_ == nullptr) {
        timer_ = new QTimer(this);
        timer_->setTimerType(Qt::PreciseTimer);
        connect(timer_, &QTimer::timeout, this, &VideoStreamWorker::produceFrame);
    }

    timer_->start(40);
    emit streamStateChanged(true, tr("正在采集: %1").arg(activeDevice_.displayName));
}

void VideoStreamWorker::stopStream()
{
    if (timer_ != nullptr) {
        timer_->stop();
    }

    running_ = false;
    emit streamStateChanged(false, tr("采集已停止"));
}

void VideoStreamWorker::produceFrame()
{
    if (!running_) {
        return;
    }

    QImage frame(frameSize_, QImage::Format_RGB32);
    frame.fill(QColor("#07101c"));

    QPainter painter(&frame);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, frame.width(), frame.height());
    background.setColorAt(0.0, QColor("#081321"));
    background.setColorAt(0.5, QColor("#0b1f37"));
    background.setColorAt(1.0, QColor("#10396b"));
    painter.fillRect(frame.rect(), background);

    const QRect scanArea(frame.width() / 6, frame.height() / 4, frame.width() * 2 / 3, frame.height() / 3);
    painter.setPen(QPen(QColor("#53c2ff"), 4));
    painter.drawRoundedRect(scanArea, 16, 16);

    const int lineY = scanArea.top() + static_cast<int>((frameCounter_ * 12) % scanArea.height());
    painter.setPen(QPen(QColor("#8df3ff"), 3));
    painter.drawLine(scanArea.left() + 12, lineY, scanArea.right() - 12, lineY);

    painter.setPen(QColor("#d8f7ff"));
    painter.setFont(QFont("Arial", 18, QFont::Medium));
    painter.drawText(
        QRect(48, 42, frame.width() - 96, 36),
        Qt::AlignLeft | Qt::AlignVCenter,
        activeDevice_.displayName.isEmpty() ? tr("实时视频流") : activeDevice_.displayName);

    painter.setFont(QFont("Consolas", 16));
    painter.drawText(
        QRect(48, frame.height() - 80, frame.width() - 96, 32),
        Qt::AlignLeft | Qt::AlignVCenter,
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"));

    painter.setFont(QFont("Arial", 14));
    painter.drawText(
        QRect(48, frame.height() - 48, frame.width() - 96, 24),
        Qt::AlignLeft | Qt::AlignVCenter,
        tr("QThread 采集线程运行中 | 目标帧率 >= 25fps"));

    ++frameCounter_;
    const double fps =
        fpsTimer_.elapsed() > 0 ? (frameCounter_ * 1000.0 / static_cast<double>(fpsTimer_.elapsed())) : 0.0;
    const QString imageId = QString("%1-%2-%3")
                                .arg(activeDevice_.deviceId.isEmpty() ? QStringLiteral("frame") : activeDevice_.deviceId)
                                .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz"))
                                .arg(frameCounter_);
    emit frameReady(frame, fps, imageId);
}
