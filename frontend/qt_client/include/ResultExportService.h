#pragma once

#include <QString>
#include <QVector>

#include "RecognitionTypes.h"

class ResultExportService
{
public:
    bool exportCsv(
        const QString& filePath,
        const QVector<RecognitionRecord>& records,
        QString* errorMessage = nullptr) const;
};
