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

#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollArea>
#include <QStringList>
#include <QThread>
#include <QtGlobal>
#include <QVBoxLayout>

#include <debug/debug.hh>

#include <bt/bundlebuilders/localfilebundlebuilder.hh>
#include <bt/bundlebuilders/bundleunmarshaller.hh>
#include <bt/globaltorrentregistry.hh>

#include "torrentstatewidget.hh"
#include "previewtorrentdialog.hh"
#include "torrentview.hh"

using namespace Hypergrace;


class BuildCompleteEvent : public QEvent
{
public:
    enum BuildSource {
        MarshalledBundle = 0,
        TorrentFile,
        MagnetLink
    };

    static const QEvent::Type EventType = QEvent::Type(QEvent::User + 1000);

public:
    BuildCompleteEvent(Bt::TorrentBundle *bundle, BuildSource sourceType, QString sourceUri) :
        QEvent(EventType),
        bundle_(bundle),
        sourceType_(sourceType),
        sourceUri_(sourceUri)
    {
    }

    bool buildSuccessful() const
    {
        return bundle_ != 0;
    }

    Bt::TorrentBundle *bundle() const
    {
        return bundle_;
    }

    BuildSource sourceType() const
    {
        return sourceType_;
    }

    QString sourceUri() const
    {
        return sourceUri_;
    }

private:
    Bt::TorrentBundle *bundle_;
    BuildSource sourceType_;
    QString sourceUri_;
};


TorrentView::TorrentView(QWidget *parent) :
    QWidget(parent)
{
    setupUi();

    // Create the storage directory for configuration files if it
    // doesn't exist yet
    storagePath_ = QDesktopServices::storageLocation(QDesktopServices::DataLocation);

    if (storagePath_.isEmpty())
        storagePath_ = QDir::homePath();

    storagePath_ += "/hypergrace";
    storagePath_ = QDir::cleanPath(storagePath_);

    hDebug() << "Set storage directory to" << storagePath_.toUtf8().data();

    restoreTorrentList();
}

