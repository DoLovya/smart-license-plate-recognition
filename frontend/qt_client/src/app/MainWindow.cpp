#include "app/MainWindow.h"

#include "api/AlgorithmServiceClient.h"
#include "services/ResultExportService.h"
#include "widgets/ImageGalleryWidget.h"
#include "widgets/ImagePreviewWidget.h"
#include "ui_MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QImageReader>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
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
constexpr qreal kPreviewPaneRatio = 0.70;
constexpr int kPreviewPaneMinWidth = 680;
constexpr int kThumbnailPaneWidth = 190;
constexpr int kResultPaneMinWidth = 320;
constexpr int kResultPaneMaxWidth = 460;
constexpr char kThemeSettingsKey[] = "ui/theme";
constexpr char kImageDirectorySettingsKey[] = "input/image_directory";

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

QString themeIdForIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(std::size(kThemeOptions))) {
        return QString::fromLatin1(kThemeOptions[0].id);
    }

    return QString::fromLatin1(kThemeOptions[index].id);
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

    clearDirectorySelection();
    inputMode_ = InputMode::SingleImage;
    populateImageList({filePath});
    applyCurrentImage(filePath, image);
    selectImageListItem(filePath);
    ui_->statusValueLabel->setText(
        tr("图片已就绪: %1 (%2x%3)")
            .arg(QFileInfo(filePath).fileName())
            .arg(importedImage_.width())
            .arg(importedImage_.height()));
    updateSourceSummary();
    applyControlState(false);
    return true;
}

