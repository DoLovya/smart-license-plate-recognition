#pragma once

#include <QHash>
#include <QSize>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

#include "core/RecognitionTypes.h"

class QNetworkReply;

class AlgorithmServiceClient final : public QObject
{
    Q_OBJECT

public:
    explicit AlgorithmServiceClient(QObject* parent = nullptr);

    void setEndpoint(const QUrl& endpoint);
    QUrl endpoint() const;

public slots:
    bool submitImage(const QString& filePath, const QString& imageId);

signals:
    void recognitionReady(const RecognitionRecord& record);
    void serviceStateChanged(const QString& stateText);
    void requestFailed(const QString& errorMessage);

private slots:
    void handleReply(QNetworkReply* reply);

private:
    struct PendingRequestContext
    {
        QString imageId;
        QSize imageSize;
    };

    QNetworkAccessManager networkManager_;
    QHash<QNetworkReply*, PendingRequestContext> pendingRequests_;
    QUrl endpoint_;
};
