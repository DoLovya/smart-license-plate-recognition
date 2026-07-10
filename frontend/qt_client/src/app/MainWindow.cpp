#include "app/MainWindow.h"

#include "api/AlgorithmServiceClient.h"
#include "services/ResultExportService.h"
#include "widgets/ImagePreviewWidget.h"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSet>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace
{
const QSet<QByteArray> kSupportedImageFormats = {
    "png",
    "jpg",
    "jpeg",
    "bmp",
};

constexpr int kMinImageWidth = 32;
constexpr int kMinImageHeight = 32;
constexpr int kMaxImageWidth = 8192;
constexpr int kMaxImageHeight = 8192;
constexpr qreal kPreviewPaneRatio = 0.72;
constexpr int kPreviewPaneMinWidth = 760;
constexpr int kResultPaneMinWidth = 360;
constexpr int kResultPaneMaxWidth = 520;
constexpr char kThemeSettingsKey[] = "ui/theme";

struct ThemeOption
{
    const char* id;
    const char* displayName;
    const char* resourcePath;
};

constexpr ThemeOption kThemeOptions[] = {
    {"industrial-dark", "A 工业深色蓝灰", ":/styles/theme_industrial_dark.qss"},
    {"minimal-dark", "B 极简黑灰青", ":/styles/theme_minimal_dark.qss"},
    {"enterprise-light", "C 浅色企业版", ":/styles/theme_enterprise_light.qss"},
};

QString loadThemeStyleSheet(const QString& themeId)
{
    for (const ThemeOption& option : kThemeOptions) {
        if (themeId == QLatin1String(option.id)) {
            QFile styleFile(QString::fromLatin1(option.resourcePath));
            if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return QString::fromUtf8(styleFile.readAll());
            }
            break;
        }
    }

    QFile fallbackFile(QString::fromLatin1(kThemeOptions[0].resourcePath));
    if (fallbackFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(fallbackFile.readAll());
    }
    return QString();
}

int themeIndexForId(const QString& themeId)
{
    for (int index = 0; index < static_cast<int>(std::size(kThemeOptions)); ++index) {
        if (themeId == QLatin1String(kThemeOptions[index].id)) {
            return index;
        }
    }
    return 0;
}
}

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::MainWindow),
      imagePreview_(nullptr),
      serviceClient_(new AlgorithmServiceClient(this)),
      exportService_(new ResultExportService)
{
    qRegisterMetaType<RecognitionRecord>("RecognitionRecord");

    ui_->setupUi(this);
    setupWindow();
    connectSignals();
    setServiceEndpoint(QUrl("http://127.0.0.1:8000/api/v1/recognize"));
}

MainWindow::~MainWindow()
{
    delete exportService_;
    delete ui_;
}

bool MainWindow::isDetectionRunning() const
{
    return detectionRunning_;
}

bool MainWindow::hasLoadedImage() const
{
    return !importedImage_.isNull() && !importedImagePath_.isEmpty();
}

int MainWindow::resultCount() const
{
    return recognitionHistory_.size();
}

QString MainWindow::latestPlateText() const
{
    return ui_->currentPlateValueLabel->text();
}

bool MainWindow::loadImageFromFile(const QString& filePath, QString* errorMessage)
{
    QImage image;
    if (!validateImportedImage(filePath, errorMessage, &image)) {
        return false;
    }

    importedImagePath_ = filePath;
    importedImageId_ = buildImageId(filePath);
    importedImage_ = image;

    imagePreview_->setImage(importedImage_);
    ui_->sourceValueLabel->setText(importedImageId_);
    ui_->statusValueLabel->setText(
        tr("图片已就绪: %1 (%2x%3)")
            .arg(QFileInfo(filePath).fileName())
            .arg(importedImage_.width())
            .arg(importedImage_.height()));
    applyControlState(false);
    return true;
}

void MainWindow::setServiceEndpoint(const QUrl& endpoint)
{
    serviceClient_->setEndpoint(endpoint);
}

