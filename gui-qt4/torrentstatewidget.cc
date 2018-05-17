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
#include <numeric>

#include <QDebug>
#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QTimer>
#include <QSize>
#include <QVBoxLayout>

#include <bt/bundle/torrentbundle.hh>
#include <bt/bundle/torrentmodel.hh>
#include <bt/bundle/torrentstate.hh>
#include <bt/globaltorrentregistry.hh>
#include <util/bitfield.hh>

#include "fileiconprovider.hh"
#include "prettyprint.hh"
#include "torrentstatewidget.hh"

using namespace Hypergrace;


TorrentStateWidget::TorrentStateWidget(Hypergrace::Bt::TorrentBundle *bundle, QWidget *parent) :
    QWidget(parent),
    bundle_(bundle),
    titleText_(new QLabel(this)),
    statusText_(new QLabel(this)),
    progress_(new QProgressBar(this)),
    updateTimer_(new QTimer(this))
{
    QHBoxLayout *titleRow = new QHBoxLayout(0);
    QHBoxLayout *progressBarRow = new QHBoxLayout(0);
    QHBoxLayout *textRow = new QHBoxLayout(0);

    // Setup title row.
    {
        titleText_->setText(QString::fromUtf8(bundle->model().name().c_str()));
        statusText_->setStyleSheet("QLabel { font-size: 9pt; }");
    }

    // Setup progress bar row.
    {
        progress_->setMaximumHeight(12);
        progress_->setStyleSheet("QProgressBar { font-size: 9px; }");
    }

    // Setup status row.
    {
        statusText_->setStyleSheet("QLabel { color: #4a4a4a; font-size: 7pt; }");
    }

    titleRow->addWidget(titleText_);
    progressBarRow->addWidget(progress_);
    textRow->addWidget(statusText_);
    textRow->addStretch();

    QVBoxLayout *rightPart = new QVBoxLayout(0);
    rightPart->setSpacing(0);
    rightPart->addLayout(titleRow);
    rightPart->addSpacing(4);
    rightPart->addLayout(progressBarRow);
    rightPart->addSpacing(3);
    rightPart->addLayout(textRow);

    QLabel *pixmap = new QLabel(this);
    {
        QIcon icon;

        if (bundle_->model().fileList().size() > 1) {
            icon = QIcon::fromTheme("folder", QIcon(":/mimetype-folder48.png"));
        } else {
            icon = FileIconProvider().icon(QString::fromUtf8(
                        bundle_->model().fileList()[0].filename.c_str()));
        }

        pixmap->setPixmap(icon.pixmap(48, 48));
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->addWidget(pixmap);
    mainLayout->addLayout(rightPart);

    updateDisplay();

    QObject::connect(updateTimer_, SIGNAL(timeout()), this, SLOT(updateDisplay()));
}

TorrentStateWidget::~TorrentStateWidget()
{
}

void TorrentStateWidget::start()
{
    Hypergrace::Bt::GlobalTorrentRegistry::self()->startTorrent(bundle_);

    updateTimer_->start(1000);
}

void TorrentStateWidget::stop()
{
    Hypergrace::Bt::GlobalTorrentRegistry::self()->stopTorrent(bundle_);

    updateTimer_->stop();
}

QSize TorrentStateWidget::sizeHint() const
{
    return QSize(1000, 25);
}

void TorrentStateWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QColor(255, 255, 255));
}

size_t TorrentStateWidget::calculateAverageDownloadRate()
{
    if (downloadRateHistory_.size() >= 30)
        downloadRateHistory_.pop_back();

    downloadRateHistory_.push_front(bundle_->state().downloadRate());
    size_t dlTotal = std::accumulate(downloadRateHistory_.begin(), downloadRateHistory_.end(), 0);

    return dlTotal / downloadRateHistory_.size();
}

ssize_t TorrentStateWidget::calculateRemainingDownloadTime()
{
    size_t downloadRate = calculateAverageDownloadRate();

    if (downloadRate >= 0) {
        unsigned int pieceCount = bundle_->state().scheduledPieces().enabledCount();
        unsigned int pieceSize = bundle_->model().pieceSize();

        if (downloadRate != 0)
            timeHistory_.push_back(pieceCount * pieceSize / downloadRate);
        else
            timeHistory_.push_back(-1024);
    }

    if (timeHistory_.size() >= 15) {
        timeHistory_.pop_front();

        ssize_t total = std::accumulate(timeHistory_.begin(), timeHistory_.end(), 0);

        if (total / timeHistory_.size() >= 0) {
            ssize_t eta = total / timeHistory_.size();

            // If we are drifting on borders, round up to the higher
            // minute. Should help to reduce time fluctuations when
            // rate is more or less stable.
            if (eta / 60 >= 1 && eta % 60 >= 50)
                eta += 60 - eta % 60;

            return eta;
        } else {
            // Looks like we are doomed to eternity.
            return -1;
        }
    } else {
        // Don't approximate remaining time if we haven't collected
        // enough statistics.
        return -1;
    }
}

void TorrentStateWidget::updateDisplay()
{
    const Hypergrace::Bt::TorrentState &state = bundle_->state();
    const Hypergrace::Bt::TorrentModel &model = bundle_->model();

    const Hypergrace::Util::Bitfield &schedPieces = state.scheduledPieces();
    const Hypergrace::Util::Bitfield &availPieces = state.availablePieces();

    unsigned int pieceSize = model.pieceSize();
    unsigned long long torrentSize = model.torrentSize();

    int goal = schedPieces.enabledCount() + availPieces.enabledCount();

    if (progress_->maximum() != goal) {
        progress_->setMinimum(0);
        progress_->setMaximum(goal);
    }

    progress_->setValue(availPieces.enabledCount());

    if (schedPieces.enabledCount() > 0) {
        // Download is not finished yet.
        unsigned long long overallSize = goal * pieceSize;

        if (schedPieces.bit(schedPieces.bitCount() - 1) && torrentSize % pieceSize != 0) {
            overallSize -= pieceSize - torrentSize % pieceSize;
        }

        auto prettyDlSize = PrettyPrint::represent(state.downloaded());
        auto prettyOvSize= PrettyPrint::represent(overallSize);

        if (prettyDlSize.second == prettyOvSize.second) {
            statusText_->setText(trUtf8("Downloading %1 of %2 (%3)  –  %4 remaining")
                    .arg(QString::number(prettyDlSize.first, 'f', 1))
                    .arg(QString::number(prettyOvSize.first, 'f', 1) + " " +
                         PrettyPrint::printUnit(prettyOvSize.second))
                    .arg(PrettyPrint::printSpeed(state.downloadRate()))
                    .arg(PrettyPrint::printRemainingTime(calculateRemainingDownloadTime()))
            );
        } else {
            statusText_->setText(trUtf8("Downloading %1 of %2 (%3)  –  %4 remaining")
                    .arg(QString::number(prettyDlSize.first, 'f', 1) + " " +
                         PrettyPrint::printUnit(prettyDlSize.second))
                    .arg(QString::number(prettyOvSize.first, 'f', 1) + " " +
                         PrettyPrint::printUnit(prettyOvSize.second))
                    .arg(PrettyPrint::printSpeed(state.downloadRate()))
                    .arg(PrettyPrint::printRemainingTime(calculateRemainingDownloadTime()))
            );
        }
    } else {
        // Download finished.
        statusText_->setText(trUtf8("%1  –  download complete")
                .arg(PrettyPrint::printSize(availPieces.enabledCount() * pieceSize)));
    }
}
