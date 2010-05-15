/*
   Copyright (C) 2010 Anton Mihalyov <anton@glyphsense.com>

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

#include <QObject>

#include "prettyprint.hh"

#define KiB (1024ULL)
#define MiB (1024ULL * KiB)
#define GiB (1024ULL * MiB)
#define TiB (1024ULL * GiB)


std::pair<double, PrettyPrint::Unit> PrettyPrint::represent(unsigned long long size)
{

    Unit name;

    unsigned long long unit;

    if (size / GiB > 0) {
        name = Gigabyte;
        unit = GiB;
    } else if (size / MiB > 0) {
        name = Megabyte;
        unit = MiB;
    } else {
        name = Kilobyte;
        unit = KiB;
    }

    return std::make_pair((double)size / unit, name);
}

QString PrettyPrint::printSize(unsigned long long size)
{
    auto prettySize = represent(size);

    return QString("%1 %2")
        .arg(QString::number(prettySize.first, 'f', 1))
        .arg(printUnit(prettySize.second));
}

QString PrettyPrint::printSpeed(int speed)
{
    if (speed >= 0) {
        auto prettySize = represent(speed);

        if (prettySize.first > 100.0 && prettySize.second == Kilobyte) {
            return QString(QObject::tr("%1 %2/sec"))
                .arg(QString::number(prettySize.first, 'f', 0))
                .arg(printUnit(prettySize.second));
        } else {
            return QString(QObject::tr("%1 %2/sec"))
                .arg(QString::number(prettySize.first, 'f', 1))
                .arg(printUnit(prettySize.second));
        }
    } else {
        return QObject::tr("? KB/sec");
    }
}

QString PrettyPrint::printUnit(Unit unit)
{
    switch (unit) {
    case Kilobyte:
        return QObject::tr("KB");
    case Megabyte:
        return QObject::tr("MB");
    case Gigabyte:
        return QObject::tr("GB");
    case Terabyte:
        return QObject::tr("TB");
    default:
        return "??";
    }
}

QString PrettyPrint::printRemainingTime(int seconds)
{
    if (seconds >= 24 * 60 * 60) {
        return QObject::tr("%n day(s) ", "", seconds / (24 * 60 * 60)) +
               QObject::tr("%n hours(s) ", "", seconds % (24 * 60 * 60) / (60 * 60)) +
               QObject::tr("%n minute(s)", "", seconds % (60 * 60) / 60);
    } else if (seconds >= 60 * 60) {
        return QObject::tr("%n hours(s) ", "", seconds / (60 * 60)) +
               QObject::tr("%n minute(s)", "", seconds % (60 * 60) / 60);
    } if (seconds >= 60) {
        return QObject::tr("%n minute(s)", "", seconds / 60);
    } else if (seconds >= 10) {
        return QObject::tr("less than a minute");
    } else if (seconds >= 0) {
        return QObject::tr("few seconds");
    } else {
        return QObject::tr("unknown time");
    }
}
