#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

#include "../include/MainWindow.h"
#include "../include/ResultExportService.h"

class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void shouldInitializeCoreControls();
    void shouldToggleDetectionControls();
    void shouldUpdateRecognitionPanelAfterSubmittingFrame();
    void shouldExportRecognitionRecords();
};

void MainWindowTest::initTestCase()
{
    qRegisterMetaType<RecognitionRecord>("RecognitionRecord");
    qRegisterMetaType<CameraDevice>("CameraDevice");
}

void MainWindowTest::shouldInitializeCoreControls()
{
    MainWindow window;

    QVERIFY(window.findChild<QPushButton*>("uploadButton") != nullptr);
    QVERIFY(window.findChild<QPushButton*>("startButton") != nullptr);
    QVERIFY(window.findChild<QPushButton*>("stopButton") != nullptr);
    QVERIFY(window.findChild<QComboBox*>("cameraComboBox") != nullptr);
    QVERIFY(window.findChild<QTableWidget*>("resultTableWidget") != nullptr);
    QVERIFY(window.cameraCount() >= 1);
}

void MainWindowTest::shouldToggleDetectionControls()
{
    MainWindow window;

    auto* startButton = window.findChild<QPushButton*>("startButton");
    auto* stopButton = window.findChild<QPushButton*>("stopButton");
    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);

    QTest::mouseClick(startButton, Qt::LeftButton);
    QTRY_VERIFY(window.isDetectionRunning());
    QVERIFY(!startButton->isEnabled());
    QVERIFY(stopButton->isEnabled());

    QTest::mouseClick(stopButton, Qt::LeftButton);
    QTRY_VERIFY(!window.isDetectionRunning());
    QVERIFY(startButton->isEnabled());
}

void MainWindowTest::shouldUpdateRecognitionPanelAfterSubmittingFrame()
{
    MainWindow window;
    QImage frame(1280, 720, QImage::Format_RGB32);
    frame.fill(Qt::black);

    window.submitFrameForRecognition(frame, "UnitTest");

    QTRY_COMPARE(window.resultCount(), 1);
    QVERIFY(!window.latestPlateText().isEmpty());

    auto* currentPlateLabel = window.findChild<QLabel*>("currentPlateValueLabel");
    QVERIFY(currentPlateLabel != nullptr);
    QVERIFY(!currentPlateLabel->text().isEmpty());
}

void MainWindowTest::shouldExportRecognitionRecords()
{
    ResultExportService exporter;
    QVector<RecognitionRecord> records;
    records.push_back(RecognitionRecord {
        "沪A12345",
        0.98,
        QDateTime::fromString("2026-07-07T10:00:00", Qt::ISODate),
        "UnitTest",
        QSize(1280, 720),
    });

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.filePath("recognition.csv");
    QString errorMessage;
    QVERIFY(exporter.exportCsv(filePath, records, &errorMessage));
    QVERIFY2(QFile::exists(filePath), qPrintable(errorMessage));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray content = file.readAll();
    QVERIFY(content.contains("沪A12345"));
    QVERIFY(content.contains("confidence"));
    QVERIFY(content.contains("image_id"));
}

QTEST_MAIN(MainWindowTest)
#include "tst_MainWindow.moc"
