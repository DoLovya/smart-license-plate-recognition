#include "widgets/ImagePreviewWidget.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QSizePolicy>

namespace
{
constexpr qreal kAspectRatio = 16.0 / 9.0;
constexpr int kOverlayPaddingX = 10;
constexpr int kOverlayPaddingY = 6;
constexpr int kOverlayRadius = 8;

bool isLightThemeActive()
{
    return qApp != nullptr && qApp->property("themeId").toString() == QStringLiteral("enterprise-light");
}

QColor previewCanvasColor()
{
    return isLightThemeActive() ? QColor("#edf2f7") : QColor("#07101c");
}

QColor previewSurfaceColor()
{
    return isLightThemeActive() ? QColor("#f8fbff") : QColor("#0d1828");
}

QColor previewPlaceholderColor()
{
    return isLightThemeActive() ? QColor("#64748b") : QColor("#5bc7ff");
}

QColor overlayStrokeColor()
{
    return isLightThemeActive() ? QColor("#2563eb") : QColor("#20d4ff");
}

QColor overlayLabelBackgroundColor()
{
    return isLightThemeActive() ? QColor(15, 23, 42, 220) : QColor(6, 18, 32, 220);
}
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

void ImagePreviewWidget::setRecognitionRecord(const RecognitionRecord* record)
{
    hasRecognitionRecord_ = record != nullptr;
    if (record != nullptr) {
        currentRecord_ = *record;
    } else {
        currentRecord_ = RecognitionRecord {};
    }
    update();
}

QImage ImagePreviewWidget::currentImage() const
{
    return currentImage_;
}

int ImagePreviewWidget::overlayCount() const
{
    return hasRecognitionRecord_ ? currentRecord_.boxes.size() : 0;
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
    painter.fillRect(rect(), previewCanvasColor());

    const QRect target = imageRect();
    QPainterPath clipPath;
    clipPath.addRoundedRect(target, 18, 18);
    painter.fillPath(clipPath, previewSurfaceColor());
    painter.setClipPath(clipPath);

    if (currentImage_.isNull()) {
        painter.fillRect(target, previewSurfaceColor());
        painter.setPen(previewPlaceholderColor());
        painter.drawText(target, Qt::AlignCenter, tr("请先导入静态图片"));
        return;
    }

    const QRect displayedRect = displayedImageRect(target);
    painter.drawImage(displayedRect, currentImage_);

    if (!hasRecognitionRecord_ || currentRecord_.boxes.isEmpty()) {
        return;
    }

    const QSize sourceSize = currentRecord_.frameSize.isValid() ? currentRecord_.frameSize : currentImage_.size();
    if (!sourceSize.isValid() || sourceSize.width() <= 0 || sourceSize.height() <= 0) {
        return;
    }

    painter.setClipping(false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const qreal scaleX = static_cast<qreal>(displayedRect.width()) / static_cast<qreal>(sourceSize.width());
    const qreal scaleY = static_cast<qreal>(displayedRect.height()) / static_cast<qreal>(sourceSize.height());
    QFont overlayFont = painter.font();
    overlayFont.setBold(true);
    overlayFont.setPointSizeF(qMax<qreal>(10.0, overlayFont.pointSizeF()));
    painter.setFont(overlayFont);
    const QFontMetrics metrics(overlayFont);

    for (int index = 0; index < currentRecord_.boxes.size(); ++index) {
        const DetectionBox& box = currentRecord_.boxes.at(index);
        const QRectF mappedRect(
            displayedRect.left() + (box.x1 * scaleX),
            displayedRect.top() + (box.y1 * scaleY),
            qMax<qreal>(2.0, (box.x2 - box.x1) * scaleX),
            qMax<qreal>(2.0, (box.y2 - box.y1) * scaleY));

        painter.setPen(QPen(overlayStrokeColor(), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(mappedRect, 8, 8);

        const QString label = overlayLabel(currentRecord_, index);
        if (label.isEmpty()) {
            continue;
        }

        const QRect textRect = metrics.boundingRect(label);
        const int labelWidth = textRect.width() + (kOverlayPaddingX * 2);
        const int labelHeight = textRect.height() + (kOverlayPaddingY * 2);
        int labelLeft = qRound(mappedRect.left());
        int labelTop = qRound(mappedRect.top()) - labelHeight - 6;
        if (labelTop < displayedRect.top() + 4) {
            labelTop = qRound(mappedRect.top()) + 6;
        }
        if (labelLeft + labelWidth > displayedRect.right()) {
            labelLeft = displayedRect.right() - labelWidth;
        }
        labelLeft = qMax(labelLeft, displayedRect.left());

        const QRect labelRect(labelLeft, labelTop, labelWidth, labelHeight);
        painter.setPen(Qt::NoPen);
        painter.setBrush(overlayLabelBackgroundColor());
        painter.drawRoundedRect(labelRect, kOverlayRadius, kOverlayRadius);
        painter.setPen(QColor("#ffffff"));
        painter.drawText(labelRect, Qt::AlignCenter, label);
    }
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

QRect ImagePreviewWidget::displayedImageRect(const QRect& target) const
{
    if (currentImage_.isNull()) {
        return target;
    }

    const QSize scaledSize = currentImage_.size().scaled(target.size(), Qt::KeepAspectRatio);
    return QRect(
        QPoint(
            target.center().x() - (scaledSize.width() / 2),
            target.center().y() - (scaledSize.height() / 2)),
        scaledSize);
}

QString ImagePreviewWidget::overlayLabel(const RecognitionRecord& record, int boxIndex) const
{
    QStringList parts;
    if (!record.plateText.trimmed().isEmpty() && record.plateText.trimmed() != QStringLiteral("PENDING")) {
        parts.push_back(record.plateText.trimmed());
    }

    if (boxIndex >= 0 && boxIndex < record.boxes.size()) {
        parts.push_back(QString("%1%").arg(QString::number(record.boxes.at(boxIndex).confidence * 100.0, 'f', 1)));
    }

    return parts.join(QStringLiteral("  "));
}