bool MainWindow::loadImagesFromDirectory(const QString& directoryPath, QString* errorMessage)
{
    const QFileInfo directoryInfo(directoryPath);
    if (!directoryInfo.exists() || !directoryInfo.isDir()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("所选路径不是有效图像文件夹。");
        }
        return false;
    }

    const QStringList imagePaths = collectDirectoryImages(directoryInfo.absoluteFilePath());
    if (imagePaths.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("所选文件夹中没有可检测的 PNG/JPG/JPEG/BMP 图片。");
        }
        return false;
    }

    clearCurrentImage();
    clearDirectorySelection();
    configuredDirectoryPath_ = directoryInfo.absoluteFilePath();
    configuredDirectoryImages_ = imagePaths;
    inputMode_ = InputMode::ImageDirectory;
    populateImageList(configuredDirectoryImages_);
    activeDirectoryIndex_ = -1;
    directorySuccessCount_ = 0;
    directoryFailureCount_ = 0;

    if (!previewFirstDirectoryImage(errorMessage)) {
        clearDirectorySelection();
        inputMode_ = InputMode::None;
        updateSourceSummary();
        applyControlState(false);
        return false;
    }

    ui_->statusValueLabel->setText(tr("文件夹已就绪: 共 %1 张图片").arg(configuredDirectoryImages_.size()));
    updateSourceSummary();
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
    resize(1480, 860);
    setMinimumSize(1280, 760);

    auto* menuBar = new QMenuBar(this);
    menuBar->setObjectName("mainMenuBar");
    auto* viewMenu = menuBar->addMenu(tr("界面"));
    auto* themeMenu = viewMenu->addMenu(tr("切换样式"));

    themeActionGroup_ = new QActionGroup(this);
    themeActionGroup_->setExclusive(true);
    for (const ThemeOption& option : kThemeOptions) {
        auto* action = themeMenu->addAction(tr(option.displayName));
        action->setCheckable(true);
        action->setData(QString::fromLatin1(option.id));
        themeActionGroup_->addAction(action);
    }
    ui_->rootLayout->setMenuBar(menuBar);

    ui_->subtitleLabel->hide();
    ui_->previewSectionHintLabel->hide();
    ui_->resultSectionHintLabel->hide();

    imagePreview_ = new ImagePreviewWidget(ui_->previewSurfaceContainer);
    imagePreview_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui_->previewSurfaceLayout->addWidget(imagePreview_);

    imageGalleryWidget_ = new ImageGalleryWidget(ui_->imageListFrame);
    imageGalleryWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ui_->imageListFrame->layout()->addWidget(imageGalleryWidget_);

    ui_->contentLayout->setStretch(0, 5);
    ui_->contentLayout->setStretch(1, 0);
    ui_->contentLayout->setStretch(2, 2);
    ui_->previewCardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ui_->imageListFrame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui_->resultPanelFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    ui_->resultTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui_->resultTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui_->resultTableWidget->verticalHeader()->setVisible(false);
    ui_->resultTableWidget->setRowCount(0);

    ui_->statusValueLabel->setText(tr("待导入图片"));
    ui_->backendStatusValueLabel->setText(tr("待连接"));
    resetCurrentResultPanel();
    ui_->sourceModeValueLabel->setText(tr("未配置"));
    ui_->directoryValueLabel->setText("--");
    ui_->batchProgressValueLabel->setText("--");
    ui_->directoryValueLabel->setToolTip(QString());
    applyControlState(false);
    updateSourceSummary();
    updateContentPaneWidths();

    const QSettings settings;
    const QString themeId =
        settings.value(QString::fromLatin1(kThemeSettingsKey), QString::fromLatin1(kThemeOptions[0].id))
            .toString();
    if (themeActionGroup_ != nullptr) {
        const int themeIndex = themeIndexForId(themeId);
        if (QAction* checkedAction = themeActionGroup_->actions().value(themeIndex, nullptr); checkedAction != nullptr) {
            const QSignalBlocker blocker(themeActionGroup_);
            checkedAction->setChecked(true);
        }
    }
    applyTheme(themeIdForIndex(themeIndexForId(themeId)), false);
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

    const int layoutWidth = contentGeometry.width() - (ui_->contentLayout->spacing() * 2);
    if (layoutWidth <= 0) {
        return;
    }

    const int thumbnailWidth = kThumbnailPaneWidth;
    int previewWidth = qRound((layoutWidth - thumbnailWidth) * kPreviewPaneRatio);
    int resultWidth = layoutWidth - thumbnailWidth - previewWidth;

    resultWidth = qMax(kResultPaneMinWidth, resultWidth);
    resultWidth = qMin(kResultPaneMaxWidth, resultWidth);
    previewWidth = qMax(kPreviewPaneMinWidth, layoutWidth - thumbnailWidth - resultWidth);

    if (previewWidth + thumbnailWidth + resultWidth > layoutWidth) {
        previewWidth = qMax(kPreviewPaneMinWidth, layoutWidth - thumbnailWidth - kResultPaneMinWidth);
        resultWidth = qMax(kResultPaneMinWidth, layoutWidth - thumbnailWidth - previewWidth);
    }

    ui_->previewCardFrame->setMinimumWidth(previewWidth);
    ui_->previewCardFrame->setMaximumWidth(previewWidth);
    ui_->imageListFrame->setMinimumWidth(thumbnailWidth);
    ui_->imageListFrame->setMaximumWidth(thumbnailWidth);
    ui_->resultPanelFrame->setMinimumWidth(resultWidth);
    ui_->resultPanelFrame->setMaximumWidth(resultWidth);
}

void MainWindow::connectSignals()
{
    connect(ui_->uploadButton, &QPushButton::clicked, this, &MainWindow::importImage);
    connect(ui_->folderButton, &QPushButton::clicked, this, &MainWindow::importImageDirectory);
    connect(ui_->startButton, &QPushButton::clicked, this, &MainWindow::startDetection);
    connect(ui_->stopButton, &QPushButton::clicked, this, &MainWindow::stopDetection);
    connect(ui_->exportButton, &QPushButton::clicked, this, &MainWindow::exportResults);
    connect(imageGalleryWidget_, &ImageGalleryWidget::imageSelected, this, &MainWindow::handleImageListSelectionChanged);
    connect(themeActionGroup_, &QActionGroup::triggered, this, &MainWindow::handleThemeActionTriggered);
    connect(serviceClient_, &AlgorithmServiceClient::recognitionReady, this, &MainWindow::handleRecognitionReady);
    connect(serviceClient_, &AlgorithmServiceClient::serviceStateChanged, this, &MainWindow::handleServiceStateChanged);
    connect(serviceClient_, &AlgorithmServiceClient::requestFailed, this, &MainWindow::handleDetectionFailed);
}

