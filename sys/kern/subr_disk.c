/*	$NetBSD: subr_disk.c,v 1.50 2003/04/03 22:20:24 fvdl Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__KERNEL_RCSID(0, "$NetBSD: subr_disk.c,v 1.50 2003/04/03 22:20:24 fvdl Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/sysctl.h>
#include <lib/libkern/libkern.h>

/*
 * A global list of all disks attached to the system.  May grow or
 * shrink over time.
 */
struct	disklist_head disklist;	/* TAILQ_HEAD */
int	disk_count;		/* number of drives in global disklist */
struct simplelock disklist_slock = SIMPLELOCK_INITIALIZER;

/*
 * Compute checksum for disk label.
 */
u_int
dkcksum(struct disklabel *lp)
{
	u_short *start, *end;
	u_short sum = 0;

	start = (u_short *)lp;
	end = (u_short *)&lp->d_partitions[lp->d_npartitions];
	while (start < end)
		sum ^= *start++;
	return (sum);
}

/*
 * Disk error is the preface to plaintive error messages
 * about failing disk transfers.  It prints messages of the form

hp0g: hard error reading fsbn 12345 of 12344-12347 (hp0 bn %d cn %d tn %d sn %d)

 * if the offset of the error in the transfer and a disk label
 * are both available.  blkdone should be -1 if the position of the error
 * is unknown; the disklabel pointer may be null from drivers that have not
 * been converted to use them.  The message is printed with printf
 * if pri is LOG_PRINTF, otherwise it uses log at the specified priority.
 * The message should be completed (with at least a newline) with printf
 * or addlog, respectively.  There is no trailing space.
 */
void
diskerr(const struct buf *bp, const char *dname, const char *what, int pri,
    int blkdone, const struct disklabel *lp)
{
	int unit = DISKUNIT(bp->b_dev), part = DISKPART(bp->b_dev);
	void (*pr)(const char *, ...);
	char partname = 'a' + part;
	int sn;

	if (pri != LOG_PRINTF) {
		static const char fmt[] = "";
		log(pri, fmt);
		pr = addlog;
	} else
		pr = printf;
	(*pr)("%s%d%c: %s %sing fsbn ", dname, unit, partname, what,
	    bp->b_flags & B_READ ? "read" : "writ");
	sn = bp->b_blkno;
	if (bp->b_bcount <= DEV_BSIZE)
		(*pr)("%d", sn);
	else {
		if (blkdone >= 0) {
			sn += blkdone;
			(*pr)("%d of ", sn);
		}
		(*pr)("%d-%d", bp->b_blkno,
		    bp->b_blkno + (bp->b_bcount - 1) / DEV_BSIZE);
	}
	if (lp && (blkdone >= 0 || bp->b_bcount <= lp->d_secsize)) {
		sn += lp->d_partitions[part].p_offset;
		(*pr)(" (%s%d bn %d; cn %d", dname, unit, sn,
		    sn / lp->d_secpercyl);
		sn %= lp->d_secpercyl;
		(*pr)(" tn %d sn %d)", sn / lp->d_nsectors,
		    sn % lp->d_nsectors);
	}
}

/*
 * Initialize the disklist.  Called by main() before autoconfiguration.
 */
void
disk_init(void)
{

	TAILQ_INIT(&disklist);
	disk_count = 0;
}

/*
 * Searches the disklist for the disk corresponding to the
 * name provided.
 */
struct disk *
disk_find(char *name)
{
	struct disk *diskp;

	if ((name == NULL) || (disk_count <= 0))
		return (NULL);

	simple_lock(&disklist_slock);
	for (diskp = TAILQ_FIRST(&disklist); diskp != NULL;
	    diskp = TAILQ_NEXT(diskp, dk_link))
		if (strcmp(diskp->dk_name, name) == 0) {
			simple_unlock(&disklist_slock);
			return (diskp);
		}
	simple_unlock(&disklist_slock);

	return (NULL);
}

