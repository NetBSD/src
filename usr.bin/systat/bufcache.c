/*	$NetBSD: bufcache.c,v 1.2 1999/11/15 10:54:40 mrg Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: bufcache.c,v 1.2 1999/11/15 10:54:40 mrg Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/vnode.h>

#include <err.h>
#include <kvm.h>
#include <nlist.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>

#include "systat.h"
#include "extern.h"


/*
 * Definitions for the buffer free lists (from sys/kern/vfs_bio.c).
 */
#define	BQUEUES		4		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* super-blocks &c */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */
#define	BQ_EMPTY	3		/* buffer headers with no memory */

#define VCACHE_SIZE	50

struct vcache {
	int vc_age;
	struct vnode *vc_addr;
	struct vnode vc_node;
};

struct ml_entry {
	int ml_count;
	long ml_size;
	long ml_valid;
	struct mount *ml_addr;
	struct mount ml_mount;
	LIST_ENTRY(ml_entry) ml_entries;
};

static struct nlist namelist[] = {
#define	X_NBUF		0
	{ "_nbuf" },
#define	X_BUF		1
	{ "_buf" },
#define	X_BUFQUEUES	2
	{ "_bufqueues" },
#define	X_BUFPAGES	3
	{ "_bufpages" },
	{ "" },
};

static struct vcache vcache[VCACHE_SIZE];
static LIST_HEAD(mount_list, ml_entry) mount_list;

static int nbuf, bufpages;
static void *bufaddr;
static struct buf *buf = NULL;
static TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];

static void	vc_init __P((void));
static void	ml_init __P((void));
static struct 	vnode *vc_lookup __P((struct vnode *));
static struct 	mount *ml_lookup __P((struct mount *, int, int));


WINDOW *
openbufcache()
{

	return (subwin(stdscr, LINES-5-1, 0, 5, 0));
}

void
closebufcache(w)
	WINDOW *w;
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
	ml_init();		/* Clear out mount list */
	if (buf != NULL)
		free(buf);
}

void
labelbufcache()
{
	mvwprintw(wnd, 0, 0, "There are %d buffers using %d kBytes of memory.",
	    nbuf, bufpages * CLSIZE * sysconf(_SC_PAGESIZE) / 1024);
	wclrtoeol(wnd);
	wmove(wnd, 1, 0);
	wclrtoeol(wnd);
	mvwaddstr(wnd, 2, 0,
	    "File System          Bufs used   kB in use  Bufsize kB");
	wclrtoeol(wnd);
}

void
showbufcache()
{
	int tbuf, i, lastrow;
	long tvalid, tsize;
	struct ml_entry *ml;

	tbuf = tvalid = tsize = 0;
	lastrow = 3;	/* Leave room for header. */
	for (i = lastrow, ml = LIST_FIRST(&mount_list); ml != NULL;
	    i++, ml = LIST_NEXT(ml, ml_entries)) {

		/* Display in window if enough room. */
		if (i < getmaxy(wnd) - 2) {
			mvwprintw(wnd, i, 0, "%-20.20s", ml->ml_addr == NULL ?
			    "NULL" : ml->ml_mount.mnt_stat.f_mntonname);
			wprintw(wnd, "    %6d    %8ld    %8ld", ml->ml_count,
			    ml->ml_valid / 1024, ml->ml_size / 1024);
			wclrtoeol(wnd);
			lastrow = i;
		}

		/* Update statistics. */
		tbuf += ml->ml_count;
		tvalid += ml->ml_valid / 1024;
		tsize += ml->ml_size / 1024;
	}

	wclrtobot(wnd);
	mvwprintw(wnd, lastrow + 2, 0, "%-20s    %6d    %8d    %8d",
	    "Total:", tbuf, tvalid, tsize);
}

int
initbufcache()
{
	if (namelist[X_NBUF].n_type == 0) {
		if (kvm_nlist(kd, namelist)) {
			nlisterr(namelist);
			return(0);
		}
		if (namelist[X_NBUF].n_type == 0) {
			error("namelist on %s failed", _PATH_UNIX);
			return(0);
		}
	}

	NREAD(X_NBUF, &nbuf, sizeof(nbuf));
	NREAD(X_BUFPAGES, &bufpages, sizeof(bufpages));

	buf = (struct buf *)malloc(nbuf * sizeof(struct buf));
	if (buf == NULL)
		errx(1, "malloc failed\n");
	NREAD(X_BUF, &bufaddr, sizeof(bufaddr));

	return(1);
}

