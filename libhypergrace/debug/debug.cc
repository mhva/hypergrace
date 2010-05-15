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

#include <sstream>

#include "outputfacility.hh"
#include "terminaloutputfacility.hh"
#include "debug.hh"

using namespace Hypergrace::Debug;


class Debug::Private
{
public:
    Private(Debug::WhitespacePolicy whitespacePolicy) :
        whitespacePolicy_(whitespacePolicy)
    {
    }

    ~Private()
    {
        // TODO: Remove trailing whitespace from the string
        Private::facility->output(stream_.str());
    }

    template<typename T> inline void output(T &t)
    {
        stream_ << t;

        if (whitespacePolicy_)
            stream_ << ' ';
    }

    inline void output(const std::string &string)
    {
        stream_ << '\'' << string << '\'';
    }

    inline void output(Debug::WhitespacePolicy wp)
    {
        whitespacePolicy_ = wp;
    }

public:
    static OutputFacility *facility;

private:
    std::ostringstream stream_;
    bool whitespacePolicy_;
};

OutputFacility *Debug::Private::facility = new TerminalOutputFacility();

Debug::Debug(WhitespacePolicy whitespacePolicy) :
    d(new Debug::Private(whitespacePolicy))
{
}

Debug::~Debug()
{
    delete d;
}

OutputFacility *Debug::setOutputFacility(OutputFacility *facility)
{
    OutputFacility *prev = Private::facility;
    Private::facility = facility;
    return prev;
}

Debug &Debug::operator<< (bool v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (short v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (unsigned short v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (int v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (unsigned int v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (long v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (long long v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (unsigned long v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (unsigned long long v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (float v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (double v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (long double v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (const char *v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (const void *v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (const std::string &v)
{
    d->output(v);
    return *this;
}

Debug &Debug::operator<< (WhitespacePolicy wp)
{
    d->output(wp);
    return *this;
}
