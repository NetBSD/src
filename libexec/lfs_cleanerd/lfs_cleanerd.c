/* $NetBSD: lfs_cleanerd.c,v 1.61 2026/04/23 16:26:05 perseant Exp $	 */

/*-
 * Copyright (c) 2005, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * The cleaner daemon for the NetBSD Log-structured File System.
 * Only tested for use with version 2 LFSs.
 */

#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <ufs/lfs/lfs.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <util.h>

#include "cleaner.h"
#include "kernelops.h"
#include "mount_lfs.h"

/*
 * Global variables.
 */
/* XXX these top few should really be fs-specific */
int use_fs_idle;	/* Use fs idle rather than cpu idle time */
int use_bytes;		/* Use bytes written rather than segments cleaned */
double load_threshold;	/* How idle is idle (CPU idle) */
int atatime;		/* How many segments (bytes) to clean at a time */

int nfss;		/* Number of filesystems monitored by this cleanerd */
struct clfs **fsp;	/* Array of extended filesystem structures */
int segwait_timeout;	/* Time to wait in lfs_segwait() */
int do_quit;		/* Quit after one cleaning loop */
int do_coalesce;	/* Coalesce filesystem */
int do_small;		/* Use small writes through markv */
char *do_asdevice;      /* Use this as the raw device */
char *copylog_filename; /* File to use for fs debugging analysis */
int inval_segment;	/* Segment to invalidate */
int stat_report;	/* Report statistics for this period of cycles */
int debug;		/* Turn on debugging */
struct cleaner_stats {
	double	util_tot;
	double	util_sos;
	off_t	bytes_read;
	off_t	bytes_written;
	off_t	segs_cleaned;
	off_t	segs_empty;
	off_t	segs_error;
} cleaner_stats;

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct dlfs *);
extern u_int32_t lfs_cksum_part(void *, size_t, u_int32_t);

/* Ugh */
#define FSMNT_SIZE MAX(sizeof(((struct dlfs *)0)->dlfs_fsmnt), \
			sizeof(((struct dlfs64 *)0)->dlfs_fsmnt))


/* Compat */
void pwarn(const char *unused, ...) { /* Does nothing */ };

/*
 * Log a message if debugging is turned on.
 */
