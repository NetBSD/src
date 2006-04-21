/*	$NetBSD: subr_iostat.c,v 1.3 2006/04/21 13:48:57 yamt Exp $	*/
/*	NetBSD: subr_disk.c,v 1.69 2005/05/29 22:24:15 christos Exp	*/

/*-
 * Copyright (c) 1996, 1997, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_iostat.c,v 1.3 2006/04/21 13:48:57 yamt Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/syslog.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/sysctl.h>
#include <lib/libkern/libkern.h>

/*
 * Function prototypes for sysctl nodes
 */
static int	sysctl_hw_disknames(SYSCTLFN_PROTO);
static int	sysctl_hw_iostatnames(SYSCTLFN_PROTO);
static int	sysctl_hw_iostats(SYSCTLFN_PROTO);

static int
iostati_getnames(int disk_only, char *oldp, size_t *oldlenp, const void *newp,
		u_int namelen);

/*
 * A global list of all drives attached to the system.  May grow or
 * shrink over time.
 */
struct iostatlist_head iostatlist = TAILQ_HEAD_INITIALIZER(iostatlist);
int iostat_count;		/* number of drives in global drivelist */
struct simplelock iostatlist_slock = SIMPLELOCK_INITIALIZER;

/*
 * Searches the iostatlist for the iostat corresponding to the
 * name provided.
 */
struct io_stats *
iostat_find(char *name)
{
	struct io_stats *iostatp;

	if ((name == NULL) || (iostat_count <= 0))
		return (NULL);

	simple_lock(&iostatlist_slock);
	for (iostatp = TAILQ_FIRST(&iostatlist); iostatp != NULL;
	    iostatp = TAILQ_NEXT(iostatp, io_link))
		if (strcmp(iostatp->io_name, name) == 0) {
			simple_unlock(&iostatlist_slock);
			return (iostatp);
		}
	simple_unlock(&iostatlist_slock);

	return (NULL);
}

/*
 * Allocate and initialise memory for the i/o statistics.
 */
struct io_stats *
iostat_alloc(int32_t type)
{
	int s;
	struct io_stats *stats;

	stats = malloc(sizeof(struct io_stats), M_DEVBUF, M_NOWAIT);

	if (stats == NULL)
		panic("iostat_alloc: cannot allocate memory for stats "
		      "buffer");

	stats->io_type = type;
	stats->io_rxfer = 0;
	stats->io_rbytes = 0;
	stats->io_wxfer = 0;
	stats->io_wbytes = 0;

	/*
	 * Set the attached timestamp.
	 */
	s = splclock();
	stats->io_attachtime = mono_time;
	splx(s);

	timerclear(&stats->io_time);
	timerclear(&stats->io_timestamp);

	/*
	 * Link into the drivelist.
	 */
	simple_lock(&iostatlist_slock);
	TAILQ_INSERT_TAIL(&iostatlist, stats, io_link);
	iostat_count++;
	simple_unlock(&iostatlist_slock);

	return stats;
}

/*
 * Remove i/o from stats collection.
 */
void
iostat_free(struct io_stats *stats)
{

	/*
	 * Remove from the iostat list.
	 */
	if (iostat_count == 0)
		panic("iostat_free: iostat_count == 0");
	simple_lock(&iostatlist_slock);
	TAILQ_REMOVE(&iostatlist, stats, io_link);
	iostat_count--;
	simple_unlock(&iostatlist_slock);
	free(stats, M_DEVBUF);
}

/*
 * Increment a iostat busy counter.  If the counter is going from
 * 0 to 1, set the timestamp.
 */
void
iostat_busy(struct io_stats *stats)
{
	int s;

	/*
	 * XXX We'd like to use something as accurate as microtime(),
	 * but that doesn't depend on the system TOD clock.
	 */
	if (stats->io_busy++ == 0) {
		s = splclock();
		stats->io_timestamp = mono_time;
		splx(s);
	}
}

/*
 * Decrement a iostat busy counter, increment the byte count, total busy
 * time, and reset the timestamp.
 */
void
iostat_unbusy(struct io_stats *stats, long bcount, int read)
{
	int s;
	struct timeval dv_time, diff_time;

	if (stats->io_busy-- == 0) {
		printf("%s: busy < 0\n", stats->io_name);
		panic("iostat_unbusy");
	}

	s = splclock();
	dv_time = mono_time;
	splx(s);

	timersub(&dv_time, &stats->io_timestamp, &diff_time);
	timeradd(&stats->io_time, &diff_time, &stats->io_time);

	stats->io_timestamp = dv_time;
	if (bcount > 0) {
		if (read) {
			stats->io_rbytes += bcount;
			stats->io_rxfer++;
		} else {
			stats->io_wbytes += bcount;
			stats->io_wxfer++;
		}
	}
}

/*
 * Increment the seek counter.  This does look almost redundant but it
 * abstracts the stats gathering.
 */
void
iostat_seek(struct io_stats *stats)
{

	stats->io_seek++;
}

