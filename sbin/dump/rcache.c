/*      $NetBSD: rcache.c,v 1.3 1999/09/30 20:39:58 perseant Exp $       */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
/*-----------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

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
struct cheader {
	volatile size_t count;
};

struct cdesc {
	volatile daddr_t blkstart;
	volatile daddr_t blkend;/* start + nblksread */
	volatile daddr_t blocksRead;
	volatile size_t time;
#ifdef DIAGNOSTICS
	volatile pid_t owner;
#endif
};

static int findlru __P((void));

static void *shareBuffer = NULL;
static struct cheader *cheader;
static struct cdesc *cdesc;
static char *cdata;
static int cachebufs;
static int nblksread;

#ifdef STATS
static int nreads;
static int nphysread;
static int64_t readsize;
static int64_t physreadsize;
#endif

#define CDATA(i)	(cdata + ((i) * nblksread * dev_bsize))

/*-----------------------------------------------------------------------*/
void 
initcache(cachesize, readblksize)
	int cachesize;
	int readblksize;
{
	size_t len;
	size_t  sharedSize;

	nblksread = (readblksize + sblock->fs_bsize - 1) / sblock->fs_bsize;
	if(cachesize == -1) {	/* Compute from memory available */
		int usermem;
		int mib[2] = { CTL_HW, HW_USERMEM };
		
		len = sizeof(usermem);
		if (sysctl(mib, 2, &usermem, &len, NULL, 0) < 0) {
			msg("sysctl(hw.usermem) failed: %s\n", strerror(errno));
			return;
		}
		cachebufs = (usermem / MAXMEMPART) / (nblksread * dev_bsize);
	} else {		/* User specified */
		cachebufs = cachesize;
	}
	
	if(cachebufs) {	/* Don't allocate if zero --> no caching */
		if (cachebufs > MAXCACHEBUFS)
			cachebufs = MAXCACHEBUFS;

		sharedSize = sizeof(struct cheader) +
	   	    sizeof(struct cdesc) * cachebufs +
	   	    nblksread * cachebufs * dev_bsize;
#ifdef STATS	
		fprintf(stderr, "Using %d buffers (%d bytes)\n", cachebufs,
	   	    sharedSize);
#endif
		shareBuffer = mmap(NULL, sharedSize, PROT_READ | PROT_WRITE,
	   	    MAP_ANON | MAP_SHARED, -1, 0);
		if (shareBuffer == (void *)-1) {
			msg("can't mmap shared memory for buffer: %s\n",
			    strerror(errno));
			return;
		}
		cheader = shareBuffer;
		cdesc = (struct cdesc *) (((char *) shareBuffer) +
		    sizeof(struct cheader));
		cdata = ((char *) shareBuffer) + sizeof(struct cheader) +
	   	    sizeof(struct cdesc) * cachebufs;

		memset(shareBuffer, '\0', sharedSize);
	}
}
/*-----------------------------------------------------------------------*/
/* Find the cache buffer descriptor that shows the minimal access time */

static int 
findlru()
{
	int     i;
	int     minTime = cdesc[0].time;
	int     minIdx = 0;

	for (i = 0; i < cachebufs; i++) {
		if (cdesc[i].time < minTime) {
			minIdx = i;
			minTime = cdesc[i].time;
		}
	}

	return minIdx;
}
/*-----------------------------------------------------------------------*/
/*
 * Read data directly from disk, with smart error handling.
 * Try to recover from hard errors by reading in sector sized pieces.
 * Error recovery is attempted at most BREADEMAX times before seeking
 * consent from the operator to continue.
 */


static int breaderrors = 0;
#define BREADEMAX 32