void
dlog(const char *fmt, ...)
{
	va_list ap;

	if (debug == 0)
		return;

	va_start(ap, fmt);
	vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

/*
 * Remove the specified filesystem from the list, due to its having
 * become unmounted or other error condition.
 */
void
handle_error(struct clfs **cfsp, int n)
{
	syslog(LOG_NOTICE, "%s: detaching cleaner", lfs_sb_getfsmnt(cfsp[n]));
	free(cfsp[n]);
	if (n != nfss - 1)
		cfsp[n] = cfsp[nfss - 1];
	--nfss;
}

/*
 * Reinitialize a filesystem if, e.g., its size changed.
 */
int
reinit_fs(struct clfs *fs)
{
	char fsname[FSMNT_SIZE];

	memcpy(fsname, lfs_sb_getfsmnt(fs), sizeof(fsname));
	fsname[sizeof(fsname) - 1] = '\0';

	kops.ko_close(fs->clfs_ifilefd);
	kops.ko_close(fs->clfs_devfd);
	free(fs->clfs_dev);
	free(fs->clfs_segtab);
	free(fs->clfs_segtabp);

	return init_fs(fs, fsname);
}


/*
 * Set up the file descriptors, including the Ifile descriptor.
 * If we can't get the Ifile, this is not an LFS (or the kernel is
 * too old to support the fcntl).
 * XXX Merge this and init_unmounted_fs, switching on whether
 * XXX "fsname" is a dir or a char special device.  Should
 * XXX also be able to read unmounted devices out of fstab, the way
 * XXX fsck does.
 */
int
init_fs(struct clfs *fs, char *fsname)
{
	char mnttmp[FSMNT_SIZE];
	struct statvfs sf;
	int rootfd;
	int i;
	void *sbuf;
	size_t mlen;

	if (do_asdevice != NULL) {
		fs->clfs_dev = strndup(do_asdevice,strlen(do_asdevice) + 2);
		if (fs->clfs_dev == NULL) {
			syslog(LOG_ERR, "couldn't malloc device name string: %m");
			return -1;
		}
	} else {
		/*
		 * Get the raw device from the block device.
		 * XXX this is ugly.  Is there a way to discover the raw device
		 * XXX for a given mount point?
		 */
		if (kops.ko_statvfs(fsname, &sf, ST_WAIT) < 0)
			return -1;
		mlen = strlen(sf.f_mntfromname) + 2;
		fs->clfs_dev = malloc(mlen);
		if (fs->clfs_dev == NULL) {
			syslog(LOG_ERR, "couldn't malloc device name string: %m");
			return -1;
		}
		if (getdiskrawname(fs->clfs_dev, mlen, sf.f_mntfromname) == NULL) {
			syslog(LOG_ERR, "couldn't convert '%s' to raw name: %m",
			    sf.f_mntfromname);
			return -1;
		}
	}
	if ((fs->clfs_devfd = kops.ko_open(fs->clfs_dev, O_RDONLY, 0)) < 0) {
		syslog(LOG_ERR, "couldn't open device %s for reading: %m",
			fs->clfs_dev);
		return -1;
	}

	/* Find the Ifile and open it */
	if ((rootfd = kops.ko_open(fsname, O_RDONLY, 0)) < 0)
		return -2;
	if (kops.ko_fcntl(rootfd, LFCNIFILEFH, &fs->clfs_ifilefh) < 0)
		return -3;
	if ((fs->clfs_ifilefd = kops.ko_fhopen(&fs->clfs_ifilefh,
	    sizeof(fs->clfs_ifilefh), O_RDONLY)) < 0)
		return -4;
	kops.ko_close(rootfd);

	sbuf = malloc(LFS_SBPAD);
	if (sbuf == NULL) {
		syslog(LOG_ERR, "couldn't malloc superblock buffer");
		return -1;
	}

	/* Load in the superblock */
	if (kops.ko_pread(fs->clfs_devfd, sbuf, LFS_SBPAD, LFS_LABELPAD) < 0) {
		free(sbuf);
		return -1;
	}

	__CTASSERT(sizeof(struct dlfs) == sizeof(struct dlfs64));
	memcpy(&fs->lfs_dlfs_u, sbuf, sizeof(struct dlfs));
	free(sbuf);

	/* If it is not LFS, complain and exit! */
	switch (fs->lfs_dlfs_u.u_32.dlfs_magic) {
	    case LFS_MAGIC:
		fs->lfs_is64 = false;
		fs->lfs_dobyteswap = false;
		break;
	    case LFS_MAGIC_SWAPPED:
		fs->lfs_is64 = false;
		fs->lfs_dobyteswap = true;
		break;
	    case LFS64_MAGIC:
		fs->lfs_is64 = true;
		fs->lfs_dobyteswap = false;
		break;
	    case LFS64_MAGIC_SWAPPED:
		fs->lfs_is64 = true;
		fs->lfs_dobyteswap = true;
		break;
	    default:
		syslog(LOG_ERR, "%s: not LFS", fsname);
		return -1;
	}
	/* XXX: can this ever need to be set? does the cleaner even care? */
	fs->lfs_hasolddirfmt = 0;

	/* If this is not a version 2 filesystem, complain and exit */
	if (lfs_sb_getversion(fs) != 2) {
		syslog(LOG_ERR, "%s: not a version 2 LFS", fsname);
		return -1;
	}

	/* Assume fsname is the mounted name */
	strncpy(mnttmp, fsname, sizeof(mnttmp));
	mnttmp[sizeof(mnttmp) - 1] = '\0';
	lfs_sb_setfsmnt(fs, mnttmp);

	/* Allocate and clear segtab */
	fs->clfs_segtab = (struct clfs_seguse *)malloc(lfs_sb_getnseg(fs) *
						sizeof(*fs->clfs_segtab));
	fs->clfs_segtabp = (struct clfs_seguse **)malloc(lfs_sb_getnseg(fs) *
						sizeof(*fs->clfs_segtabp));
	if (fs->clfs_segtab == NULL || fs->clfs_segtabp == NULL) {
		syslog(LOG_ERR, "%s: couldn't malloc segment table: %m",
			fs->clfs_dev);
		return -1;
	}

	for (i = 0; i < lfs_sb_getnseg(fs); i++) {
		fs->clfs_segtabp[i] = &(fs->clfs_segtab[i]);
		fs->clfs_segtab[i].flags = 0x0;
	}

	syslog(LOG_NOTICE, "%s: attaching cleaner", fsname);
	return 0;
}

void
calc_cb(struct clfs *fs, int sn, struct clfs_seguse *t)
{
	time_t now;
	int64_t age, benefit, cost;

	time(&now);
	age = (now < t->lastmod ? 0 : now - t->lastmod);

	/* Under no circumstances clean active or already-clean segments */
	if ((t->flags & SEGUSE_ACTIVE) || !(t->flags & SEGUSE_DIRTY)) {
		t->priority = 0;
		return;
	}

	/*
	 * If the segment is empty, there is no reason to clean it.
	 * Clear its error condition, if any, since we are never going to
	 * try to parse this one.
	 */
	if (t->nbytes == 0) {
		t->flags &= ~SEGUSE_ERROR; /* Strip error once empty */
		t->priority = 0;
		return;
	}

	if (t->flags & SEGUSE_ERROR) {	/* No good if not already empty */
		/* No benefit */
		t->priority = 0;
		return;
	}

	if (t->nbytes > lfs_sb_getssize(fs)) {
		/* Another type of error */
		syslog(LOG_WARNING, "segment %d: bad seguse count %d",
		       sn, t->nbytes);
		t->flags |= SEGUSE_ERROR;
		t->priority = 0;
		return;
	}

	/*
	 * The non-degenerate case.  Use Rosenblum's cost-benefit algorithm.
	 * Calculate the benefit from cleaning this segment (one segment,
	 * minus fragmentation, dirty blocks and a segment summary block)
	 * and weigh that against the cost (bytes read plus bytes written).
	 * We count the summary headers as "dirty" to avoid cleaning very
	 * old and very full segments.
	 */
	benefit = (int64_t)lfs_sb_getssize(fs) - t->nbytes -
		  (t->nsums + 1) * lfs_sb_getfsize(fs);
	if (lfs_sb_getbsize(fs) > lfs_sb_getfsize(fs)) /* fragmentation */
		benefit -= (lfs_sb_getbsize(fs) / 2);
	if (benefit <= 0) {
		t->priority = 0;
		return;
	}

	cost = lfs_sb_getssize(fs) + t->nbytes;
	t->priority = (256 * benefit * age) / cost;

	return;
}

/*
 * Comparator for sort_segments: cost-benefit equation.
 */
static int
cb_comparator(const void *va, const void *vb)
{
	const struct clfs_seguse *a, *b;

	a = *(const struct clfs_seguse * const *)va;
	b = *(const struct clfs_seguse * const *)vb;
	return a->priority > b->priority ? -1 : 1;
}

static void
add_stats(int nsegs, struct lfs_write_stats *lws)
{
	/* XXX */
}

/*
 * Clean a segment and mark it invalid.
 */
int
invalidate_segment(struct clfs *fs, int sn)
{
	int r;
	struct lfs_segnum_array lsa;

	dlog("%s: inval seg %d", lfs_sb_getfsmnt(fs), sn);

	/*
	 * Use LFCNREWRITESEG to move the blocks.
	 */
	lsa.len = 1;
	lsa.segments = &sn;
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNREWRITESEGS, &lsa)) < 0) {
		syslog(LOG_WARNING, "%s: rewriteseg returned %d (%m) "
		       "for seg %d", lfs_sb_getfsmnt(fs), r, sn);
		return r;
	}

	/* Record stats */
	add_stats(1, &lsa.stats);

	/*
	 * Finally call invalidate to invalidate the segment.
	 */
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNINVAL, &sn)) < 0) {
		syslog(LOG_WARNING, "%s: inval returned %d (%m) "
		       "for seg %d", lfs_sb_getfsmnt(fs), r, sn);
		return r;
	}

	return 0;
}