static int
sysctl_hw_disknames(SYSCTLFN_ARGS)
{

	return iostati_getnames(1, oldp, oldlenp, newp, namelen);
}

static int
sysctl_hw_iostatnames(SYSCTLFN_ARGS)
{

	return iostati_getnames(0, oldp, oldlenp, newp, namelen);
}

static int
iostati_getnames(int disk_only, char *oldp, size_t *oldlenp, const void *newp,
		 u_int namelen)
{
	char bf[IOSTATNAMELEN + 1];
	char *where = oldp;
	struct io_stats *stats;
	size_t needed, left, slen;
	int error, first;

	if (newp != NULL)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);

	first = 1;
	error = 0;
	needed = 0;
	left = *oldlenp;

	simple_lock(&iostatlist_slock);
	for (stats = TAILQ_FIRST(&iostatlist); stats != NULL;
	    stats = TAILQ_NEXT(stats, io_link)) {
		if ((disk_only == 1) && (stats->io_type != IOSTAT_DISK))
			continue;

		if (where == NULL)
			needed += strlen(stats->io_name) + 1;
		else {
			memset(bf, 0, sizeof(bf));
			if (first) {
				strncpy(bf, stats->io_name, sizeof(bf));
				first = 0;
			} else {
				bf[0] = ' ';
				strncpy(bf + 1, stats->io_name,
				    sizeof(bf) - 1);
			}
			bf[IOSTATNAMELEN] = '\0';
			slen = strlen(bf);
			if (left < slen + 1)
				break;
			/* +1 to copy out the trailing NUL byte */
			error = copyout(bf, where, slen + 1);
			if (error)
				break;
			where += slen;
			needed += slen;
			left -= slen;
		}
	}
	simple_unlock(&iostatlist_slock);
	*oldlenp = needed;
	return (error);
}

static int
sysctl_hw_iostats(SYSCTLFN_ARGS)
{
	struct io_sysctl sdrive;
	struct io_stats *stats;
	char *where = oldp;
	size_t tocopy, left;
	int error;

	if (newp != NULL)
		return (EPERM);

	/*
	 * The original hw.diskstats call was broken and did not require
	 * the userland to pass in it's size of struct disk_sysctl.  This
	 * was fixed after NetBSD 1.6 was released, and any applications
	 * that do not pass in the size are given an error only, unless
	 * we care about 1.6 compatibility.
	 */
	if (namelen == 0)
#ifdef COMPAT_16
		tocopy = offsetof(struct io_sysctl, busy);
#else
		return (EINVAL);
#endif
	else
		tocopy = name[0];

	if (where == NULL) {
		*oldlenp = iostat_count * tocopy;
		return (0);
	}

	error = 0;
	left = *oldlenp;
	memset(&sdrive, 0, sizeof(sdrive));
	*oldlenp = 0;

	simple_lock(&iostatlist_slock);
	TAILQ_FOREACH(stats, &iostatlist, io_link) {
		if (left < tocopy)
			break;
		strncpy(sdrive.name, stats->io_name, sizeof(sdrive.name));
		sdrive.xfer = stats->io_rxfer + stats->io_wxfer;
		sdrive.rxfer = stats->io_rxfer;
		sdrive.wxfer = stats->io_wxfer;
		sdrive.seek = stats->io_seek;
		sdrive.bytes = stats->io_rbytes + stats->io_wbytes;
		sdrive.rbytes = stats->io_rbytes;
		sdrive.wbytes = stats->io_wbytes;
		sdrive.attachtime_sec = stats->io_attachtime.tv_sec;
		sdrive.attachtime_usec = stats->io_attachtime.tv_usec;
		sdrive.timestamp_sec = stats->io_timestamp.tv_sec;
		sdrive.timestamp_usec = stats->io_timestamp.tv_usec;
		sdrive.time_sec = stats->io_time.tv_sec;
		sdrive.time_usec = stats->io_time.tv_usec;
		sdrive.busy = stats->io_busy;

		error = copyout(&sdrive, where, min(tocopy, sizeof(sdrive)));
		if (error)
			break;
		where += tocopy;
		*oldlenp += tocopy;
		left -= tocopy;
	}
	simple_unlock(&iostatlist_slock);
	return (error);
}

SYSCTL_SETUP(sysctl_io_stats_setup, "sysctl i/o stats setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "disknames",
		       SYSCTL_DESCR("List of disk drives present"),
		       sysctl_hw_disknames, 0, NULL, 0,
		       CTL_HW, HW_DISKNAMES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "iostatnames",
		       SYSCTL_DESCR("I/O stats are being collected for these"
				    " devices"),
		       sysctl_hw_iostatnames, 0, NULL, 0,
		       CTL_HW, HW_IOSTATNAMES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "drivestats",
		       SYSCTL_DESCR("Statistics on device I/O operations"),
		       sysctl_hw_iostats, 0, NULL, 0,
		       CTL_HW, HW_IOSTATS, CTL_EOL);
}
