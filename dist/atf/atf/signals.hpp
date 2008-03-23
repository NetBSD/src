//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#if !defined(_ATF_SIGNALS_HPP_)
#define _ATF_SIGNALS_HPP_

extern "C" {
#include <signal.h>
}

#include <map>

namespace atf {
namespace signals {

//
// Define last_signo to the last signal number valid for the system.
// This is tricky.  For example, NetBSD defines SIGPWR as the last valid
// number, whereas Mac OS X defines it as SIGTHR.  Both share the same
// signal number (32).  If none of these are available, we assume that
// the highest signal is SIGUSR2.
//
// TODO: Make this a configure check that uses ::kill and finds the first
// number that returns EINVAL.  The result is probably usable in the
// shell interface too.
//
#if defined(SIGTHR) && defined(SIGPWR)
#   if SIGTHR > SIGPWR
static const int last_signo = SIGTHR;
#   elif SIGPWR < SIGTHR
static const int last_signo = SIGPWR;
#   else
static const int last_signo = SIGPWR;
#   endif
#elif defined(SIGTHR)
static const int last_signo = SIGTHR;
#elif defined(SIGPWR)
static const int last_signo = SIGPWR;
#else
static const int last_signo = SIGUSR2;
#endif

// ------------------------------------------------------------------------
// The "signal_holder" class.
// ------------------------------------------------------------------------

//
// A RAII model to hold a signal while the object is alive.
//
class signal_holder {
    int m_signal;
    bool m_happened;
    struct sigaction m_sanew, m_saold;
    static std::map< int, signal_holder* > m_holders;

    static void handler(int);

    void program(void);

public:
    signal_holder(int);
    ~signal_holder(void);

    void process(void);
};

// ------------------------------------------------------------------------
// The "signal_programmer" class.
// ------------------------------------------------------------------------

//
// A RAII model to program a signal while the object is alive.
//
class signal_programmer {
    int m_signal;
    struct sigaction m_saold;

public:
    signal_programmer(int s, void (*handler)(int));
    ~signal_programmer(void);
};

} // namespace signals
} // namespace atf

#endif // !defined(_ATF_SIGNALS_HPP_)
