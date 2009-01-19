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
#include "atf-c/error.h"
}

#include "atf-c++/exceptions.hpp"
#include "atf-c++/signals.hpp"

namespace impl = atf::signals;
#define IMPL_NAME "atf::signals"

const int impl::last_signo = atf_signals_last_signo;

// ------------------------------------------------------------------------
// The "signal_holder" class.
// ------------------------------------------------------------------------

impl::signal_holder::signal_holder(int signo)
{
    atf_error_t err = atf_signal_holder_init(&m_sh, signo);
    if (atf_is_error(err))
        throw_atf_error(err);
}

impl::signal_holder::~signal_holder(void)
{
    atf_signal_holder_fini(&m_sh);
}

void
impl::signal_holder::process(void)
{
    atf_signal_holder_process(&m_sh);
}

// ------------------------------------------------------------------------
// The "signal_programmer" class.
// ------------------------------------------------------------------------

impl::signal_programmer::signal_programmer(int signo, handler h)
{
    atf_error_t err = atf_signal_programmer_init(&m_sp, signo, h);
    if (atf_is_error(err))
        throw_atf_error(err);
}

impl::signal_programmer::~signal_programmer(void)
{
    atf_signal_programmer_fini(&m_sp);
}
