/* $NetBSD */
/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#define vnode uvnode
#define buf ubuf
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_balloc.h> /* for bitmap_init */
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_vfsops.h> /* for getnewvnode */
#undef buf
#undef vnode

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "kernelops.h"

#define panic call_panic

extern void pwarn(const char *, ...);

#if 0
extern struct uvnodelst vnodelist;
extern struct uvnodelst getvnodelist[VNODE_HASH_MAX];
extern int nvnodes;
#endif

long dev_bsize = DEV_BSIZE;
int g_devfd;
int g_verbose = 0;

/*
 * exFAT buffer and uvnode operations
 */
struct exfatfs *exfatfs_init(int, int);

static int
exfatfs_vop_strategy(struct ubuf * bp);
static int
exfatfs_vop_bwrite(struct ubuf * bp);
static int
exfatfs_vop_bmap(struct uvnode * vp, daddr_t lbn, daddr_t * daddrp);
struct uvnode *
exfatfs_vget(void * v, ino_t ino, void *);
int
exfatfs_freevnode(struct uvnode *);
static void
my_vpanic(int fatal, const char *fmt, va_list ap);
int
exfatfs_finish_mountfs(struct exfatfs *unused);

#if 0
static void
call_panic(const char *fmt, ...);
#endif

void (*panic_func)(int, const char *, va_list) = my_vpanic;

static int
exfatfs_vop_strategy(struct ubuf * bp)
{
	int count;

	if (bp->b_flags & B_READ) {
		count = kops.ko_pread(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    bp->b_blkno * dev_bsize);
		if (count == bp->b_bcount)
			bp->b_flags |= B_DONE;
	} else {
		count = kops.ko_pwrite(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    bp->b_blkno * dev_bsize);
		if (count == 0) {
			perror("pwrite");
			return -1;
		}
		bp->b_flags &= ~B_DELWRI;
		reassignbuf(bp, bp->b_vp);
	}
	return 0;
}

static int
exfatfs_vop_bwrite(struct ubuf * bp)
{
	int error;

	if ((error = exfatfs_vop_bmap(bp->b_vp, bp->b_lblkno,
				      &bp->b_blkno)) != 0)
		return error;
	exfatfs_vop_strategy(bp);
	brelse(bp, 0);
	return 0;
}

static int
exfatfs_vop_bmap(struct uvnode * vp, daddr_t lbn, daddr_t * daddrp)
{
	return exfatfs_bmap_shared(vp, lbn, NULL, daddrp, NULL);
}

int
exfatfs_freevnode(struct uvnode *vp)
{
	exfatfs_freexfinode(VTOXI(vp), 0);
	return 1; /* Already free */
}

/*
 * Take an inode number (raw disk block # and primary dirent offset)
 * and produce a vnode out of it.
 */
/* XXX merge with the below! */
struct uvnode *
exfatfs_vget(void *v, ino_t ino, void *arg)
{
	struct exfatfs *fs = (struct exfatfs *)v;
	struct uvnode *vp;
	struct xfinode *xip;

	vp = ecalloc(1, sizeof(*vp));
	vp->v_fd = g_devfd;
	vp->v_fs = fs;
	vp->v_usecount = 0;
	vp->v_strategy_op = exfatfs_vop_strategy;
	vp->v_bwrite_op = exfatfs_vop_bwrite;
	vp->v_bmap_op = exfatfs_vop_bmap;
	LIST_INIT(&vp->v_cleanblkhd);
	LIST_INIT(&vp->v_dirtyblkhd);

	if (arg == NULL) {
		xip = exfatfs_newxfinode(fs, INO2CLUST(ino), INO2ENTRY(ino));
	} else {
		xip = (struct xfinode *)arg;
	}
	vp->v_data = xip;
	xip->xi_vnode = vp;

	return vp;
}

/* XXX generic enough to be in vnode.c - just encode clust/off */
int exfatfs_getnewvnode(struct exfatfs *fs, struct uvnode *dvp,
			uint32_t clust, uint32_t off, unsigned type,
			void *extra, struct uvnode **vpp)
{
	struct uvnode *vp;
	struct xfinode *xip = extra;

	vp = ecalloc(1, sizeof(*vp));
	vp->v_fs = fs;
	vp->v_strategy_op = exfatfs_vop_strategy;
	vp->v_bwrite_op = exfatfs_vop_bwrite;
	vp->v_bmap_op = exfatfs_vop_bmap;
	vp->v_fd = fs->xf_devvp->v_fd;
	LIST_INIT(&vp->v_cleanblkhd);
	LIST_INIT(&vp->v_dirtyblkhd);

	*vpp = vp;

	if (!xip) {
		xip = ecalloc(1, sizeof(*xip));
		xip->xi_fs = fs;
		xip->xi_dirclust  = clust;
		xip->xi_diroffset = off;
		xip->xi_dirgen = 0; /* vap->va_gen; */
	}
	vp->v_data = xip;

        return 0;
}

/* Initialize EXFATFS library; load superblocks and choose which to use. */
struct exfatfs *
exfatfs_init(int devfd, int verbose)
{
	struct uvnode *devvp;
	struct exfatfs *fs;
	int error;

	g_verbose = verbose;
	g_devfd = devfd;
	vfs_init();
	bufinit(0);

	devvp = ecalloc(1, sizeof(*devvp));
	devvp->v_fs = NULL;
	devvp->v_fd = devfd;
	devvp->v_strategy_op = raw_vop_strategy;
	devvp->v_bwrite_op = raw_vop_bwrite;
	devvp->v_bmap_op = raw_vop_bmap;
	LIST_INIT(&devvp->v_cleanblkhd);
	LIST_INIT(&devvp->v_dirtyblkhd);

	/* Load superblocks, build up fs */
	if ((error = exfatfs_mountfs_shared(devvp, NULL, DEV_BSIZE, &fs)) != 0) {
		errno = error;
		err(1, "mount");
	}
	if (g_verbose)
		printf("fs = %p, devvp = %p, fs->xf_devvp = %p\n",
		       fs, devvp, fs->xf_devvp);

	register_vget((void *)fs, exfatfs_vget, exfatfs_freevnode);

	return fs;
}

#if 0
int
exfatfs_newxfinode(struct exfatfs *fs, struct uvnode *vp,
		   uint32_t dirclust, uint32_t diroffset,
		   struct xfinode **xipp)
{
	struct xfinode *xip;
	
	xip = malloc(sizeof(*xip));
	memset(xip, 0, sizeof(*xip));
	xip->xi_fs = fs;
	xip->xi_refcnt = 1;
	xip->xi_devvp = fs->xf_devvp;
	xip->xi_dirclust = dirclust;
	xip->xi_diroffset = diroffset;
	xip->xi_vnode = vp;
	*xipp = xip;

	return 0;
}
#endif

int
exfatfs_finish_mountfs(struct exfatfs *unused)
{
	return 0;
}

/* print message and exit */
static void
my_vpanic(int fatal, const char *fmt, va_list ap)
{
        (void) vprintf(fmt, ap);
	exit(8);
}

#if 0
static void
call_panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
        panic_func(1, fmt, ap);
	va_end(ap);
}
#endif

