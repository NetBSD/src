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

#include <atf-c.h>

#include "atf-c/tcr.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

/* ---------------------------------------------------------------------
 * Test cases for the "atf_tcr_t" type.
 * --------------------------------------------------------------------- */

ATF_TC(init);
ATF_TC_HEAD(init, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_tcr_init function");
}
ATF_TC_BODY(init, tc)
{
    atf_tcr_t tcr;

    CE(atf_tcr_init(&tcr, atf_tcr_passed_state));
    atf_tcr_fini(&tcr);
}

ATF_TC(init_reason);
ATF_TC_HEAD(init_reason, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_tcr_init_reason_fmt "
                      "function");
}
ATF_TC_BODY(init_reason, tc)
{
    atf_tcr_t tcr;

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "The reason"));
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "The reason "
                               "with %s %d", "string", 5));
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "The reason"));
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "The reason "
                               "with %s %d", "string", 5));
    atf_tcr_fini(&tcr);
}

ATF_TC(get_state);
ATF_TC_HEAD(get_state, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_tcr_get_state function");
}
ATF_TC_BODY(get_state, tc)
{
    atf_tcr_t tcr;

    CE(atf_tcr_init(&tcr, atf_tcr_passed_state));
    ATF_CHECK_EQUAL(atf_tcr_get_state(&tcr), atf_tcr_passed_state);
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "The reason"));
    ATF_CHECK_EQUAL(atf_tcr_get_state(&tcr), atf_tcr_failed_state);
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "The reason"));
    ATF_CHECK_EQUAL(atf_tcr_get_state(&tcr), atf_tcr_skipped_state);
    atf_tcr_fini(&tcr);
}

ATF_TC(get_reason);
ATF_TC_HEAD(get_reason, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_tcr_get_reason function");
}
ATF_TC_BODY(get_reason, tc)
{
    atf_tcr_t tcr;
    const atf_dynstr_t *reason;

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "Failed"));
    reason = atf_tcr_get_reason(&tcr);
    ATF_CHECK(atf_equal_dynstr_cstring(reason, "Failed"));
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "Failed with "
                               "%s %d", "string", 5));
    reason = atf_tcr_get_reason(&tcr);
    ATF_CHECK(atf_equal_dynstr_cstring(reason, "Failed with string 5"));
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "Skipped"));
    reason = atf_tcr_get_reason(&tcr);
    ATF_CHECK(atf_equal_dynstr_cstring(reason, "Skipped"));
    atf_tcr_fini(&tcr);

    CE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "Skipped with "
                               "%s %d", "string", 5));
    reason = atf_tcr_get_reason(&tcr);
    ATF_CHECK(atf_equal_dynstr_cstring(reason, "Skipped with string 5"));
    atf_tcr_fini(&tcr);
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    /* Add the test cases for the "atf_tcr_t" type. */
    ATF_TP_ADD_TC(tp, init);
    ATF_TP_ADD_TC(tp, init_reason);
    ATF_TP_ADD_TC(tp, get_state);
    ATF_TP_ADD_TC(tp, get_reason);

    return atf_no_error();
}