void TorrentView::setupUi()
{
    QScrollArea *scrollArea = new QScrollArea();

    scrollArea->setWidget(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addStretch();

    setLayout(mainLayout);
}

void TorrentView::restoreTorrentList()
{
    QDir storageDirectory(storagePath_);

    QStringList torrents = storageDirectory.entryList(
            QStringList(), QDir::Dirs | QDir::Readable | QDir::Executable | QDir::NoDotAndDotDot);

    for (auto torrent = torrents.begin(); torrent != torrents.end(); ++torrent) {
        QString bundleDirectory = storageDirectory.path() + "/" + *torrent;

        Bt::BundleUnmarshaller *unmarshaller =
            new Bt::BundleUnmarshaller(bundleDirectory.toUtf8().data());

        unmarshaller->onBundleReady = Delegate::bind(&TorrentView::postBuildCompleteEvent, this,
                _1, (int)BuildCompleteEvent::MarshalledBundle, bundleDirectory);

        unmarshaller->onBuildFailed = Delegate::bind(&TorrentView::postBuildCompleteEvent, this,
                (Bt::TorrentBundle *)0, (int)BuildCompleteEvent::MarshalledBundle, bundleDirectory);

        unmarshaller->startBuilding();
    }
}

TorrentView::~TorrentView()
{
}

void TorrentView::postBuildCompleteEvent(
        Bt::TorrentBundle *bundle,
        int sourceType,
        QString sourceUri)
{
    // This function will be called asynchroniously from other thread
    // thus making impossible to take any actions here.

    BuildCompleteEvent *event = new BuildCompleteEvent(
            bundle, (BuildCompleteEvent::BuildSource)sourceType, sourceUri);

    QApplication::postEvent(this, event);
}

void TorrentView::handleBundleConfiguredEvent(
        Hypergrace::Bt::TorrentBundle *bundle,
        bool autostart)
{
    QVBoxLayout *mainLayout = reinterpret_cast<QVBoxLayout *>(this->layout());

    if (!Bt::GlobalTorrentRegistry::self()->createTorrent(bundle)) {
        hSevere() << "Failed to create torrent";
        return;
    }

    TorrentStateWidget *torrentState = new TorrentStateWidget(bundle, this);

    mainLayout->insertWidget(mainLayout->count() - 1, torrentState);

    if (autostart)
        torrentState->start();
}

void TorrentView::initiateLocalOpen()
{
    QFileDialog *selectFileDialog = new QFileDialog(this, QObject::tr("Open Torrent"));

    selectFileDialog->setFileMode(QFileDialog::ExistingFile);
    selectFileDialog->setNameFilter(tr("Torrent Files (*.torrent);;All files (*)"));

    QObject::connect(
            selectFileDialog, SIGNAL(fileSelected(const QString &)),
            this, SLOT(openLocalFile(const QString &))
    );

    selectFileDialog->show();
}

void TorrentView::openLocalFile(const QString &filename)
{
    if (filename.isEmpty())
        return;

    QDir bundleDirectory;

    // Find the storage directory name for the new torrent
    do {
        QString path;

        path += storagePath_ + "/" + QFileInfo(filename).baseName();
        path += "-" + QString::number(qrand());

        bundleDirectory.setPath(path);
    } while (bundleDirectory.exists());

    hDebug() << "Bundle directory for the new torrent has been set to"
             << bundleDirectory.absolutePath().toUtf8().data();

    Bt::LocalFileBundleBuilder *tbb =
        new Bt::LocalFileBundleBuilder(
                bundleDirectory.absolutePath().toUtf8().data(),
                filename.toUtf8().data());

    // We cannot bootstrap torrent outside the GUI thread because Qt
    // doesn't allow interacting with widgets outside the main thread.
    // As a workaround the event handlers will post a QEvent event in
    // the GUI thread so the thread can notice it and process it
    // accordingly.
    tbb->onBundleReady = Delegate::bind(&TorrentView::postBuildCompleteEvent, this,
            _1, (int)BuildCompleteEvent::TorrentFile, filename);

    tbb->onBuildFailed = Delegate::bind(&TorrentView::postBuildCompleteEvent, this,
            (Bt::TorrentBundle *)0, (int)BuildCompleteEvent::TorrentFile, filename);

    tbb->startBuilding();
}

bool TorrentView::event(QEvent *event)
{
    if (event->type() != BuildCompleteEvent::EventType) {
        return QWidget::event(event);
    } else {
        BuildCompleteEvent *buildCompleteEvent = static_cast<BuildCompleteEvent *>(event);

        if (buildCompleteEvent->sourceType() == BuildCompleteEvent::TorrentFile) {
            if (!buildCompleteEvent->buildSuccessful()) {
                QString message = QObject::tr(
                    "An error occurred while building a torrent bundle "
                    "from %1.\n\nSee message log for more details.")
                    .arg(buildCompleteEvent->sourceUri());

                QMessageBox::critical(this, QObject::tr("Open Torrent Error"), message);
                return true;
            }

            PreviewTorrentDialog *previewDialog = new PreviewTorrentDialog(
                    buildCompleteEvent->sourceUri(), buildCompleteEvent->bundle(), this);

            QObject::connect(
                previewDialog, SIGNAL(onBundleConfigured(Hypergrace::Bt::TorrentBundle *, bool)),
                this, SLOT(handleBundleConfiguredEvent(Hypergrace::Bt::TorrentBundle *, bool))
            );

            previewDialog->show();
        } else if (buildCompleteEvent->sourceType() == BuildCompleteEvent::MarshalledBundle) {
            if (!buildCompleteEvent->buildSuccessful()) {
                hSevere() << "Failed to restore torrent";
                return true;
            }

            // FIXME: Read torrent's started/stopped state.
            handleBundleConfiguredEvent(buildCompleteEvent->bundle(), true);
        }

        return true;
    }
}