void MainWindow::setupWindow()
{
    setWindowTitle(tr("智能车牌检测识别系统"));
    resize(1680, 960);
    setMinimumSize(1440, 810);

    auto* themeSwitcherFrame = new QFrame(ui_->headerFrame);
    themeSwitcherFrame->setObjectName("themeSwitcherFrame");

    auto* themeSwitcherLayout = new QHBoxLayout(themeSwitcherFrame);
    themeSwitcherLayout->setContentsMargins(14, 10, 14, 10);
    themeSwitcherLayout->setSpacing(10);

    auto* themeLabel = new QLabel(tr("界面样式"), themeSwitcherFrame);
    themeLabel->setObjectName("themeLabel");
    themeSwitcherLayout->addWidget(themeLabel);

    themeComboBox_ = new QComboBox(themeSwitcherFrame);
    themeComboBox_->setObjectName("themeComboBox");
    for (const ThemeOption& option : kThemeOptions) {
        themeComboBox_->addItem(tr(option.displayName), QString::fromLatin1(option.id));
    }
    themeSwitcherLayout->addWidget(themeComboBox_);
    ui_->headerLayout->addWidget(themeSwitcherFrame, 0, Qt::AlignRight);

    imagePreview_ = new ImagePreviewWidget(ui_->previewSurfaceContainer);
    imagePreview_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui_->previewSurfaceLayout->addWidget(imagePreview_);
    ui_->contentLayout->setStretch(0, 3);
    ui_->contentLayout->setStretch(1, 1);
    ui_->previewCardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ui_->resultPanelFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    ui_->resultTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui_->resultTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui_->resultTableWidget->verticalHeader()->setVisible(false);
    ui_->resultTableWidget->setRowCount(0);

    ui_->statusValueLabel->setText(tr("待导入图片"));
    ui_->backendStatusValueLabel->setText(tr("待连接"));
    ui_->sourceValueLabel->setText("--");
    applyControlState(false);
    updateContentPaneWidths();

    const QSettings settings;
    const QString themeId =
        settings.value(QString::fromLatin1(kThemeSettingsKey), QString::fromLatin1(kThemeOptions[0].id))
            .toString();
    {
        const QSignalBlocker blocker(themeComboBox_);
        themeComboBox_->setCurrentIndex(themeIndexForId(themeId));
    }
    applyTheme(themeComboBox_->currentData().toString(), false);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateContentPaneWidths();
}

void MainWindow::updateContentPaneWidths()
{
    const QRect contentGeometry = ui_->contentLayout->geometry();
    if (!contentGeometry.isValid()) {
        return;
    }

    const int layoutWidth = contentGeometry.width() - ui_->contentLayout->spacing();
    if (layoutWidth <= 0) {
        return;
    }

    int previewWidth = qRound(layoutWidth * kPreviewPaneRatio);
    int resultWidth = layoutWidth - previewWidth;

    resultWidth = qMax(kResultPaneMinWidth, resultWidth);
    resultWidth = qMin(kResultPaneMaxWidth, resultWidth);
    previewWidth = qMax(kPreviewPaneMinWidth, layoutWidth - resultWidth);

    if (previewWidth + resultWidth > layoutWidth) {
        previewWidth = qMax(kPreviewPaneMinWidth, layoutWidth - kResultPaneMinWidth);
        resultWidth = qMax(kResultPaneMinWidth, layoutWidth - previewWidth);
    }

    ui_->previewCardFrame->setMinimumWidth(previewWidth);
    ui_->previewCardFrame->setMaximumWidth(previewWidth);
    ui_->resultPanelFrame->setMinimumWidth(resultWidth);
    ui_->resultPanelFrame->setMaximumWidth(resultWidth);
}

void MainWindow::connectSignals()
{
    connect(ui_->uploadButton, &QPushButton::clicked, this, &MainWindow::importImage);
    connect(ui_->startButton, &QPushButton::clicked, this, &MainWindow::startDetection);
    connect(ui_->exportButton, &QPushButton::clicked, this, &MainWindow::exportResults);
    connect(themeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::handleThemeSelectionChanged);
    connect(serviceClient_, &AlgorithmServiceClient::recognitionReady, this, &MainWindow::handleRecognitionReady);
    connect(serviceClient_, &AlgorithmServiceClient::serviceStateChanged, this, &MainWindow::handleServiceStateChanged);
    connect(serviceClient_, &AlgorithmServiceClient::requestFailed, this, &MainWindow::handleDetectionFailed);
}

void MainWindow::applyControlState(bool running)
{
    detectionRunning_ = running;
    ui_->uploadButton->setEnabled(!running);
    ui_->startButton->setEnabled(!running && hasLoadedImage());
    ui_->exportButton->setEnabled(!running);
}

