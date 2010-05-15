#ifndef OUTPUTSUPPRESSOR_HH_
#define OUTPUTSUPPRESSOR_HH_

#include <debug/debug.hh>
#include <debug/nulloutputfacility.hh>

#define SUPPRESS_OUTPUT OutputSuppressor __output_supressor__

class OutputSuppressor
{
public:
    OutputSuppressor()
    {
        previous = Hypergrace::Debug::Debug::setOutputFacility(&nullFacility);
    }

    ~OutputSuppressor()
    {
        Hypergrace::Debug::Debug::setOutputFacility(previous);
    }

private:
    Hypergrace::Debug::NullOutputFacility nullFacility;
    Hypergrace::Debug::OutputFacility *previous;
};

#endif // OUTPUTSUPPRESSOR_HH_
