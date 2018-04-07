/*	$NetBSD: misc_unittest.c,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/*
 * Copyright (C) 2014-2017 Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>
#include <atf-c.h>
#include "dhcpd.h"

struct basic_test {
	int count;
	int used_0;
	int used_25;
	int used_50;
	int used_75;
	int used_100;
};

struct basic_test basic_values[] =
	{{         0, 0,         0,          0,          0,          0},
	 {        10, 0,         2,          5,          7,         10},
	 {        20, 0,         5,         10,         15,         20},
	 {        50, 0,        12,         25,         37,         50},
	 {       100, 0,        25,         50,         75,        100},
	 {      1000, 0,       250,        500,        750,       1000},
	 {     10000, 0,      2500,       5000,       7500,      10000},
	 {    100000, 0,     25000,      50000,      75000,     100000},
	 {   1000000, 0,    250000,     500000,     750000,    1000000},
	 {  10000000, 0,   2500000,    5000000,    7500000,   10000000},
	 { 100000000, 0,  25000000,   50000000,   75000000,  100000000},
	 {1000000000, 0, 250000000,  500000000,  750000000, 1000000000},
	 {2000000000, 0, 500000000, 1000000000, 1500000000, 2000000000},
	 {-1, 0, 0, 0, 0, 0}};

ATF_TC(find_percent_basic);

ATF_TC_HEAD(find_percent_basic, tc)
{
    atf_tc_set_md_var(tc, "descr", "Verify basic percent calculation.");
}

/* This test does a simple check to see if we get the right value
 * given a count and a percnetage of that count
 */
ATF_TC_BODY(find_percent_basic, tc)
{
    struct basic_test *basic_value;
    int used;

    for (basic_value = &basic_values[0];
	 basic_value->count != -1;
	 basic_value++) {
	used = FIND_PERCENT(basic_value->count, 0);
	if (used != basic_value->used_0) {
	    atf_tc_fail("Wrong value for 0 - count: %d, used %d",
			basic_value->count, used);
	}

	used = FIND_PERCENT(basic_value->count, 25);
	if (used != basic_value->used_25) {
	    atf_tc_fail("Wrong value for 25 - count: %d, used %d",
			basic_value->count, used);
	}

	used = FIND_PERCENT(basic_value->count, 50);
	if (used != basic_value->used_50) {
	    atf_tc_fail("Wrong value for 50 - count: %d, used %d",
			basic_value->count, used);
	}

	used = FIND_PERCENT(basic_value->count, 75);
	if (used != basic_value->used_75) {
	    atf_tc_fail("Wrong value for 75 - count: %d, used %d",
			basic_value->count, used);
	}

	used = FIND_PERCENT(basic_value->count, 100);
	if (used != basic_value->used_100) {
	    atf_tc_fail("Wrong value for 100 - count: %d, used %d",
			basic_value->count, used);
	}
    }
    return;
}

ATF_TC(find_percent_adv);

ATF_TC_HEAD(find_percent_adv, tc)
{
    atf_tc_set_md_var(tc, "descr", "Verify advanced percent calculation.");
}

/* This test tries some more complicated items, such as using the macro
 * in a function or passing expressions into macro
 */
ATF_TC_BODY(find_percent_adv, tc)
{
    if (FIND_PERCENT(10*10, 10) != 10) {
	atf_tc_fail("Wrong value for adv 1");
    }

    if (FIND_PERCENT(20*20, 80) != 320) {
	atf_tc_fail("Wrong value for adv 2");
    }

    if (FIND_PERCENT(1000000*1000, 50) != 500000000) {
	atf_tc_fail("Wrong value for adv 3");
    }

    if (FIND_PERCENT(100+100, 10) != 20) {
	atf_tc_fail("Wrong value for adv 4");
    }

    if (FIND_PERCENT(1000+2000, 90) != 2700) {
	atf_tc_fail("Wrong value for adv 5");
    }

    if (FIND_PERCENT(10000 - 8000, 70) != 1400) {
	atf_tc_fail("Wrong value for adv 6");
    }

    if (FIND_PERCENT(2000000000 / 1000, 25) != 500000) {
	atf_tc_fail("Wrong value for adv 7");
    }

    if (FIND_PERCENT(10*10, 10)/2 != 5) {
	atf_tc_fail("Wrong value for adv 8");
    }

    if (FIND_PERCENT(100*10, 50) * 2 != 1000) {
	atf_tc_fail("Wrong value for adv 9");
    }

    if (FIND_PERCENT(100*10, 50) * 2 > 1000) {
	atf_tc_fail("Wrong value for adv 10");
    }

    if (FIND_PERCENT(100+100, 20) * 2 < 60) {
	atf_tc_fail("Wrong value for adv 11");
    }

    return;
}

ATF_TC(print_hex_only);

ATF_TC_HEAD(print_hex_only, tc)
{
    atf_tc_set_md_var(tc, "descr", "Verify hex data formatting.");
}

/* This test exercises the print_hex_only function
 */
ATF_TC_BODY(print_hex_only, tc)
{
    unsigned char data[] =  {0xaa,0xbb,0xcc,0xdd};
    char* ref = "aa:bb:cc:dd";
    char buf[14];
    memset(buf, 'x', sizeof(buf));
    int data_len = sizeof(data);
    int expected_len = 12;

    /* Proper input values should produce proper result */
    print_hex_only (data_len, data, expected_len, buf);
    if (strlen(buf) != strlen(ref)) {
	    atf_tc_fail("len of result is wrong");
    }

    if (strcmp(buf, ref)) {
	    atf_tc_fail("result doesn't match ref");
    }

    /* Make sure we didn't overrun the buffer */
    if (buf[expected_len] != 'x') {
	    atf_tc_fail("data over run detected");
    }

    /* Buffer == null doesn't crash */
    print_hex_only (data_len, data, expected_len, NULL);

    /* Limit == 0 doesn't write (or crash) */
    *buf = '-';
    print_hex_only (data_len, data, 0, buf);
    if (*buf != '-') {
	    atf_tc_fail("limit of zero, altered buffer");
    }

    /* data == NULL doesn't write (or crash) */
    print_hex_only (data_len, NULL, expected_len, buf);
    if (*buf != '-') {
	    atf_tc_fail("limit of zero, altered buffer");
    }

    /* Limit too small should produce zero length string */
    *buf = '-';
    print_hex_only (data_len, data, expected_len - 1, buf);
    if (*buf != 0x0) {
	    atf_tc_fail("limit too small should have failed");
    }

    /* Data length of 0 should produce zero length string */
    *buf = '-';
    print_hex_only (0, data, expected_len, buf);
    if (*buf != 0x0) {
	    atf_tc_fail("limit too small should have failed");
    }
}
    	
/* This macro defines main() method that will call specified
   test cases. tp and simple_test_case names can be whatever you want
   as long as it is a valid variable identifier. */
ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, find_percent_basic);
    ATF_TP_ADD_TC(tp, find_percent_adv);
    ATF_TP_ADD_TC(tp, print_hex_only);

    return (atf_no_error());
}
