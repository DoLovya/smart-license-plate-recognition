#include "widgets/ImageGalleryWidget.h"

#include <QAbstractItemView>
#include <QAbstractListModel>
#include <QImageReader>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
constexpr int kThumbnailIconSize = 120;
constexpr int kThumbnailGridWidth = 148;
constexpr int kThumbnailGridHeight = 156;
constexpr int kThumbnailSpacing = 8;
constexpr int kThumbnailBatchLoadCount = 3;

enum ImageGalleryRoles
{
    FilePathRole = Qt::UserRole + 1,
    ThumbnailPixmapRole,
};

struct ImageGalleryItemState
{
    ImageGalleryEntry entry;
    QPixmap thumbnail;
    bool thumbnailLoaded = false;
    bool thumbnailQueued = false;
};

class ImageGalleryModel final : public QAbstractListModel
{
public:
    explicit ImageGalleryModel(QObject* parent = nullptr) : QAbstractListModel(parent)
    {
        thumbnailLoadTimer_.setInterval(0);
        connect(&thumbnailLoadTimer_, &QTimer::timeout, this, [this]() { processThumbnailQueue(); });
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : items_.size();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
            return QVariant();
        }

        const ImageGalleryItemState& item = items_.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return item.entry.displayText;
        case Qt::ToolTipRole:
            return item.entry.toolTip;
        case FilePathRole:
            return item.entry.filePath;
        case ThumbnailPixmapRole:
            return item.thumbnailLoaded ? item.thumbnail : placeholderThumbnail();
        default:
            return QVariant();
        }
    }

    void setEntries(const QVector<ImageGalleryEntry>& entries)
    {
        beginResetModel();
        items_.clear();
        items_.reserve(entries.size());
        thumbnailQueue_.clear();
        thumbnailLoadTimer_.stop();

        for (const ImageGalleryEntry& entry : entries) {
            items_.push_back(ImageGalleryItemState {entry, QPixmap(), false, false});
        }

        endResetModel();
    }

    void clearEntries()
    {
        beginResetModel();
        items_.clear();
        thumbnailQueue_.clear();
        thumbnailLoadTimer_.stop();
        endResetModel();
    }

    void updateEntry(const ImageGalleryEntry& entry)
    {
        const int row = rowForFilePath(entry.filePath);
        if (row < 0) {
            return;
        }

        items_[row].entry.displayText = entry.displayText;
        items_[row].entry.toolTip = entry.toolTip;
        const QModelIndex modelIndex = index(row, 0);
        emit dataChanged(modelIndex, modelIndex, {Qt::DisplayRole, Qt::ToolTipRole});
    }

    QModelIndex indexForFilePath(const QString& filePath) const
    {
        const int row = rowForFilePath(filePath);
        return row >= 0 ? index(row, 0) : QModelIndex();
    }

    void requestThumbnail(const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid() || modelIndex.row() < 0 || modelIndex.row() >= items_.size()) {
            return;
        }

        ImageGalleryItemState& item = items_[modelIndex.row()];
        if (item.thumbnailLoaded || item.thumbnailQueued) {
            return;
        }

        item.thumbnailQueued = true;
        thumbnailQueue_.push_back(modelIndex.row());
        if (!thumbnailLoadTimer_.isActive()) {
            thumbnailLoadTimer_.start();
        }
    }

    void setPlaceholderColors(const QColor& fill, const QColor& border, const QColor& text)
    {
        placeholderFillColor_ = fill;
        placeholderBorderColor_ = border;
        placeholderTextColor_ = text;
        placeholderThumbnail_ = QPixmap();
        if (!items_.isEmpty()) {
            emit dataChanged(index(0, 0), index(items_.size() - 1, 0), {ThumbnailPixmapRole});
        }
    }

