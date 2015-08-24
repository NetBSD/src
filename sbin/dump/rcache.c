/*	$NetBSD: rcache.c,v 1.25 2015/08/24 17:34:03 bouyer Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin J. Laubach <mjl@emsi.priv.at> and
 *    Manuel Bouyer <Manuel.Bouyer@lip6.fr>.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rcache.c,v 1.25 2015/08/24 17:34:03 bouyer Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "dump.h"

/*-----------------------------------------------------------------------*/
#define MAXCACHEBUFS	512	/* max 512 buffers */
#define MAXMEMPART	6	/* max 15% of the user mem */

/*-----------------------------------------------------------------------*/
union cdesc {
	volatile size_t cd_count;
	struct {
		volatile daddr_t blkstart;
		volatile daddr_t blkend;	/* start + nblksread */
		volatile daddr_t blocksRead;
		volatile size_t time;
#ifdef DIAGNOSTICS
		volatile pid_t owner;
#endif
	} desc;
#define cd_blkstart	desc.blkstart
#define cd_blkend	desc.blkend
#define cd_blocksRead	desc.blocksRead
#define cd_time		desc.time
#define cd_owner	desc.owner
};

static int findlru(void);

static void *shareBuffer = NULL;
static union cdesc *cheader;
static union cdesc *cdesc;
static char *cdata;
static int cachebufs;
static int nblksread;

#ifdef STATS
static int nreads;
static int nphysread;
static int64_t readsize;
static int64_t physreadsize;
#endif

#define	CSIZE		(nblksread << dev_bshift)	/* cache buf size */
#define	CDATA(desc)	(cdata + ((desc) - cdesc) * CSIZE)

void
initcache(int cachesize, int readblksize)
{
	size_t len;
	size_t sharedSize;

	if (readblksize == -1) { /* use kern.maxphys */
		int kern_maxphys;
		int mib[2] = { CTL_KERN, KERN_MAXPHYS };

		len = sizeof(kern_maxphys);
		if (sysctl(mib, 2, &kern_maxphys, &len, NULL, 0) < 0) {
			msg("sysctl(kern.maxphys) failed: %s\n",
			    strerror(errno));
			return;
		}
		readblksize = kern_maxphys;
	}

	/* Convert read block size in terms of filesystem block size */
	nblksread = howmany(readblksize, ufsib->ufs_bsize);

	/* Then, convert it in terms of device block size */
	nblksread <<= ufsib->ufs_bshift - dev_bshift;

	if (cachesize == -1) {	/* Compute from memory available */
		uint64_t usermem, cachetmp;
		int mib[2] = { CTL_HW, HW_USERMEM64 };

		len = sizeof(usermem);
		if (sysctl(mib, 2, &usermem, &len, NULL, 0) < 0) {
			msg("sysctl(hw.usermem) failed: %s\n",
			    strerror(errno));
			return;
		}
		cachetmp = (usermem / MAXMEMPART) / CSIZE;
		/* for those with TB of RAM */
		cachebufs = (cachetmp > INT_MAX) ? INT_MAX : cachetmp;
	} else {		/* User specified */
		cachebufs = cachesize;
	}

	if (cachebufs) {	/* Don't allocate if zero --> no caching */
		if (cachebufs > MAXCACHEBUFS)
			cachebufs = MAXCACHEBUFS;

		sharedSize = sizeof(union cdesc) +
	   	    sizeof(union cdesc) * cachebufs +
	   	    cachebufs * CSIZE;
#ifdef STATS
		fprintf(stderr, "Using %d buffers (%d bytes)\n", cachebufs,
	   	    sharedSize);
#endif
		shareBuffer = mmap(NULL, sharedSize, PROT_READ | PROT_WRITE,
	   	    MAP_ANON | MAP_SHARED, -1, 0);
		if (shareBuffer == MAP_FAILED) {
			msg("can't mmap shared memory for buffer: %s\n",
			    strerror(errno));
			return;
		}
		cheader = shareBuffer;
		cdesc = (union cdesc *) (((char *) shareBuffer) +
		    sizeof(union cdesc));
		cdata = ((char *) shareBuffer) + sizeof(union cdesc) +
	   	    sizeof(union cdesc) * cachebufs;

		memset(shareBuffer, '\0', sharedSize);
	}
}

/*
 * Find the cache buffer descriptor that shows the minimal access time
 */
static int
findlru(void)
{
	int	i;
	size_t	minTime = cdesc[0].cd_time;
	int	minIdx = 0;

	for (i = 0; i < cachebufs; i++) {
		if (cdesc[i].cd_time < minTime) {
			minIdx = i;
			minTime = cdesc[i].cd_time;
		}
	}

	return minIdx;
}