#define NRECENT 10
static int recent[NRECENT];
static int recent_last = 0;

static SEGUSE seguse_array[LFS_SEGUSE_MAXCNT];

/*
 * Select segments to clean, and call the fcntl to clean them.
 */
static int
clean_fs(struct clfs *fs, const CLEANERINFO64 *cip)
{
	int i, j, ngood, sn, bic, r, npos;
	SEGUSE *sup;
	static BLOCK_INFO *bip;
	off_t totbytes;
	off_t nb;
	off_t goal;
	double util;
	struct lfs_segnum_array sna;
	int segtab[LFS_REWRITE_MAXCNT];
	struct lfs_seguse_array sua;

	/* Read the segment table into our private structure */
	sua.len = LFS_SEGUSE_MAXCNT;
	sua.seguse = seguse_array;
	npos = 0;

	for (i = 0; i < lfs_sb_getnseg(fs); i+= LFS_SEGUSE_MAXCNT) {
		sua.start = i;
		if (fcntl(fs->clfs_ifilefd, LFCNSEGUSE, &sua) < 0) {
			dlog("%s: fcntl returned %d", lfs_sb_getfsmnt(fs),
			     errno);
			break;
		}

		for (j = 0; j < LFS_SEGUSE_MAXCNT && i + j < lfs_sb_getnseg(fs); j++) {
			sup = &sua.seguse[j];
			fs->clfs_segtab[i + j].nbytes  = sup->su_nbytes;
			fs->clfs_segtab[i + j].nsums = sup->su_nsums;
			fs->clfs_segtab[i + j].lastmod = sup->su_lastmod;
			/* Keep error status but renew other flags */
			fs->clfs_segtab[i + j].flags  &= SEGUSE_ERROR;
			fs->clfs_segtab[i + j].flags  |= sup->su_flags;

			/* Compute cost-benefit coefficient */
			calc_cb(fs, i + j, fs->clfs_segtab + i + j);
			if (fs->clfs_segtab[i + j].priority > 0)
				++npos;
		}
	}

	/* Lower the priority of recently treated segments */
	for (i = 0; i < NRECENT; i++)
		fs->clfs_segtab[recent[i]].priority /= 2;

	/* Sort segments based on cleanliness, fulness, and condition */
	heapsort(fs->clfs_segtabp, lfs_sb_getnseg(fs), sizeof(struct clfs_seguse *),
		 cb_comparator);

	/* If no segment is cleanable, just return */
	if (fs->clfs_segtabp[0]->priority == 0) {
		dlog("%s: no segment cleanable", lfs_sb_getfsmnt(fs));
		return 0;
	}

	/* Load some segments' blocks into bip */
	bic = 0;
	fs->clfs_nactive = 0;
	ngood = 0;
	sna.len = 0;
	sna.segments = segtab;
	if (use_bytes) {
		/* Set attainable goal */
		goal = lfs_sb_getssize(fs) * atatime;
		if (goal > (cip->clean - 1) * lfs_sb_getssize(fs) / 2)
			goal = MAX((cip->clean - 1) * lfs_sb_getssize(fs),
				   lfs_sb_getssize(fs)) / 2;

		dlog("%s: cleaning with goal %" PRId64
		     " bytes (%d segs clean, %d cleanable)",
		     lfs_sb_getfsmnt(fs), goal, cip->clean, npos);
		syslog(LOG_INFO, "%s: cleaning with goal %" PRId64
		       " bytes (%d segs clean, %d cleanable)",
		       lfs_sb_getfsmnt(fs), goal, cip->clean, npos);
		totbytes = 0;
		for (i = 0; i < lfs_sb_getnseg(fs) && totbytes < goal; i++) {
			if (fs->clfs_segtabp[i]->priority == 0)
				break;
			/* Upper bound on number of segments at once */
			if (ngood * lfs_sb_getssize(fs) > 4 * goal)
				break;
			sn = (fs->clfs_segtabp[i] - fs->clfs_segtab);
			dlog("%s: add seg %d prio %" PRIu64
			     " containing %ld bytes",
			     lfs_sb_getfsmnt(fs), sn, fs->clfs_segtabp[i]->priority,
			     fs->clfs_segtabp[i]->nbytes);
			sna.segments[sna.len++] = sn;
			recent[recent_last] = sn;
			recent_last = (recent_last + 1) % NRECENT;
			totbytes += fs->clfs_segtabp[i]->nbytes;
			++ngood;
		}
	} else {
		/* Set attainable goal */
		goal = atatime;
		if (goal > cip->clean - 1)
			goal = MAX(cip->clean - 1, 1);

		dlog("%s: cleaning with goal %d segments (%d clean, %d cleanable)",
		       lfs_sb_getfsmnt(fs), (int)goal, cip->clean, npos);
		for (i = 0; i < lfs_sb_getnseg(fs) && ngood < goal; i++) {
			if (fs->clfs_segtabp[i]->priority == 0)
				break;
			sn = (fs->clfs_segtabp[i] - fs->clfs_segtab);
			dlog("%s: add seg %d prio %" PRIu64,
			     lfs_sb_getfsmnt(fs), sn, fs->clfs_segtabp[i]->priority);
			sna.segments[sna.len++] = sn;
			recent[recent_last] = sn;
			recent_last = (recent_last + 1) % NRECENT;
			++ngood;
		}
	}

	/* If there is nothing to do, try again later. */
	if (sna.len == 0) {
		dlog("%s: no blocks to clean in %d cleanable segments",
		       lfs_sb_getfsmnt(fs), (int)ngood);
		return 0;
	}

	/* Record statistics */
	for (i = nb = 0; i < bic; i++)
		nb += bip[i].bi_size;
	util = ((double)nb) / (fs->clfs_nactive * lfs_sb_getssize(fs));
	cleaner_stats.util_tot += util;
	cleaner_stats.util_sos += util * util;
	cleaner_stats.bytes_written += nb;

	/*
	 * Use LFCNREWRITESEGS to move the blocks.
	 */
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNREWRITESEGS, &sna)) < 0) {
		int oerrno = errno;
		syslog(LOG_WARNING, "%s: fcntl returned %d (errno %d, %m)",
		       lfs_sb_getfsmnt(fs), r, errno);
		if (oerrno != EAGAIN && oerrno != ESHUTDOWN) {
			syslog(LOG_DEBUG, "%s: errno %d, returning",
			       lfs_sb_getfsmnt(fs), oerrno);
			return r;
		}
		if (oerrno == ESHUTDOWN) {
			syslog(LOG_NOTICE, "%s: filesystem unmounted",
			       lfs_sb_getfsmnt(fs));
			return r;
		}
	}

	/* Even with an error, some blocks may have been written */
	add_stats(sna.len, &sna.stats);

	/*
	 * Call reclaim to prompt cleaning of the segments.
	 */
	struct lfs_write_stats reclaim_stats;
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNRECLAIM, &reclaim_stats)) < 0)
		dlog("%s: reclaim failed with errno=%d",
		     lfs_sb_getfsmnt(fs), errno);

	/*
	 * Report progress (or lack thereof)
	 */
	double cost = (sna.stats.offset + reclaim_stats.offset)
		* lfs_sb_getfsize(fs);
	double revenue = sna.len * lfs_sb_getssize(fs);
	syslog(LOG_INFO, "%s: wrote %d direct and %d total frags"
	       " to clean %d segs (%d%% recovery)",
	       lfs_sb_getfsmnt(fs), sna.stats.direct, sna.stats.offset,
	       (int)sna.len, (int)(revenue == 0.0 ? 0
				   : 100 * (revenue - cost) / revenue));
	if (cost > revenue)
		syslog(LOG_WARNING, "%s: cleaner not making forward progress",
		       lfs_sb_getfsmnt(fs));

	return 0;
}

