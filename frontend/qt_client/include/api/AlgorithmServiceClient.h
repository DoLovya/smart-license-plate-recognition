#pragma once

#include <QHash>
#include <QImage>
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

    void setMockMode(bool enabled);
    bool isMockMode() const;

public slots:
    void submitFrame(const QImage& frame, const QString& imageId);

signals:
    void recognitionReady(const RecognitionRecord& record);
    void serviceStateChanged(const QString& stateText);

private slots:
    void handleReply(QNetworkReply* reply);

private:
    RecognitionRecord buildMockRecord(const QImage& frame, const QString& imageId) const;

    QNetworkAccessManager networkManager_;
    QHash<QNetworkReply*, QString> pendingRequests_;
    QUrl endpoint_;
    bool mockMode_ = true;
};
