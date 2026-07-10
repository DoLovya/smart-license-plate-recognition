#include "app/MainWindow.h"

#include "api/AlgorithmServiceClient.h"
#include "services/ResultExportService.h"
#include "widgets/VideoDisplayWidget.h"
#include "workers/VideoStreamWorker.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>

#if defined(HAVE_QT_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QCameraDevice>
#include <QMediaDevices>
#elif defined(HAVE_QT_MULTIMEDIA)
#include <QCameraInfo>
#endif

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::MainWindow),
      videoDisplay_(nullptr),
      videoThread_(new QThread(this)),
      videoWorker_(new VideoStreamWorker),
      serviceClient_(new AlgorithmServiceClient(this)),
      exportService_(new ResultExportService)
{
    qRegisterMetaType<CameraDevice>("CameraDevice");
    qRegisterMetaType<RecognitionRecord>("RecognitionRecord");

    ui_->setupUi(this);
    setupWindow();
    setupVideoPipeline();
    connectSignals();
    refreshCameraDevices();

    serviceClient_->setEndpoint(QUrl("http://127.0.0.1:8000/api/v1/recognize"));
    serviceClient_->setMockMode(true);
}

MainWindow::~MainWindow()
{
    stopDetection();
    videoThread_->quit();
    videoThread_->wait(1000);
    delete videoWorker_;
    delete exportService_;
    delete ui_;
}

bool MainWindow::isDetectionRunning() const
{
    return detectionRunning_;
}

int MainWindow::cameraCount() const
{
    return cameraDevices_.size();
}

int MainWindow::resultCount() const
{
    return recognitionHistory_.size();
}

QString MainWindow::latestPlateText() const
{
    return ui_->currentPlateValueLabel->text();
}

void MainWindow::submitFrameForRecognition(const QImage& frame, const QString& imageId)
{
    serviceClient_->submitFrame(frame, imageId);
}

void MainWindow::setServiceEndpoint(const QUrl& endpoint)
{
    serviceClient_->setMockMode(false);
    serviceClient_->setEndpoint(endpoint);
}

void MainWindow::setupWindow()
{
    setWindowTitle(tr("智能车牌检测识别系统"));
    resize(1680, 960);
    setMinimumSize(1440, 810);

    videoDisplay_ = new VideoDisplayWidget(ui_->videoSurfaceContainer);
    ui_->videoSurfaceLayout->addWidget(videoDisplay_);
    ui_->contentLayout->setStretch(0, 3);
    ui_->contentLayout->setStretch(1, 1);

    ui_->stopButton->setEnabled(false);
    ui_->resultTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui_->resultTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui_->resultTableWidget->verticalHeader()->setVisible(false);
    ui_->resultTableWidget->setRowCount(0);
}

void MainWindow::setupVideoPipeline()
{
    videoWorker_->moveToThread(videoThread_);
    videoThread_->start();

    connect(videoWorker_, &VideoStreamWorker::frameReady, this, &MainWindow::handleFrameReady);
    connect(videoWorker_, &VideoStreamWorker::streamStateChanged, this, [this](bool running, const QString& message) {
        if (detectionRunning_ || !running) {
            ui_->statusValueLabel->setText(message);
        }
    });
}

void MainWindow::connectSignals()
{
    connect(ui_->uploadButton, &QPushButton::clicked, this, &MainWindow::importImage);
    connect(ui_->startButton, &QPushButton::clicked, this, &MainWindow::startDetection);
    connect(ui_->stopButton, &QPushButton::clicked, this, &MainWindow::stopDetection);
    connect(ui_->exportButton, &QPushButton::clicked, this, &MainWindow::exportResults);
    connect(ui_->cameraComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::handleCameraSelectionChanged);
    connect(serviceClient_, &AlgorithmServiceClient::recognitionReady, this, &MainWindow::handleRecognitionReady);
    connect(serviceClient_, &AlgorithmServiceClient::serviceStateChanged, this, &MainWindow::handleServiceStateChanged);
}

void MainWindow::applyControlState(bool running)
{
    ui_->startButton->setEnabled(!running);
    ui_->stopButton->setEnabled(running);
    ui_->cameraComboBox->setEnabled(!running || cameraDevices_.size() > 1);
    ui_->statusValueLabel->setText(running ? tr("视频流启动中") : tr("待机"));
}

void MainWindow::refreshResultPanel(const RecognitionRecord& record)
{
    ui_->currentPlateValueLabel->setText(record.plateText);
    ui_->confidenceValueLabel->setText(formatConfidence(record.confidence));
    ui_->timestampValueLabel->setText(record.timestamp.toString("yyyy-MM-dd HH:mm:ss"));
    ui_->sourceValueLabel->setText(record.imageId);
}

void MainWindow::appendHistoryRow(const RecognitionRecord& record)
{
    ui_->resultTableWidget->insertRow(0);
    ui_->resultTableWidget->setItem(0, 0, new QTableWidgetItem(record.plateText));
    ui_->resultTableWidget->setItem(0, 1, new QTableWidgetItem(formatConfidence(record.confidence)));
    ui_->resultTableWidget->setItem(0, 2, new QTableWidgetItem(record.timestamp.toString("HH:mm:ss")));
    ui_->resultTableWidget->setItem(0, 3, new QTableWidgetItem(record.imageId));
}

