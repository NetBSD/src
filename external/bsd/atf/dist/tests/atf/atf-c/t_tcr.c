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

#include <fcntl.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/tcr.h"

#include "h_lib.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
void
serialize(const atf_tcr_t *tcr, const char *filename)
{
    int fd;

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ATF_REQUIRE(fd != -1);
    RE(atf_tcr_serialize(tcr, fd));
    close(fd);
}

static
void
deserialize(atf_tcr_t *tcr, const char *filename)
{
    int fd;

    fd = open(filename, O_RDONLY);
    ATF_REQUIRE(fd != -1);
    RE(atf_tcr_deserialize(tcr, fd));
    close(fd);
}

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

    RE(atf_tcr_init(&tcr, atf_tcr_passed_state));
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

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "The reason"));
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "The reason "
                               "with %s %d", "string", 5));
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "The reason"));
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "The reason "
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

    RE(atf_tcr_init(&tcr, atf_tcr_passed_state));
    ATF_REQUIRE_EQ(atf_tcr_get_state(&tcr), atf_tcr_passed_state);
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "The reason"));
    ATF_REQUIRE_EQ(atf_tcr_get_state(&tcr), atf_tcr_failed_state);
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "The reason"));
    ATF_REQUIRE_EQ(atf_tcr_get_state(&tcr), atf_tcr_skipped_state);
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

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "Failed"));
    reason = atf_tcr_get_reason(&tcr);
    ATF_REQUIRE(atf_equal_dynstr_cstring(reason, "Failed"));
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "Failed with "
                               "%s %d", "string", 5));
    reason = atf_tcr_get_reason(&tcr);
    ATF_REQUIRE(atf_equal_dynstr_cstring(reason, "Failed with string 5"));
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "Skipped"));
    reason = atf_tcr_get_reason(&tcr);
    ATF_REQUIRE(atf_equal_dynstr_cstring(reason, "Skipped"));
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "Skipped with "
                               "%s %d", "string", 5));
    reason = atf_tcr_get_reason(&tcr);
    ATF_REQUIRE(atf_equal_dynstr_cstring(reason, "Skipped with string 5"));
    atf_tcr_fini(&tcr);
}

ATF_TC(equal);
ATF_TC_HEAD(equal, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_equal_tcr_tcr function");
}
ATF_TC_BODY(equal, tc)
{
    atf_tcr_t passed1, passed2;
    atf_tcr_t failed1, failed2, failed3;
    atf_tcr_t skipped1, skipped2, skipped3;

    RE(atf_tcr_init(&passed1, atf_tcr_passed_state));
    RE(atf_tcr_init(&passed2, atf_tcr_passed_state));
    RE(atf_tcr_init_reason_fmt(&failed1, atf_tcr_failed_state, "F1"));
    RE(atf_tcr_init_reason_fmt(&failed2, atf_tcr_failed_state, "F1"));
    RE(atf_tcr_init_reason_fmt(&failed3, atf_tcr_failed_state, "F2"));
    RE(atf_tcr_init_reason_fmt(&skipped1, atf_tcr_skipped_state, "F1"));
    RE(atf_tcr_init_reason_fmt(&skipped2, atf_tcr_skipped_state, "F1"));
    RE(atf_tcr_init_reason_fmt(&skipped3, atf_tcr_skipped_state, "F2"));

    ATF_CHECK(atf_equal_tcr_tcr(&passed1, &passed1));
    ATF_CHECK(atf_equal_tcr_tcr(&passed1, &passed2));
    ATF_CHECK(!atf_equal_tcr_tcr(&passed1, &failed1));
    ATF_CHECK(!atf_equal_tcr_tcr(&passed1, &skipped1));

    ATF_CHECK(atf_equal_tcr_tcr(&failed1, &failed1));
    ATF_CHECK(atf_equal_tcr_tcr(&failed1, &failed2));
    ATF_CHECK(!atf_equal_tcr_tcr(&failed1, &failed3));
    ATF_CHECK(!atf_equal_tcr_tcr(&failed1, &passed1));
    ATF_CHECK(!atf_equal_tcr_tcr(&failed1, &skipped1));

    ATF_CHECK(atf_equal_tcr_tcr(&skipped1, &skipped1));
    ATF_CHECK(atf_equal_tcr_tcr(&skipped1, &skipped2));
    ATF_CHECK(!atf_equal_tcr_tcr(&skipped1, &skipped3));
    ATF_CHECK(!atf_equal_tcr_tcr(&skipped1, &passed1));
    ATF_CHECK(!atf_equal_tcr_tcr(&skipped1, &failed1));

    atf_tcr_fini(&skipped3);
    atf_tcr_fini(&skipped2);
    atf_tcr_fini(&skipped1);
    atf_tcr_fini(&failed3);
    atf_tcr_fini(&failed2);
    atf_tcr_fini(&failed1);
    atf_tcr_fini(&passed2);
    atf_tcr_fini(&passed1);
}

ATF_TC(serialization);
ATF_TC_HEAD(serialization, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_tcr_serialize and "
                      "atf_tcr_deserialize functions");
}
ATF_TC_BODY(serialization, tc)
{
    atf_tcr_t tcr, tcr2;

    RE(atf_tcr_init(&tcr, atf_tcr_passed_state));
    serialize(&tcr, "passed");
    deserialize(&tcr2, "passed");
    ATF_CHECK(atf_equal_tcr_tcr(&tcr, &tcr2));
    atf_tcr_fini(&tcr2);
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_failed_state, "Failed"));
    serialize(&tcr, "failed");
    deserialize(&tcr2, "failed");
    ATF_CHECK(atf_equal_tcr_tcr(&tcr, &tcr2));
    atf_tcr_fini(&tcr2);
    atf_tcr_fini(&tcr);

    RE(atf_tcr_init_reason_fmt(&tcr, atf_tcr_skipped_state, "Skipped"));
    serialize(&tcr, "skipped");
    deserialize(&tcr2, "skipped");
    ATF_CHECK(atf_equal_tcr_tcr(&tcr, &tcr2));
    atf_tcr_fini(&tcr2);
    atf_tcr_fini(&tcr);
}

/* ---------------------------------------------------------------------
 * Tests cases for the header file.
 * --------------------------------------------------------------------- */

HEADER_TC(include, "atf-c/tcr.h", "d_include_tcr_h.c");

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
    ATF_TP_ADD_TC(tp, equal);
    ATF_TP_ADD_TC(tp, serialization);

    /* Add the test cases for the header file. */
    ATF_TP_ADD_TC(tp, include);

    return atf_no_error();
}
