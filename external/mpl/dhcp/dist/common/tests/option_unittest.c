/*	$NetBSD: option_unittest.c,v 1.4 2022/04/03 01:10:58 christos Exp $	*/

/*
 * Copyright (C) 2018-2022 Internet Systems Consortium, Inc. ("ISC")
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

ATF_TC(parse_X);

ATF_TC_HEAD(parse_X, tc)
{
    atf_tc_set_md_var(tc, "descr",
		      "Verify parse_X services option too big.");
}

/* Initializes a parse struct from an input buffer of data. */
static void init_parse(struct parse *cfile, char* name, char *input) {
    memset(cfile, 0, sizeof(struct parse));
    cfile->tlname = name;
    cfile->lpos = cfile->line = 1;
    cfile->cur_line = cfile->line1;
    cfile->prev_line = cfile->line2;
    cfile->token_line = cfile->cur_line;
    cfile->cur_line[0] = cfile->prev_line[0] = 0;
    cfile->file = -1;
    cfile->eol_token = 0;

    cfile->inbuf = input;
    cfile->buflen = strlen(input);
    cfile->bufsiz = 0;
}

/*
 * This test verifies that parse_X does not overwrite the output
 * buffer when given input data that exceeds the output buffer
 * capacity.
*/
ATF_TC_BODY(parse_X, tc)
{
    struct parse cfile;
    u_int8_t output[10];
    unsigned len;

    /* Input hex literal */
    char *input = "01:02:03:04:05:06:07:08";
    unsigned expected_len = 8;

    /* Normal output plus two filler bytes */
    u_int8_t expected_plus_two[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xff, 0xff
    };

    /* Safe output when option is too long */
    unsigned short_buf_len = 4;
    u_int8_t expected_too_long[] = {
        0x01, 0x02, 0x03, 0x04, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    /* First we'll run one that works normally */
    memset(output, 0xff, sizeof(output));
    init_parse(&cfile, "hex_fits", input);

    len = parse_X(&cfile, output, expected_len);

    // Len should match the expected len.
    if (len != expected_len) {
	    atf_tc_fail("parse_X failed, output len: %d", len);
    }

    // We should not have written anything past the end of the buffer.
    if (memcmp(output, expected_plus_two, sizeof(output))) {
	    atf_tc_fail("parse_X failed, output does not match expected");
    }

    // Now we'll try it with a buffer that's too small.
    init_parse(&cfile, "hex_too_long", input);
    memset(output, 0xff, sizeof(output));

    len = parse_X(&cfile, output, short_buf_len);

    // On errors, len should be zero.
    if (len != 0) {
	    atf_tc_fail("parse_X failed, we should have had an error");
    }

    // We should not have written anything past the end of the buffer.
    if (memcmp(output, expected_too_long, sizeof(output))) {
        atf_tc_fail("parse_X overwrote buffer!");
    }
}

/* This macro defines main() method that will call specified
   test cases. tp and simple_test_case names can be whatever you want
   as long as it is a valid variable identifier. */
ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, option_refcnt);
    ATF_TP_ADD_TC(tp, pretty_print_option);
    ATF_TP_ADD_TC(tp, parse_X);

    return (atf_no_error());
}
