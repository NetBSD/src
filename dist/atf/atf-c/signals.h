/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

#if !defined(ATF_C_SIGNALS_H)
#define ATF_C_SIGNALS_H

#include <signal.h>
#include <stdbool.h>

#include <atf-c/error.h>
#include <atf-c/object.h>

extern const int atf_signals_last_signo;
typedef void (*atf_signal_handler_t)(int);

/* ---------------------------------------------------------------------
 * The "atf_signal_programmer" type.
 * --------------------------------------------------------------------- */

struct atf_signal_programmer {
    atf_object_t m_object;

    int m_signo;
    atf_signal_handler_t m_handler;
    struct sigaction m_oldsa;
};
typedef struct atf_signal_programmer atf_signal_programmer_t;

/* Constructors/destructors. */
atf_error_t atf_signal_programmer_init(atf_signal_programmer_t *, int,
                                       atf_signal_handler_t);
void atf_signal_programmer_fini(atf_signal_programmer_t *);

/* ---------------------------------------------------------------------
 * The "atf_signal_holder" type.
 * --------------------------------------------------------------------- */

struct atf_signal_holder {
    atf_object_t m_object;

    int m_signo;
    atf_signal_programmer_t m_sp;
};
typedef struct atf_signal_holder atf_signal_holder_t;

/* Constructors/destructors. */
atf_error_t atf_signal_holder_init(atf_signal_holder_t *, int);
void atf_signal_holder_fini(atf_signal_holder_t *);

/* Modifiers. */
void atf_signal_holder_process(atf_signal_holder_t *);

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

void atf_signal_reset(int);

#endif /* ATF_C_SIGNALS_H */
