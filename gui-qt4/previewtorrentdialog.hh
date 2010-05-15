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

#ifndef PREVIEWTORRENTDIALOG_HH_
#define PREVIEWTORRENTDIALOG_HH_

#include <QDialog>
#include <QString>
#include <ui_previewtorrent.h>

class QPointer;
class QTreeWidgetItem;
class TorrentFilesystemModel;

namespace Hypergrace { namespace Bt { class TorrentBundle; } }


class PreviewTorrentDialog : public QDialog
{
    Q_OBJECT

public:
    PreviewTorrentDialog(const QString &, Hypergrace::Bt::TorrentBundle *, QWidget *);

signals:
    void onBundleConfigured(Hypergrace::Bt::TorrentBundle *, bool);

private:
    void setTorrentName(const QString &);

    void setupTorrentIcon();
    void setupInfoHashLabel();
    void setupTotalSizeLabel();
    void setupCreationDateLabel();
    void setupPieceInfoLabels();
    void setupDestDirItemGroup();
    void setupButtonBox();

    void populateTorrentFilesystem();

private slots:
    void setDownloadSizeLabelText(unsigned long long);

    void handleShowContextMenuEvent(const QPoint &);
    void handleToggleDestDirRadioEvent(bool);
    void handleClickSelectDirButtonEvent();
    void handleDblClickTreeItemEvent(QTreeWidgetItem *, int);

    void handleAcceptDownloadEvent();
    void handleCancelDownloadEvent();

private:
    Hypergrace::Bt::TorrentBundle *bundle_;
    TorrentFilesystemModel *model_;
    Ui_PreviewTorrentDialog ui_;
};

#endif /* PREVIEWTORRENTDIALOG_HH_ */