private:
    int rowForFilePath(const QString& filePath) const
    {
        for (int row = 0; row < items_.size(); ++row) {
            if (items_.at(row).entry.filePath == filePath) {
                return row;
            }
        }

        return -1;
    }

    void processThumbnailQueue()
    {
        int processedCount = 0;
        while (!thumbnailQueue_.isEmpty() && processedCount < kThumbnailBatchLoadCount) {
            const int row = thumbnailQueue_.takeFirst();
            if (row < 0 || row >= items_.size()) {
                continue;
            }

            ImageGalleryItemState& item = items_[row];
            item.thumbnailQueued = false;
            if (item.thumbnailLoaded) {
                continue;
            }

            QImageReader reader(item.entry.filePath);
            reader.setAutoTransform(true);
            const QSize originalSize = reader.size();
            if (originalSize.isValid()) {
                reader.setScaledSize(originalSize.scaled(
                    QSize(kThumbnailIconSize, kThumbnailIconSize),
                    Qt::KeepAspectRatio));
            }

            const QImage image = reader.read();
            item.thumbnail = image.isNull()
                                 ? placeholderThumbnail()
                                 : QPixmap::fromImage(image.scaled(
                                       kThumbnailIconSize,
                                       kThumbnailIconSize,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
            item.thumbnailLoaded = true;

            const QModelIndex modelIndex = index(row, 0);
            emit dataChanged(modelIndex, modelIndex, {ThumbnailPixmapRole});
            ++processedCount;
        }

        if (thumbnailQueue_.isEmpty()) {
            thumbnailLoadTimer_.stop();
        }
    }

    QPixmap placeholderThumbnail() const
    {
        if (placeholderThumbnail_.isNull()) {
            placeholderThumbnail_ = QPixmap(kThumbnailIconSize, kThumbnailIconSize);
            placeholderThumbnail_.fill(placeholderFillColor_);

            QPainter painter(&placeholderThumbnail_);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(placeholderBorderColor_);
            painter.drawRoundedRect(placeholderThumbnail_.rect().adjusted(1, 1, -2, -2), 12, 12);
            painter.setPen(placeholderTextColor_);
            painter.drawText(placeholderThumbnail_.rect(), Qt::AlignCenter, QObject::tr("加载中"));
        }
        return placeholderThumbnail_;
    }

    QVector<ImageGalleryItemState> items_;
    QVector<int> thumbnailQueue_;
    QTimer thumbnailLoadTimer_;
    QColor placeholderFillColor_ = QColor("#132235");
    QColor placeholderBorderColor_ = QColor("#5bc7ff");
    QColor placeholderTextColor_ = QColor("#d7e3ee");
    mutable QPixmap placeholderThumbnail_;
};

class ImageGalleryDelegate final : public QStyledItemDelegate
{
public:
    explicit ImageGalleryDelegate(const ImageGalleryWidget* owner, QObject* parent = nullptr)
        : QStyledItemDelegate(parent),
          owner_(owner)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto* model = static_cast<ImageGalleryModel*>(const_cast<QAbstractItemModel*>(index.model()));
        if (model != nullptr) {
            model->requestThumbnail(index);
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRect cardRect = option.rect.adjusted(4, 4, -4, -4);
        const bool selected = (option.state & QStyle::State_Selected) != 0;
        painter->setPen(selected ? owner_->selectedCardBorderColor() : owner_->cardBorderColor());
        painter->setBrush(selected ? owner_->selectedCardFillColor() : owner_->cardFillColor());
        painter->drawRoundedRect(cardRect, 12, 12);

        const QRect thumbnailRect(cardRect.left() + 10, cardRect.top() + 10, kThumbnailIconSize, kThumbnailIconSize);
        const QPixmap thumbnail = index.data(ThumbnailPixmapRole).value<QPixmap>();
        if (!thumbnail.isNull()) {
            const QSize scaledSize = thumbnail.size().scaled(thumbnailRect.size(), Qt::KeepAspectRatio);
            const QRect centeredRect(
                QPoint(
                    thumbnailRect.center().x() - (scaledSize.width() / 2),
                    thumbnailRect.center().y() - (scaledSize.height() / 2)),
                scaledSize);
            painter->drawPixmap(centeredRect, thumbnail);
        }

        QRect textRect = cardRect.adjusted(8, kThumbnailIconSize + 18, -8, -8);
        painter->setPen(selected ? owner_->selectedCardTextColor() : owner_->cardTextColor());
        painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, index.data(Qt::DisplayRole).toString());

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(kThumbnailGridWidth, kThumbnailGridHeight);
    }

private:
    const ImageGalleryWidget* owner_ = nullptr;
};
}

ImageGalleryWidget::ImageGalleryWidget(QWidget* parent) : QFrame(parent)
{
    setObjectName("imageGalleryWidget");
    setFrameShape(QFrame::NoFrame);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(10);
    rootLayout->setContentsMargins(10, 12, 10, 12);

    titleLabel_ = new QLabel(tr("图像缩略图"), this);
    titleLabel_->setObjectName("imageGalleryTitleLabel");
    rootLayout->addWidget(titleLabel_);

    listView_ = new QListView(this);
    listView_->setObjectName("imageListView");
    listView_->setMinimumSize(150, 420);
    listView_->setViewMode(QListView::IconMode);
    listView_->setFlow(QListView::TopToBottom);
    listView_->setMovement(QListView::Static);
    listView_->setResizeMode(QListView::Adjust);
    listView_->setWrapping(false);
    listView_->setSpacing(kThumbnailSpacing);
    listView_->setLayoutMode(QListView::Batched);
    listView_->setBatchSize(24);
    listView_->setUniformItemSizes(true);
    listView_->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView_->setModel(new ImageGalleryModel(listView_));
    listView_->setItemDelegate(new ImageGalleryDelegate(this, listView_));
    rootLayout->addWidget(listView_);

    refreshGalleryColors();

    connect(
        listView_->selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        &ImageGalleryWidget::handleCurrentIndexChanged);
}