void 
rawread(blkno, buf, size)
	daddr_t blkno;
	char *buf;
	int size;
{
	int cnt, i;
#ifdef STATS
	nphysread++;
	physreadsize += size;
#endif

	if (lseek(diskfd, ((off_t) blkno << dev_bshift), 0) < 0) {
		msg("rawread: lseek fails\n");
		goto err;
	}
	if ((cnt =  read(diskfd, buf, size)) == size)
		return;
	if (cnt == -1)
		msg("read error from %s: %s: [block %d]: count=%d\n",
			disk, strerror(errno), blkno, size);
	else
		msg("short read error from %s: [block %d]: count=%d, got=%d\n",
			disk, blkno, size, cnt);
err:
	if (++breaderrors > BREADEMAX) {
		msg("More than %d block read errors from %d\n",
			BREADEMAX, disk);
		broadcast("DUMP IS AILING!\n");
		msg("This is an unrecoverable error.\n");
		if (!query("Do you want to attempt to continue?")){
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
		if (lseek(diskfd, ((off_t)blkno << dev_bshift), 0) < 0) {
			msg("rawread: lseek2 fails: %s!\n",
			    strerror(errno));
			continue;
		}
		if ((cnt = read(diskfd, buf, (int)dev_bsize)) == dev_bsize)
			continue;
		if (cnt == -1) {
			msg("read error from %s: %s: [sector %d]: count=%d: "
			    "%s\n", disk, strerror(errno), blkno, dev_bsize,
			    strerror(errno));
			continue;
		}
		msg("short read error from %s: [sector %d]: count=%d, got=%d\n",
		    disk, blkno, dev_bsize, cnt);
	}
}

/*-----------------------------------------------------------------------*/
#define min(a,b)	(((a) < (b)) ? (a) : (b))

void 
bread(blkno, buf, size)
	daddr_t blkno;
	char *buf;
	int size;
{
	int     osize = size;
	daddr_t oblkno = blkno;
	char   *obuf = buf;
	daddr_t numBlocks = (size + dev_bsize -1) / dev_bsize;

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
	while(size > 0) {
		int     i;
		
		for (i = 0; i < cachebufs; i++) {
			struct cdesc *curr = &cdesc[i];

#ifdef DIAGNOSTICS
			if (curr->owner) {
				fprintf(stderr, "Owner is set (%d, me=%d), can"
				    "not happen.\n", curr->owner, getpid());
			}
#endif

			if (curr->blkend == 0)
				continue;
			/*
			 * If we find a bit of the read in the buffers,
			 * now compute how many blocks we can copy,
			 * copy them out, adjust blkno, buf and size,
			 * and restart
			 */
			if (curr->blkstart <= blkno &&
			    blkno < curr->blkend) {
				/* Number of data blocks to be copied */
				int toCopy = min(size,
				    (curr->blkend - blkno) * dev_bsize);
#ifdef DIAGNOSTICS
				if (toCopy <= 0 ||
				    toCopy > nblksread * dev_bsize) {
					fprintf(stderr, "toCopy %d !\n",
					    toCopy);
					dumpabort(0);
				}
				if (CDATA(i) + (blkno - curr->blkstart) *
			   	    dev_bsize < CDATA(i) ||
			   	    CDATA(i) + (blkno - curr->blkstart) *
			   	    dev_bsize >
				    CDATA(i) + nblksread * dev_bsize) {
					fprintf(stderr, "%p < %p !!!\n",
				   	   CDATA(i) + (blkno -
						curr->blkstart) * dev_bsize,
					   CDATA(i));
					fprintf(stderr, "cdesc[i].blkstart %d "
					    "blkno %d dev_bsize %ld\n", 
				   	    curr->blkstart, blkno, dev_bsize);
					dumpabort(0);
				}
#endif
				memcpy(buf, CDATA(i) +
				    (blkno - curr->blkstart) * dev_bsize,
			   	    toCopy);

				buf 	+= toCopy;
				size 	-= toCopy;
				blkno 	+= (toCopy + dev_bsize - 1) / dev_bsize;
				numBlocks -=
				    (toCopy  + dev_bsize - 1) / dev_bsize;

				curr->time = cheader->count++;

				/*
				 * If all data of a cache block have been
				 * read, chances are good no more reads
				 * will occur, so expire the cache immediately
				 */

				curr->blocksRead +=
				    (toCopy + dev_bsize -1) / dev_bsize;
				if (curr->blocksRead >= nblksread)
					curr->time = 0;

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
		if (numBlocks > nblksread) {
			rawread(oblkno, obuf, osize);
			break;
		} else {
			int     idx;
			ssize_t rsize;
			daddr_t blockBlkNo;

			blockBlkNo = (blkno / nblksread) * nblksread;
			idx = findlru();
			rsize = min(nblksread,
			    fsbtodb(sblock, sblock->fs_size) - blockBlkNo) *
			    dev_bsize;

#ifdef DIAGNOSTICS
			if (cdesc[idx].owner)
				fprintf(stderr, "Owner is set (%d, me=%d), can"
				    "not happen(2).\n", cdesc[idx].owner,
				    getpid());
			cdesc[idx].owner = getpid();
#endif
			cdesc[idx].time = cheader->count++;
			cdesc[idx].blkstart = blockBlkNo;
			cdesc[idx].blocksRead = 0;

			if (lseek(diskfd,
			    ((off_t) (blockBlkNo) << dev_bshift), 0) < 0) {
				msg("readBlocks: lseek fails: %s\n",
				    strerror(errno));
				rsize = -1;
			} else {
				rsize = read(diskfd, CDATA(idx), rsize);
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
				if (cdesc[idx].owner != getpid())
					fprintf(stderr, "Owner changed from "
					    "%d to %d, can't happen\n",
					    getpid(), cdesc[idx].owner);
				cdesc[idx].owner = 0;
#endif
				break;
			}

			/* On short read, just note the fact and go on */
			cdesc[idx].blkend = blockBlkNo + rsize / dev_bsize;

#ifdef STATS
			nphysread++;
			physreadsize += rsize;
#endif
#ifdef DIAGNOSTICS
			if (cdesc[idx].owner != getpid())
				fprintf(stderr, "Owner changed from "
				    "%d to %d, can't happen\n",
				    getpid(), cdesc[idx].owner);
			cdesc[idx].owner = 0;
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
	return;
}

/*-----------------------------------------------------------------------*/
void
printcachestats()
{
#ifdef STATS
	fprintf(stderr, "Pid %d: %d reads (%u bytes) "
	    "%d physical reads (%u bytes) %d%% hits, %d%% overhead\n",
	    getpid(), nreads, (u_int) readsize, nphysread,
	    (u_int) physreadsize, (nreads - nphysread) * 100 / nreads,
	    (int) (((physreadsize - readsize) * 100) / readsize));
#endif
}

/*-----------------------------------------------------------------------*/
