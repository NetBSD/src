//
// Automated Testing Framework (atf)
//
// Copyright (c) 2010 The NetBSD Foundation, Inc.
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
#include <signal.h>
}

#include <cerrno>

#include "atf-c++/detail/exceptions.hpp"
#include "atf-c++/detail/sanity.hpp"

#include "signals.hpp"
#include "timer.hpp"

namespace impl = atf::atf_run;
#define IMPL_NAME "atf::atf_run"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

namespace sigalrm {

bool m_fired = false;
impl::timer* m_timer = NULL;

void
handler(const int signo)
{
    PRE(signo == SIGALRM);

    m_fired = true;
    m_timer->timeout_callback();
}

} // anonymous namespace

// ------------------------------------------------------------------------
// The "timer" class.
// ------------------------------------------------------------------------

impl::timer::timer(const unsigned int seconds)
{
    sigalrm::m_fired = false;
    sigalrm::m_timer = this;
    m_sigalrm.reset(new signal_programmer(SIGALRM, sigalrm::handler));

    ::itimerval timeval;
    timeval.it_interval.tv_sec = 0;
    timeval.it_interval.tv_usec = 0;
    timeval.it_value.tv_sec = seconds;
    timeval.it_value.tv_usec = 0;

    if (::setitimer(ITIMER_REAL, &timeval, &m_old_timeval) == -1)
        throw system_error(IMPL_NAME "::timer::timer",
                           "Failed to program timer", errno);
}

impl::timer::~timer(void)
{
    const int ret = ::setitimer(ITIMER_REAL, &m_old_timeval, NULL);
    INV(ret != -1);
    sigalrm::m_timer = NULL;
    sigalrm::m_fired = false;
}

bool
impl::timer::fired(void)
    const
{
    return sigalrm::m_fired;
}

// ------------------------------------------------------------------------
// The "child_timer" class.
// ------------------------------------------------------------------------

impl::child_timer::child_timer(const unsigned int seconds, const pid_t pid,
                               volatile bool& terminate) :
    timer(seconds),
    m_pid(pid),
    m_terminate(terminate)
{
}

impl::child_timer::~child_timer(void)
{
}

void
impl::child_timer::timeout_callback(void)
{
    m_terminate = true;

    // Should use killpg(2) but, according to signal(7), using this system
    // call in a signal handler context is not safe.
    ::kill(m_pid, SIGKILL);
}
