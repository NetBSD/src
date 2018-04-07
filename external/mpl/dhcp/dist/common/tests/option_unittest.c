/*	$NetBSD: option_unittest.c,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/*
 * Copyright (C) 2018 Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.	 IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>
#include <atf-c.h>
#include "dhcpd.h"

ATF_TC(option_refcnt);

ATF_TC_HEAD(option_refcnt, tc)
{
    atf_tc_set_md_var(tc, "descr",
		      "Verify option reference count does not overflow.");
}

/* This test does a simple check to see if option reference count is
 * decremented even an error path exiting parse_option_buffer()
 */
ATF_TC_BODY(option_refcnt, tc)
{
    struct option_state *options;
    struct option *option;
    unsigned code;
    int refcnt;
    unsigned char buffer[3] = { 15, 255, 0 };

    initialize_common_option_spaces();

    options = NULL;
    if (!option_state_allocate(&options, MDL)) {
	atf_tc_fail("can't allocate option state");
    }

    option = NULL;
    code = 15; /* domain-name */
    if (!option_code_hash_lookup(&option, dhcp_universe.code_hash,
				 &code, 0, MDL)) {
	atf_tc_fail("can't find option 15");
    }
    if (option == NULL) {
	atf_tc_fail("option is NULL");
    }
    refcnt = option->refcnt;

    buffer[0] = 15;
    buffer[1] = 255; /* invalid */
    buffer[2] = 0;

    if (parse_option_buffer(options, buffer, 3, &dhcp_universe)) {
	atf_tc_fail("parse_option_buffer is expected to fail");
    }

    if (refcnt != option->refcnt) {
	atf_tc_fail("refcnt changed from %d to %d", refcnt, option->refcnt);
    }
}

ATF_TC(pretty_print_option);

ATF_TC_HEAD(pretty_print_option, tc)
{
    atf_tc_set_md_var(tc, "descr",
		      "Verify pretty_print_option does not overrun its buffer.");
}


/*
 * This test verifies that pretty_print_option() will not overrun its
 * internal, static buffer when given large 'x/X' format options.
 *
 */
ATF_TC_BODY(pretty_print_option, tc)
{
    struct option *option;
    unsigned code;
    unsigned char bad_data[32*1024];
    unsigned char good_data[] = { 1,2,3,4,5,6 };
    int emit_commas = 1;
    int emit_quotes = 1;
    const char *output_buf;

    /* Initialize whole thing to non-printable chars */
    memset(bad_data, 0x1f, sizeof(bad_data));

    initialize_common_option_spaces();

    /* We'll use dhcp_client_identitifer because it happens to be format X */
    code = 61;
    option = NULL;
    if (!option_code_hash_lookup(&option, dhcp_universe.code_hash,
				 &code, 0, MDL)) {
	    atf_tc_fail("can't find option %d", code);
    }

    if (option == NULL) {
	    atf_tc_fail("option is NULL");
    }

    /* First we will try a good value we know should fit. */
    output_buf = pretty_print_option (option, good_data, sizeof(good_data),
                                      emit_commas, emit_quotes);

    /* Make sure we get what we expect */
    if (!output_buf || strcmp(output_buf, "1:2:3:4:5:6")) {
	    atf_tc_fail("pretty_print_option did not return \"<error>\"");
    }


    /* Now we'll try a data value that's too large */
    output_buf = pretty_print_option (option, bad_data, sizeof(bad_data),
                                      emit_commas, emit_quotes);

    /* Make sure we safely get an error */
    if (!output_buf || strcmp(output_buf, "<error>")) {
	    atf_tc_fail("pretty_print_option did not return \"<error>\"");
    }
}


/* This macro defines main() method that will call specified
   test cases. tp and simple_test_case names can be whatever you want
   as long as it is a valid variable identifier. */
ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, option_refcnt);
    ATF_TP_ADD_TC(tp, pretty_print_option);

    return (atf_no_error());
}
