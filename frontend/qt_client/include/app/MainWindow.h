#pragma once

#include <QImage>
#include <QResizeEvent>
#include <QStringList>
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
class QActionGroup;
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
    bool loadImagesFromDirectory(const QString& directoryPath, QString* errorMessage = nullptr);

public slots:
    void setServiceEndpoint(const QUrl& endpoint);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class InputMode
    {
        None,
        SingleImage,
        ImageDirectory,
    };

    void setupWindow();
    void connectSignals();
    void applyControlState(bool running);
    void applyTheme(const QString& themeId, bool persist = true);
    void updateContentPaneWidths();
    void updateSourceSummary();
    void refreshResultPanel(const RecognitionRecord& record);
    void appendHistoryRow(const RecognitionRecord& record);
    void clearCurrentImage();
    void applyCurrentImage(const QString& filePath, const QImage& image);
    void clearDirectorySelection();
    bool hasConfiguredDirectory() const;
    QStringList collectDirectoryImages(const QString& directoryPath) const;
    bool previewFirstDirectoryImage(QString* errorMessage);
    void startSingleImageDetection();
    void startDirectoryDetection();
    void submitNextDirectoryImage();
    void finishDirectoryDetection();
    void stopDetectionInternal();
    bool validateImportedImage(const QString& filePath, QString* errorMessage, QImage* image = nullptr) const;
    QString buildImageId(const QString& filePath) const;
    static QString formatConfidence(double confidence);

private slots:
    void startDetection();
    void stopDetection();
    void importImage();
    void importImageDirectory();
    void exportResults();
    void handleRecognitionReady(const RecognitionRecord& record);
    void handleServiceStateChanged(const QString& statusText);
    void handleDetectionFailed(const QString& errorMessage);
    void handleThemeActionTriggered();

private:
    Ui::MainWindow* ui_;
    QActionGroup* themeActionGroup_ = nullptr;
    ImagePreviewWidget* imagePreview_;
    AlgorithmServiceClient* serviceClient_;
    ResultExportService* exportService_;
    QVector<RecognitionRecord> recognitionHistory_;
    QString importedImagePath_;
    QString importedImageId_;
    QString configuredDirectoryPath_;
    QStringList configuredDirectoryImages_;
    bool detectionRunning_ = false;
    QImage importedImage_;
    InputMode inputMode_ = InputMode::None;
    int activeDirectoryIndex_ = -1;
    int directorySuccessCount_ = 0;
    int directoryFailureCount_ = 0;
    bool stopRequested_ = false;
};
