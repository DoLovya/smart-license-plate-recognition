#include "../include/AlgorithmServiceClient.h"

#include <QBuffer>
#include <QDateTime>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QTimer>

AlgorithmServiceClient::AlgorithmServiceClient(QObject* parent) : QObject(parent)
{
    connect(&networkManager_, &QNetworkAccessManager::finished, this, &AlgorithmServiceClient::handleReply);
}

void AlgorithmServiceClient::setEndpoint(const QUrl& endpoint)
{
    endpoint_ = endpoint;
    emit serviceStateChanged(endpoint_.isValid() ? tr("HTTP Ready") : tr("Mock Ready"));
}

QUrl AlgorithmServiceClient::endpoint() const
{
    return endpoint_;
}

void AlgorithmServiceClient::setMockMode(bool enabled)
{
    mockMode_ = enabled;
    emit serviceStateChanged(mockMode_ ? tr("Mock Ready") : tr("HTTP Ready"));
}

bool AlgorithmServiceClient::isMockMode() const
{
    return mockMode_;
}

void AlgorithmServiceClient::submitFrame(const QImage& frame, const QString& sourceHint)
{
    if (frame.isNull()) {
        emit serviceStateChanged(tr("忽略空帧"));
        return;
    }

    if (mockMode_ || !endpoint_.isValid()) {
        const RecognitionRecord record = buildMockRecord(frame, sourceHint);
        QTimer::singleShot(30, this, [this, record]() {
            emit recognitionReady(record);
        });
        return;
    }

    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPG", 82);

    QJsonObject payload;
    payload.insert("source", sourceHint);
    payload.insert("frame_width", frame.width());
    payload.insert("frame_height", frame.height());
    payload.insert("captured_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    payload.insert("image_base64", QString::fromLatin1(imageBytes.toBase64()));

    QNetworkRequest request(endpoint_);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    pendingRequests_.insert(reply, sourceHint);
    emit serviceStateChanged(tr("算法服务处理中"));
}

void AlgorithmServiceClient::handleReply(QNetworkReply* reply)
{
    const QString sourceHint = pendingRequests_.take(reply);
    RecognitionRecord record;
    record.source = sourceHint;
    record.timestamp = QDateTime::currentDateTime();

    if (reply->error() != QNetworkReply::NoError) {
        emit serviceStateChanged(tr("算法服务异常，已回退 Mock"));
        record = buildMockRecord(QImage(1280, 720, QImage::Format_RGB32), sourceHint);
        emit recognitionReady(record);
        reply->deleteLater();
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
    record.plateText = root.value("plate_text").toString("PENDING");
    record.confidence = root.value("confidence").toDouble(0.0);
    record.timestamp = QDateTime::fromString(root.value("timestamp").toString(), Qt::ISODate);
    if (!record.timestamp.isValid()) {
        record.timestamp = QDateTime::currentDateTime();
    }
    record.source = root.value("source").toString(sourceHint);
    record.frameSize = QSize(
        root.value("frame_width").toInt(1280),
        root.value("frame_height").toInt(720));

    emit serviceStateChanged(tr("算法服务已响应"));
    emit recognitionReady(record);
    reply->deleteLater();
}

RecognitionRecord AlgorithmServiceClient::buildMockRecord(const QImage& frame, const QString& sourceHint) const
{
    static const QStringList mockPlates = {
        "沪A8723P",
        "粤B6N8T2",
        "苏E392QK",
        "京N3Y7F8",
    };

    RecognitionRecord record;
    record.plateText = mockPlates.at(QRandomGenerator::global()->bounded(mockPlates.size()));
    record.confidence = 0.85 + (QRandomGenerator::global()->bounded(14) / 100.0);
    record.timestamp = QDateTime::currentDateTime();
    record.source = sourceHint.isEmpty() ? tr("本地 Mock 通道") : sourceHint;
    record.frameSize = frame.size();
    return record;
}
