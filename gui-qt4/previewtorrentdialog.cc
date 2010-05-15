/*
   Copyright (C) 2009 Anton Mihalyov <anton@glyphsense.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <algorithm>
#include <limits>
#include <set>
#include <string>

#include <QAction>
#include <QDate>
#include <QDesktopServices>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QPoint>
#include <QPushButton>
#include <QString>

#include <debug/debug.hh>
#include <delegate/delegate.hh>

#include <bt/bundle/torrentconfiguration.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentbundle.hh>

#include "torrentfilesystemmodel.hh"
#include "torrentfilesystemproxymodel.hh"
#include "previewtorrentdialog.hh"

using namespace Hypergrace;


PreviewTorrentDialog::PreviewTorrentDialog(const QString &filename,
                                           Hypergrace::Bt::TorrentBundle *bundle,
                                           QWidget *parent) :
    QDialog(parent),
    bundle_(bundle),
    model_(0)
{
    ui_.setupUi(this);

    setTorrentName(filename);

    setupTorrentIcon();
    setupInfoHashLabel();
    setupTotalSizeLabel();
    setupCreationDateLabel();
    setupPieceInfoLabels();
    setupDestDirItemGroup();
    setupButtonBox();

    populateTorrentFilesystem();
}

void PreviewTorrentDialog::setupTorrentIcon()
{
    QIcon fallback(":/mimetype-bittorrent.png");
    QIcon mimetype = QIcon::fromTheme("application-x-bittorrent", fallback);

    ui_.torrentImage->setPixmap(mimetype.pixmap(48, 48));
}

void PreviewTorrentDialog::setTorrentName(const QString &filename)
{
    if (!bundle_->model().name().empty()) {
        QString name = QString::fromUtf8(bundle_->model().name().c_str()).simplified();
        ui_.torrentName->setText(name);
    } else {
        QFileInfo fileInfo(filename);
        QString name = fileInfo.completeBaseName();

        if (name.isEmpty())
            name = fileInfo.fileName();

        ui_.torrentName->setText(filename);
    }
}

void PreviewTorrentDialog::setupTotalSizeLabel()
{
    unsigned long long size = bundle_->model().torrentSize();
    unsigned long long gib = size / (1024ULL * 1024 * 1024);
    unsigned long long mib = size / (1024ULL * 1024);

    if (gib > 0) {
        unsigned int remMiB = size % (1024ULL * 1024 * 1024) / (1024 * 1024);
        QString fraction = QString::number((float)remMiB / 1024.0 + 0.005, 'f', 2);

        fraction.remove(0, 2);

        ui_.totalSize->setText(QString("%1.%2 GB (%3 MB)").arg(QString::number(gib)) \
                                                          .arg(fraction) \
                                                          .arg(QString::number(mib)));
    } else {
        // Don't print a size in gigabytes if the size is less than 1GB
        ui_.totalSize->setText(QString("%1 MB").arg(QString::number(mib)));
    }
}

static void toHex(QString &storage, unsigned char c)
{
        const char *hexMap = "0123456789abcdef";
        char result[] = { hexMap[c >> 4], hexMap[c & 15], 0 };

        storage.append(result);
}

void PreviewTorrentDialog::setupInfoHashLabel()
{
    QString hexHash;
    const Bt::InfoHash &rawHash = bundle_->model().hash();

    std::for_each(rawHash.begin(), rawHash.end(), Delegate::bind(&toHex, std::ref(hexHash), _1));

    ui_.torrentHash->setText(hexHash);
}

void PreviewTorrentDialog::populateTorrentFilesystem()
{
    model_ = new TorrentFilesystemModel(bundle_->model(), ui_.fileList);
    TorrentFilesystemProxyModel *proxy = new TorrentFilesystemProxyModel(ui_.fileList);

    proxy->setSourceModel(model_);

    model_->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model_->setHeaderData(1, Qt::Horizontal, tr("Size"));

    ui_.fileList->setModel(proxy);
    ui_.fileList->setAnimated(true);
    ui_.fileList->sortByColumn(0, Qt::AscendingOrder);

    // FIXME: There's some culpits in the sorting implementation that
    // need to be fixed first before making it available to user
    //ui_.fileList->setSortingEnabled(true);

    // Make the first column to take up all the space
    QHeaderView *header = ui_.fileList->header();
    header->setStretchLastSection(false);
    header->setResizeMode(0, QHeaderView::Stretch);
    header->setResizeMode(1, QHeaderView::ResizeToContents);

    setDownloadSizeLabelText(model_->selectedSize());

    QObject::connect(
            ui_.fileList, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(handleShowContextMenuEvent(const QPoint &))
    );

    QObject::connect(
            model_, SIGNAL(onDownloadAmountChanged(unsigned long long)),
            this, SLOT(setDownloadSizeLabelText(unsigned long long))
    );
}

void PreviewTorrentDialog::setDownloadSizeLabelText(unsigned long long size)
{
    unsigned long long mib = size / (1024ULL * 1024);

    if (mib > 0) {
        ui_.downloadSize->setText(QString("%3 MB").arg(QString::number(mib)));
    } else {
        unsigned int kib = size % (1024 * 1024);
        ui_.downloadSize->setText(QString("%1 KB").arg(QString::number(kib)));
    }
}

void PreviewTorrentDialog::setupCreationDateLabel()
{
    long long creationDate = bundle_->model().creationDate();

    if (creationDate <= std::numeric_limits<uint>::max() && creationDate != 0) {
        QString dateString = QDateTime::fromTime_t(creationDate).toString("yyyy/MM/dd");
        ui_.creationDate->setText(dateString);
    } else if (creationDate == 0) {
        ui_.creationDate->setText("Unknown");
    } else {
        ui_.creationDate->setText("Overflow");
    }
}

void PreviewTorrentDialog::setupPieceInfoLabels()
{
    unsigned long pieceSize = bundle_->model().pieceSize();
    ui_.pieceSize->setText(QString("%1 KB").arg(QString::number(pieceSize / 1024)));
    ui_.pieceCount->setText(QString::number(bundle_->model().pieceCount()));
}

void PreviewTorrentDialog::handleShowContextMenuEvent(const QPoint &point)
{
    QMenu menu;
    QAction checkAction(tr("Check All"), 0);
    QAction uncheckAction(tr("Uncheck All"), 0);

    QObject::connect(&checkAction, SIGNAL(triggered()), model_, SLOT(checkAll()));
    QObject::connect(&uncheckAction, SIGNAL(triggered()), model_, SLOT(uncheckAll()));

    menu.addAction(&checkAction);
    menu.addAction(&uncheckAction);

    menu.exec(ui_.fileList->viewport()->mapToGlobal(point));
}

void PreviewTorrentDialog::setupDestDirItemGroup()
{
    // TODO: Make default download directory customizeable
    ui_.customDownloadDirectory->setText(
            QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
    QObject::connect(ui_.defaultDirRadio, SIGNAL(toggled(bool)),
            this, SLOT(handleToggleDestDirRadioEvent(bool)));
    QObject::connect(ui_.customDirRadio, SIGNAL(toggled(bool)),
            this, SLOT(handleToggleDestDirRadioEvent(bool)));
    QObject::connect(ui_.selectDirectoryButton, SIGNAL(clicked()),
            this, SLOT(handleClickSelectDirButtonEvent()));
}

void PreviewTorrentDialog::handleToggleDestDirRadioEvent(bool)
{
    if (ui_.defaultDirRadio->isChecked()) {
        ui_.customDownloadDirectory->setEnabled(false);
        ui_.selectDirectoryButton->setEnabled(false);
        ui_.customDownloadDirectory->setText(
                QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
    } else {
        ui_.customDownloadDirectory->setEnabled(true);
        ui_.selectDirectoryButton->setEnabled(true);
    }
}

void PreviewTorrentDialog::handleClickSelectDirButtonEvent()
{
    QFileDialog *selectDir = new QFileDialog(this, tr("Select Directory to Save Files"));

    selectDir->setDirectory(ui_.customDownloadDirectory->text());
    selectDir->setViewMode(QFileDialog::Detail);
    selectDir->setFileMode(QFileDialog::DirectoryOnly);
    //selectDir->setOption(QFileDialog::ShowDirsOnly);

    QObject::connect(
            selectDir, SIGNAL(fileSelected(const QString &)),
            ui_.customDownloadDirectory, SLOT(setText(const QString &))
    );

    selectDir->show();
}

void PreviewTorrentDialog::handleDblClickTreeItemEvent(QTreeWidgetItem *item, int column)
{
}

void PreviewTorrentDialog::handleAcceptDownloadEvent()
{
    auto selectedFiles = model_->enumSelectedFiles();
    std::set<std::string> unmaskedFiles;

    for (auto file = selectedFiles.begin(); file != selectedFiles.end(); ++file)
        unmaskedFiles.insert((*file).toLatin1().data());

    QString location = ui_.customDownloadDirectory->text();
    bundle_->configuration().setUnmaskedFiles(std::move(unmaskedFiles));
    bundle_->configuration().setStorageDirectory(location.toLatin1().data());

    onBundleConfigured(bundle_, ui_.startDownloadAutomatically->isChecked());

    close();
}

void PreviewTorrentDialog::handleCancelDownloadEvent()
{
    delete bundle_;
    close();
}

void PreviewTorrentDialog::setupButtonBox()
{
    ui_.okCancelButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Start"));

    QObject::connect(
            ui_.okCancelButtonBox, SIGNAL(accepted()),
            this, SLOT(handleAcceptDownloadEvent())
    );

    QObject::connect(
            ui_.okCancelButtonBox, SIGNAL(rejected()),
            this, SLOT(handleCancelDownloadEvent())
    );
}
