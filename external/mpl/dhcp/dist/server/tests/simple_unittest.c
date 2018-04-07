/*	$NetBSD: simple_unittest.c,v 1.1.1.1 2018/04/07 22:34:28 christos Exp $	*/

/*
 * Copyright (C) 2012-2017 by Internet Systems Consortium, Inc. ("ISC")
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
#include <dhcpd.h>
#include <atf-c.h>

/* That is an example ATF test case, tailored to ISC DHCP sources.
   For detailed description with examples, see man 3 atf-c-api. */

/* this macro defines a name of a test case. Typical test case constists
   of an initial test declaration (ATF_TC()) followed by 3 phases:

   - Initialization: ATF_TC_HEAD()
   - Main body: ATF_TC_BODY()
   - Cleanup: ATF_TC_CLEANUP()

 In many cases initialization or cleanup are not needed. Use
 ATF_TC_WITHOUT_HEAD() or ATF_TC_WITH_CLEANUP() as needed. */
ATF_TC(simple_test_case);


ATF_TC_HEAD(simple_test_case, tc)
{
    atf_tc_set_md_var(tc, "descr", "This test case is a simple DHCP test.");
}
ATF_TC_BODY(simple_test_case, tc)
{
    int condition = 1;
    int this_is_linux = 1;
    /* Failing condition will fail the test, but the code
       itself will continue */
    ATF_CHECK( 2 > 1 );

    /* assert style check. Test will abort if the condition is not met. */
    ATF_REQUIRE( 5 > 4 );

    ATF_CHECK_EQ(4, 2 + 2); /* Non-fatal test. */
    ATF_REQUIRE_EQ(4, 2 + 2); /* Fatal test. */

    /* tests can also explicitly report test result */
    if (!condition) {
        atf_tc_fail("Condition not met!"); /* Explicit failure. */
    }

    if (!this_is_linux) {
        atf_tc_skip("Skipping test. This Linux-only test.");
    }

    if (condition && this_is_linux) {
        /* no extra comments for pass needed. It just passed. */
        atf_tc_pass();
    }

}

#ifdef DHCPv6
ATF_TC(parse_byte_order);

ATF_TC_HEAD(parse_byte_order, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests byte-order conversion.");
}

ATF_TC_BODY(parse_byte_order, tc)
{
    uint32_t ret_value = 0;
    uint32_t source_value = 0xaabbccdd;

    /* With order set to 0, function should default to no conversion */
    authoring_byte_order = 0;
    ret_value = parse_byte_order_uint32(&source_value);
    if (ret_value != source_value) {
        atf_tc_fail("default/non-conversion failed!");
    }

    /* With matching byte order, function should not do the conversion */
    authoring_byte_order = DHCP_BYTE_ORDER;
    ret_value = parse_byte_order_uint32(&source_value);
    if (ret_value != source_value) {
        atf_tc_fail("matching/non-conversion failed!");
    }

    /* With opposite byte order, function should do the conversion */
    authoring_byte_order = (DHCP_BYTE_ORDER == LITTLE_ENDIAN ?
                            BIG_ENDIAN : LITTLE_ENDIAN);
    ret_value = parse_byte_order_uint32(&source_value);
    if (ret_value != 0xddccbbaa) {
        atf_tc_fail("conversion failed!");
    }

    /* Converting the converted value should give us the original value */
    ret_value = parse_byte_order_uint32(&ret_value);
    if (ret_value != source_value) {
        atf_tc_fail("round trip conversion failed!");
    }

    atf_tc_pass();
}
#endif /*  DHCPv6 */

/* This macro defines main() method that will call specified
   test cases. tp and simple_test_case names can be whatever you want
   as long as it is a valid variable identifier. */
ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, simple_test_case);
#ifdef DHCPv6
    ATF_TP_ADD_TC(tp, parse_byte_order);
#endif
    return (atf_no_error());
}
