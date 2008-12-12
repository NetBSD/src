/*	$NetBSD: timestamp.c,v 1.1.1.2 2008/12/12 11:42:39 haad Exp $	*/

/*
 * Copyright (C) 2006 Rackable Systems All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Abstract out the time methods used so they can be adjusted later -
 * the results of these routines should stay in-core.  This implementation
 * requires librt.
 */

#include "lib.h"
#include <stdlib.h>

#include "timestamp.h"

/*
 * The realtime section uses clock_gettime with the CLOCK_MONOTONIC
 * parameter to prevent issues with time warps
 */
#ifdef HAVE_REALTIME

#include <time.h>
#include <bits/time.h>

struct timestamp {
	struct timespec t;
};

struct timestamp *get_timestamp(void)
{
	struct timestamp *ts = NULL;

	if (!(ts = dm_malloc(sizeof(*ts))))
		return_NULL;

	if (clock_gettime(CLOCK_MONOTONIC, &ts->t)) {
		log_sys_error("clock_gettime", "get_timestamp");
		return NULL;
	}

	return ts;
}

/* cmp_timestamp: Compare two timestamps
 *
 * Return: -1 if t1 is less than t2
 *          0 if t1 is equal to t2
 *          1 if t1 is greater than t2
 */
int cmp_timestamp(struct timestamp *t1, struct timestamp *t2)
{
	if(t1->t.tv_sec < t2->t.tv_sec)
		return -1;
	if(t1->t.tv_sec > t2->t.tv_sec)
		return 1;

	if(t1->t.tv_nsec < t2->t.tv_nsec)
		return -1;
	if(t1->t.tv_nsec > t2->t.tv_nsec)
		return 1;

	return 0;
}

#else /* ! HAVE_REALTIME */

/*
 * The !realtime section just uses gettimeofday and is therefore subject
 * to ntp-type time warps - not sure if should allow that.
 */

#include <sys/time.h>

struct timestamp {
	struct timeval t;
};

struct timestamp *get_timestamp(void)
{
	struct timestamp *ts = NULL;

	if (!(ts = dm_malloc(sizeof(*ts))))
		return_NULL;

	if (gettimeofday(&ts->t, NULL)) {
		log_sys_error("gettimeofday", "get_timestamp");
		return NULL;
	}

	return ts;
}

/* cmp_timestamp: Compare two timestamps
 *
 * Return: -1 if t1 is less than t2
 *          0 if t1 is equal to t2
 *          1 if t1 is greater than t2
 */
int cmp_timestamp(struct timestamp *t1, struct timestamp *t2)
{
	if(t1->t.tv_sec < t2->t.tv_sec)
		return -1;
	if(t1->t.tv_sec > t2->t.tv_sec)
		return 1;

	if(t1->t.tv_usec < t2->t.tv_usec)
		return -1;
	if(t1->t.tv_usec > t2->t.tv_usec)
		return 1;

	return 0;
}

#endif /* HAVE_REALTIME */

void destroy_timestamp(struct timestamp *t)
{
	if (t)
		dm_free(t);
}
