#include <QMenuBar>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSharedPointer>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtTest>

#include "app/MainWindow.h"
#include "services/ResultExportService.h"

class TestRecognitionServer final : public QObject
{
    Q_OBJECT

public:
    explicit TestRecognitionServer(int responseDelayMs = 0) : responseDelayMs_(responseDelayMs)
    {
        connect(&server_, &QTcpServer::newConnection, this, &TestRecognitionServer::handleNewConnection);
    }

    bool start()
    {
        return server_.listen(QHostAddress::LocalHost, 0);
    }

    QUrl endpoint() const
    {
        return QUrl(QString("http://127.0.0.1:%1/api/v1/recognize").arg(server_.serverPort()));
    }

    bool requestReceived() const
    {
        return requestReceived_;
    }

    int requestCount() const
    {
        return requestCount_;
    }

    QString requestPath() const
    {
        return requestPath_;
    }

    QByteArray rawRequest() const
    {
        return rawRequest_;
    }

private slots:
    void handleNewConnection()
    {
        QTcpSocket* socket = server_.nextPendingConnection();
        if (socket == nullptr) {
            return;
        }

        const auto buffer = QSharedPointer<QByteArray>::create();
        const auto expectedBodyLength = QSharedPointer<qint64>::create(-1);

        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer, expectedBodyLength]() {
            buffer->append(socket->readAll());

            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }

            if (*expectedBodyLength < 0) {
                const QList<QByteArray> headerLines = buffer->left(headerEnd).split('\n');
                if (!headerLines.isEmpty()) {
                    const QList<QByteArray> requestLineParts = headerLines.first().trimmed().split(' ');
                    if (requestLineParts.size() >= 2) {
                        requestPath_ = QString::fromLatin1(requestLineParts.at(1));
                    }
                }

                for (const QByteArray& rawLine : headerLines) {
                    const QByteArray line = rawLine.trimmed();
                    if (line.toLower().startsWith("content-length:")) {
                        *expectedBodyLength = line.mid(QByteArray("Content-Length:").size()).trimmed().toLongLong();
                        break;
                    }
                }

                if (*expectedBodyLength < 0) {
                    *expectedBodyLength = 0;
                }
            }

            const qint64 currentBodyLength = buffer->size() - (headerEnd + 4);
            if (currentBodyLength < *expectedBodyLength) {
                return;
            }

            rawRequest_ = *buffer;
            requestReceived_ = true;
            ++requestCount_;

            const QByteArray responseBody =
                "{\"image_id\":\"UnitTestImage\",\"plate_text\":\"沪A12345\",\"confidence\":0.98,"
                "\"boxes\":[],\"elapsed_ms\":12.3}";
            const QByteArray response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + QByteArray::number(responseBody.size()) + "\r\n"
                "Connection: close\r\n\r\n" + responseBody;

            const auto sendResponse = [socket, response]() {
                if (socket->state() == QAbstractSocket::ConnectedState) {
                    socket->write(response);
                    socket->flush();
                }
                socket->disconnectFromHost();
            };

            if (responseDelayMs_ > 0) {
                QTimer::singleShot(responseDelayMs_, socket, sendResponse);
            } else {
                sendResponse();
            }
        });

        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }

private:
    QTcpServer server_;
    bool requestReceived_ = false;
    int requestCount_ = 0;
    QString requestPath_;
    QByteArray rawRequest_;
    int responseDelayMs_ = 0;
};

class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void shouldInitializeCoreControls();
    void shouldRejectInvalidImportedImage();
    void shouldTriggerBackendRecognitionAfterLoadingImage();
    void shouldTriggerBackendRecognitionForConfiguredDirectory();
    void shouldStopDetectionForConfiguredDirectory();
    void shouldExportRecognitionRecords();
};

void MainWindowTest::initTestCase()
{
    qRegisterMetaType<RecognitionRecord>("RecognitionRecord");
}

