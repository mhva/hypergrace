set(LIBHYPERGRACE_VERSION 0.1.0)

set(LIBH_SOURCES
    bencode/reader.cc
    bencode/dictionaryworker.cc
    bencode/integerworker.cc
    bencode/listworker.cc
    bencode/stringworker.cc
    bencode/releasememoryvisitor.cc
    bencode/serializationvisitor.cc
    debug/debug.cc
    debug/outputfacility.cc
    debug/terminaloutputfacility.cc
    debug/nulloutputfacility.cc
    delegate/bind.cc
    delegate/connectable.cc
    delegate/signalbase.cc
    bt/announce/announcescheduler.cc
    bt/announce/tracker.cc
    bt/announce/trackerresponse.cc
    bt/bundlebuilders/bundlebuilder.cc
    bt/bundlebuilders/bundleunmarshaller.cc
    bt/bundlebuilders/localfilebundlebuilder.cc
    bt/bundle/peerregistry.cc
    bt/bundle/torrentconfiguration.cc
    bt/bundle/torrentbundle.cc
    bt/bundle/torrentstate.cc
    bt/bundle/torrentmodel.cc
    bt/bundle/trackerregistry.cc
    bt/io/blockcache.cc
    bt/io/diskio.cc
    bt/peerwire/commandtask.cc
    bt/peerwire/choketask.cc
    bt/peerwire/downloadtask.cc
    bt/peerwire/interesttask.cc
    bt/peerwire/eventhub.cc
    bt/peerwire/pieceadvisor.cc
    bt/peerwire/peerdata.cc
    bt/peerwire/peerdatacollector.cc
    bt/peerwire/uploadtask.cc
    bt/peerwire/acceptorservice.cc
    bt/peerwire/connectioninitiator.cc
    bt/peerwire/handshakenegotiator.cc
    bt/peerwire/inputmiddleware.cc
    bt/peerwire/messageassembler.cc
    bt/globaltorrentregistry.cc
    http/inputmiddleware.cc
    http/responseassembler.cc
    http/request.cc
    http/response.cc
    http/uri.cc
    net/acceptorservice.cc
    net/bandwidthallocator.cc
    net/socket.cc
    net/bootstrapconnection.cc
    net/rateaccumulator.cc
    net/hostaddress.cc
    net/inputmiddleware.cc
    net/outputmiddleware.cc
    net/packet.cc
    net/reactor_linux.cc               # <-- FIXME: MUST BE INCLUDED ONLY IN THE LINUX BUILDS
    net/task.cc
    net/tcpsocket.cc
    thread/event_linux.cc              # <-- FIXME: MUST BE INCLUDED ONLY IN THE LINUX BUILDS
    util/backtrace.cc
    util/bitfield.cc
    util/filesystem.cc
    util/rating.cc
    util/time.cc
    util/sha1hash.cc
)

include_directories(${CMAKE_SOURCE_DIR}/libhypergrace)

add_library(hypergrace SHARED ${LIBH_SOURCES})
set_target_properties(hypergrace PROPERTIES VERSION ${LIBHYPERGRACE_VERSION} LINKER_LANGUAGE CXX)
