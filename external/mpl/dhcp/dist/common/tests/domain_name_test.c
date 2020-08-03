/*	$NetBSD: domain_name_test.c,v 1.2 2020/08/03 21:10:56 christos Exp $	*/

/*
 * Copyright (C) 2019 Internet Systems Consortium, Inc. ("ISC")
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

ATF_TC(pretty_print_domain_name);

ATF_TC_HEAD(pretty_print_domain_name, tc)
{
    atf_tc_set_md_var(tc, "descr",
		      "Verify pretty_print_option can render a domain name.");
}

/*
 * This test verifies that pretty_print_option() correctly render a
 * domain name.
 *
 */
ATF_TC_BODY(pretty_print_domain_name, tc)
{
    struct option *option;
    unsigned code;
    unsigned char good_data[] =
	{ 0x05, 0x62, 0x6f, 0x6f, 0x79, 0x61, 0x03, 0x63, 0x6f, 0x6d, 0x00 };
    unsigned char short_data[] =
	{ 0x05, 0x62, 0x6f, 0x6f, 0x79, 0x61, 0x03, 0x63, 0x6f, 0x6d };
    unsigned char long_data[] =
	{ 0x05, 0x62, 0x6f, 0x6f, 0x79, 0x61, 0x03, 0x63, 0x6f, 0x6d, 0x00,
	  0x01, 0x02 };
    const char *output_buf;

    initialize_common_option_spaces();

    /* We'll use v4-lost because it happens to be format d */
    code = 137;
    option = NULL;
    if (!option_code_hash_lookup(&option, dhcp_universe.code_hash,
				 &code, 0, MDL)) {
	    atf_tc_fail("can't find option %d", code);
    }

    if (option == NULL) {
	    atf_tc_fail("option is NULL");
    }

    /* First we will try a good value we know should fit. */
    output_buf = pretty_print_option(option, good_data, sizeof(good_data), 0,
				     0);

    /* Make sure we get what we expect */
    if (!output_buf || strcmp(output_buf, "booya.com.")) {
	    atf_tc_fail("pretty_print_option did not return 'booya.com.'");
    }

    fprintf(stderr, "good!\n");

    /* Now we'll try a data value that's too short */
    output_buf = pretty_print_option(option, short_data, sizeof(short_data),
				     0, 0);

    /* Make sure we safely get an error */
    if (!output_buf || strcmp(output_buf, "<error>")) {
	    atf_tc_fail("pretty_print_option did not return \"<error>\"");
    }

    /* Now we'll try a data value that's too large */
    output_buf = pretty_print_option(option, long_data, sizeof(long_data), 0,
				     0);

    /* This logs but does not return an error */
    if (!output_buf || strcmp(output_buf, "booya.com.")) {
	atf_tc_fail("pretty_print_option did not return 'booya.com.' (large)");
    }
}


/* This macro defines main() method that will call specified
   test cases. tp and simple_test_case names can be whatever you want
   as long as it is a valid variable identifier. */
ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, pretty_print_domain_name);

    return (atf_no_error());
}
