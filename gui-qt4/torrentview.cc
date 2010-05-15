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
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollArea>
#include <QThread>
#include <QtGlobal>
#include <QVBoxLayout>

#include <debug/debug.hh>

#include <bt/bundlebuilders/localfilebundlebuilder.hh>
#include <bt/globaltorrentregistry.hh>

#include "torrentstatewidget.hh"
#include "previewtorrentdialog.hh"
#include "torrentview.hh"

using namespace Hypergrace;

class OpenPreviewEvent : public QEvent
{
public:
    OpenPreviewEvent(QString filename, Bt::TorrentBundle *bundle) :
        QEvent(OpenPreviewEvent::type()),
        filename_(filename),
        bundle_(bundle)
    {
    }

    Bt::TorrentBundle *bundle() { return bundle_; }
    const QString &filename() const { return filename_; }

    static QEvent::Type type() { return QEvent::Type(QEvent::User + 1000); }

private:
    QString filename_;
    Bt::TorrentBundle *bundle_;
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

    QDir dir(storagePath_);

    if (!dir.exists()) {
        if (dir.mkpath(storagePath_)) {
            hDebug() << "Created storage directory at" << storagePath_.toLatin1().data();
        } else {
            hSevere() << "Failed to create storage directory at" << storagePath_.toLatin1().data();
            storagePath_ = QString();
        }
    } else {
        hDebug() << "Storage directory exists at" << storagePath_.toLatin1().data();
    }
}

void TorrentView::setupUi()
{
    QScrollArea *scrollArea = new QScrollArea();
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addStretch();

    this->setLayout(mainLayout);
    scrollArea->setWidget(this);
}

TorrentView::~TorrentView()
{
}

void TorrentView::postBuildSuccessEvent(QString filename, Bt::TorrentBundle *bundle)
{
    // This function will be called asynchroniously from other thread
    // thus making impossible to open the preview dialog here.
    QApplication::postEvent(this, new OpenPreviewEvent(filename, bundle));
}

void TorrentView::postBuildFailureEvent(QString filename)
{
    QApplication::postEvent(this, new OpenPreviewEvent(filename, 0));
}

void TorrentView::handleBundleConfiguredEvent(Hypergrace::Bt::TorrentBundle *bundle, bool autostart)
{
    hDebug() << "Inserting torrent state widget ...";

    QVBoxLayout *mainLayout = reinterpret_cast<QVBoxLayout *>(this->layout());

    if (!Bt::GlobalTorrentRegistry::self()->createTorrent(bundle)) {
        hSevere() << "Failed to create torrent";
        return;
    }

    TorrentStateWidget *torrentState = new TorrentStateWidget(bundle, this);
    mainLayout->insertWidget(mainLayout->count() - 1, torrentState);

    torrentState->start();
}

void TorrentView::initiateLocalOpen()
{
    if (storagePath_.isEmpty()) {
        hSevere() << "Storage directory is not specified";
        QMessageBox::critical(this,
                tr("No Storage Directory"),
                tr("I'm sorry, but it looks like you cannot create torrents "
                   "because the storage was not specified."));
        return;
    }

    QFileDialog *selectFile = new QFileDialog(this, QObject::tr("Open Torrent"));
    selectFile->setFileMode(QFileDialog::ExistingFile);
    selectFile->setNameFilter(tr("Torrent Files (*.torrent);;All files (*)"));

    QObject::connect(selectFile, SIGNAL(fileSelected(const QString &)),
                     this, SLOT(openLocalFile(const QString &)));

    selectFile->show();
}

void TorrentView::openLocalFile(const QString &filename)
{
    if (filename.isEmpty())
        return;

    QDir bundleDir;

    // Find the storage directory name for the new torrent
    do {
        QString path;

        path += storagePath_ + "/" + QFileInfo(filename).baseName();
        path += "-" + QString::number(qrand());

        bundleDir.setPath(path);
    } while (bundleDir.exists());

    hDebug() << "Storage for the new torrent has been set to"
             << bundleDir.absolutePath().toLatin1().data();

    Bt::LocalFileBundleBuilder *tbb =
        new Bt::LocalFileBundleBuilder(
                bundleDir.absolutePath().toLatin1().data(),
                filename.toLatin1().data());

    // We cannot bootstrap torrent outside the GUI thread because Qt
    // doesn't allow interacting with widgets outside the main GUI
    // thread. As a workaround the event handlers will post a QEvent
    // event in the GUI thread so we can notice it and process it
    // accordingly.
    tbb->onBundleReady = Delegate::bind(&TorrentView::postBuildSuccessEvent, this, filename, _1);
    tbb->onBuildFailed = Delegate::bind(&TorrentView::postBuildFailureEvent, this, filename);
    tbb->startBuilding();
}

bool TorrentView::event(QEvent *e)
{
    if (e->type() == OpenPreviewEvent::type()) {
        const QString &filename = static_cast<OpenPreviewEvent *>(e)->filename();
        Bt::TorrentBundle *bundle = static_cast<OpenPreviewEvent *>(e)->bundle();

        if (bundle != 0)  {
            PreviewTorrentDialog *dialog = new PreviewTorrentDialog(filename, bundle, this);

            QObject::connect(
                    dialog, SIGNAL(onBundleConfigured(Hypergrace::Bt::TorrentBundle *, bool)),
                    this, SLOT(handleBundleConfiguredEvent(Hypergrace::Bt::TorrentBundle *, bool))
            );

            dialog->show();
        } else {
            QString message =
                QObject::tr(
                        "An error occurred while building a torrent bundle "
                        "from %1.\n\nSee message log for more details."
                ).arg(filename);

            QMessageBox::critical(this, QObject::tr("Preview Error"), message);
        }

        return true;
    }

    return QWidget::event(e);
}