/*
 * Attach a disk.
 */
void
disk_attach(struct disk *diskp)
{
	int s;

	/*
	 * Allocate and initialize the disklabel structures.  Note that
	 * it's not safe to sleep here, since we're probably going to be
	 * called during autoconfiguration.
	 */
	diskp->dk_label = malloc(sizeof(struct disklabel), M_DEVBUF, M_NOWAIT);
	diskp->dk_cpulabel = malloc(sizeof(struct cpu_disklabel), M_DEVBUF,
	    M_NOWAIT);
	if ((diskp->dk_label == NULL) || (diskp->dk_cpulabel == NULL))
		panic("disk_attach: can't allocate storage for disklabel");

	memset(diskp->dk_label, 0, sizeof(struct disklabel));
	memset(diskp->dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	/*
	 * Set the attached timestamp.
	 */
	s = splclock();
	diskp->dk_attachtime = mono_time;
	splx(s);

	/*
	 * Link into the disklist.
	 */
	simple_lock(&disklist_slock);
	TAILQ_INSERT_TAIL(&disklist, diskp, dk_link);
	simple_unlock(&disklist_slock);
	++disk_count;
}

/*
 * Detach a disk.
 */
void
disk_detach(struct disk *diskp)
{

	/*
	 * Remove from the disklist.
	 */
	if (--disk_count < 0)
		panic("disk_detach: disk_count < 0");
	simple_lock(&disklist_slock);
	TAILQ_REMOVE(&disklist, diskp, dk_link);
	simple_unlock(&disklist_slock);

	/*
	 * Free the space used by the disklabel structures.
	 */
	free(diskp->dk_label, M_DEVBUF);
	free(diskp->dk_cpulabel, M_DEVBUF);
}

/*
 * Increment a disk's busy counter.  If the counter is going from
 * 0 to 1, set the timestamp.
 */
void
disk_busy(struct disk *diskp)
{
	int s;

	/*
	 * XXX We'd like to use something as accurate as microtime(),
	 * but that doesn't depend on the system TOD clock.
	 */
	if (diskp->dk_busy++ == 0) {
		s = splclock();
		diskp->dk_timestamp = mono_time;
		splx(s);
	}
}

/*
 * Decrement a disk's busy counter, increment the byte count, total busy
 * time, and reset the timestamp.
 */
void
disk_unbusy(struct disk *diskp, long bcount, int read)
{
	int s;
	struct timeval dv_time, diff_time;

	if (diskp->dk_busy-- == 0) {
		printf("%s: dk_busy < 0\n", diskp->dk_name);
		panic("disk_unbusy");
	}

	s = splclock();
	dv_time = mono_time;
	splx(s);

	timersub(&dv_time, &diskp->dk_timestamp, &diff_time);
	timeradd(&diskp->dk_time, &diff_time, &diskp->dk_time);

	diskp->dk_timestamp = dv_time;
	if (bcount > 0) {
		if (read) {
			diskp->dk_rbytes += bcount;
			diskp->dk_rxfer++;
		} else {
			diskp->dk_wbytes += bcount;
			diskp->dk_wxfer++;
		}
	}
}

/*
 * Reset the metrics counters on the given disk.  Note that we cannot
 * reset the busy counter, as it may case a panic in disk_unbusy().
 * We also must avoid playing with the timestamp information, as it
 * may skew any pending transfer results.
 */
void
disk_resetstat(struct disk *diskp)
{
	int s = splbio(), t;

	diskp->dk_rxfer = 0;
	diskp->dk_rbytes = 0;
	diskp->dk_wxfer = 0;
	diskp->dk_wbytes = 0;

	t = splclock();
	diskp->dk_attachtime = mono_time;
	splx(t);

	timerclear(&diskp->dk_time);

	splx(s);
}

int
sysctl_disknames(void *vwhere, size_t *sizep)
{
	char buf[DK_DISKNAMELEN + 1];
	char *where = vwhere;
	struct disk *diskp;
	size_t needed, left, slen;
	int error, first;

	first = 1;
	error = 0;
	needed = 0;
	left = *sizep;

	simple_lock(&disklist_slock);
	for (diskp = TAILQ_FIRST(&disklist); diskp != NULL;
	    diskp = TAILQ_NEXT(diskp, dk_link)) {
		if (where == NULL)
			needed += strlen(diskp->dk_name) + 1;
		else {
			memset(buf, 0, sizeof(buf));
			if (first) {
				strncpy(buf, diskp->dk_name, sizeof(buf));
				first = 0;
			} else {
				buf[0] = ' ';
				strncpy(buf + 1, diskp->dk_name,
				    sizeof(buf) - 1);
			}
			buf[DK_DISKNAMELEN] = '\0';
			slen = strlen(buf);
			if (left < slen + 1)
				break;
			/* +1 to copy out the trailing NUL byte */
			error = copyout(buf, where, slen + 1);
			if (error)
				break;
			where += slen;
			needed += slen;
			left -= slen;
		}
	}
	simple_unlock(&disklist_slock);
	*sizep = needed;
	return (error);
}

int
sysctl_diskstats(int *name, u_int namelen, void *vwhere, size_t *sizep)
{
	struct disk_sysctl sdisk;
	struct disk *diskp;
	char *where = vwhere;
	size_t tocopy, left;
	int error;

	/*
	 * The original hw.diskstats call was broken and did not require
	 * the userland to pass in it's size of struct disk_sysctl.  This
	 * was fixed after NetBSD 1.6 was released, and any applications
	 * that do not pass in the size are given an error only, unless
	 * we care about 1.6 compatibility.
	 */
	if (namelen == 0)
#ifdef COMPAT_16
		tocopy = offsetof(struct disk_sysctl, dk_rxfer);
#else
		return (EINVAL);
#endif
	else
		tocopy = name[0];

	if (where == NULL) {
		*sizep = disk_count * tocopy;
		return (0);
	}

	error = 0;
	left = *sizep;
	memset(&sdisk, 0, sizeof(sdisk));
	*sizep = 0;

	simple_lock(&disklist_slock);
	TAILQ_FOREACH(diskp, &disklist, dk_link) {
		if (left < tocopy)
			break;
		strncpy(sdisk.dk_name, diskp->dk_name, sizeof(sdisk.dk_name));
		sdisk.dk_xfer = diskp->dk_rxfer + diskp->dk_wxfer;
		sdisk.dk_rxfer = diskp->dk_rxfer;
		sdisk.dk_wxfer = diskp->dk_wxfer;
		sdisk.dk_seek = diskp->dk_seek;
		sdisk.dk_bytes = diskp->dk_rbytes + diskp->dk_wbytes;
		sdisk.dk_rbytes = diskp->dk_rbytes;
		sdisk.dk_wbytes = diskp->dk_wbytes;
		sdisk.dk_attachtime_sec = diskp->dk_attachtime.tv_sec;
		sdisk.dk_attachtime_usec = diskp->dk_attachtime.tv_usec;
		sdisk.dk_timestamp_sec = diskp->dk_timestamp.tv_sec;
		sdisk.dk_timestamp_usec = diskp->dk_timestamp.tv_usec;
		sdisk.dk_time_sec = diskp->dk_time.tv_sec;
		sdisk.dk_time_usec = diskp->dk_time.tv_usec;
		sdisk.dk_busy = diskp->dk_busy;

		error = copyout(&sdisk, where, min(tocopy, sizeof(sdisk)));
		if (error)
			break;
		where += tocopy;
		*sizep += tocopy;
		left -= tocopy;
	}
	simple_unlock(&disklist_slock);
	return (error);
}

struct bufq_fcfs {
	TAILQ_HEAD(, buf) bq_head;	/* actual list of buffers */
};

struct bufq_disksort {
	TAILQ_HEAD(, buf) bq_head;	/* actual list of buffers */
};

#define PRIO_READ_BURST		48
#define PRIO_WRITE_REQ		16

struct bufq_prio {
	TAILQ_HEAD(, buf) bq_read, bq_write; /* actual list of buffers */
	struct buf *bq_write_next;	/* next request in bq_write */
	struct buf *bq_next;		/* current request */
	int bq_read_burst;		/* # of consecutive reads */
};


/*
 * Check if two buf's are in ascending order.
 */
static __inline int
buf_inorder(struct buf *bp, struct buf *bq, int sortby)
{
	int r;

	if (bp == NULL || bq == NULL)
		return (bq == NULL);

	if (sortby == BUFQ_SORT_CYLINDER)
		r = bp->b_cylinder - bq->b_cylinder;
	else
		r = 0;

	if (r == 0)
		r = bp->b_rawblkno - bq->b_rawblkno;

	return (r <= 0);
}


/*
 * First-come first-served sort for disks.
 *
 * Requests are appended to the queue without any reordering.
 */
static void
bufq_fcfs_put(struct bufq_state *bufq, struct buf *bp)
{
	struct bufq_fcfs *fcfs = bufq->bq_private;

	TAILQ_INSERT_TAIL(&fcfs->bq_head, bp, b_actq);
}

static struct buf *
bufq_fcfs_get(struct bufq_state *bufq, int remove)
{
	struct bufq_fcfs *fcfs = bufq->bq_private;
	struct buf *bp;

	bp = TAILQ_FIRST(&fcfs->bq_head);

	if (bp != NULL && remove)
		TAILQ_REMOVE(&fcfs->bq_head, bp, b_actq);

	return (bp);
}


/*
 * Seek sort for disks.
 *
 * There are actually two queues, sorted in ascendening order.  The first
 * queue holds those requests which are positioned after the current block;
 * the second holds requests which came in after their position was passed.
 * Thus we implement a one-way scan, retracting after reaching the end of
 * the drive to the first request on the second queue, at which time it
 * becomes the first queue.
 *
 * A one-way scan is natural because of the way UNIX read-ahead blocks are
 * allocated.
 */
static void
bufq_disksort_put(struct bufq_state *bufq, struct buf *bp)
{
	struct bufq_disksort *disksort = bufq->bq_private;
	struct buf *bq, *nbq;
	int sortby;

	sortby = bufq->bq_flags & BUFQ_SORT_MASK;

	bq = TAILQ_FIRST(&disksort->bq_head);

	/*
	 * If the queue is empty it's easy; we just go on the end.
	 */
	if (bq == NULL) {
		TAILQ_INSERT_TAIL(&disksort->bq_head, bp, b_actq);
		return;
	}

	/*
	 * If we lie before the currently active request, then we
	 * must locate the second request list and add ourselves to it.
	 */
	if (buf_inorder(bp, bq, sortby)) {
		while ((nbq = TAILQ_NEXT(bq, b_actq)) != NULL) {
			/*
			 * Check for an ``inversion'' in the normally ascending
			 * block numbers, indicating the start of the second
			 * request list.
			 */
			if (buf_inorder(nbq, bq, sortby)) {
				/*
				 * Search the second request list for the first
				 * request at a larger block number.  We go
				 * after that; if there is no such request, we
				 * go at the end.
				 */
				do {
					if (buf_inorder(bp, nbq, sortby))
						goto insert;
					bq = nbq;
				} while ((nbq =
				    TAILQ_NEXT(bq, b_actq)) != NULL);
				goto insert;		/* after last */
			}
			bq = nbq;
		}
		/*
		 * No inversions... we will go after the last, and
		 * be the first request in the second request list.
		 */
		goto insert;
	}
	/*
	 * Request is at/after the current request...
	 * sort in the first request list.
	 */
	while ((nbq = TAILQ_NEXT(bq, b_actq)) != NULL) {
		/*
		 * We want to go after the current request if there is an
		 * inversion after it (i.e. it is the end of the first
		 * request list), or if the next request is a larger cylinder
		 * than our request.
		 */
		if (buf_inorder(nbq, bq, sortby) ||
		    buf_inorder(bp, nbq, sortby))
			goto insert;
		bq = nbq;
	}
	/*
	 * Neither a second list nor a larger request... we go at the end of
	 * the first list, which is the same as the end of the whole schebang.
	 */
insert:	TAILQ_INSERT_AFTER(&disksort->bq_head, bq, bp, b_actq);
}

static struct buf *
bufq_disksort_get(struct bufq_state *bufq, int remove)
{
	struct bufq_disksort *disksort = bufq->bq_private;
	struct buf *bp;

	bp = TAILQ_FIRST(&disksort->bq_head);

	if (bp != NULL && remove)
		TAILQ_REMOVE(&disksort->bq_head, bp, b_actq);

	return (bp);
}


/*
 * Seek sort for disks.
 *
 * There are two queues.  The first queue holds read requests; the second
 * holds write requests.  The read queue is first-come first-served; the
 * write queue is sorted in ascendening block order.
 * The read queue is processed first.  After PRIO_READ_BURST consecutive
 * read requests with non-empty write queue PRIO_WRITE_REQ requests from
 * the write queue will be processed.
 */
static void
bufq_prio_put(struct bufq_state *bufq, struct buf *bp)
{
	struct bufq_prio *prio = bufq->bq_private;
	struct buf *bq;
	int sortby;

	sortby = bufq->bq_flags & BUFQ_SORT_MASK;

	/*
	 * If it's a read request append it to the list.
	 */
	if ((bp->b_flags & B_READ) == B_READ) {
		TAILQ_INSERT_TAIL(&prio->bq_read, bp, b_actq);
		return;
	}

	bq = TAILQ_FIRST(&prio->bq_write);

	/*
	 * If the write list is empty, simply append it to the list.
	 */
	if (bq == NULL) {
		TAILQ_INSERT_TAIL(&prio->bq_write, bp, b_actq);
		prio->bq_write_next = bp;
		return;
	}

	/*
	 * If we lie after the next request, insert after this request.
	 */
	if (buf_inorder(prio->bq_write_next, bp, sortby))
		bq = prio->bq_write_next;

	/*
	 * Search for the first request at a larger block number.
	 * We go before this request if it exists.
	 */
	while (bq != NULL && buf_inorder(bq, bp, sortby))
		bq = TAILQ_NEXT(bq, b_actq);

	if (bq != NULL)
		TAILQ_INSERT_BEFORE(bq, bp, b_actq);
	else
		TAILQ_INSERT_TAIL(&prio->bq_write, bp, b_actq);
}

static struct buf *
bufq_prio_get(struct bufq_state *bufq, int remove)
{
	struct bufq_prio *prio = bufq->bq_private;
	struct buf *bp;

	/*
	 * If no current request, get next from the lists.
	 */
	if (prio->bq_next == NULL) {
		/*
		 * If at least one list is empty, select the other.
		 */
		if (TAILQ_FIRST(&prio->bq_read) == NULL) {
			prio->bq_next = prio->bq_write_next;
			prio->bq_read_burst = 0;
		} else if (prio->bq_write_next == NULL) {
			prio->bq_next = TAILQ_FIRST(&prio->bq_read);
			prio->bq_read_burst = 0;
		} else {
			/*
			 * Both list have requests.  Select the read list up
			 * to PRIO_READ_BURST times, then select the write
			 * list PRIO_WRITE_REQ times.
			 */
			if (prio->bq_read_burst++ < PRIO_READ_BURST)
				prio->bq_next = TAILQ_FIRST(&prio->bq_read);
			else if (prio->bq_read_burst <
			    PRIO_READ_BURST + PRIO_WRITE_REQ)
				prio->bq_next = prio->bq_write_next;
			else {
				prio->bq_next = TAILQ_FIRST(&prio->bq_read);
				prio->bq_read_burst = 0;
			}
		}
	}

	bp = prio->bq_next;

	if (bp != NULL && remove) {
		if ((bp->b_flags & B_READ) == B_READ)
			TAILQ_REMOVE(&prio->bq_read, bp, b_actq);
		else {
			/*
			 * Advance the write pointer before removing
			 * bp since it is actually prio->bq_write_next.
			 */
			prio->bq_write_next =
			    TAILQ_NEXT(prio->bq_write_next, b_actq);
			TAILQ_REMOVE(&prio->bq_write, bp, b_actq);
			if (prio->bq_write_next == NULL)
				prio->bq_write_next =
				    TAILQ_FIRST(&prio->bq_write);
		}

		prio->bq_next = NULL;
	}

	return (bp);
}

/*
 * Create a device buffer queue.
 */
void
bufq_alloc(struct bufq_state *bufq, int flags)
{
	struct bufq_fcfs *fcfs;
	struct bufq_disksort *disksort;
	struct bufq_prio *prio;

	bufq->bq_flags = flags;

	switch (flags & BUFQ_SORT_MASK) {
	case BUFQ_SORT_RAWBLOCK:
	case BUFQ_SORT_CYLINDER:
		break;
	case 0:
		if ((flags & BUFQ_METHOD_MASK) == BUFQ_FCFS)
			break;
		/* FALLTHROUGH */
	default:
		panic("bufq_alloc: sort out of range");
	}

	switch (flags & BUFQ_METHOD_MASK) {
	case BUFQ_FCFS:
		bufq->bq_get = bufq_fcfs_get;
		bufq->bq_put = bufq_fcfs_put;
		MALLOC(bufq->bq_private, struct bufq_fcfs *,
		    sizeof(struct bufq_fcfs), M_DEVBUF, M_ZERO);
		fcfs = (struct bufq_fcfs *)bufq->bq_private;
		TAILQ_INIT(&fcfs->bq_head);
		break;
	case BUFQ_DISKSORT:
		bufq->bq_get = bufq_disksort_get;
		bufq->bq_put = bufq_disksort_put;
		MALLOC(bufq->bq_private, struct bufq_disksort *,
		    sizeof(struct bufq_disksort), M_DEVBUF, M_ZERO);
		disksort = (struct bufq_disksort *)bufq->bq_private;
		TAILQ_INIT(&disksort->bq_head);
		break;
	case BUFQ_READ_PRIO:
		bufq->bq_get = bufq_prio_get;
		bufq->bq_put = bufq_prio_put;
		MALLOC(bufq->bq_private, struct bufq_prio *,
		    sizeof(struct bufq_prio), M_DEVBUF, M_ZERO);
		prio = (struct bufq_prio *)bufq->bq_private;
		TAILQ_INIT(&prio->bq_read);
		TAILQ_INIT(&prio->bq_write);
		break;
	default:
		panic("bufq_alloc: method out of range");
	}
}

/*
 * Destroy a device buffer queue.
 */
void
bufq_free(struct bufq_state *bufq)
{

	KASSERT(bufq->bq_private != NULL);
	KASSERT(BUFQ_PEEK(bufq) == NULL);

	FREE(bufq->bq_private, M_DEVBUF);
	bufq->bq_get = NULL;
	bufq->bq_put = NULL;
}

/*
 * Bounds checking against the media size, used for the raw partition.
 * The sector size passed in should currently always be DEV_BSIZE,
 * and the media size the size of the device in DEV_BSIZE sectors.
 */
int
bounds_check_with_mediasize(struct buf *bp, int secsize, u_int64_t mediasize)
{
	int sz;

	sz = howmany(bp->b_bcount, secsize);

	if (bp->b_blkno + sz > mediasize) {
		sz = mediasize - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			goto bad;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	return 1;

bad:
	bp->b_flags |= B_ERROR;
done:
	return 0;
}