/*
 * Read the cleanerinfo block and apply cleaning policy to determine whether
 * the given filesystem needs to be cleaned.  Returns 1 if it does, 0 if it
 * does not, or -1 on error.
 */
static int
needs_cleaning(struct clfs *fs, CLEANERINFO64 *cip)
{
	struct stat st;
	daddr_t fsb_per_seg, max_free_segs;
	time_t now;
	double loadavg;

	/* If this fs is "on hold", don't clean it. */
	if (fs->clfs_onhold) {
#if defined(__GNUC__) && \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)) && \
    defined(__OPTIMIZE_SIZE__)
	/*
	 * XXX: Work around apparent bug with GCC >= 4.8 and -Os: it
	 * claims that ci.clean is uninitialized in clean_fs (at one
	 * of the several uses of it, which is neither the first nor
	 * last use) -- this doesn't happen with plain -O2.
	 *
	 * Hopefully in the future further rearrangements will allow
	 * removing this hack.
	 */
		cip->clean = 0;
#endif
		return 0;
	}

	/* Read the cleanerinfo from the Ifile. */
	if (fcntl(fs->clfs_ifilefd, LFCNCLEANERINFO, cip) < 0) {
		syslog(LOG_ERR, "%s: failed to read cleaner info", lfs_sb_getfsmnt(fs));
	}
	dlog("Read cleanerinfo: %d clean %d dirty %ld bfree %ld avail flags 0x%x",
	     (int)cip->clean,
	     (int)cip->dirty,
	     (long)cip->bfree,
	     (long)cip->avail,
	     (unsigned)cip->flags);

	/*
	 * If the number of segments changed under us, reinit.
	 * We don't have to start over from scratch, however,
	 * since we don't hold any buffers.
	 */
	if (lfs_sb_getnseg(fs) != cip->clean + cip->dirty) {
		if (reinit_fs(fs) < 0) {
			/* The normal case for unmount */
			syslog(LOG_NOTICE, "%s: filesystem unmounted", lfs_sb_getfsmnt(fs));
			return -1;
		}
		syslog(LOG_NOTICE, "%s: nsegs changed", lfs_sb_getfsmnt(fs));
	}

	/* Compute theoretical "free segments" maximum based on usage */
	fsb_per_seg = lfs_segtod(fs, 1);
	max_free_segs = MAX(cip->bfree, 0) / fsb_per_seg + lfs_sb_getminfreeseg(fs);

	dlog("%s: bfree = %d, avail = %d, clean = %d/%d",
	     lfs_sb_getfsmnt(fs), cip->bfree, cip->avail, cip->clean,
	     lfs_sb_getnseg(fs));

	/* If the writer is waiting on us, clean it */
	if (cip->clean <= lfs_sb_getminfreeseg(fs) ||
	    (cip->flags & LFS_CLEANER_MUST_CLEAN)) {
		dlog("%s: must clean", lfs_sb_getfsmnt(fs));
		return 1;
	}

	/* If there are enough segments, don't clean it */
	if (cip->bfree - cip->avail <= fsb_per_seg &&
	    cip->avail > fsb_per_seg) {
		dlog("%s: enough clean", lfs_sb_getfsmnt(fs));
		return 0;
	}

	/* If we are in dire straits, clean it */
	if (cip->bfree - cip->avail > fsb_per_seg &&
	    cip->avail <= fsb_per_seg) {
		dlog("%s: dire straits", lfs_sb_getfsmnt(fs));
		return 1;
	}

	/* If under busy threshold, clean regardless of load */
	if (cip->clean < max_free_segs * BUSY_LIM) {
		dlog("%s: busy clean", lfs_sb_getfsmnt(fs));
		return 1;
	}

	/* Check busy status; clean if idle and under idle limit */
	if (use_fs_idle) {
		/* Filesystem idle */
		time(&now);
		if (fstat(fs->clfs_ifilefd, &st) < 0) {
			syslog(LOG_ERR, "%s: failed to stat ifile",
			       lfs_sb_getfsmnt(fs));
			return -1;
		}
		if (now - st.st_mtime > segwait_timeout &&
		    cip->clean < max_free_segs * IDLE_LIM) {
			dlog("%s: idle clean with fs idle %d",
			     lfs_sb_getfsmnt(fs), (int)(now - st.st_mtime));
			return 1;
		}
	} else {
		/* CPU idle - use one-minute load avg */
		if (getloadavg(&loadavg, 1) == -1) {
			syslog(LOG_ERR, "%s: failed to get load avg",
			       lfs_sb_getfsmnt(fs));
			return -1;
		}
		if (loadavg < load_threshold &&
		    cip->clean < max_free_segs * IDLE_LIM) {
			dlog("%s: idle clean with load %f",
			     lfs_sb_getfsmnt(fs), loadavg);
			return 1;
		}
	}

	dlog("%s: default no clean", lfs_sb_getfsmnt(fs));
	return 0;
}