void MainWindow::applyControlState(bool running)
{
    detectionRunning_ = running;
    const bool readyToStart =
        (inputMode_ == InputMode::SingleImage && hasLoadedImage())
        || (inputMode_ == InputMode::ImageDirectory && hasConfiguredDirectory());
    ui_->uploadButton->setEnabled(!running);
    ui_->folderButton->setEnabled(!running);
    ui_->startButton->setEnabled(!running && readyToStart);
    ui_->stopButton->setEnabled(running);
    ui_->exportButton->setEnabled(!running);
    imageGalleryWidget_->setGalleryEnabled(!running && imageGalleryWidget_->imageCount() > 0);
}

void MainWindow::applyTheme(const QString& themeId, bool persist)
{
    const QString styleSheet = loadThemeStyleSheet(themeId);
    if (styleSheet.isEmpty()) {
        return;
    }

    if (auto* application = qobject_cast<QApplication*>(QApplication::instance()); application != nullptr) {
        application->setProperty("themeId", themeId);
        application->setStyleSheet(styleSheet);
    }

    if (imagePreview_ != nullptr) {
        imagePreview_->update();
    }
    if (imageGalleryWidget_ != nullptr) {
        imageGalleryWidget_->update();
    }

    if (persist) {
        QSettings settings;
        settings.setValue(QString::fromLatin1(kThemeSettingsKey), themeId);
    }
}

void MainWindow::resetCurrentResultPanel()
{
    ui_->currentPlateValueLabel->setText("--");
    ui_->confidenceValueLabel->setText("--");
    ui_->timestampValueLabel->setText("--");
    ui_->sourceValueLabel->setText("--");
}

void MainWindow::updateSourceSummary()
{
    QString sourceModeText = tr("未配置");
    QString directoryText = QStringLiteral("--");
    QString progressText = QStringLiteral("--");

    switch (inputMode_) {
    case InputMode::SingleImage:
        sourceModeText = tr("单图");
        progressText = hasLoadedImage() ? tr("1 / 1") : QStringLiteral("--");
        break;
    case InputMode::ImageDirectory: {
        sourceModeText = tr("文件夹");
        directoryText = QDir::toNativeSeparators(configuredDirectoryPath_);
        const int totalCount = configuredDirectoryImages_.size();
        const int processedCount = directorySuccessCount_ + directoryFailureCount_;
        if (detectionRunning_ && activeDirectoryIndex_ >= 0 && totalCount > 0) {
            progressText = tr("%1 / %2 (成功 %3, 失败 %4)")
                               .arg(qMin(activeDirectoryIndex_ + 1, totalCount))
                               .arg(totalCount)
                               .arg(directorySuccessCount_)
                               .arg(directoryFailureCount_);
        } else if (processedCount > 0 && totalCount > 0) {
            progressText = tr("完成 %1 / %2 (成功 %3, 失败 %4)")
                               .arg(processedCount)
                               .arg(totalCount)
                               .arg(directorySuccessCount_)
                               .arg(directoryFailureCount_);
        } else if (totalCount > 0) {
            progressText = tr("待处理 %1 张").arg(totalCount);
        }
        break;
    }
    case InputMode::None:
        break;
    }

    ui_->sourceModeValueLabel->setText(sourceModeText);
    ui_->directoryValueLabel->setText(directoryText);
    ui_->directoryValueLabel->setToolTip(directoryText == QStringLiteral("--") ? QString() : directoryText);
    ui_->batchProgressValueLabel->setText(progressText);
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

void MainWindow::clearCurrentImage()
{
    importedImagePath_.clear();
    importedImageId_.clear();
    importedImage_ = QImage();
    imagePreview_->setImage(QImage());
    imagePreview_->setRecognitionRecord(nullptr);
    resetCurrentResultPanel();
}

void MainWindow::applyCurrentImage(const QString& filePath, const QImage& image)
{
    importedImagePath_ = filePath;
    importedImageId_ = ensureImageId(filePath);
    importedImage_ = image;
    imagePreview_->setImage(importedImage_);
    selectImageListItem(filePath);
    syncResultPanelForCurrentImage();
}

void MainWindow::clearDirectorySelection()
{
    configuredDirectoryPath_.clear();
    configuredDirectoryImages_.clear();
    recognitionRecordsByPath_.clear();
    imageIdsByPath_.clear();
    pendingRecognitionImagePath_.clear();
    activeDirectoryIndex_ = -1;
    directorySuccessCount_ = 0;
    directoryFailureCount_ = 0;
    populateImageList({});
}

bool MainWindow::hasConfiguredDirectory() const
{
    return !configuredDirectoryPath_.isEmpty() && !configuredDirectoryImages_.isEmpty();
}

QStringList MainWindow::collectDirectoryImages(const QString& directoryPath) const
{
    const QDir directory(directoryPath);
    const QFileInfoList fileInfos =
        directory.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);

    QStringList imagePaths;
    imagePaths.reserve(fileInfos.size());
    for (const QFileInfo& fileInfo : fileInfos) {
        if (kSupportedImageFormats.contains(fileInfo.suffix().toLower().toLatin1())) {
            imagePaths.push_back(fileInfo.absoluteFilePath());
        }
    }
    return imagePaths;
}

