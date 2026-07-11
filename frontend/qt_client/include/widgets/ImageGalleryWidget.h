#pragma once

#include <QColor>
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
    Q_PROPERTY(QColor cardFillColor READ cardFillColor WRITE setCardFillColor)
    Q_PROPERTY(QColor selectedCardFillColor READ selectedCardFillColor WRITE setSelectedCardFillColor)
    Q_PROPERTY(QColor cardBorderColor READ cardBorderColor WRITE setCardBorderColor)
    Q_PROPERTY(QColor selectedCardBorderColor READ selectedCardBorderColor WRITE setSelectedCardBorderColor)
    Q_PROPERTY(QColor cardTextColor READ cardTextColor WRITE setCardTextColor)
    Q_PROPERTY(QColor selectedCardTextColor READ selectedCardTextColor WRITE setSelectedCardTextColor)
    Q_PROPERTY(QColor placeholderFillColor READ placeholderFillColor WRITE setPlaceholderFillColor)
    Q_PROPERTY(QColor placeholderBorderColor READ placeholderBorderColor WRITE setPlaceholderBorderColor)
    Q_PROPERTY(QColor placeholderTextColor READ placeholderTextColor WRITE setPlaceholderTextColor)

public:
    explicit ImageGalleryWidget(QWidget* parent = nullptr);

    void setEntries(const QVector<ImageGalleryEntry>& entries);
    void clearEntries();
    void updateEntry(const ImageGalleryEntry& entry);
    void setCurrentImage(const QString& filePath);
    void setGalleryEnabled(bool enabled);
    int imageCount() const;
    QColor cardFillColor() const;
    void setCardFillColor(const QColor& color);
    QColor selectedCardFillColor() const;
    void setSelectedCardFillColor(const QColor& color);
    QColor cardBorderColor() const;
    void setCardBorderColor(const QColor& color);
    QColor selectedCardBorderColor() const;
    void setSelectedCardBorderColor(const QColor& color);
    QColor cardTextColor() const;
    void setCardTextColor(const QColor& color);
    QColor selectedCardTextColor() const;
    void setSelectedCardTextColor(const QColor& color);
    QColor placeholderFillColor() const;
    void setPlaceholderFillColor(const QColor& color);
    QColor placeholderBorderColor() const;
    void setPlaceholderBorderColor(const QColor& color);
    QColor placeholderTextColor() const;
    void setPlaceholderTextColor(const QColor& color);

signals:
    void imageSelected(const QString& filePath);

private slots:
    void handleCurrentIndexChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    void refreshGalleryColors();

    QLabel* titleLabel_ = nullptr;
    QListView* listView_ = nullptr;
    QColor cardFillColor_ = QColor("#0b1827");
    QColor selectedCardFillColor_ = QColor("#0f2235");
    QColor cardBorderColor_ = QColor("#24374a");
    QColor selectedCardBorderColor_ = QColor("#5bc7ff");
    QColor cardTextColor_ = QColor("#d7e3ee");
    QColor selectedCardTextColor_ = QColor("#ffffff");
    QColor placeholderFillColor_ = QColor("#132235");
    QColor placeholderBorderColor_ = QColor("#5bc7ff");
    QColor placeholderTextColor_ = QColor("#d7e3ee");
};
