#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QSize>
#include <QString>
#include <QVector>

struct DetectionBox
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    double confidence = 0.0;
};

struct RecognitionRecord
{
    QString plateText;
    double confidence = 0.0;
    QDateTime timestamp;
    QString imageId;
    QSize frameSize;
    QVector<DetectionBox> boxes;
};

Q_DECLARE_METATYPE(RecognitionRecord)
