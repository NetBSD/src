/*	$NetBSD: leaseq_unittest.c,v 1.2 2018/04/07 22:37:30 christos Exp $	*/

/*
 * Copyright (C) 2015-2017 by Internet Systems Consortium, Inc. ("ISC")
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

#include "dhcpd.h"

#include <atf-c.h>

/*
 * Test the lease queue code.  These tests will verify that we can
 * add, find and remove leases from the lease queue code
 *
 * Thoughout the tests
 * lq will be the lease queue for which we add or removing leases
 * test_leaseX will be the leases we add or remove
 * We only care about the sort_time in the lease structure for these
 * tests but need to do a lease_reference in order to keep the ref
 * count positive to avoid the omapi code trying to free the object.
 * We can't use lease_allocate easily as we haven't set up the omapi
 * object information in the test.
 */

#if defined (BINARY_LEASES)
#define INIT_LQ(LQ) memset(&(LQ), 0, sizeof(struct leasechain))
#else
#define INIT_LQ(LQ) lq = NULL
#endif

/* Test basic leaseq functions with a single lease */
/*- empty, add, get, and remove */
ATF_TC(leaseq_basic);
ATF_TC_HEAD(leaseq_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify basic functions");
}

ATF_TC_BODY(leaseq_basic, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease1, *check_lease;

	INIT_LQ(lq);

	memset(&test_lease1, 0, sizeof(struct lease));
	test_lease1.sort_time = 10;
	check_lease = NULL;
	lease_reference(&check_lease, &test_lease1, MDL);

	/* Check that the lq is empty */
	if ((LEASE_NOT_EMPTY(lq)) || (LEASE_NOT_EMPTYP(&lq)))
		atf_tc_fail("lq not empty at start.");

	/* And that getting the first lease is okay when queue is empty  */
	if ((LEASE_GET_FIRST(lq) != NULL) || (LEASE_GET_FIRSTP(&lq) != NULL))
		atf_tc_fail("lease not null");

	/* Add a lease */
	LEASE_INSERTP(&lq, &test_lease1);

	/* lq shouldn't be empty anymore */
	if (!(LEASE_NOT_EMPTY(lq)) || !(LEASE_NOT_EMPTYP(&lq)))
		atf_tc_fail("lq empty after insertion.");

	/* We should have the same lease we inserted */
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease1)
		atf_tc_fail("leases don't match");

	/* We should have the same lease we inserted */
	check_lease = LEASE_GET_FIRSTP(&lq);
	if (check_lease != &test_lease1)
		atf_tc_fail("leases don't match");

	/* Check that doing a get on the last lease returns NULL */
	if ((LEASE_GET_NEXT(lq, check_lease) != NULL) ||
	    (LEASE_GET_NEXTP(&lq, check_lease) != NULL)) {
		atf_tc_fail("Next not null");
	}

	/* Remove the lease */
	LEASE_REMOVEP(&lq, &test_lease1);

	/* and verify the lease queue is empty again */
	if ((LEASE_NOT_EMPTY(lq)) || (LEASE_NOT_EMPTYP(&lq)))
		atf_tc_fail("lq not empty afer removal");

	/* And that getting the first lease is okay when queue is empty  */
	if ((LEASE_GET_FIRST(lq) != NULL) || (LEASE_GET_FIRSTP(&lq) != NULL))
		atf_tc_fail("lease not null");
}

/* Test if we can add leases to the end of the list and remove them
 * from the end */
ATF_TC(leaseq_add_to_end);
ATF_TC_HEAD(leaseq_add_to_end, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify adding to end of list");
}

ATF_TC_BODY(leaseq_add_to_end, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease[3], *check_lease;
	int i;

	INIT_LQ(lq);

	/* create and add 3 leases */
	for (i = 0; i < 3; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		test_lease[i].sort_time = i;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);
		LEASE_INSERTP(&lq, &test_lease[i]);
	}

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	for (i = 0; i < 3; i++) {
		if (check_lease != &test_lease[i])
			atf_tc_fail("leases don't match, %d", i);
		check_lease = LEASE_GET_NEXT(lq, check_lease);
	}
	if (check_lease != NULL)
		atf_tc_fail("lease not null");

	/* Remove the last lease and check the list */
	LEASE_REMOVEP(&lq, &test_lease[2]);
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[0])
		atf_tc_fail("wrong lease after remove, 1");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[1])
		atf_tc_fail("wrong lease after remove, 2");

	LEASE_REMOVEP(&lq, &test_lease[1]);
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[0])
		atf_tc_fail("wrong lease after remove, 3");

	LEASE_REMOVEP(&lq, check_lease);

	/* The lease queue should now be empty */
	if (LEASE_NOT_EMPTY(lq))
		atf_tc_fail("lq not empty afer removal");
}