void MainWindow::applyTheme(const QString& themeId, bool persist)
{
    const QString styleSheet = loadThemeStyleSheet(themeId);
    if (styleSheet.isEmpty()) {
        return;
    }

    if (auto* application = qobject_cast<QApplication*>(QApplication::instance()); application != nullptr) {
        application->setStyleSheet(styleSheet);
    }

    if (persist) {
        QSettings settings;
        settings.setValue(QString::fromLatin1(kThemeSettingsKey), themeId);
    }
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

bool MainWindow::validateImportedImage(const QString& filePath, QString* errorMessage, QImage* image) const
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("所选路径不是有效图片文件。");
        }
        return false;
    }

    if (fileInfo.size() <= 0) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("图片文件为空，无法检测。");
        }
        return false;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    const QByteArray format = reader.format().toLower();
    if (!reader.canRead() || !kSupportedImageFormats.contains(format)) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("仅支持 PNG/JPG/JPEG/BMP 格式的静态图片。");
        }
        return false;
    }

    const QSize imageSize = reader.size();
    if (!imageSize.isValid()
        || imageSize.width() < kMinImageWidth
        || imageSize.height() < kMinImageHeight
        || imageSize.width() > kMaxImageWidth
        || imageSize.height() > kMaxImageHeight) {
        if (errorMessage != nullptr) {
            *errorMessage =
                tr("图片尺寸不合法，需介于 %1x%2 和 %3x%4 之间。")
                    .arg(kMinImageWidth)
                    .arg(kMinImageHeight)
                    .arg(kMaxImageWidth)
                    .arg(kMaxImageHeight);
        }
        return false;
    }

    const QImage loadedImage = reader.read();
    if (loadedImage.isNull()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("图片内容不完整或已损坏，无法解析。");
        }
        return false;
    }

    if (image != nullptr) {
        *image = loadedImage;
    }
    return true;
}

QString MainWindow::buildImageId(const QString& filePath) const
{
    return QString("image-%1-%2")
        .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz"))
        .arg(QFileInfo(filePath).completeBaseName());
}

QString MainWindow::formatConfidence(double confidence)
{
    return QString("%1%").arg(QString::number(confidence * 100.0, 'f', 1));
}

void MainWindow::startDetection()
{
    QString errorMessage;
    QImage validatedImage;
    if (!validateImportedImage(importedImagePath_, &errorMessage, &validatedImage)) {
        importedImage_ = QImage();
        importedImagePath_.clear();
        importedImageId_.clear();
        imagePreview_->setImage(QImage());
        ui_->statusValueLabel->setText(tr("待导入图片"));
        ui_->sourceValueLabel->setText("--");
        applyControlState(false);
        QMessageBox::warning(this, tr("图片校验失败"), errorMessage);
        return;
    }

    importedImage_ = validatedImage;
    imagePreview_->setImage(importedImage_);
    importedImageId_ = buildImageId(importedImagePath_);
    ui_->sourceValueLabel->setText(importedImageId_);

    if (!serviceClient_->submitImage(importedImagePath_, importedImageId_)) {
        applyControlState(false);
        return;
    }

    ui_->statusValueLabel->setText(tr("正在上传图片并等待检测结果"));
    applyControlState(true);
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

    QString errorMessage;
    if (!loadImageFromFile(filePath, &errorMessage)) {
        QMessageBox::warning(this, tr("图像加载失败"), errorMessage);
    }
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

void MainWindow::handleRecognitionReady(const RecognitionRecord& record)
{
    recognitionHistory_.prepend(record);
    refreshResultPanel(record);
    appendHistoryRow(record);
    ui_->statusValueLabel->setText(tr("检测完成"));
    applyControlState(false);
}

void MainWindow::handleServiceStateChanged(const QString& statusText)
{
    ui_->backendStatusValueLabel->setText(statusText);
}

void MainWindow::handleDetectionFailed(const QString& errorMessage)
{
    ui_->statusValueLabel->setText(tr("检测失败"));
    applyControlState(false);
    QMessageBox::warning(this, tr("后端检测失败"), errorMessage);
}

void MainWindow::handleThemeSelectionChanged(int index)
{
    if (themeComboBox_ == nullptr || index < 0) {
        return;
    }

    applyTheme(themeComboBox_->itemData(index).toString());
}
