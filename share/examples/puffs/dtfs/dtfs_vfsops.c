/*	$NetBSD: dtfs_vfsops.c,v 1.4 2006/11/13 20:11:36 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/resource.h>

#include <err.h>
#include <puffs.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "dtfs.h"

int
dtfs_mount(struct puffs_usermount *pu, void **rootcookie)
{
	struct dtfs_mount *dtm;
	struct dtfs_file *dff;
	struct puffs_node *pn;

	/* create mount-local thingie */
	dtm = emalloc(sizeof(struct dtfs_mount));
	dtm->dtm_nextfileid = 2;
	dtm->dtm_nfiles = 1;
	dtm->dtm_fsizes = 0;
	pu->pu_privdata = dtm;

	/*
	 * create root directory, do it "by hand" to avoid special-casing
	 * dtfs_genfile()
	 */
	dff = dtfs_newdir();
	dff->df_dotdot = NULL;
	pn = puffs_newpnode(pu, dff, VDIR);
	if (!pn)
		errx(1, "puffs_newpnode");

	pu->pu_rootnode = pn;
	*rootcookie = pn;

	return 0;
}

int
dtfs_start(struct puffs_usermount *pu)
{
	struct dtfs_mount *dtm;

	/* we don't have the fsidx in mount, so do attrs here */
	dtm = pu->pu_privdata;
	dtfs_baseattrs(&pu->pu_rootnode->pn_va, VDIR,pu->pu_fsidx.__fsid_val[0],
	    dtm->dtm_nextfileid);
	/* not adddented, so compensate */
	pu->pu_rootnode->pn_va.va_nlink = 2;

	dtm->dtm_nextfileid++;

	return 0;
}

int
dtfs_unmount(struct puffs_usermount *pu, int flags, pid_t pid)
{

	/* goodbye blue sky */
	return 0;
}

/*
 * statvfs() should fill in the following members of struct statvfs:
 * 
 * unsigned long   f_bsize;         file system block size
 * unsigned long   f_frsize;        fundamental file system block size
 * unsigned long   f_iosize;        optimal file system block size
 * fsblkcnt_t      f_blocks;        number of blocks in file system,
 *                                            (in units of f_frsize)
 *
 * fsblkcnt_t      f_bfree;         free blocks avail in file system
 * fsblkcnt_t      f_bavail;        free blocks avail to non-root
 * fsblkcnt_t      f_bresvd;        blocks reserved for root
 *
 * fsfilcnt_t      f_files;         total file nodes in file system
 * fsfilcnt_t      f_ffree;         free file nodes in file system
 * fsfilcnt_t      f_favail;        free file nodes avail to non-root
 * fsfilcnt_t      f_fresvd;        file nodes reserved for root
 *
 * fsid_t          f_fsidx;         NetBSD compatible fsid
 *
 *
 * The rest are filled in by the kernel.
 */
#define ROUND(a,b) (((a) + ((b)-1)) & ~((b)-1))
#define NFILES 1024*1024
int
dtfs_statvfs(struct puffs_usermount *pu, struct statvfs *sbp, pid_t pid)
{
	struct rlimit rlim;
	struct dtfs_mount *dtm;
	off_t btot, bfree;
	int pgsize;

	dtm = pu->pu_privdata;
	pgsize = getpagesize();
	memset(sbp, 0, sizeof(struct statvfs));

	/*
	 * Use datasize rlimit as an _approximation_ for free size.
	 * This, of course, is not accurate due to overhead and not
	 * accounting for metadata.
	 */
	if (getrlimit(RLIMIT_DATA, &rlim) == 0)
		btot = rlim.rlim_cur;
	else
		btot = 16*1024*1024;
	bfree = btot - dtm->dtm_fsizes;

	sbp->f_blocks = ROUND(btot, pgsize) / pgsize;
	sbp->f_files = NFILES;

	sbp->f_bsize = sbp->f_frsize = sbp->f_iosize = pgsize;
	sbp->f_bfree = sbp->f_bavail = ROUND(bfree, pgsize) / pgsize;
	sbp->f_ffree = sbp->f_favail = NFILES - dtm->dtm_nfiles;

	sbp->f_bresvd = sbp->f_fresvd = 0;

	sbp->f_fsidx = pu->pu_fsidx;

	return 0;
}
#undef ROUND 

/* we're pretty much in sync all the time */
int
dtfs_sync(struct puffs_usermount *pu, int waitfor,
	const struct puffs_cred *cred, pid_t pid)
{

	return 0;
}