void
fetchbufcache()
{
	int i, count;
	struct buf *bp;
	struct vnode *vn;
	struct mount *mt;
	struct ml_entry *ml;

	/* Re-read bufqueues lists and buffer cache headers */
	NREAD(X_BUFQUEUES, bufqueues, sizeof(bufqueues));
	KREAD(bufaddr, buf, sizeof(struct buf) * nbuf);

	/* Initialise vnode cache and mount list. */
	vc_init();
	ml_init();
	for (i = 0; i < BQUEUES; i++) {
		for (bp = bufqueues[i].tqh_first; bp != NULL;
		    bp = bp->b_freelist.tqe_next) {
			if (bp != NULL) {
				bp = (struct buf *)((u_long)bp + (u_long)buf -
				    (u_long)bufaddr);

				if (bp->b_vp != NULL) {
					vn = vc_lookup(bp->b_vp);
					if (vn == NULL)
						errx(1,
						    "vc_lookup returns NULL!\n");
					if (vn->v_mount != NULL)
						mt = ml_lookup(vn->v_mount,
						    bp->b_bufsize,
						    bp->b_bcount);
				}
			}
		}
	}

	/* simple sort - there's not that many entries */
	do {
		if ((ml = LIST_FIRST(&mount_list)) == NULL ||
		    LIST_NEXT(ml, ml_entries) == NULL)
			break;

		count = 0;
		for (ml = LIST_FIRST(&mount_list); ml != NULL;
		    ml = LIST_NEXT(ml, ml_entries)) {
			if (LIST_NEXT(ml, ml_entries) == NULL)
				break;
			if (ml->ml_count < LIST_NEXT(ml, ml_entries)->ml_count) {
				ml = LIST_NEXT(ml, ml_entries);
				LIST_REMOVE(ml, ml_entries);
				LIST_INSERT_HEAD(&mount_list, ml, ml_entries);
				count++;
			}
		}
	} while (count != 0);
}

static void
vc_init()
{
	int i;

	/* vc_addr == NULL for unused cache entry. */
	for (i = 0; i < VCACHE_SIZE; i++)
		vcache[i].vc_addr = NULL;
}

static void
ml_init()
{
	struct ml_entry *ml;

	/* Throw out the current mount list and start again. */
	while ((ml = LIST_FIRST(&mount_list)) != NULL) {
		LIST_REMOVE(ml, ml_entries);
		free(ml);
	}
}


static struct vnode *
vc_lookup(vaddr)
	struct vnode *vaddr;
{
	struct vnode *ret;
	int i, oldest, match;

	ret = NULL;
	oldest = match = 0;
	for (i = 0; i < VCACHE_SIZE || vcache[i].vc_addr == NULL; i++) {
		vcache[i].vc_age++;
		if (vcache[i].vc_addr == NULL)
			break;
		if (vcache[i].vc_age < vcache[oldest].vc_age)
			oldest = i;
		if (vcache[i].vc_addr == vaddr) {
			vcache[i].vc_age = 0;
			match = i;
			ret = &vcache[i].vc_node;
		}
	}

	/* Find an entry in the cache? */
	if (ret != NULL)
		return(ret);

	/* Go past the end of the cache? */
	if  (i >= VCACHE_SIZE)
		i = oldest;

	/* Read in new vnode and reset age counter. */
	KREAD(vaddr, &vcache[i].vc_node, sizeof(struct vnode));
	vcache[i].vc_addr = vaddr;
	vcache[i].vc_age = 0;

	return(&vcache[i].vc_node);
}

static struct mount *
ml_lookup(maddr, size, valid)
	struct mount *maddr;
	int size, valid;
{
	struct ml_entry *ml;

	for (ml = LIST_FIRST(&mount_list); ml != NULL;
	    ml = LIST_NEXT(ml, ml_entries))
		if (ml->ml_addr == maddr) {
			ml->ml_count++;
			ml->ml_size += size;
			ml->ml_valid += valid;
			if (ml->ml_addr == NULL)
				return(NULL);
			else
				return(&ml->ml_mount);
		}

	if ((ml = malloc(sizeof(struct ml_entry))) == NULL)
		errx(1, "out of memory\n");
	LIST_INSERT_HEAD(&mount_list, ml, ml_entries);
	ml->ml_count = 1;
	ml->ml_size = size;
	ml->ml_valid = valid;
	ml->ml_addr = maddr;
	if (maddr == NULL)
		return(NULL);

	KREAD(maddr, &ml->ml_mount, sizeof(struct mount));
	return(&ml->ml_mount);
}
