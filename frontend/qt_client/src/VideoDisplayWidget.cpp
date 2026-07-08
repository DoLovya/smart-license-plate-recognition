#include "../include/VideoDisplayWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

namespace
{
constexpr qreal kAspectRatio = 16.0 / 9.0;
}

VideoDisplayWidget::VideoDisplayWidget(QWidget* parent) : QFrame(parent)
{
    setObjectName("videoDisplayWidget");
    setFrameShape(QFrame::NoFrame);
    setMinimumSize(960, 540);
}

void VideoDisplayWidget::setFrame(const QImage& frame)
{
    currentFrame_ = frame;
    update();
}

QImage VideoDisplayWidget::currentFrame() const
{
    return currentFrame_;
}

QSize VideoDisplayWidget::sizeHint() const
{
    return QSize(1280, 720);
}

void VideoDisplayWidget::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#07101c"));

    const QRect target = videoRect();
    QPainterPath clipPath;
    clipPath.addRoundedRect(target, 18, 18);
    painter.fillPath(clipPath, QColor("#0d1828"));
    painter.setClipPath(clipPath);

    if (currentFrame_.isNull()) {
        painter.fillRect(target, QColor("#0d1828"));
        painter.setPen(QColor("#5bc7ff"));
        painter.drawText(target, Qt::AlignCenter, tr("等待视频流或图像输入"));
        return;
    }

    const QImage scaledFrame =
        currentFrame_.scaled(target.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QPoint topLeft(
        target.center().x() - (scaledFrame.width() / 2),
        target.center().y() - (scaledFrame.height() / 2));
    painter.drawImage(topLeft, scaledFrame);
}

QRect VideoDisplayWidget::videoRect() const
{
    QRect content = rect().adjusted(8, 8, -8, -8);
    int width = content.width();
    int height = static_cast<int>(width / kAspectRatio);

    if (height > content.height()) {
        height = content.height();
        width = static_cast<int>(height * kAspectRatio);
    }

    const int x = content.x() + (content.width() - width) / 2;
    const int y = content.y() + (content.height() - height) / 2;
    return QRect(x, y, width, height);
}
