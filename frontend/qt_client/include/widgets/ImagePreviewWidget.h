#pragma once

#include <QFrame>
#include <QImage>

class ImagePreviewWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit ImagePreviewWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    QImage currentImage() const;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QRect imageRect() const;

    QImage currentImage_;
};
