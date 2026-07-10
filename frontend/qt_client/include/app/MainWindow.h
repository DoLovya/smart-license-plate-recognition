#pragma once

#include <QImage>
#include <QUrl>
#include <QVector>
#include <QWidget>

#include "core/RecognitionTypes.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class AlgorithmServiceClient;
class ImagePreviewWidget;
class ResultExportService;

class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool isDetectionRunning() const;
    bool hasLoadedImage() const;
    int resultCount() const;
    QString latestPlateText() const;
    bool loadImageFromFile(const QString& filePath, QString* errorMessage = nullptr);

public slots:
    void setServiceEndpoint(const QUrl& endpoint);

private:
    void setupWindow();
    void connectSignals();
    void applyControlState(bool running);
    void refreshResultPanel(const RecognitionRecord& record);
    void appendHistoryRow(const RecognitionRecord& record);
    bool validateImportedImage(const QString& filePath, QString* errorMessage, QImage* image = nullptr) const;
    QString buildImageId(const QString& filePath) const;
    static QString formatConfidence(double confidence);

private slots:
    void startDetection();
    void importImage();
    void exportResults();
    void handleRecognitionReady(const RecognitionRecord& record);
    void handleServiceStateChanged(const QString& statusText);
    void handleDetectionFailed(const QString& errorMessage);

private:
    Ui::MainWindow* ui_;
    ImagePreviewWidget* imagePreview_;
    AlgorithmServiceClient* serviceClient_;
    ResultExportService* exportService_;
    QVector<RecognitionRecord> recognitionHistory_;
    QString importedImagePath_;
    QString importedImageId_;
    bool detectionRunning_ = false;
    QImage importedImage_;
};