/*
 * Report statistics.  If the signal was SIGUSR2, clear the statistics too.
 * If the signal was SIGINT, exit.
 */
static void
sig_report(int sig)
{
	double avg = 0.0, stddev;

	avg = cleaner_stats.util_tot / MAX(cleaner_stats.segs_cleaned, 1.0);
	stddev = cleaner_stats.util_sos / MAX(cleaner_stats.segs_cleaned -
					      avg * avg, 1.0);
	syslog(LOG_INFO, "bytes read:	     %" PRId64, cleaner_stats.bytes_read);
	syslog(LOG_INFO, "bytes written:     %" PRId64, cleaner_stats.bytes_written);
	syslog(LOG_INFO, "segments cleaned:  %" PRId64, cleaner_stats.segs_cleaned);
#if 0
	/* "Empty segments" is meaningless, since the kernel handles those */
	syslog(LOG_INFO, "empty segments:    %" PRId64, cleaner_stats.segs_empty);
#endif
	syslog(LOG_INFO, "error segments:    %" PRId64, cleaner_stats.segs_error);
	syslog(LOG_INFO, "utilization total: %g", cleaner_stats.util_tot);
	syslog(LOG_INFO, "utilization sos:   %g", cleaner_stats.util_sos);
	syslog(LOG_INFO, "utilization avg:   %4.2f", avg);
	syslog(LOG_INFO, "utilization sdev:  %9.6f", stddev);

	if (sig == SIGUSR2)
		memset(&cleaner_stats, 0, sizeof(cleaner_stats));
	if (sig == SIGINT)
		exit(0);
}