QVector<CameraDevice> MainWindow::enumerateCameras() const
{
    QVector<CameraDevice> devices;

#if defined(HAVE_QT_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice& camera : cameras) {
        devices.push_back(CameraDevice {camera.description(), QString::fromUtf8(camera.id()), false});
    }
#elif defined(HAVE_QT_MULTIMEDIA)
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo& camera : cameras) {
        devices.push_back(CameraDevice {camera.description(), camera.deviceName(), false});
    }
#endif

    if (devices.isEmpty()) {
        devices.push_back(CameraDevice {tr("Demo Camera"), QStringLiteral("demo-camera-0"), true});
    }

    return devices;
}

CameraDevice MainWindow::currentCameraDevice() const
{
    const int index = ui_->cameraComboBox->currentIndex();
    if (index >= 0 && index < cameraDevices_.size()) {
        return cameraDevices_.at(index);
    }

    return CameraDevice {tr("Demo Camera"), QStringLiteral("demo-camera-0"), true};
}

QString MainWindow::formatConfidence(double confidence)
{
    return QString("%1%").arg(QString::number(confidence * 100.0, 'f', 1));
}

void MainWindow::refreshCameraDevices()
{
    cameraDevices_ = enumerateCameras();

    ui_->cameraComboBox->blockSignals(true);
    ui_->cameraComboBox->clear();
    for (const CameraDevice& device : cameraDevices_) {
        ui_->cameraComboBox->addItem(device.displayName, device.deviceId);
    }
    ui_->cameraComboBox->blockSignals(false);

    if (!cameraDevices_.isEmpty()) {
        ui_->sourceValueLabel->setText("--");
    }
}

void MainWindow::startDetection()
{
    detectionRunning_ = true;
    submissionThrottle_.invalidate();
    applyControlState(true);

    const CameraDevice device = currentCameraDevice();
    ui_->sourceValueLabel->setText("--");
    QMetaObject::invokeMethod(
        videoWorker_,
        "startStream",
        Qt::QueuedConnection,
        Q_ARG(CameraDevice, device));
}

void MainWindow::stopDetection()
{
    detectionRunning_ = false;
    applyControlState(false);

    if (videoWorker_ != nullptr) {
        QMetaObject::invokeMethod(videoWorker_, "stopStream", Qt::QueuedConnection);
    }
}

void MainWindow::importImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        tr("Images (*.png *.jpg *.jpeg *.bmp)"));

    if (filePath.isEmpty()) {
        return;
    }

    QImage image(filePath);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("图像加载失败"), tr("无法读取所选图片。"));
        return;
    }

    latestFrame_ = image;
    videoDisplay_->setFrame(image);
    ui_->statusValueLabel->setText(tr("已载入图像"));
    const QString imageId = QString("import-%1-%2")
                                .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz"))
                                .arg(QFileInfo(filePath).fileName());
    submitFrameForRecognition(image, imageId);
}

void MainWindow::exportResults()
{
    if (recognitionHistory_.isEmpty()) {
        QMessageBox::information(this, tr("暂无结果"), tr("当前没有可导出的识别结果。"));
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("导出识别结果"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recognition_results.csv",
        tr("CSV Files (*.csv)"));

    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!exportService_->exportCsv(filePath, recognitionHistory_, &errorMessage)) {
        QMessageBox::critical(this, tr("导出失败"), errorMessage);
        return;
    }

    QMessageBox::information(this, tr("导出成功"), tr("识别结果已导出到:\n%1").arg(filePath));
}

void MainWindow::handleCameraSelectionChanged(int index)
{
    if (index < 0 || index >= cameraDevices_.size()) {
        return;
    }

    ui_->sourceValueLabel->setText("--");
    if (detectionRunning_) {
        QMetaObject::invokeMethod(
            videoWorker_,
            "startStream",
            Qt::QueuedConnection,
            Q_ARG(CameraDevice, cameraDevices_.at(index)));
    }
}

void MainWindow::handleFrameReady(const QImage& frame, double fps, const QString& imageId)
{
    latestFrame_ = frame;
    videoDisplay_->setFrame(frame);
    ui_->fpsValueLabel->setText(QString("%1 fps").arg(QString::number(fps, 'f', 1)));

    if (!detectionRunning_) {
        return;
    }

    if (!submissionThrottle_.isValid() || submissionThrottle_.elapsed() >= 200) {
        submitFrameForRecognition(frame, imageId);
        submissionThrottle_.restart();
    }
}

void MainWindow::handleRecognitionReady(const RecognitionRecord& record)
{
    recognitionHistory_.prepend(record);
    refreshResultPanel(record);
    appendHistoryRow(record);
}

void MainWindow::handleServiceStateChanged(const QString& statusText)
{
    ui_->backendStatusValueLabel->setText(statusText);
}
