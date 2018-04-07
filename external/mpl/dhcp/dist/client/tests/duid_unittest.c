/*	$NetBSD: duid_unittest.c,v 1.1.1.1 2018/04/07 22:34:25 christos Exp $	*/

/*
 * Copyright (c) 2017 by Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: duid_unittest.c,v 1.1.1.1 2018/04/07 22:34:25 christos Exp $");

#include "config.h"
#include <atf-c.h>
#include <omapip/omapip_p.h>
#include "dhcpd.h"


/* Tests to see if the routine to read a secondary lease file
 * for the DUID works properly.  The tests:
 * Test file x:
 * no test file - should result in no duid
 * Test filx 0:
 * A test file but no DUID def, no duid
 * Test file 1:
 * Can it read a single DUID in the file?
 * Test file 2:
 * Can it find a second DUID in the file after a good lease and
 * a badly formatted lease?
 * Test file 3:
 * Can it find a later DUID after a good one and a bad one?
 * and to give a bit more coverage test file 1 should use LLT
 * test file 2 should use LL and test file 3 should use LL for
 * the first one and LLT for the third one.
 */

int duidx_len = 0;

int duid0_len = 0;

int duid1_len = 14;
char duid1_data[] = {0, 1, 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

int duid2_len = 10;
char duid2_data[] = {0, 3, 0, 1, 15, 16, 17, 18, 19, 20};

int duid3_len = 14;
char duid3_data[] = {0, 1, 0, 1, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};

ATF_TC(read_duid_test);

ATF_TC_HEAD(read_duid_test, tc) {
    atf_tc_set_md_var(tc, "descr", "read secondary file looking for duid");
}

ATF_TC_BODY(read_duid_test, tc) {

    static const char *srcdir;
    char duid_fname[1024];

    /* Get the srcidr so we can find our test files */
    if (atf_tc_has_config_var(tc, "srcdir"))
	srcdir = atf_tc_get_config_var(tc, "srcdir");
    /* point the duid file at our filename space
       We will update it per test below */
    path_dhclient_duid = duid_fname;

    /* Initialize client globals. */
    memset(&default_duid, 0, sizeof(default_duid));

    /* Try to read a nonexistent test file
     */
    sprintf(duid_fname, "%s/duidx_test.txt", srcdir);
    read_client_duid();
    if (default_duid.len != duidx_len) {
	atf_tc_fail("failed to properly read duid1");
    }

    /* Try to read test file 0
     * This doesn't have a DUID.
     */
    sprintf(duid_fname, "%s/duid0_test.txt", srcdir);
    read_client_duid();
    if (default_duid.len != duid0_len) {
	atf_tc_fail("failed to properly read duid0");
    }

    /* Try to read test file 1
     * This has a single good LLT DUID in it
     */
    sprintf(duid_fname, "%s/duid1_test.txt", srcdir);
    read_client_duid();
    if ((default_duid.len != duid1_len) ||
	(memcmp(default_duid.data, duid1_data, duid1_len) != 0)) {
	atf_tc_fail("failed to properly read duid1");
    }

    /* Try to read test file 2
     * This has two good LL DUIDs in it with several good and bad leases between them.
     */
    sprintf(duid_fname, "%s/duid2_test.txt", srcdir);
    read_client_duid();
    if ((default_duid.len != duid2_len) ||
	(memcmp(default_duid.data, duid2_data, duid2_len) != 0)) {
	atf_tc_fail("failed to properly read duid2");
    }

    /* Try to read test file 3
     * This has a good LL DUID, a bad LLT DUID and a good LLT DUID
     */
    sprintf(duid_fname, "%s/duid3_test.txt", srcdir);
    read_client_duid();
    if ((default_duid.len != duid3_len) ||
	(memcmp(default_duid.data, duid3_data, duid3_len) != 0)) {
	atf_tc_fail("failed to properly read duid3");
    }

}


ATF_TP_ADD_TCS(tp) {
    ATF_TP_ADD_TC(tp, read_duid_test);

    return (atf_no_error());
}
