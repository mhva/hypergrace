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

#ifndef BT_PEERWIRE_UPLOADTASK_HH_
#define BT_PEERWIRE_UPLOADTASK_HH_

#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <bt/peerwire/message.hh>
#include <bt/peerwire/peerdata.hh>

#include <net/task.hh>

namespace Hypergrace { namespace Bt { class DiskIo; }}
namespace Hypergrace { namespace Bt { class TorrentBundle; }}


namespace Hypergrace {
namespace Bt {

class UploadTask : public Net::Task
{
public:
    UploadTask(TorrentBundle &, std::shared_ptr<DiskIo>);

    void registerPeer(PeerData *);
    void unregisterPeer(PeerData *);

    void notifyRequestEvent(PeerData *, unsigned int, unsigned int, unsigned int);
    void notifyCancelEvent(PeerData *, unsigned int, unsigned int, unsigned int);

    void execute();

private:
    void uploadBlocks(PeerData *);
    void cancelUpload(PeerData *);

    struct UploadState;

    void handleReadSuccess(std::shared_ptr<UploadState>, unsigned int, unsigned int, std::string);
    void handleReadFailure(std::shared_ptr<UploadState>, unsigned int, unsigned int);
    void handleMessageSentEvent(std::shared_ptr<UploadState>);

private:
    struct UploadState : public PeerData::CustomData
    {
        volatile size_t requestsInProcessing;
        std::deque<PieceMessage *> assembledMessages;
        std::deque<PieceMessage *> sentMessages;
        std::mutex anchor;
    };

private:
    TorrentBundle &bundle_;
    std::shared_ptr<DiskIo> ioThread_;
};

} /* namespace Bt */
} /* namespace Hypergrace */

#endif /* BT_PEERWIRE_UPLOADTASK_HH_ */