void MainWindow::populateImageList(const QStringList& imagePaths)
{
    QVector<ImageGalleryEntry> entries;
    entries.reserve(imagePaths.size());
    for (const QString& imagePath : imagePaths) {
        entries.push_back(ImageGalleryEntry {
            imagePath,
            buildImageListText(imagePath),
            QDir::toNativeSeparators(imagePath),
        });
    }
    imageGalleryWidget_->setEntries(entries);
    imageGalleryWidget_->setGalleryEnabled(!detectionRunning_ && imageGalleryWidget_->imageCount() > 0);
}

void MainWindow::selectImageListItem(const QString& filePath)
{
    imageGalleryWidget_->setCurrentImage(filePath);
}

void MainWindow::updateImageListItem(const QString& filePath)
{
    const QString tooltip =
        recognitionRecordsByPath_.contains(filePath)
            ? tr("%1\n识别结果: %2")
                  .arg(QDir::toNativeSeparators(filePath), recognitionRecordsByPath_.value(filePath).plateText)
            : QDir::toNativeSeparators(filePath);

    imageGalleryWidget_->updateEntry(ImageGalleryEntry {
        filePath,
        buildImageListText(filePath),
        tooltip,
    });
}

void MainWindow::syncResultPanelForCurrentImage()
{
    if (importedImagePath_.isEmpty()) {
        imagePreview_->setRecognitionRecord(nullptr);
        resetCurrentResultPanel();
        return;
    }

    if (recognitionRecordsByPath_.contains(importedImagePath_)) {
        const RecognitionRecord& record = recognitionRecordsByPath_.value(importedImagePath_);
        imagePreview_->setRecognitionRecord(&record);
        refreshResultPanel(record);
        return;
    }

    imagePreview_->setRecognitionRecord(nullptr);
    ui_->currentPlateValueLabel->setText("--");
    ui_->confidenceValueLabel->setText("--");
    ui_->timestampValueLabel->setText("--");
    ui_->sourceValueLabel->setText(importedImageId_.isEmpty() ? ensureImageId(importedImagePath_) : importedImageId_);
}

QString MainWindow::ensureImageId(const QString& filePath)
{
    const auto it = imageIdsByPath_.constFind(filePath);
    if (it != imageIdsByPath_.constEnd()) {
        return it.value();
    }

    const QString imageId = buildImageId(filePath);
    imageIdsByPath_.insert(filePath, imageId);
    return imageId;
}

QString MainWindow::buildImageListText(const QString& filePath)
{
    return ensureImageId(filePath);
}

bool MainWindow::previewFirstDirectoryImage(QString* errorMessage)
{
    for (const QString& imagePath : configuredDirectoryImages_) {
        QImage image;
        if (validateImportedImage(imagePath, nullptr, &image)) {
            applyCurrentImage(imagePath, image);
            return true;
        }
    }

    clearCurrentImage();
    if (errorMessage != nullptr) {
        *errorMessage = tr("文件夹中的图片均无法通过格式或尺寸校验。");
    }
    return false;
}

