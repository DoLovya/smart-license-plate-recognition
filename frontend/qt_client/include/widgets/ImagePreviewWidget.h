#pragma once

#include "core/RecognitionTypes.h"

#include <QFrame>
#include <QImage>
#include <QStringList>

class ImagePreviewWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit ImagePreviewWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setRecognitionRecord(const RecognitionRecord* record);
    QImage currentImage() const;
    int overlayCount() const;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QRect imageRect() const;
    QRect displayedImageRect(const QRect& target) const;
    QString overlayLabel(const RecognitionRecord& record, int boxIndex) const;

    QImage currentImage_;
    RecognitionRecord currentRecord_;
    bool hasRecognitionRecord_ = false;
};
