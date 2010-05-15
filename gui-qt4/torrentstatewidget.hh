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

#ifndef TORRENTSTATEWIDGET_HH_
#define TORRENTSTATEWIDGET_HH_

#include <deque>
#include <memory>

#include <QLabel>
#include <QProgressBar>
#include <QWidget>

#include <util/time.hh>


class QLabel;
class QProgressBar;
class QPaintEvent;
class QTimer;
class QSize;
namespace Hypergrace { namespace Bt { class TorrentBundle; }}


class TorrentStateWidget : public QWidget
{
    Q_OBJECT

public:
    TorrentStateWidget(Hypergrace::Bt::TorrentBundle *, QWidget *);
    ~TorrentStateWidget();

    void start();
    void stop();

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);

private:
    size_t calculateAverageDownloadRate();
    ssize_t calculateRemainingDownloadTime();

private slots:
    void updateDisplay();

private:
    Hypergrace::Bt::TorrentBundle *bundle_;

    QLabel *titleText_;
    QLabel *statusText_;
    QProgressBar *progress_;

    QTimer *updateTimer_;

    std::deque<size_t> downloadRateHistory_;
    std::deque<ssize_t> timeHistory_;
};

#endif /* TORRENTSTATEWIDGET_HH_ */