void MainWindow::startSingleImageDetection()
{
    stopRequested_ = false;
    QString errorMessage;
    QImage validatedImage;
    if (!validateImportedImage(importedImagePath_, &errorMessage, &validatedImage)) {
        clearCurrentImage();
        inputMode_ = InputMode::None;
        ui_->statusValueLabel->setText(tr("待导入图片"));
        updateSourceSummary();
        applyControlState(false);
        QMessageBox::warning(this, tr("图片校验失败"), errorMessage);
        return;
    }

    applyCurrentImage(importedImagePath_, validatedImage);
    pendingRecognitionImagePath_ = importedImagePath_;

    if (!serviceClient_->submitImage(importedImagePath_, importedImageId_)) {
        pendingRecognitionImagePath_.clear();
        applyControlState(false);
        return;
    }

    ui_->statusValueLabel->setText(tr("正在上传图片并等待检测结果"));
    updateSourceSummary();
    applyControlState(true);
}

void MainWindow::startDirectoryDetection()
{
    if (!hasConfiguredDirectory()) {
        QMessageBox::information(this, tr("未配置文件夹"), tr("请先选择包含待检测图片的文件夹。"));
        return;
    }

    stopRequested_ = false;
    activeDirectoryIndex_ = 0;
    directorySuccessCount_ = 0;
    directoryFailureCount_ = 0;
    ui_->statusValueLabel->setText(tr("准备批量检测"));
    updateSourceSummary();
    applyControlState(true);
    submitNextDirectoryImage();
}

void MainWindow::submitNextDirectoryImage()
{
    while (activeDirectoryIndex_ >= 0 && activeDirectoryIndex_ < configuredDirectoryImages_.size()) {
        const QString filePath = configuredDirectoryImages_.at(activeDirectoryIndex_);

        QString errorMessage;
        QImage validatedImage;
        if (!validateImportedImage(filePath, &errorMessage, &validatedImage)) {
            ++directoryFailureCount_;
            ++activeDirectoryIndex_;
            continue;
        }

        applyCurrentImage(filePath, validatedImage);
        ui_->statusValueLabel->setText(
            tr("批量检测中 %1/%2: %3")
                .arg(activeDirectoryIndex_ + 1)
                .arg(configuredDirectoryImages_.size())
                .arg(QFileInfo(filePath).fileName()));
        updateSourceSummary();

        pendingRecognitionImagePath_ = importedImagePath_;
        if (!serviceClient_->submitImage(importedImagePath_, importedImageId_)) {
            pendingRecognitionImagePath_.clear();
            return;
        }

        applyControlState(true);
        return;
    }

    finishDirectoryDetection();
}

void MainWindow::finishDirectoryDetection()
{
    pendingRecognitionImagePath_.clear();
    activeDirectoryIndex_ = -1;
    applyControlState(false);
    if (stopRequested_) {
        ui_->backendStatusValueLabel->setText(tr("检测已停止"));
        ui_->statusValueLabel->setText(
            tr("批量检测已停止: 成功 %1 张, 失败 %2 张")
                .arg(directorySuccessCount_)
                .arg(directoryFailureCount_));
        stopRequested_ = false;
    } else {
        ui_->backendStatusValueLabel->setText(tr("批量处理完成"));
        ui_->statusValueLabel->setText(
            tr("批量检测完成: 成功 %1 张, 失败 %2 张")
                .arg(directorySuccessCount_)
                .arg(directoryFailureCount_));
    }
    updateSourceSummary();
}

void MainWindow::stopDetectionInternal()
{
    pendingRecognitionImagePath_.clear();
    applyControlState(false);
    ui_->backendStatusValueLabel->setText(tr("检测已停止"));
    ui_->statusValueLabel->setText(tr("检测已停止"));
    updateSourceSummary();
    stopRequested_ = false;
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
    Q_UNUSED(filePath);

    static constexpr char kIdAlphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    QString imageId = QStringLiteral("img_");
    imageId.reserve(16);
    for (int index = 0; index < 12; ++index) {
        const int alphabetIndex = QRandomGenerator::global()->bounded(static_cast<int>(sizeof(kIdAlphabet) - 1));
        imageId.append(QChar::fromLatin1(kIdAlphabet[alphabetIndex]));
    }
    return imageId;
}

QString MainWindow::formatConfidence(double confidence)
{
    return QString("%1%").arg(QString::number(confidence * 100.0, 'f', 1));
}

void MainWindow::startDetection()
{
    if (inputMode_ == InputMode::ImageDirectory) {
        startDirectoryDetection();
    } else {
        startSingleImageDetection();
    }
}

