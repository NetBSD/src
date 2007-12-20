/* $NetBSD: lkminit_vfs.c,v 1.14 2007/12/20 23:03:13 dsl Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lkminit_vfs.c,v 1.14 2007/12/20 23:03:13 dsl Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

#ifndef LFS
# define LFS
#endif

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <ufs/lfs/lfs_extern.h>

int lfs_lkmentry(struct lkm_table *, int, int);

static int lfs_load(struct lkm_table *, int);
static int lfs_unload(struct lkm_table *, int);
static struct sysctllog *_lfs_log;

#define LFS_SYSENT_CNT		4
static const struct {
	int sysno;
	struct sysent sysent;
} lfs_sysents[LFS_SYSENT_CNT] = {
	{ SYS_lfs_bmapv,
		{ 3, sizeof(struct sys_lfs_bmapv_args), 0,
			(sy_call_t *)sys_lfs_bmapv } },
	{ SYS_lfs_markv,
		{ 3, sizeof(struct sys_lfs_markv_args), 0,
			(sy_call_t *)sys_lfs_markv } },
	{ SYS_lfs_segclean,
		{ 2, sizeof(struct sys_lfs_segclean_args), 0,
			(sy_call_t *)sys_lfs_segclean } },
	{ SYS_lfs_segwait,
		{ 2, sizeof(struct sys_lfs_segwait_args), 0,
			(sy_call_t *)sys_lfs_segwait } },
};
static struct sysent lfs_osysent[LFS_SYSENT_CNT];

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

	DISPATCH(lkmtp, cmd, ver, lfs_load, lfs_unload, lkm_nofunc);
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
		lfs_osysent[i] = sysent[lfs_sysents[i].sysno];
		sysent[lfs_sysents[i].sysno] = lfs_sysents[i].sysent;
	}

	sysctl_vfs_lfs_setup(&_lfs_log);

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

	sysctl_teardown(&_lfs_log);

	/* reset lfs syscall entries back to nosys */
	for(i=0; i < LFS_SYSENT_CNT; i++) {
		if (sysent[lfs_sysents[i].sysno].sy_call !=
				lfs_sysents[i].sysent.sy_call) {
			panic("LKM lfs_unload(): syscall %d not lfs syscall",
				lfs_sysents[i].sysno);
		}

		sysent[lfs_sysents[i].sysno] = lfs_osysent[i];
	}

	return 0;
}