/* Test if we can add leases to the start of the list and remove them
 * from the start */
ATF_TC(leaseq_add_to_start);
ATF_TC_HEAD(leaseq_add_to_start, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify adding to start of list");
}

ATF_TC_BODY(leaseq_add_to_start, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease[3], *check_lease;
	int i;

	INIT_LQ(lq);

	/* create 3 leases */
	for (i = 0; i < 3; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		test_lease[i].sort_time = i;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);
	}

	/* Add leases */
	LEASE_INSERTP(&lq, &test_lease[2]);
	LEASE_INSERTP(&lq, &test_lease[1]);
	LEASE_INSERTP(&lq, &test_lease[0]);

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	for (i = 0; i < 3; i++) {
		if (check_lease != &test_lease[i])
			atf_tc_fail("leases don't match, %d", i);
		check_lease = LEASE_GET_NEXT(lq, check_lease);
	}
	if (check_lease != NULL)
		atf_tc_fail("lease not null");

	/* Remove the first lease and check the next one */
	check_lease = LEASE_GET_FIRST(lq);
	LEASE_REMOVEP(&lq, check_lease);
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[1])
		atf_tc_fail("wrong lease after remove, 1");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[2])
		atf_tc_fail("wrong lease after remove, 2");

	check_lease = LEASE_GET_FIRST(lq);
	LEASE_REMOVEP(&lq, check_lease);
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[2])
		atf_tc_fail("wrong lease after remove, 3");

	LEASE_REMOVEP(&lq, check_lease);

	/* The lease queue should now be empty */
	if (LEASE_NOT_EMPTY(lq))
		atf_tc_fail("lq not empty afer removal");
}

/* Test if we can add leases to the middle of the list and remove them
 * from the middle */
ATF_TC(leaseq_add_to_middle);
ATF_TC_HEAD(leaseq_add_to_middle, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify adding to end of list");
}

ATF_TC_BODY(leaseq_add_to_middle, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease[3], *check_lease;
	int i;

	INIT_LQ(lq);

	/* create 3 leases */
	for (i = 0; i < 3; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		test_lease[i].sort_time = i;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);
	}

	/* Add leases */
	LEASE_INSERTP(&lq, &test_lease[0]);
	LEASE_INSERTP(&lq, &test_lease[2]);
	LEASE_INSERTP(&lq, &test_lease[1]);

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	for (i = 0; i < 3; i++) {
		if (check_lease != &test_lease[i])
			atf_tc_fail("leases don't match, %d", i);
		check_lease = LEASE_GET_NEXT(lq, check_lease);
	}
	if (check_lease != NULL)
		atf_tc_fail("lease not null");

	/* Remove the middle lease and check the list */
	LEASE_REMOVEP(&lq, &test_lease[1]);
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[0])
		atf_tc_fail("wrong lease after remove, 1");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[2])
		atf_tc_fail("wrong lease after remove, 2");

	LEASE_REMOVEP(&lq, &test_lease[0]);
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[2])
		atf_tc_fail("wrong lease after remove, 3");

	LEASE_REMOVEP(&lq, check_lease);

	/* The lease queue should now be empty */
	if (LEASE_NOT_EMPTY(lq))
		atf_tc_fail("lq not empty afer removal");
}

/* Test if we can cycle the leases we add three leases then remove
 * the first one, update it's sort time and re add it */
ATF_TC(leaseq_cycle);
ATF_TC_HEAD(leaseq_cycle, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify cycling the list");
}

ATF_TC_BODY(leaseq_cycle, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease[3], *check_lease;
	int i;

	INIT_LQ(lq);

	/* create and add 3 leases */
	for (i = 0; i < 3; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		test_lease[i].sort_time = i;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);
		LEASE_INSERTP(&lq, &test_lease[i]);
	}

	/* Remove first lease, update it and re-insert it */
	LEASE_REMOVEP(&lq, &test_lease[0]);
	test_lease[0].sort_time = 4;
	LEASE_INSERTP(&lq, &test_lease[0]);

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[1])
		atf_tc_fail("leases don't match, 1");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[2])
		atf_tc_fail("leases don't match, 2");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[0])
		atf_tc_fail("leases don't match, 3");

	/* Remove first lease, update it and re-insert it */
	LEASE_REMOVEP(&lq, &test_lease[1]);
	test_lease[1].sort_time = 5;
	LEASE_INSERTP(&lq, &test_lease[1]);

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[2])
		atf_tc_fail("leases don't match, 4");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[0])
		atf_tc_fail("leases don't match, 5");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[1])
		atf_tc_fail("leases don't match, 6");

	/* Remove first lease, update it and re-insert it */
	LEASE_REMOVEP(&lq, &test_lease[2]);
	test_lease[2].sort_time = 6;
	LEASE_INSERTP(&lq, &test_lease[2]);

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	if (check_lease != &test_lease[0])
		atf_tc_fail("leases don't match, 7");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[1])
		atf_tc_fail("leases don't match, 8");
	check_lease = LEASE_GET_NEXT(lq, check_lease);
	if (check_lease != &test_lease[2])
		atf_tc_fail("leases don't match, 9");
}