static void
sig_exit(int sig)
{
	exit(0);
}

static void
usage(void)
{
	fprintf(stderr, "usage: lfs_cleanerd [-bcdfmqsJ] [-i segnum] [-l load]\n"
	     "\t[-n nsegs] [-r report_freq] [-t timeout] fs_name ...\n");
	exit(1);
}

#ifndef LFS_CLEANER_AS_LIB
/*
 * Main.
 */
int
main(int argc, char **argv)
{

	return lfs_cleaner_main(argc, argv);
}
#endif

int
lfs_cleaner_main(int argc, char **argv)
{
	int i, opt, error, r, loopcount, nodetach;
	struct timeval tv;
#ifdef LFS_CLEANER_AS_LIB
	sem_t *semaddr = NULL;
#endif
	CLEANERINFO64 ci;
#ifndef USE_CLIENT_SERVER
	char *cp, *pidname;
#endif

	/*
	 * Set up defaults
	 */
	atatime	 = 1;
	segwait_timeout = 300; /* Five minutes */
	load_threshold	= 0.2;
	stat_report	= 0;
	inval_segment	= -1;
	copylog_filename = NULL;
	nodetach        = 0;
	do_asdevice     = NULL;

	/*
	 * Parse command-line arguments
	 */
	while ((opt = getopt(argc, argv, "bC:cdDfi:J:l:mn:qr:sS:t:")) != -1) {
		switch (opt) {
		    case 'b':	/* Use bytes written, not segments read */
			    use_bytes = 1;
			    break;
		    case 'C':	/* copy log */
			    copylog_filename = optarg;
			    break;
		    case 'c':	/* Coalesce files */
			    do_coalesce++;
			    break;
		    case 'd':	/* Debug mode. */
			    nodetach++;
			    debug++;
			    break;
		    case 'D':	/* stay-on-foreground */
			    nodetach++;
			    break;
		    case 'f':	/* Use fs idle time rather than cpu idle */
			    use_fs_idle = 1;
			    break;
		    case 'i':	/* Invalidate this segment */
			    inval_segment = atoi(optarg);
			    break;
		    case 'l':	/* Load below which to clean */
			    load_threshold = atof(optarg);
			    break;
		    case 'm':	/* [compat only] */
			    break;
		    case 'n':	/* How many segs to clean at once */
			    atatime = atoi(optarg);
			    break;
		    case 'q':	/* Quit after one run */
			    do_quit = 1;
			    break;
		    case 'r':	/* Report every stat_report segments */
			    stat_report = atoi(optarg);
			    break;
		    case 's':	/* Small writes */
			    do_small = 1;
			    break;
#ifdef LFS_CLEANER_AS_LIB
		    case 'S':	/* semaphore */
			    semaddr = (void*)(uintptr_t)strtoull(optarg,NULL,0);
			    break;
#endif
		    case 't':	/* timeout */
			    segwait_timeout = atoi(optarg);
			    break;
		    case 'J': /* do as a device */
			    do_asdevice = optarg;
			    break;
		    default:
			    usage();
			    /* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	if (inval_segment >= 0 && argc != 1) {
		errx(1, "lfs_cleanerd: may only specify one filesystem when "
		     "using -i flag");
	}

	if (do_coalesce) {
		errx(1, "lfs_cleanerd: -c disabled due to reports of file "
		     "corruption; you may re-enable it by rebuilding the "
		     "cleaner");
	}

	/*
	 * Set up daemon mode or foreground mode
	 */
	if (nodetach) {
		openlog("lfs_cleanerd", LOG_NDELAY | LOG_PID | LOG_PERROR,
			LOG_DAEMON);
		signal(SIGINT, sig_report);
	} else {
		if (daemon(0, 0) == -1)
			err(1, "lfs_cleanerd: couldn't become a daemon!");
		openlog("lfs_cleanerd", LOG_NDELAY | LOG_PID, LOG_DAEMON);
		signal(SIGINT, sig_exit);
	}

	/*
	 * Look for an already-running master daemon.  If there is one,
	 * send it our filesystems to add to its list and exit.
	 * If there is none, become the master.
	 */
#ifdef USE_CLIENT_SERVER
	try_to_become_master(argc, argv);
#else
	/* XXX think about this */
	asprintf(&pidname, "lfs_cleanerd:m:%s", argv[0]);
	if (pidname == NULL) {
		syslog(LOG_ERR, "malloc failed: %m");
		exit(1);
	}
	for (cp = pidname; cp != NULL; cp = strchr(cp, '/'))
		*cp = '|';
	pidfile(pidname);
#endif

	/*
	 * Signals mean daemon should report its statistics
	 */
	memset(&cleaner_stats, 0, sizeof(cleaner_stats));
	signal(SIGUSR1, sig_report);
	signal(SIGUSR2, sig_report);

	/*
	 * Initialize cleaning structures, open devices, etc.
	 */
	nfss = argc;
	fsp = (struct clfs **)malloc(nfss * sizeof(*fsp));
	if (fsp == NULL) {
		syslog(LOG_ERR, "couldn't allocate fs table: %m");
		exit(1);
	}
	for (i = 0; i < nfss; i++) {
		fsp[i] = (struct clfs *)calloc(1, sizeof(**fsp));
		if ((r = init_fs(fsp[i], argv[i])) < 0) {
			syslog(LOG_ERR, "%s: couldn't init: error code %d",
			       argv[i], r);
			handle_error(fsp, i);
			--i; /* Do the new #i over again */
		}
	}

	/*
	 * If asked to coalesce, do so and exit.
	 */
	if (do_coalesce) {
		for (i = 0; i < nfss; i++)
			clean_all_inodes(fsp[i]);
		exit(0);
	}

	/*
	 * If asked to invalidate a segment, do that and exit.
	 */
	if (inval_segment >= 0) {
		invalidate_segment(fsp[0], inval_segment);
		exit(0);
	}

	/*
	 * Main cleaning loop.
	 */
	loopcount = 0;
#ifdef LFS_CLEANER_AS_LIB
	if (semaddr)
		sem_post(semaddr);
#endif
	error = 0;
	while (nfss > 0) {
		int cleaned_one;
		do {
#ifdef USE_CLIENT_SERVER
			check_control_socket();
#endif
			cleaned_one = 0;
			for (i = 0; i < nfss; i++) {
				if ((error = needs_cleaning(fsp[i], &ci)) < 0) {
					syslog(LOG_DEBUG, "%s: needs_cleaning returned %d",
					       getprogname(), error);
					handle_error(fsp, i);
					continue;
				}
				if (error == 0) /* No need to clean */
					continue;

				if ((error = clean_fs(fsp[i], &ci)) < 0) {
					syslog(LOG_DEBUG, "%s: clean_fs returned %d",
					       getprogname(), error);
					handle_error(fsp, i);
					continue;
				}
				++cleaned_one;
			}
			++loopcount;
			if (stat_report && loopcount % stat_report == 0)
				sig_report(0);
			if (do_quit)
				exit(0);
		} while(cleaned_one);
		tv.tv_sec = segwait_timeout;
		tv.tv_usec = 0;
		/* XXX: why couldn't others work if fsp socket is shutdown? */
		error = kops.ko_fcntl(fsp[0]->clfs_ifilefd,LFCNSEGWAITALL,&tv);
		if (error) {
			if (errno == ESHUTDOWN) {
				for (i = 0; i < nfss; i++) {
					syslog(LOG_INFO, "%s: shutdown",
					       getprogname());
					handle_error(fsp, i);
					assert(nfss == 0);
				}
			} else {
#ifdef LFS_CLEANER_AS_LIB
				error = ESHUTDOWN;
				break;
#else
				err(1, "LFCNSEGWAITALL");
#endif
			}
		}
	}

	/* NOTREACHED */
	return error;
}
