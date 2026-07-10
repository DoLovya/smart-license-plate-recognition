#pragma once

#include <QElapsedTimer>
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
class QThread;
class VideoDisplayWidget;
class VideoStreamWorker;
class ResultExportService;

class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool isDetectionRunning() const;
    int cameraCount() const;
    int resultCount() const;
    QString latestPlateText() const;

    void submitFrameForRecognition(const QImage& frame, const QString& imageId = QString());

public slots:
    void setServiceEndpoint(const QUrl& endpoint);

private:
    void setupWindow();
    void setupVideoPipeline();
    void connectSignals();
    void applyControlState(bool running);
    void refreshResultPanel(const RecognitionRecord& record);
    void appendHistoryRow(const RecognitionRecord& record);
    QVector<CameraDevice> enumerateCameras() const;
    CameraDevice currentCameraDevice() const;
    static QString formatConfidence(double confidence);

private slots:
    void refreshCameraDevices();
    void startDetection();
    void stopDetection();
    void importImage();
    void exportResults();
    void handleCameraSelectionChanged(int index);
    void handleFrameReady(const QImage& frame, double fps, const QString& imageId);
    void handleRecognitionReady(const RecognitionRecord& record);
    void handleServiceStateChanged(const QString& statusText);

private:
    Ui::MainWindow* ui_;
    VideoDisplayWidget* videoDisplay_;
    QThread* videoThread_;
    VideoStreamWorker* videoWorker_;
    AlgorithmServiceClient* serviceClient_;
    ResultExportService* exportService_;
    QVector<CameraDevice> cameraDevices_;
    QVector<RecognitionRecord> recognitionHistory_;
    QElapsedTimer submissionThrottle_;
    bool detectionRunning_ = false;
    QImage latestFrame_;
};