void MainWindowTest::shouldInitializeCoreControls()
{
    MainWindow window;

    auto* uploadButton = window.findChild<QPushButton*>("uploadButton");
    auto* folderButton = window.findChild<QPushButton*>("folderButton");
    auto* startButton = window.findChild<QPushButton*>("startButton");
    auto* stopButton = window.findChild<QPushButton*>("stopButton");
    auto* exportButton = window.findChild<QPushButton*>("exportButton");
    auto* menuBar = window.findChild<QMenuBar*>("mainMenuBar");

    QVERIFY(uploadButton != nullptr);
    QVERIFY(folderButton != nullptr);
    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(exportButton != nullptr);
    QVERIFY(menuBar != nullptr);
    QVERIFY(window.findChild<QObject*>("themeComboBox") == nullptr);
    QVERIFY(menuBar->actions().size() == 1);
    QVERIFY(menuBar->actions().constFirst()->menu() != nullptr);
    QVERIFY(menuBar->actions().constFirst()->menu()->actions().constFirst()->menu() != nullptr);
    QVERIFY(menuBar->actions().constFirst()->menu()->actions().constFirst()->menu()->actions().size() == 3);
    QVERIFY(window.findChild<QTableWidget*>("resultTableWidget") != nullptr);
    QVERIFY(!window.hasLoadedImage());
    QVERIFY(!startButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
}

void MainWindowTest::shouldRejectInvalidImportedImage()
{
    MainWindow window;
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.filePath("invalid.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("not-an-image");
    file.close();

    QString errorMessage;
    QVERIFY(!window.loadImageFromFile(filePath, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(!window.hasLoadedImage());
    QVERIFY(!window.findChild<QPushButton*>("startButton")->isEnabled());
}

void MainWindowTest::shouldTriggerBackendRecognitionAfterLoadingImage()
{
    TestRecognitionServer server;
    QVERIFY(server.start());

    MainWindow window;
    window.setServiceEndpoint(server.endpoint());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.filePath("plate.png");
    QImage image(1280, 720, QImage::Format_RGB32);
    image.fill(Qt::black);
    QVERIFY(image.save(filePath, "PNG"));

    QString errorMessage;
    QVERIFY(window.loadImageFromFile(filePath, &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    auto* startButton = window.findChild<QPushButton*>("startButton");
    QVERIFY(startButton != nullptr);
    QVERIFY(startButton->isEnabled());

    QTest::mouseClick(startButton, Qt::LeftButton);

    QTRY_VERIFY(server.requestReceived());
    QTRY_COMPARE(window.resultCount(), 1);
    QCOMPARE(server.requestPath(), QString("/api/v1/recognize"));
    QVERIFY(server.rawRequest().contains("form-data; name=\"file\"; filename=\"image-"));
    QCOMPARE(window.latestPlateText(), QString::fromUtf8("沪A12345"));

    auto* backendStatusLabel = window.findChild<QLabel*>("backendStatusValueLabel");
    QVERIFY(backendStatusLabel != nullptr);
    QCOMPARE(backendStatusLabel->text(), QString::fromUtf8("检测完成"));
}

void MainWindowTest::shouldTriggerBackendRecognitionForConfiguredDirectory()
{
    TestRecognitionServer server;
    QVERIFY(server.start());

    MainWindow window;
    window.setServiceEndpoint(server.endpoint());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QImage imageA(1280, 720, QImage::Format_RGB32);
    imageA.fill(Qt::darkBlue);
    QVERIFY(imageA.save(tempDir.filePath("plate_a.png"), "PNG"));

    QImage imageB(960, 540, QImage::Format_RGB32);
    imageB.fill(Qt::darkGreen);
    QVERIFY(imageB.save(tempDir.filePath("plate_b.jpg"), "JPG"));

    QFile ignoredFile(tempDir.filePath("readme.txt"));
    QVERIFY(ignoredFile.open(QIODevice::WriteOnly | QIODevice::Text));
    ignoredFile.write("not-an-image");
    ignoredFile.close();

    QString errorMessage;
    QVERIFY(window.loadImagesFromDirectory(tempDir.path(), &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    auto* startButton = window.findChild<QPushButton*>("startButton");
    auto* statusLabel = window.findChild<QLabel*>("statusValueLabel");
    auto* progressLabel = window.findChild<QLabel*>("batchProgressValueLabel");
    QVERIFY(startButton != nullptr);
    QVERIFY(statusLabel != nullptr);
    QVERIFY(progressLabel != nullptr);
    QVERIFY(startButton->isEnabled());

    QTest::mouseClick(startButton, Qt::LeftButton);

    QTRY_COMPARE(server.requestCount(), 2);
    QTRY_COMPARE(window.resultCount(), 2);
    QTRY_COMPARE(statusLabel->text(), QString::fromUtf8("批量检测完成: 成功 2 张, 失败 0 张"));
    QCOMPARE(progressLabel->text(), QString::fromUtf8("完成 2 / 2 (成功 2, 失败 0)"));
}

void MainWindowTest::shouldStopDetectionForConfiguredDirectory()
{
    TestRecognitionServer server(1500);
    QVERIFY(server.start());

    MainWindow window;
    window.setServiceEndpoint(server.endpoint());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QImage imageA(1280, 720, QImage::Format_RGB32);
    imageA.fill(Qt::darkBlue);
    QVERIFY(imageA.save(tempDir.filePath("plate_a.png"), "PNG"));

    QImage imageB(960, 540, QImage::Format_RGB32);
    imageB.fill(Qt::darkGreen);
    QVERIFY(imageB.save(tempDir.filePath("plate_b.jpg"), "JPG"));

    QString errorMessage;
    QVERIFY(window.loadImagesFromDirectory(tempDir.path(), &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    auto* startButton = window.findChild<QPushButton*>("startButton");
    auto* stopButton = window.findChild<QPushButton*>("stopButton");
    auto* statusLabel = window.findChild<QLabel*>("statusValueLabel");
    auto* backendStatusLabel = window.findChild<QLabel*>("backendStatusValueLabel");
    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(statusLabel != nullptr);
    QVERIFY(backendStatusLabel != nullptr);

    QTest::mouseClick(startButton, Qt::LeftButton);
    QTRY_COMPARE(server.requestCount(), 1);
    QVERIFY(stopButton->isEnabled());

    QTest::mouseClick(stopButton, Qt::LeftButton);

    QTRY_COMPARE(statusLabel->text(), QString::fromUtf8("批量检测已停止: 成功 0 张, 失败 0 张"));
    QCOMPARE(backendStatusLabel->text(), QString::fromUtf8("检测已停止"));
    QCOMPARE(server.requestCount(), 1);
    QCOMPARE(window.resultCount(), 0);
    QVERIFY(!window.isDetectionRunning());
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
