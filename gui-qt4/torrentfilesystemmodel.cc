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

#include <string>

#include <QDebug>
#include <QHash>
#include <QIcon>
#include <QStandardItem>
#include <QString>

#include <debug/debug.hh>
#include <delegate/bind.hh>
#include <bt/bundle/torrentmodel.hh>

#include "fileiconprovider.hh"
#include "prettyprint.hh"
#include "torrentfilesystemmodel.hh"

using namespace Hypergrace;


static void insertFile(QHash<QString, QStandardItem *> &storage,
                       FileIconProvider &iconProvider,
                       const Bt::FileDescriptor &fileDescriptor)
{
    QString path = QString::fromUtf8(fileDescriptor.filename.c_str());
    QList<QStandardItem *> childRow;
    int slash = path.size();

    do {
        auto pos = storage.find(path);

        if (pos == storage.end()) {
            // Setup the Filename column
            QStandardItem *filename = new QStandardItem();

            if (path.endsWith('/')) {
                QString name = path.mid(0, path.size() - 1);

                filename->setData(QVariant(name));
                name = name.mid(name.lastIndexOf('/') + 1);
                filename->setText(name);
                filename->setIcon(QIcon::fromTheme("folder", QIcon(":/mimetype-folder.png")));
            } else {
                filename->setText(path.mid(path.lastIndexOf('/') + 1));
                filename->setData(QVariant(path));
                filename->setIcon(iconProvider.icon(path));
            }

            filename->setToolTip(filename->text());
            filename->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            filename->setCheckState(Qt::Checked);

            // Setup the Size column
            QStandardItem *size = new QStandardItem();

            size->setText(PrettyPrint::printSize(fileDescriptor.size));
            size->setData(QVariant(fileDescriptor.size));
            size->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

            QList<QStandardItem *> currentRow;
            currentRow.push_back(filename);
            currentRow.push_back(size);

            // If there's a child row, insert it into the current one
            if (!childRow.empty())
                filename->appendRow(childRow);

            childRow = currentRow;
            storage.insert(path, filename);

            path = path.mid(0, path.mid(0, path.size() - 1).lastIndexOf('/') + 1);
        } else {
            (*pos)->appendRow(childRow);
            break;
        }
    } while ((slash = path.lastIndexOf('/')) != -1);
}

static void calculateSizeRecursively(QStandardItem *filenameCell, QStandardItem *sizeCell)
{
    // FIXME: Recursion.. BANG!
    unsigned long long currentSize = 0;

    for (int row = 0; row < filenameCell->rowCount(); ++row) {
        QStandardItem *childFilenameCell = filenameCell->child(row, 0);
        QStandardItem *childSizeCell = filenameCell->child(row, 1);

        if (childFilenameCell->hasChildren())
            calculateSizeRecursively(childFilenameCell, childSizeCell);

        currentSize += childSizeCell->data().toULongLong();
    }

    sizeCell->setData(QVariant(currentSize));
    sizeCell->setText(PrettyPrint::printSize(currentSize));
}

static unsigned long long calculateFolderSizes(QStandardItem *root)
{
    QStandardItem fakeSizeColumn;
    fakeSizeColumn.setData(QVariant(0ULL));
    calculateSizeRecursively(root, &fakeSizeColumn);

    return fakeSizeColumn.data().toULongLong();
}

TorrentFilesystemModel::TorrentFilesystemModel(
        const Hypergrace::Bt::TorrentModel &model,
        QObject *parent) :
    QStandardItemModel(parent)
{
    QHash<QString, QStandardItem *> files;

    files.reserve(model.fileList().size() * 2);
    files["/"] = this->invisibleRootItem();

    FileIconProvider iconProvider;

    for (auto file = model.fileList().begin(); file != model.fileList().end(); ++file)
        insertFile(files, iconProvider, *file);

    selectedSize_ = calculateFolderSizes(this->invisibleRootItem());

    this->setHeaderData(0, Qt::Horizontal, QObject::tr("Name"));
    this->setHeaderData(1, Qt::Horizontal, QObject::tr("Size"));
}