/* Test what happens if we set the growth factor to a smallish number
 * and add enough leases to require it to grow multiple times
 * Mostly this is for the binary leases case but we can most of the
 * test for both.
 */

ATF_TC(leaseq_long);
ATF_TC_HEAD(leaseq_long, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify a long list");
}

ATF_TC_BODY(leaseq_long, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease[20], *check_lease;
	int i;

	INIT_LQ(lq);
#if defined (BINARY_LEASES)
	lc_init_growth(&lq, 5);
#endif

	/* create and add 10 leases */
	for (i = 0; i < 10; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		test_lease[i].sort_time = i;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);

		LEASE_INSERTP(&lq, &test_lease[i]);
	}

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	for (i = 0; i < 10; i++) {
		if (check_lease != &test_lease[i])
			atf_tc_fail("leases don't match, %d", i);
		check_lease = LEASE_GET_NEXT(lq, check_lease);
	}

	/* Remove first 5 */
	for (i = 0; i < 5; i++) {
		LEASE_REMOVEP(&lq, &test_lease[i]);
	}

	/* Add 10 more */
	for (i = 10; i < 20; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		test_lease[i].sort_time = i;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);

		LEASE_INSERTP(&lq, &test_lease[i]);
	}

	/* and the first 5 */
	for (i = 0; i < 5; i++) {
		LEASE_INSERTP(&lq, &test_lease[i]);
	}

	/* and check them all out */
	check_lease = LEASE_GET_FIRST(lq);
	for (i = 0; i < 20; i++) {
		if (check_lease != &test_lease[i])
			atf_tc_fail("leases don't match, %d", i);
		check_lease = LEASE_GET_NEXT(lq, check_lease);
	}
}

/* Test if we can add leases that all have the same sort time
 */
ATF_TC(leaseq_same_time);
ATF_TC_HEAD(leaseq_same_time, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verify adding leases with same time");
}

ATF_TC_BODY(leaseq_same_time, tc)
{
	LEASE_STRUCT lq;
	struct lease test_lease[20], *check_lease;
	int i;

	INIT_LQ(lq);

	/* create and add 20 leases */
	for (i = 0; i < 20; i++) {
		memset(&test_lease[i], 0, sizeof(struct lease));
		if (i < 10) 
			test_lease[i].sort_time = 10;
		else
			test_lease[i].sort_time = 20;
		check_lease = NULL;
		lease_reference(&check_lease, &test_lease[i], MDL);
		LEASE_INSERTP(&lq, &test_lease[i]);
	}

#if defined (BINARY_LEASES)
	/* the ordering isn't well defined for either but we check
	 * to see what happens in the binary case.  At the start
	 * the leases should stay in the order they were added.
	 */

	/* check ordering of leases */
	check_lease = LEASE_GET_FIRST(lq);
	for (i = 0; i < 20; i++) {
		if (check_lease != &test_lease[i])
			atf_tc_fail("leases don't match 1, %d", i);
		check_lease = LEASE_GET_NEXT(lq, check_lease);
	}
	if (check_lease != NULL)
		atf_tc_fail("lease not null");
#endif

	/* Remove first 10, update their time, but still before
	 * the previous 10 and re add them */
	for (i = 0; i < 10; i++) {
		LEASE_REMOVEP(&lq, &test_lease[i]);
		test_lease[i].sort_time = 15;
		LEASE_INSERTP(&lq, &test_lease[i]);
	}

	/* The ordering becomes random at this point so we don't
	 * check it.  We try to remove them all and see that
	 * we got to an empty queue.
	 */
	for (i = 0; i < 20; i++) {
		LEASE_REMOVEP(&lq, &test_lease[i]);
	}
	if (LEASE_NOT_EMPTY(lq))
		atf_tc_fail("queue not empty");

}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, leaseq_basic);
	ATF_TP_ADD_TC(tp, leaseq_add_to_end);
	ATF_TP_ADD_TC(tp, leaseq_add_to_start);
	ATF_TP_ADD_TC(tp, leaseq_add_to_middle);
	ATF_TP_ADD_TC(tp, leaseq_cycle);
	ATF_TP_ADD_TC(tp, leaseq_long);
	ATF_TP_ADD_TC(tp, leaseq_same_time);
	return (atf_no_error());
}
