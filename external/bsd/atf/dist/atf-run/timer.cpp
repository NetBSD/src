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

#include <csignal>

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

static
void
handler(int signo, siginfo_t *si, void *uc)
{
    impl::timer *timer = static_cast<impl::timer *>(si->si_value.sival_ptr);

    timer->setfired();
    timer->timeout_callback();
}

// ------------------------------------------------------------------------
// The "timer" class.
// ------------------------------------------------------------------------

impl::timer::timer(const unsigned int seconds) : m_fired(false)
{
    struct sigaction sa;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = ::handler;
    if (::sigaction(SIGALRM, &sa, &m_old_sa) == -1)
        throw system_error(IMPL_NAME "::timer::timer",
                           "Failed to set signal handler", errno);
	

    ::sigevent se;
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGALRM;
    se.sigev_value.sival_ptr = static_cast<void *>(this);
    se.sigev_notify_function = NULL;
    se.sigev_notify_attributes = NULL;
    if (::timer_create(CLOCK_MONOTONIC, &se, &m_timer) == -1) {
	::sigaction(SIGALRM, &m_old_sa, NULL);
        throw system_error(IMPL_NAME "::timer::timer",
                           "Failed to create timer", errno);
    }

    ::itimerspec it;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = seconds;
    it.it_value.tv_nsec = 0;
    if (::timer_settime(m_timer, 0, &it, &m_old_it) == -1) {
	::sigaction(SIGALRM, &m_old_sa, NULL);
	::timer_delete(m_timer);
        throw system_error(IMPL_NAME "::timer::timer",
                           "Failed to program timer", errno);
    }
}

impl::timer::~timer(void)
{
    int ret = ::timer_delete(m_timer);
    INV(ret != -1);
    ret = ::sigaction(SIGALRM, &m_old_sa, NULL);
    INV(ret != -1);
}

bool
impl::timer::fired(void)
    const
{
    return m_fired;
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
    static const timespec ts = { 1, 0 };
    m_terminate = true;
    ::kill(-m_pid, SIGTERM);
    ::nanosleep(&ts, NULL);
    if (::kill(-m_pid, 0) != -1)
	::kill(-m_pid, SIGKILL);
}