void TorrentFilesystemModel::updateCheckStateUpwards(QStandardItem *me)
{
    if (me == 0)
        return;

    bool hasChecked = false;
    bool hasUnchecked = false;

    for (unsigned int row = 0; me->child(row) != 0; ++row) {
        QStandardItem *child = me->child(row);

        switch (child->checkState()) {
        case Qt::PartiallyChecked:
            hasChecked = true;
            hasUnchecked = true;
            break;
        case Qt::Checked:
            hasChecked = true;
            break;
        case Qt::Unchecked:
            hasUnchecked = true;
            break;
        default:
            hWarning() << "Unidentified check state";
            break;
        }

        if (hasChecked && hasUnchecked)
            break;
    }

    if (hasChecked && hasUnchecked) {
        me->setCheckState(Qt::PartiallyChecked);
        updateCheckStateUpwards(me->parent());
    } else if (hasChecked) {
        me->setCheckState(Qt::Checked);
        updateCheckStateUpwards(me->parent());
    } else /* if (hasUnchecked) */ {
        me->setCheckState(Qt::Unchecked);
        updateCheckStateUpwards(me->parent());
    }
}

void TorrentFilesystemModel::updateCheckStateDownwards(QStandardItem *me, Qt::CheckState state)
{
    //if (me->checkState() == state)
    //    return;

    // If the given item is a leaf node (a file) then the selected
    // size field need to be updated
    if (!me->hasChildren()) {
        QStandardItem *size = this->itemFromIndex(me->index().sibling(me->index().row(), 1));

        switch (state) {
        case Qt::Checked:
            selectedSize_ += size->data().toULongLong();
            break;
        case Qt::Unchecked:
            selectedSize_ -= size->data().toULongLong();
            break;
        default:
            hWarning() << "Tri-state selection (or whatever) is not implemented";
            break;
        }
    }

    me->setCheckState(state);

    for (unsigned int row = 0; me->child(row) != 0; ++row)
        updateCheckStateDownwards(me->child(row), state);
}

static void enumSelectedFilesHelper(const QStandardItem *item, QVector<QString> &accumulator)
{
    for (unsigned int row = 0; item->child(row) != 0; ++row) {
        QStandardItem *child = item->child(row);

        if (!child->hasChildren()) {
            if (child->checkState() == Qt::Checked)
                accumulator.push_back(child->data().toString());
        } else {
            enumSelectedFilesHelper(child, accumulator);
        }
    }
}

QVector<QString> TorrentFilesystemModel::enumSelectedFiles() const
{
    QVector<QString> selectedFiles;

    enumSelectedFilesHelper(invisibleRootItem(), selectedFiles);

    return selectedFiles;
}

unsigned long long TorrentFilesystemModel::selectedSize() const
{
    return selectedSize_;
}

bool TorrentFilesystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        QStandardItem *item = this->itemFromIndex(index);

        if (item->data(role) == value)
            return true;

        unsigned long long prevSelectedSize = selectedSize_;

        updateCheckStateDownwards(item, Qt::CheckState(value.toInt()));
        updateCheckStateUpwards(item->parent());

        if (prevSelectedSize != selectedSize_)
            onDownloadAmountChanged(selectedSize_);

        return true;
    }

    return QStandardItemModel::setData(index, value, role);
}

void TorrentFilesystemModel::checkAll()
{
    unsigned long long prevSize = selectedSize_;

    //invisibleRootItem()->setCheckState(Qt::Checked);

    updateCheckStateDownwards(invisibleRootItem(), Qt::Checked);

    if (prevSize != selectedSize_)
        onDownloadAmountChanged(selectedSize_);
}

void TorrentFilesystemModel::uncheckAll()
{
    unsigned long long prevSize = selectedSize_;

    //invisibleRootItem()->setCheckState(Qt::Unchecked);

    updateCheckStateDownwards(invisibleRootItem(), Qt::Unchecked);

    if (prevSize != selectedSize_)
        onDownloadAmountChanged(selectedSize_);
}