void MainWindow::stopDetection()
{
    if (!detectionRunning_) {
        return;
    }

    stopRequested_ = true;
    ui_->statusValueLabel->setText(tr("正在停止检测"));
    ui_->backendStatusValueLabel->setText(tr("正在取消检测"));
    updateSourceSummary();
    serviceClient_->cancelPendingRequests();
}

void MainWindow::importImage()
{
    const QString initialDirectory = !configuredDirectoryPath_.isEmpty()
                                         ? configuredDirectoryPath_
                                         : QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        initialDirectory,
        tr("Images (*.png *.jpg *.jpeg *.bmp)"));

    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!loadImageFromFile(filePath, &errorMessage)) {
        QMessageBox::warning(this, tr("图像加载失败"), errorMessage);
        return;
    }

    QSettings settings;
    settings.setValue(QString::fromLatin1(kImageDirectorySettingsKey), QFileInfo(filePath).absolutePath());
}

void MainWindow::importImageDirectory()
{
    QSettings settings;
    const QString initialDirectory =
        settings.value(
                    QString::fromLatin1(kImageDirectorySettingsKey),
                    QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))
            .toString();

    const QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        tr("选择图像文件夹"),
        initialDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (directoryPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!loadImagesFromDirectory(directoryPath, &errorMessage)) {
        QMessageBox::warning(this, tr("文件夹加载失败"), errorMessage);
        return;
    }

    settings.setValue(QString::fromLatin1(kImageDirectorySettingsKey), directoryPath);
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
    const QString completedImagePath = pendingRecognitionImagePath_;
    pendingRecognitionImagePath_.clear();

    if (stopRequested_) {
        stopDetectionInternal();
        return;
    }

    if (!completedImagePath.isEmpty()) {
        recognitionRecordsByPath_.insert(completedImagePath, record);
        updateImageListItem(completedImagePath);
    }

    recognitionHistory_.prepend(record);
    refreshResultPanel(record);
    imagePreview_->setRecognitionRecord(&record);
    appendHistoryRow(record);

    if (inputMode_ == InputMode::ImageDirectory && detectionRunning_) {
        ++directorySuccessCount_;
        ++activeDirectoryIndex_;
        updateSourceSummary();

        if (activeDirectoryIndex_ < configuredDirectoryImages_.size()) {
            submitNextDirectoryImage();
            return;
        }

        finishDirectoryDetection();
        return;
    }

    ui_->statusValueLabel->setText(tr("检测完成"));
    updateSourceSummary();
    applyControlState(false);
}

void MainWindow::handleServiceStateChanged(const QString& statusText)
{
    ui_->backendStatusValueLabel->setText(statusText);
}

void MainWindow::handleDetectionFailed(const QString& errorMessage)
{
    pendingRecognitionImagePath_.clear();

    if (stopRequested_) {
        if (inputMode_ == InputMode::ImageDirectory) {
            finishDirectoryDetection();
        } else {
            stopDetectionInternal();
        }
        return;
    }

    if (inputMode_ == InputMode::ImageDirectory && detectionRunning_) {
        ++directoryFailureCount_;
        ++activeDirectoryIndex_;
        updateSourceSummary();

        if (activeDirectoryIndex_ < configuredDirectoryImages_.size()) {
            submitNextDirectoryImage();
            return;
        }

        finishDirectoryDetection();
        return;
    }

    ui_->statusValueLabel->setText(tr("检测失败"));
    applyControlState(false);
    QMessageBox::warning(this, tr("后端检测失败"), errorMessage);
}

void MainWindow::handleThemeActionTriggered()
{
    if (themeActionGroup_ == nullptr) {
        return;
    }

    QAction* checkedAction = themeActionGroup_->checkedAction();
    if (checkedAction == nullptr) {
        return;
    }

    applyTheme(checkedAction->data().toString());
}

void MainWindow::handleImageListSelectionChanged(const QString& filePath)
{
    if (detectionRunning_) {
        return;
    }

    if (filePath.isEmpty() || filePath == importedImagePath_) {
        return;
    }

    QImage image;
    if (!validateImportedImage(filePath, nullptr, &image)) {
        return;
    }

    applyCurrentImage(filePath, image);
}
