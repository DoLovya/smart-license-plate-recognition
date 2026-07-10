#include "services/ResultExportService.h"

#include <QSaveFile>
#include <QTextStream>

bool ResultExportService::exportCsv(
    const QString& filePath,
    const QVector<RecognitionRecord>& records,
    QString* errorMessage) const
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QTextStream stream(&file);
    stream << "plate_text,confidence,timestamp,image_id,frame_width,frame_height\n";

    for (const RecognitionRecord& record : records) {
        stream << record.plateText << ','
               << QString::number(record.confidence, 'f', 2) << ','
               << record.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ','
               << record.imageId << ','
               << record.frameSize.width() << ','
               << record.frameSize.height() << '\n';
    }

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    return true;
}
