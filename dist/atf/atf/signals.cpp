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

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
}

#include <cerrno>

#include "atf/exceptions.hpp"
#include "atf/sanity.hpp"
#include "atf/signals.hpp"

namespace impl = atf::signals;
#define IMPL_NAME "atf::signals"

// ------------------------------------------------------------------------
// The "signal_holder" class.
// ------------------------------------------------------------------------

std::map< int, impl::signal_holder* > impl::signal_holder::m_holders;

void
impl::signal_holder::handler(int s)
{
    m_holders[s]->m_happened = true;
}

void
impl::signal_holder::program(void)
{
    if (::sigaction(m_signal, &m_sanew, &m_saold) == -1)
        throw atf::system_error(IMPL_NAME "::signal_holder::signal_holder",
                                "sigaction(2) failed", errno);
}

impl::signal_holder::signal_holder(int s) :
    m_signal(s),
    m_happened(false)
{
    m_sanew.sa_handler = handler;
    sigemptyset(&m_sanew.sa_mask);
    m_sanew.sa_flags = 0;

    program();
    PRE(m_holders.find(m_signal) == m_holders.end());
    m_holders[m_signal] = this;
}

impl::signal_holder::~signal_holder(void)
{
    int res = ::sigaction(m_signal, &m_saold, NULL);
    INV(res == 0);
    if (m_happened)
        ::kill(::getpid(), m_signal);
}

void
impl::signal_holder::process(void)
{
    if (m_happened) {
        int res = ::sigaction(m_signal, &m_saold, NULL);
        if (res == -1)
            throw atf::system_error(IMPL_NAME "::signal_holder::signal_holder",
                                    "sigaction(2) failed", errno);
        m_happened = false;
        ::kill(::getpid(), m_signal);
        program();
    }
}

// ------------------------------------------------------------------------
// The "signal_programmer" class.
// ------------------------------------------------------------------------

impl::signal_programmer::signal_programmer(int s, void (*handler)(int)) :
    m_signal(s)
{
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (::sigaction(m_signal, &sa, &m_saold) == -1)
        throw atf::system_error(IMPL_NAME
                                "::signal_programmer::signal_programmer",
                                "sigaction(2) failed", errno);
}

impl::signal_programmer::~signal_programmer(void)
{
    int res = ::sigaction(m_signal, &m_saold, NULL);
    INV(res == 0);
}
