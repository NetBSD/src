/* $NetBSD: lkminit_vfs.c,v 1.1.2.2 2000/12/08 09:14:45 bouyer Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

#define LFS
#include <sys/syscall.h>
#include <sys/syscallargs.h>

/* used for lfs syscal entry table */
struct lfs_sysent {
	int sysno;
	struct sysent sysent;
};

int lfs_lkmentry __P((struct lkm_table *, int, int));
static int lfs_load __P((struct lkm_table *, int));
static int lfs_unload __P((struct lkm_table *, int));

#define LFS_SYSENT_CNT		4
const struct lfs_sysent lfs_sysents[LFS_SYSENT_CNT] = {
	{ SYS_lfs_bmapv,
		{ 3, sizeof(struct sys_lfs_bmapv_args), sys_lfs_bmapv } },
	{ SYS_lfs_markv,
		{ 3, sizeof(struct sys_lfs_markv_args), sys_lfs_markv } },
	{ SYS_lfs_segclean,
		{ 2, sizeof(struct sys_lfs_segclean_args), sys_lfs_segclean } },
	{ SYS_lfs_segwait,
		{ 2, sizeof(struct sys_lfs_segwait_args), sys_lfs_segwait } },
};

/*
 * This is the vfsops table for the file system in question
 */
extern struct vfsops lfs_vfsops;

/*
 * declare the filesystem
 */
MOD_VFS("lfs", -1, &lfs_vfsops);

/*
 * entry point
 */
int
lfs_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;	
	int cmd;
	int ver;
{
	int error;

	/*
	 * Since we need to call lkmdispatch() first, we can't use
	 * DISPATCH().
	 */

	if (ver != LKM_VERSION)
		return EINVAL;	/* version mismatch */

	/* do this first, lkmdispatch() expects this to be already filled in */
	if (cmd == LKM_E_LOAD)
		lkmtp->private.lkm_any = (struct lkm_any *)&_module;

	if ((error = lkmdispatch(lkmtp, cmd)))
		return error;

	switch (cmd) {
	case LKM_E_LOAD:
		if ((error = lfs_load(lkmtp, cmd)) != 0)
			return error;
		break;
	case LKM_E_UNLOAD:
		if ((error = lfs_unload(lkmtp, cmd)) != 0)
			return error;
		break;
	case LKM_E_STAT:
	default:
		break;
	}

	return 0;
}

/*
 * Insert lfs sycalls entries into sysent[].
 */
static int
lfs_load(lkmtp, cmd)
	struct lkm_table *lkmtp;	
	int cmd;
{
	int i;
	
	/* first check if the entries are empty */
	for(i=0; i < LFS_SYSENT_CNT; i++) {
		if (sysent[lfs_sysents[i].sysno].sy_call != sys_nosys) {
			printf("LKM lfs_load(): lfs syscall %d already used\n",
				lfs_sysents[i].sysno);
			return EEXIST;
		}
	}

	/* now, put in the lfs syscalls */
	for(i=0; i < LFS_SYSENT_CNT; i++) {
		sysent[lfs_sysents[i].sysno].sy_narg =
			lfs_sysents[i].sysent.sy_narg;
		sysent[lfs_sysents[i].sysno].sy_argsize =
			lfs_sysents[i].sysent.sy_argsize;
		sysent[lfs_sysents[i].sysno].sy_call =
			lfs_sysents[i].sysent.sy_call;
	}

	return 0;
}

/*
 * Remove lfs sycalls entries from sysent[].
 */
static int
lfs_unload(lkmtp, cmd)
	struct lkm_table *lkmtp;	
	int cmd;
{
	int i;

	/* reset lfs syscall entries back to nosys */
	for(i=0; i < LFS_SYSENT_CNT; i++) {
		if (sysent[lfs_sysents[i].sysno].sy_call !=
				lfs_sysents[i].sysent.sy_call) {
			panic("LKM lfs_unload(): syscall %d not lfs syscall",
				lfs_sysents[i].sysno);
		}
		sysent[lfs_sysents[i].sysno].sy_narg = 0;
		sysent[lfs_sysents[i].sysno].sy_argsize = 0;
		sysent[lfs_sysents[i].sysno].sy_call = sys_nosys;
	}

	return 0;
}
