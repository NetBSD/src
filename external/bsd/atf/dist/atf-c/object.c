/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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

#include <err.h>
#include <stdlib.h>

#include "atf-c/object.h"
#include "atf-c/sanity.h"

static size_t balance = 0;
static bool initialized = false;

/* ---------------------------------------------------------------------
 * The "atf_object" type.
 * --------------------------------------------------------------------- */

static const uint32_t atf_object_magic_alive = 0xcafebabe;
static const uint32_t atf_object_magic_dead  = 0xdeadbeef;

void
atf_object_init(atf_object_t *o)
{
    PRE(initialized);

    o->m_magic = atf_object_magic_alive;
    o->m_finis = 0;

    balance++;
}

void
atf_object_copy(atf_object_t *dest, const atf_object_t *src)
{
    atf_object_init(dest);
}

void
atf_object_fini(atf_object_t *o)
{
    PRE(initialized);

    PRE(o->m_magic == atf_object_magic_alive);
    PRE(o->m_finis == 0);
    PRE(balance > 0);

    o->m_magic = atf_object_magic_dead;
    o->m_finis++;

    balance--;
}

/* ---------------------------------------------------------------------
 * Free functions.
 * --------------------------------------------------------------------- */

static
void
check_balance(void)
{
    PRE(initialized);

    if (balance > 0) {
        warnx("FATAL ERROR: Invalid balance: %zd objects were not "
              "released", balance);
        abort();
    }
}

void
atf_init_objects(void)
{
    initialized = true;

    if (atexit(check_balance) == -1)
        err(EXIT_FAILURE, "FATAL ERROR: Cannot initialize object system");
}

void
atf_reset_exit_checks(void)
{
    balance = 0;
}
