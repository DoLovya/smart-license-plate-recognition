#include "api/AlgorithmServiceClient.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QImageReader>

AlgorithmServiceClient::AlgorithmServiceClient(QObject* parent) : QObject(parent)
{
    connect(&networkManager_, &QNetworkAccessManager::finished, this, &AlgorithmServiceClient::handleReply);
}

void AlgorithmServiceClient::setEndpoint(const QUrl& endpoint)
{
    endpoint_ = endpoint;
    emit serviceStateChanged(endpoint_.isValid() ? tr("后端接口已就绪") : tr("后端接口未配置"));
}

QUrl AlgorithmServiceClient::endpoint() const
{
    return endpoint_;
}

bool AlgorithmServiceClient::submitImage(const QString& filePath, const QString& imageId)
{
    if (!endpoint_.isValid()) {
        emit serviceStateChanged(tr("后端接口未配置"));
        emit requestFailed(tr("未配置可用的后端检测接口。"));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit serviceStateChanged(tr("图片读取失败"));
        emit requestFailed(tr("无法读取图片文件: %1").arg(filePath));
        return false;
    }

    const QByteArray fileBytes = file.readAll();
    if (fileBytes.isEmpty()) {
        emit serviceStateChanged(tr("图片内容为空"));
        emit requestFailed(tr("图片文件为空，无法上传检测。"));
        return false;
    }

    QImageReader reader(filePath);
    const QSize imageSize = reader.size();

    const QFileInfo fileInfo(filePath);
    const QString suffix = fileInfo.suffix().toLower();
    const QString uploadFileName =
        suffix.isEmpty() ? imageId : QString("%1.%2").arg(imageId, suffix);

    const QMimeDatabase mimeDatabase;
    const QString mimeType = mimeDatabase.mimeTypeForFile(fileInfo).name();

    auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(uploadFileName)));
    filePart.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QVariant(mimeType.isEmpty() ? QStringLiteral("application/octet-stream") : mimeType));
    filePart.setBody(fileBytes);
    multipart->append(filePart);

    QNetworkRequest request(endpoint_);
    QNetworkReply* reply = networkManager_.post(request, multipart);
    multipart->setParent(reply);

    pendingRequests_.insert(reply, PendingRequestContext {imageId, imageSize});
    emit serviceStateChanged(tr("图片上传中"));
    return true;
}

void AlgorithmServiceClient::cancelPendingRequests()
{
    if (pendingRequests_.isEmpty()) {
        return;
    }

    emit serviceStateChanged(tr("正在取消检测"));
    const QList<QNetworkReply*> replies = pendingRequests_.keys();
    for (QNetworkReply* reply : replies) {
        if (reply != nullptr) {
            reply->abort();
        }
    }
}

void AlgorithmServiceClient::handleReply(QNetworkReply* reply)
{
    const PendingRequestContext context = pendingRequests_.take(reply);
    RecognitionRecord record;
    record.imageId = context.imageId;
    record.timestamp = QDateTime::currentDateTime();
    record.frameSize = context.imageSize;

    if (reply->error() != QNetworkReply::NoError) {
        emit serviceStateChanged(tr("后端检测失败"));
        emit requestFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit serviceStateChanged(tr("后端响应异常"));
        emit requestFailed(tr("后端返回了不可解析的检测结果。"));
        reply->deleteLater();
        return;
    }

    const QJsonObject root = document.object();
    record.plateText = root.value("plate_text").toString("PENDING");
    record.confidence = root.value("confidence").toDouble(0.0);
    record.imageId = root.value("image_id").toString(context.imageId);

    emit serviceStateChanged(tr("检测完成"));
    emit recognitionReady(record);
    reply->deleteLater();
}
