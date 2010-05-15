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

#include <QByteArray>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMultiMap>
#include <QRegExp>

#include <debug/debug.hh>
#include "fileiconprovider.hh"


class FileIconProvider::Private
{
public:
    Private() :
        fallback_(QIcon::fromTheme("application-octet-stream",
                    QIcon(":/mimetype-unknown-file.png")))
    {
        QFile mimeTypes("/etc/mime.types");

        if (!mimeTypes.open(QIODevice::ReadOnly)) {
            hWarning() << "Failed to open /etc/mime.types for resolving file icons";
            return;
        }

        QByteArray line;
        QRegExp re("^(\\w\\S+)\\s+([\\s\\S]+)\n?$");

        if (!re.isValid()) {
            hWarning() << "Failed to compile a regex to parse /etc/mime.types";
            return;
        }

        while (!(line = mimeTypes.readLine()).isEmpty()) {
            int pos = re.indexIn(QString(line));

            if (pos == -1)
                continue;

            QString mimetype = re.cap(1).simplified();
            QStringList exts = re.cap(2).simplified().split(" ", QString::SkipEmptyParts);

            mimetype.replace(QChar('/'), "-");

            for (auto it = exts.begin(); it != exts.end(); ++it)
                extensionMap_.insert(*it, mimetype);
        }
    }

    QIcon iconForMimeType(const QString &mimetype)
    {
        QIcon result = QIcon::fromTheme(mimetype);

        qDebug() << mimetype;

        if (!result.isNull()) {
            return result;
        } else if (mimetype == "application-x-iso9660-image") {
            // XXX: Oxygen workaround for ISO files.
            return QIcon::fromTheme("application-x-cd-image", fallback_);
        } else {
            // Try a bit harder to find a suitable icon
            int slash = mimetype.indexOf('-');

            if (slash == -1)
                return fallback_;

            QString genericMimetype = mimetype.mid(0, slash) + "-x-generic";
            return QIcon::fromTheme(genericMimetype, fallback_);
        }
    }

public:
    QMultiMap<QString, QString> extensionMap_;
    QIcon fallback_;
};

FileIconProvider::FileIconProvider() :
    d(new Private())
{
}

FileIconProvider::~FileIconProvider()
{
    delete d;
}

QIcon FileIconProvider::icon(const QString &filename)
{
    QString ext = QFileInfo(filename).suffix().toLower();
    auto pos = d->extensionMap_.find(ext);

    if (pos != d->extensionMap_.end()) {
        for (; pos != d->extensionMap_.end(); ++pos) {
            QIcon icon = d->iconForMimeType(*pos);

            if (!icon.isNull())
                return icon;
        }

        return d->fallback_;
    } else if (!ext.isEmpty()) {
        return d->iconForMimeType(QString("application-x-") + ext);
    } else {
        return d->fallback_;
    }
}
