#include "widgets/ImagePreviewWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSizePolicy>

namespace
{
constexpr qreal kAspectRatio = 16.0 / 9.0;
}

ImagePreviewWidget::ImagePreviewWidget(QWidget* parent) : QFrame(parent)
{
    setObjectName("imagePreviewWidget");
    setFrameShape(QFrame::NoFrame);
    setMinimumSize(720, 405);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ImagePreviewWidget::setImage(const QImage& image)
{
    currentImage_ = image;
    update();
}

QImage ImagePreviewWidget::currentImage() const
{
    return currentImage_;
}

QSize ImagePreviewWidget::sizeHint() const
{
    return QSize(1280, 720);
}

void ImagePreviewWidget::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#07101c"));

    const QRect target = imageRect();
    QPainterPath clipPath;
    clipPath.addRoundedRect(target, 18, 18);
    painter.fillPath(clipPath, QColor("#0d1828"));
    painter.setClipPath(clipPath);

    if (currentImage_.isNull()) {
        painter.fillRect(target, QColor("#0d1828"));
        painter.setPen(QColor("#5bc7ff"));
        painter.drawText(target, Qt::AlignCenter, tr("请先导入静态图片"));
        return;
    }

    const QImage scaledFrame =
        currentImage_.scaled(target.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QPoint topLeft(
        target.center().x() - (scaledFrame.width() / 2),
        target.center().y() - (scaledFrame.height() / 2));
    painter.drawImage(topLeft, scaledFrame);
}

QRect ImagePreviewWidget::imageRect() const
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