/*
 * Read data directly from disk, with smart error handling.
 * Try to recover from hard errors by reading in sector sized pieces.
 * Error recovery is attempted at most BREADEMAX times before seeking
 * consent from the operator to continue.
 */

static int breaderrors = 0;
#define BREADEMAX 32

void
rawread(daddr_t blkno, char *buf, int size)
{
	int cnt, i;

#ifdef STATS
	nphysread++;
	physreadsize += size;
#endif

loop:
	if (lseek(diskfd, ((off_t) blkno << dev_bshift), SEEK_SET) == -1) {
		msg("rawread: lseek fails\n");
		goto err;
	}
	if ((cnt = read(diskfd, buf, size)) == size)
		return;
	if (blkno + (size >> dev_bshift) > ufsib->ufs_dsize) {
		/*
		 * Trying to read the final fragment.
		 *
		 * NB - dump only works in TP_BSIZE blocks, hence
		 * rounds `dev_bsize' fragments up to TP_BSIZE pieces.
		 * It should be smarter about not actually trying to
		 * read more than it can get, but for the time being
		 * we punt and scale back the read only when it gets
		 * us into trouble. (mkm 9/25/83)
		 */
		size -= dev_bsize;
		goto loop;
	}
	if (cnt == -1)
		msg("read error from %s: %s: [block %lld]: count=%d\n",
		    disk, strerror(errno), (long long)blkno, size);
	else
		msg("short read error from %s: [block %lld]: "
		    "count=%d, got=%d\n",
		    disk, (long long)blkno, size, cnt);
err:
	if (++breaderrors > BREADEMAX) {
		msg("More than %d block read errors from %s\n",
		    BREADEMAX, disk);
		broadcast("DUMP IS AILING!\n");
		msg("This is an unrecoverable error.\n");
		if (!query("Do you want to attempt to continue?")) {
			dumpabort(0);
			/*NOTREACHED*/
		} else
			breaderrors = 0;
	}
	/*
	 * Zero buffer, then try to read each sector of buffer separately.
	 */
	memset(buf, 0, size);
	for (i = 0; i < size; i += dev_bsize, buf += dev_bsize, blkno++) {
		if (lseek(diskfd, ((off_t)blkno << dev_bshift),
		    SEEK_SET) == -1) {
			msg("rawread: lseek2 fails: %s!\n",
			    strerror(errno));
			continue;
		}
		if ((cnt = read(diskfd, buf, (int)dev_bsize)) == dev_bsize)
			continue;
		if (cnt == -1) {
			msg("read error from %s: %s: [sector %lld]: "
			    "count=%ld\n", disk, strerror(errno),
			    (long long)blkno, dev_bsize);
			continue;
		}
		msg("short read error from %s: [sector %lld]: "
		    "count=%ld, got=%d\n",
		    disk, (long long)blkno, dev_bsize, cnt);
	}
}

