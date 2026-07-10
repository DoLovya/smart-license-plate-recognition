#pragma once

#include <QFrame>
#include <QImage>

class VideoDisplayWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit VideoDisplayWidget(QWidget* parent = nullptr);

    void setFrame(const QImage& frame);
    QImage currentFrame() const;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QRect videoRect() const;

    QImage currentFrame_;
};
