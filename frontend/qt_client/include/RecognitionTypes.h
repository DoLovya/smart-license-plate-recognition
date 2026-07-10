#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QSize>
#include <QString>

struct CameraDevice
{
    QString displayName;
    QString deviceId;
    bool isDemo = false;
};

Q_DECLARE_METATYPE(CameraDevice)

struct RecognitionRecord
{
    QString plateText;
    double confidence = 0.0;
    QDateTime timestamp;
    QString imageId;
    QSize frameSize;
};

Q_DECLARE_METATYPE(RecognitionRecord)
