/*	$NetBSD: dtfs_vfsops.c,v 1.13 2007/04/12 15:09:01 pooka Exp $	*/

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
#include <errno.h>
#include <puffs.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "dtfs.h"

int
dtfs_domount(struct puffs_usermount *pu)
{
	struct statvfs sb;
	struct dtfs_mount *dtm;
	struct dtfs_file *dff;
	struct puffs_node *pn;
	struct vattr *va;

	/* create mount-local thingie */
	dtm = puffs_getspecific(pu);
	dtm->dtm_nextfileid = 2;
	dtm->dtm_nfiles = 1;
	dtm->dtm_fsizes = 0;

	/*
	 * create root directory, do it "by hand" to avoid special-casing
	 * dtfs_genfile()
	 */
	dff = dtfs_newdir();
	dff->df_dotdot = NULL;
	pn = puffs_pn_new(pu, dff);
	if (!pn)
		errx(1, "puffs_newpnode");

	va = &pn->pn_va;
	dtfs_baseattrs(va, VDIR, dtm->dtm_nextfileid++);
	/* not adddented, so compensate */
	va->va_nlink = 2;

	puffs_setroot(pu, pn);

	/* XXX: should call dtfs_fs_statvfs */
	puffs_zerostatvfs(&sb);

	if (puffs_start(pu, pn, &sb) == -1)
		return errno;

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
 *
 * The rest are filled in by the kernel.
 */
#define ROUND(a,b) (((a) + ((b)-1)) & ~((b)-1))
#define NFILES 1024*1024
int
dtfs_fs_statvfs(struct puffs_cc *pcc, struct statvfs *sbp, pid_t pid)
{
	struct puffs_usermount *pu;
	struct rlimit rlim;
	struct dtfs_mount *dtm;
	off_t btot, bfree;
	int pgsize;

	pu = puffs_cc_getusermount(pcc);
	dtm = puffs_getspecific(pu);
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

	return 0;
}
#undef ROUND 

static void *
addrcmp(struct puffs_usermount *pu, struct puffs_node *pn, void *arg)
{

	if (pn == arg)
		return pn;

	return NULL;
}

int
dtfs_fs_fhtonode(struct puffs_cc *pcc, void *fid, size_t fidsize,
	void **fcookie, enum vtype *ftype, voff_t *fsize, dev_t *fdev)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct dtfs_fid *dfid;
	struct puffs_node *pn;

	dfid = fid;

	pn = puffs_pn_nodewalk(pu, addrcmp, dfid->dfid_addr);
	if (pn == NULL)
		return EINVAL;

	if (pn->pn_va.va_fileid != dfid->dfid_fileid
	    || pn->pn_va.va_gen != dfid->dfid_gen)
		return EINVAL;
	
	*fcookie = pn;
	*ftype = pn->pn_va.va_type;
	*fsize = pn->pn_va.va_size;
	*fdev = pn->pn_va.va_rdev;

	return 0;
}

int
dtfs_fs_nodetofh(struct puffs_cc *pcc, void *cookie,
	void *fid, size_t *fidsize)
{
	struct puffs_node *pn = cookie;
	struct dtfs_fid *dfid;

	memset(fid, 0xff, PUFFS_FHSIZE);
	dfid = fid;

	dfid->dfid_addr = pn;
	dfid->dfid_fileid = pn->pn_va.va_fileid;
	dfid->dfid_gen = pn->pn_va.va_gen;

	return 0;
}

void
dtfs_fs_suspend(struct puffs_cc *pcc, int status)
{

	printf("suspend status %d\n", status);
	if (status == 1)
		sleep(3);
}