void
bread(daddr_t blkno, char *buf, int size)
{
	int	osize = size, idx;
	daddr_t oblkno = blkno;
	char   *obuf = buf;
	daddr_t numBlocks = howmany(size, dev_bsize);

#ifdef STATS
	nreads++;
	readsize += size;
#endif

	if (!shareBuffer) {
		rawread(blkno, buf, size);
		return;
	}

	if (flock(diskfd, LOCK_EX)) {
		msg("flock(LOCK_EX) failed: %s\n",
		    strerror(errno));
		rawread(blkno, buf, size);
		return;
	}

retry:
	idx = 0;
	while (size > 0) {
		int	i;

		for (i = 0; i < cachebufs; i++) {
			union cdesc *curr = &cdesc[(i + idx) % cachebufs];

#ifdef DIAGNOSTICS
			if (curr->cd_owner) {
				fprintf(stderr, "Owner is set (%d, me=%d), can"
				    "not happen.\n", curr->cd_owner, getpid());
			}
#endif

			if (curr->cd_blkend == 0)
				continue;
			/*
			 * If we find a bit of the read in the buffers,
			 * now compute how many blocks we can copy,
			 * copy them out, adjust blkno, buf and size,
			 * and restart
			 */
			if (curr->cd_blkstart <= blkno &&
			    blkno < curr->cd_blkend) {
				/* Number of data blocks to be copied */
				int toCopy = MIN(size,
				    (curr->cd_blkend - blkno) << dev_bshift);
#ifdef DIAGNOSTICS
				if (toCopy <= 0 || toCopy > CSIZE) {
					fprintf(stderr, "toCopy %d !\n",
					    toCopy);
					dumpabort(0);
				}
				if (CDATA(curr) +
				    ((blkno - curr->cd_blkstart) <<
				    dev_bshift) < CDATA(curr) ||
			   	    CDATA(curr) +
				    ((blkno - curr->cd_blkstart) <<
			   	    dev_bshift) > CDATA(curr) + CSIZE) {
					fprintf(stderr, "%p < %p !!!\n",
				   	   CDATA(curr) + ((blkno -
					   curr->cd_blkstart) << dev_bshift),
					   CDATA(curr));
					fprintf(stderr,
					    "cdesc[i].cd_blkstart %lld "
					    "blkno %lld dev_bsize %ld\n",
				   	    (long long)curr->cd_blkstart,
					    (long long)blkno,
					    dev_bsize);
					dumpabort(0);
				}
#endif
				memcpy(buf, CDATA(curr) +
				    ((blkno - curr->cd_blkstart) <<
				    dev_bshift),
			   	    toCopy);

				buf 	+= toCopy;
				size 	-= toCopy;
				blkno 	+= howmany(toCopy, dev_bsize);
				numBlocks -= howmany(toCopy, dev_bsize);

				curr->cd_time = cheader->cd_count++;

				/*
				 * If all data of a cache block have been
				 * read, chances are good no more reads
				 * will occur, so expire the cache immediately
				 */

				curr->cd_blocksRead +=
				    howmany(toCopy, dev_bsize);
				if (curr->cd_blocksRead >= nblksread)
					curr->cd_time = 0;

				goto retry;
			}
		}

		/* No more to do? */
		if (size == 0)
			break;

		/*
		 * This does actually not happen if fs blocks are not greater
		 * than nblksread.
		 */
		if (numBlocks > nblksread || blkno >= ufsib->ufs_dsize) {
			rawread(oblkno, obuf, osize);
			break;
		} else {
			ssize_t	rsize;
			daddr_t	blockBlkNo;

			blockBlkNo = (blkno / nblksread) * nblksread;
			idx = findlru();
			rsize = MIN(nblksread,
			    ufsib->ufs_dsize - blockBlkNo) << dev_bshift;

#ifdef DIAGNOSTICS
			if (cdesc[idx].cd_owner)
				fprintf(stderr, "Owner is set (%d, me=%d), can"
				    "not happen(2).\n", cdesc[idx].cd_owner,
				    getpid());
			cdesc[idx].cd_owner = getpid();
#endif
			cdesc[idx].cd_time = cheader->cd_count++;
			cdesc[idx].cd_blkstart = blockBlkNo;
			cdesc[idx].cd_blkend = 0;
			cdesc[idx].cd_blocksRead = 0;

			if (lseek(diskfd, ((off_t) blockBlkNo << dev_bshift),
			    SEEK_SET) == -1) {
				msg("readBlocks: lseek fails: %s\n",
				    strerror(errno));
				rsize = -1;
			} else {
				rsize = read(diskfd,
				    CDATA(&cdesc[idx]), rsize);
				if (rsize < 0) {
					msg("readBlocks: read fails: %s\n",
					    strerror(errno));
				}
			}

			/* On errors, panic, punt, try to read without
			 * cache and let raw read routine do the rest.
			 */

			if (rsize <= 0) {
				rawread(oblkno, obuf, osize);
#ifdef DIAGNOSTICS
				if (cdesc[idx].cd_owner != getpid())
					fprintf(stderr, "Owner changed from "
					    "%d to %d, can't happen\n",
					    getpid(), cdesc[idx].cd_owner);
				cdesc[idx].cd_owner = 0;
#endif
				break;
			}

			/* On short read, just note the fact and go on */
			cdesc[idx].cd_blkend = blockBlkNo + rsize / dev_bsize;

#ifdef STATS
			nphysread++;
			physreadsize += rsize;
#endif
#ifdef DIAGNOSTICS
			if (cdesc[idx].cd_owner != getpid())
				fprintf(stderr, "Owner changed from "
				    "%d to %d, can't happen\n",
				    getpid(), cdesc[idx].cd_owner);
			cdesc[idx].cd_owner = 0;
#endif
			/*
			 * We swapped some of data in, let the loop fetch
			 * them from cache
			 */
		}
	}

	if (flock(diskfd, LOCK_UN))
		msg("flock(LOCK_UN) failed: %s\n",
		    strerror(errno));
}

void
printcachestats(void)
{

#ifdef STATS
	fprintf(stderr, "Pid %d: %d reads (%u bytes) "
	    "%d physical reads (%u bytes) %d%% hits, %d%% overhead\n",
	    getpid(), nreads, (u_int) readsize, nphysread,
	    (u_int) physreadsize, (nreads - nphysread) * 100 / nreads,
	    (int) (((physreadsize - readsize) * 100) / readsize));
#endif
}
