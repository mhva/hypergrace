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

#ifndef TORRENTFILESYSTEMMODEL_HH_
#define TORRENTFILESYSTEMMODEL_HH_

#include <QStandardItemModel>
#include <QString>
#include <QVector>

#include <delegate/delegate.hh>

class QModelIndex;
class QObject;
class QStandardItem;
class QVariant;
namespace Hypergrace { namespace Bt { class TorrentModel; }}


class TorrentFilesystemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TorrentFilesystemModel(const Hypergrace::Bt::TorrentModel &, QObject *);

    QVector<QString> enumSelectedFiles() const;
    unsigned long long selectedSize() const;

signals:
    void onDownloadAmountChanged(unsigned long long);

public slots:
    void checkAll();
    void uncheckAll();

public:
    bool setData(const QModelIndex &index, const QVariant &value, int role);

private:
    void updateCheckStateDownwards(QStandardItem *me, Qt::CheckState state);
    void updateCheckStateUpwards(QStandardItem *);

private:
    unsigned long long selectedSize_;
};

#endif /* TORRENTFILESYSTEMMODEL_HH_ */
