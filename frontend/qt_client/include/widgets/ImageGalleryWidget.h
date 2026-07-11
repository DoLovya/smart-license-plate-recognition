#pragma once

#include <QFrame>
#include <QString>
#include <QVector>

class QLabel;
class QListView;
class QModelIndex;

struct ImageGalleryEntry
{
    QString filePath;
    QString displayText;
    QString toolTip;
};

class ImageGalleryWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit ImageGalleryWidget(QWidget* parent = nullptr);

    void setEntries(const QVector<ImageGalleryEntry>& entries);
    void clearEntries();
    void updateEntry(const ImageGalleryEntry& entry);
    void setCurrentImage(const QString& filePath);
    void setGalleryEnabled(bool enabled);
    int imageCount() const;

signals:
    void imageSelected(const QString& filePath);

private slots:
    void handleCurrentIndexChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    QLabel* titleLabel_ = nullptr;
    QListView* listView_ = nullptr;
};
