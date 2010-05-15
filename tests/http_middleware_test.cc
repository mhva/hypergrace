#include <stdexcept>

#include <gtest/gtest.h>

#include <http/inputmiddleware.hh>
#include <http/responseassembler.hh>
#include <http/response.hh>
#include <http/request.hh>

#include <net/bootstrapconnection.hh>
#include <net/reactor.hh>
#include <net/socket.hh>

#include <thread/event.hh>

using namespace Hypergrace;


class ResponseDeliveryBridge : public Http::InputMiddleware
{
public:
    ResponseDeliveryBridge() :
        response(0)
    {
    }

    ~ResponseDeliveryBridge()
    {
        delete response;
    }

    void processResponse(Net::Socket &, Http::Response &response)
    {
        this->response = new Http::Response(response);
        responseArrived.broadcastSignal();
    }

public:
    Thread::Event responseArrived;
    Http::Response *response;
};

class HttpMiddlewareTest : public ::testing::Test
{
public:
    HttpMiddlewareTest()
    {
        if (!reactor_.start())
            throw std::runtime_error("Reactor has failed to initialize!");
    }

public:
    Net::Reactor reactor_;
};

TEST_F(HttpMiddlewareTest, TestResponseDelivery)
{
    Http::InputMiddleware::Pointer bridge(new ResponseDeliveryBridge());

    Net::BootstrapConnection()
        .withReactor(&reactor_)
        .withEndpoint("www.hypergrace.com", 80)
        .withMiddleware(new Http::ResponseAssembler(bridge))
        .withQueuedPacket(new Http::Request("http://www.hypergrace.com/"))
        .initiate();

    std::static_pointer_cast<ResponseDeliveryBridge>(bridge)->responseArrived.wait();
    auto *response = std::static_pointer_cast<ResponseDeliveryBridge>(bridge)->response;

    ASSERT_TRUE(response != 0);
    ASSERT_EQ(200, response->statusCode());
}
