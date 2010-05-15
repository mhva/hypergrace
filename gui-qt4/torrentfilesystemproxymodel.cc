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

#include <QDebug>
#include <QVariant>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>

#include "torrentfilesystemmodel.hh"
#include "torrentfilesystemproxymodel.hh"

TorrentFilesystemProxyModel::TorrentFilesystemProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

void TorrentFilesystemProxyModel::setSourceModel(QAbstractItemModel *model)
{
    // Ensure that we've got an instance of TorrentFilesystemModel
    dynamic_cast<TorrentFilesystemModel *>(model);
    QSortFilterProxyModel::setSourceModel(model);
}

bool TorrentFilesystemProxyModel::lessThan(const QModelIndex &left,
                                           const QModelIndex &right) const
{
    auto *model = reinterpret_cast<TorrentFilesystemModel *>(sourceModel());
    QVariant leftData = model->data(left);
    QVariant rightData = model->data(right);

    if (leftData.type() == QVariant::String) {
        // Got an item from the Name column
        QStandardItem *leftItem = model->itemFromIndex(left);
        QStandardItem *rightItem = model->itemFromIndex(right);

        if (leftItem->hasChildren() && !rightItem->hasChildren())
            return true;
        else if (!leftItem->hasChildren() && rightItem->hasChildren())
            return false;
        else
            return QString::localeAwareCompare(leftItem->text(), rightItem->text()) < 0;
    } else {
        return true;
    }
}
