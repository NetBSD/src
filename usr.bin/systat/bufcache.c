/*	$NetBSD: bufcache.c,v 1.24 2012/11/23 06:44:38 pgoyette Exp $	*/

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
__RCSID("$NetBSD: bufcache.c,v 1.24 2012/11/23 06:44:38 pgoyette Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <miscfs/specfs/specdev.h>

#include "systat.h"
#include "extern.h"

#define VCACHE_SIZE	50
#define	PAGEINFO_ROWS	 5

struct vcache {
	int vc_age;
	struct vnode *vc_addr;
	struct vnode vc_node;
};

struct ml_entry {
	u_int ml_count;
	u_long ml_size;
	u_long ml_valid;
	struct mount *ml_addr;
	LIST_ENTRY(ml_entry) ml_entries;
	struct mount ml_mount;
};

static struct vcache vcache[VCACHE_SIZE];
static LIST_HEAD(mount_list, ml_entry) mount_list;

static uint64_t bufmem;
static u_int nbuf, pgwidth, kbwidth;
static struct uvmexp_sysctl uvmexp;

static void	vc_init(void);
static void	ml_init(void);
static struct 	vnode *vc_lookup(struct vnode *);
static struct 	mount *ml_lookup(struct mount *, int, int);
static void	fetchuvmexp(void);


WINDOW *
openbufcache(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closebufcache(WINDOW *w)
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
	ml_init();		/* Clear out mount list */
}

void
labelbufcache(void)
{
	int i;

	for (i = 0; i <= PAGEINFO_ROWS; i++) {
		wmove(wnd, i, 0);
		wclrtoeol(wnd);
	}
	mvwaddstr(wnd, PAGEINFO_ROWS + 1, 0, "File System          Bufs used"
	    "   %   kB in use   %  Bufsize kB   %  Util %");
	wclrtoeol(wnd);
}

void
showbufcache(void)
{
	int tbuf, i, lastrow;
	double tvalid, tsize;
	struct ml_entry *ml;
	size_t len;

	len = sizeof(bufmem);
	if (sysctlbyname("vm.bufmem", &bufmem, &len, NULL, 0))
		error("can't get \"vm.bufmmem\": %s", strerror(errno));

	mvwprintw(wnd, 0, 0,
	    "   %*d metadata buffers using             %*"PRId64" kBytes of "
	    "memory (%2.0f%%).",
	    pgwidth, nbuf, kbwidth, bufmem / 1024,
	    ((bufmem * 100.0) + 0.5) / getpagesize() / uvmexp.npages);
	wclrtoeol(wnd);
	mvwprintw(wnd, 1, 0,
	    "   %*" PRIu64 " pages for cached file data using   %*"
	    PRIu64 " kBytes of memory (%2.0f%%).",
	    pgwidth, uvmexp.filepages,
	    kbwidth, uvmexp.filepages * getpagesize() / 1024,
	    (uvmexp.filepages * 100 + 0.5) / uvmexp.npages);
	wclrtoeol(wnd);
	mvwprintw(wnd, 2, 0,
	    "   %*" PRIu64 " pages for executables using        %*"
	    PRIu64 " kBytes of memory (%2.0f%%).",
	    pgwidth, uvmexp.execpages,
	    kbwidth, uvmexp.execpages * getpagesize() / 1024,
	    (uvmexp.execpages * 100 + 0.5) / uvmexp.npages);
	wclrtoeol(wnd);
	mvwprintw(wnd, 3, 0,
	    "   %*" PRIu64 " pages for anon (non-file) data     %*"
	    PRIu64 " kBytes of memory (%2.0f%%).",
	    pgwidth, uvmexp.anonpages,
	    kbwidth, uvmexp.anonpages * getpagesize() / 1024,
	    (uvmexp.anonpages * 100 + 0.5) / uvmexp.npages);
	wclrtoeol(wnd);
	mvwprintw(wnd, 4, 0,
	    "   %*" PRIu64 " free pages                         %*"
	    PRIu64 " kBytes of memory (%2.0f%%).",
	    pgwidth, uvmexp.free,
	    kbwidth, uvmexp.free * getpagesize() / 1024,
	    (uvmexp.free * 100 + 0.5) / uvmexp.npages);
	wclrtoeol(wnd);

	if (nbuf == 0 || bufmem == 0) {
		wclrtobot(wnd);
		return;
	}

	tbuf = 0;
	tvalid = tsize = 0;
	lastrow = PAGEINFO_ROWS + 2;	/* Leave room for header. */
	for (i = lastrow, ml = LIST_FIRST(&mount_list); ml != NULL;
	    i++, ml = LIST_NEXT(ml, ml_entries)) {

		int cnt = ml->ml_count;
		double v = ml->ml_valid;
		double s = ml->ml_size;

		/* Display in window if enough room. */
		if (i < getmaxy(wnd) - 2) {
			mvwprintw(wnd, i, 0, "%-20.20s", ml->ml_addr == NULL ?
			    "NULL" : ml->ml_mount.mnt_stat.f_mntonname);
			wprintw(wnd,
			    "    %6d %3d    %8ld %3.0f    %8ld %3.0f     %3.0f",
			    cnt, (100 * cnt) / nbuf,
			    (long)(v/1024), 100 * v / bufmem,
			    (long)(s/1024), 100 * s / bufmem,
			    100 * v / s);
			wclrtoeol(wnd);
			lastrow = i;
		}

		/* Update statistics. */
		tbuf += cnt;
		tvalid += v;
		tsize += s;
	}

	wclrtobot(wnd);
	mvwprintw(wnd, lastrow + 2, 0,
	    "%-20s    %6d %3d    %8ld %3.0f    %8ld %3.0f     %3.0f",
	    "Total:", tbuf, (100 * tbuf) / nbuf,
	    (long)(tvalid/1024), 100 * tvalid / bufmem,
	    (long)(tsize/1024), 100 * tsize / bufmem,
	    tsize != 0 ? ((100 * tvalid) / tsize) : 0);
}

int
initbufcache(void)
{
	fetchuvmexp();
	pgwidth = (int)(floor(log10((double)uvmexp.npages)) + 1);
	kbwidth = (int)(floor(log10(uvmexp.npages * getpagesize() / 1024.0)) +
	    1);

	return(1);
}

static void
fetchuvmexp(void)
{
	int mib[2];
	size_t size;

	/* Re-read pages used for vnodes & executables */
	size = sizeof(uvmexp);
	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP2;
	if (sysctl(mib, 2, &uvmexp, &size, NULL, 0) < 0) {
		error("can't get uvmexp: %s\n", strerror(errno));
		memset(&uvmexp, 0, sizeof(uvmexp));
	}
}

void
fetchbufcache(void)
{
	int count;
	struct buf_sysctl *bp, *buffers;
	struct vnode *vn;
	struct mount *mt;
	struct ml_entry *ml;
	int mib[6];
	size_t size;
	int extraslop = 0;

	/* Re-read pages used for vnodes & executables */
	fetchuvmexp();

	/* Initialise vnode cache and mount list. */
	vc_init();
	ml_init();

	/* Get metadata buffers */
	size = 0;
	buffers = NULL;
	mib[0] = CTL_KERN;
	mib[1] = KERN_BUF;
	mib[2] = KERN_BUF_ALL;
	mib[3] = KERN_BUF_ALL;
	mib[4] = (int)sizeof(struct buf_sysctl);
	mib[5] = INT_MAX; /* we want them all */
again:
	if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0) {
		error("can't get buffers size: %s\n", strerror(errno));
		return;
	}
	if (size == 0)
		return;

	size += extraslop * sizeof(struct buf_sysctl);
	buffers = malloc(size);
	if (buffers == NULL) {
		error("can't allocate buffers: %s\n", strerror(errno));
		return;
	}
	if (sysctl(mib, 6, buffers, &size, NULL, 0) < 0) {
		free(buffers);
		if (extraslop == 0) {
			extraslop = 100;
			goto again;
		}
		error("can't get buffers: %s\n", strerror(errno));
		return;
	}

	nbuf = size / sizeof(struct buf_sysctl);
	for (bp = buffers; bp < buffers + nbuf; bp++) {
		if (UINT64TOPTR(bp->b_vp) != NULL) {
			struct mount *mp;
			vn = vc_lookup(UINT64TOPTR(bp->b_vp));
			if (vn == NULL)
				break;

			mp = vn->v_mount;
			/*
			 * References to mounted-on vnodes should be
			 * counted towards the mounted filesystem.
			 */
			if (vn->v_type == VBLK && vn->v_specnode != NULL) {
				specnode_t sn;
				specdev_t sd;
				if (!KREAD(vn->v_specnode, &sn, sizeof(sn)))
					continue;
				if (!KREAD(sn.sn_dev, &sd, sizeof(sd)))
					continue;
				if (sd.sd_mountpoint)
					mp = sd.sd_mountpoint;
			}
			if (mp != NULL)
				mt = ml_lookup(mp,
				    bp->b_bufsize,
				    bp->b_bcount);
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

	free(buffers);
}

static void
vc_init(void)
{
	int i;

	/* vc_addr == NULL for unused cache entry. */
	for (i = 0; i < VCACHE_SIZE; i++)
		vcache[i].vc_addr = NULL;
}

static void
ml_init(void)
{
	struct ml_entry *ml;

	/* Throw out the current mount list and start again. */
	while ((ml = LIST_FIRST(&mount_list)) != NULL) {
		LIST_REMOVE(ml, ml_entries);
		free(ml);
	}
}


static struct vnode *
vc_lookup(struct vnode *vaddr)
{
	struct vnode *ret;
	size_t i, oldest;

	ret = NULL;
	oldest = 0;
	for (i = 0; i < VCACHE_SIZE; i++) {
		if (vcache[i].vc_addr == NULL)
			break;
		vcache[i].vc_age++;
		if (vcache[i].vc_age < vcache[oldest].vc_age)
			oldest = i;
		if (vcache[i].vc_addr == vaddr) {
			vcache[i].vc_age = 0;
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
	if (KREAD(vaddr, &vcache[i].vc_node, sizeof(struct vnode)) == 0)
		return NULL;
	vcache[i].vc_addr = vaddr;
	vcache[i].vc_age = 0;

	return(&vcache[i].vc_node);
}

static struct mount *
ml_lookup(struct mount *maddr, int size, int valid)
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

	if ((ml = malloc(sizeof(struct ml_entry))) == NULL) {
		error("out of memory");
		die(0);
	}
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