void ImageGalleryWidget::setEntries(const QVector<ImageGalleryEntry>& entries)
{
    const QSignalBlocker blocker(listView_->selectionModel());
    auto* model = static_cast<ImageGalleryModel*>(listView_->model());
    model->setEntries(entries);
}

void ImageGalleryWidget::clearEntries()
{
    const QSignalBlocker blocker(listView_->selectionModel());
    auto* model = static_cast<ImageGalleryModel*>(listView_->model());
    model->clearEntries();
}

void ImageGalleryWidget::updateEntry(const ImageGalleryEntry& entry)
{
    auto* model = static_cast<ImageGalleryModel*>(listView_->model());
    model->updateEntry(entry);
}

void ImageGalleryWidget::setCurrentImage(const QString& filePath)
{
    const QSignalBlocker blocker(listView_->selectionModel());
    auto* model = static_cast<ImageGalleryModel*>(listView_->model());
    const QModelIndex modelIndex = model->indexForFilePath(filePath);
    if (modelIndex.isValid()) {
        listView_->setCurrentIndex(modelIndex);
    }
}

void ImageGalleryWidget::setGalleryEnabled(bool enabled)
{
    listView_->setEnabled(enabled);
}

int ImageGalleryWidget::imageCount() const
{
    return listView_->model() == nullptr ? 0 : listView_->model()->rowCount();
}

QColor ImageGalleryWidget::cardFillColor() const
{
    return cardFillColor_;
}

void ImageGalleryWidget::setCardFillColor(const QColor& color)
{
    if (cardFillColor_ == color) {
        return;
    }
    cardFillColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::selectedCardFillColor() const
{
    return selectedCardFillColor_;
}

void ImageGalleryWidget::setSelectedCardFillColor(const QColor& color)
{
    if (selectedCardFillColor_ == color) {
        return;
    }
    selectedCardFillColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::cardBorderColor() const
{
    return cardBorderColor_;
}

void ImageGalleryWidget::setCardBorderColor(const QColor& color)
{
    if (cardBorderColor_ == color) {
        return;
    }
    cardBorderColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::selectedCardBorderColor() const
{
    return selectedCardBorderColor_;
}

void ImageGalleryWidget::setSelectedCardBorderColor(const QColor& color)
{
    if (selectedCardBorderColor_ == color) {
        return;
    }
    selectedCardBorderColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::cardTextColor() const
{
    return cardTextColor_;
}

void ImageGalleryWidget::setCardTextColor(const QColor& color)
{
    if (cardTextColor_ == color) {
        return;
    }
    cardTextColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::selectedCardTextColor() const
{
    return selectedCardTextColor_;
}

void ImageGalleryWidget::setSelectedCardTextColor(const QColor& color)
{
    if (selectedCardTextColor_ == color) {
        return;
    }
    selectedCardTextColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::placeholderFillColor() const
{
    return placeholderFillColor_;
}

void ImageGalleryWidget::setPlaceholderFillColor(const QColor& color)
{
    if (placeholderFillColor_ == color) {
        return;
    }
    placeholderFillColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::placeholderBorderColor() const
{
    return placeholderBorderColor_;
}

void ImageGalleryWidget::setPlaceholderBorderColor(const QColor& color)
{
    if (placeholderBorderColor_ == color) {
        return;
    }
    placeholderBorderColor_ = color;
    refreshGalleryColors();
}

QColor ImageGalleryWidget::placeholderTextColor() const
{
    return placeholderTextColor_;
}

void ImageGalleryWidget::setPlaceholderTextColor(const QColor& color)
{
    if (placeholderTextColor_ == color) {
        return;
    }
    placeholderTextColor_ = color;
    refreshGalleryColors();
}

void ImageGalleryWidget::handleCurrentIndexChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    if (!current.isValid()) {
        return;
    }

    emit imageSelected(current.data(FilePathRole).toString());
}

void ImageGalleryWidget::refreshGalleryColors()
{
    auto* model = static_cast<ImageGalleryModel*>(listView_->model());
    if (model != nullptr) {
        model->setPlaceholderColors(placeholderFillColor_, placeholderBorderColor_, placeholderTextColor_);
    }

    if (listView_ != nullptr && listView_->viewport() != nullptr) {
        listView_->viewport()->update();
    }
}
