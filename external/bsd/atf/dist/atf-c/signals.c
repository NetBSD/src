/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(HAVE_CONFIG_H)
#include "bconfig.h"
#endif

#include <sys/types.h>

#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#include "atf-c/error.h"
#include "atf-c/sanity.h"
#include "atf-c/signals.h"

const int atf_signals_last_signo = LAST_SIGNO;

/* ---------------------------------------------------------------------
 * The "atf_signal_holder" type.
 * --------------------------------------------------------------------- */

static bool happened[LAST_SIGNO + 1];

static
void
holder_handler(int signo)
{
    happened[signo] = true;
}

/*
 * Constructors/destructors.
 */

atf_error_t
atf_signal_holder_init(atf_signal_holder_t *sh, int signo)
{
    atf_error_t err;

    err = atf_signal_programmer_init(&sh->m_sp, signo, holder_handler);
    if (atf_is_error(err))
        goto out;

    atf_object_init(&sh->m_object);
    sh->m_signo = signo;
    happened[signo] = false;

    INV(!atf_is_error(err));
out:
    return err;
}

void
atf_signal_holder_fini(atf_signal_holder_t *sh)
{
    atf_signal_programmer_fini(&sh->m_sp);

    if (happened[sh->m_signo])
        kill(getpid(), sh->m_signo);

    atf_object_fini(&sh->m_object);
}

/*
 * Getters.
 */

void
atf_signal_holder_process(atf_signal_holder_t *sh)
{
    if (happened[sh->m_signo]) {
        int ret;
        atf_error_t err;

        atf_signal_programmer_fini(&sh->m_sp);
        happened[sh->m_signo] = false;

        ret = kill(getpid(), sh->m_signo);
        INV(ret != -1);

        err = atf_signal_programmer_init(&sh->m_sp, sh->m_signo,
                                         holder_handler);
        INV(!atf_is_error(err));
    }
}

/* ---------------------------------------------------------------------
 * The "atf_signal_programmer" type.
 * --------------------------------------------------------------------- */

/*
 * Constructors/destructors.
 */

atf_error_t
atf_signal_programmer_init(atf_signal_programmer_t *sp, int signo,
                           atf_signal_handler_t handler)
{
    atf_error_t err;
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(signo, &sa, &sp->m_oldsa) == -1)
        err = atf_libc_error(errno, "Failed to program signal %d", signo);
    else {
        atf_object_init(&sp->m_object);
        sp->m_signo = signo;
        err = atf_no_error();
    }

    return err;
}

void
atf_signal_programmer_fini(atf_signal_programmer_t *sp)
{
    if (sigaction(sp->m_signo, &sp->m_oldsa, NULL) == -1)
        UNREACHABLE;

    atf_object_fini(&sp->m_object);
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

void
atf_signal_reset(int signo)
{
    struct sigaction sa;

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    (void)sigaction(signo, &sa, NULL);
}
