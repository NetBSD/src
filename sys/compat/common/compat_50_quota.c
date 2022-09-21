/*	$NetBSD: compat_50_quota.c,v 1.4 2022/09/21 07:15:24 dholland Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: compat_50_quota.c,v 1.4 2022/09/21 07:15:24 dholland Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/module.h>
#include <sys/namei.h>
#include <sys/param.h>
#include <sys/quota.h>
#include <sys/quotactl.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>
#include <sys/vnode.h>

#include <ufs/ufs/quota1.h>

static const struct syscall_package vfs_syscalls_50_quota_syscalls[] = {
	{ SYS_compat_50_quotactl, 0, (sy_call_t *)compat_50_sys_quotactl },
	{ 0, 0, NULL }
};

/* ARGSUSED */
int   
compat_50_sys_quotactl(struct lwp *l, const struct compat_50_sys_quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(void *) arg; 
	} */
	struct vnode *vp;
	struct mount *mp;
	int q1cmd;
	int idtype;
	char *qfile;
	struct dqblk dqblk;
	struct quotakey key;
	struct quotaval blocks, files;
	struct quotastat qstat;
	int error;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);       

	mp = vp->v_mount;
	q1cmd = SCARG(uap, cmd);
	idtype = quota_idtype_from_ufs(q1cmd & SUBCMDMASK);
	if (idtype == -1) {
		return EINVAL;
	}

	switch ((q1cmd & ~SUBCMDMASK) >> SUBCMDSHIFT) {
	case Q_QUOTAON:
		qfile = PNBUF_GET();
		error = copyinstr(SCARG(uap, arg), qfile, PATH_MAX, NULL);
		if (error != 0) {
			PNBUF_PUT(qfile);
			break;
		}

		error = vfs_quotactl_quotaon(mp, idtype, qfile);

		PNBUF_PUT(qfile);
		break;

	case Q_QUOTAOFF:
		error = vfs_quotactl_quotaoff(mp, idtype);
		break;

	case Q_GETQUOTA:
		key.qk_idtype = idtype;
		key.qk_id = SCARG(uap, uid);

		key.qk_objtype = QUOTA_OBJTYPE_BLOCKS;
		error = vfs_quotactl_get(mp, &key, &blocks);
		if (error) {
			break;
		}

		key.qk_objtype = QUOTA_OBJTYPE_FILES;
		error = vfs_quotactl_get(mp, &key, &files);
		if (error) {
			break;
		}

		quotavals_to_dqblk(&blocks, &files, &dqblk);
		error = copyout(&dqblk, SCARG(uap, arg), sizeof(dqblk));
		break;
		
	case Q_SETQUOTA:
		error = copyin(SCARG(uap, arg), &dqblk, sizeof(dqblk));
		if (error) {
			break;
		}
		dqblk_to_quotavals(&dqblk, &blocks, &files);

		key.qk_idtype = idtype;
		key.qk_id = SCARG(uap, uid);

		key.qk_objtype = QUOTA_OBJTYPE_BLOCKS;
		error = vfs_quotactl_put(mp, &key, &blocks);
		if (error) {
			break;
		}

		key.qk_objtype = QUOTA_OBJTYPE_FILES;
		error = vfs_quotactl_put(mp, &key, &files);
		break;
		
	case Q_SYNC:
		/*
		 * not supported but used only to see if quota is supported,
		 * emulate with stat
		 *
		 * XXX should probably be supported
		 */
		(void)idtype; /* not used */

		error = vfs_quotactl_stat(mp, &qstat);
		break;

	case Q_SETUSE:
	default:
		error = EOPNOTSUPP;
		break;
	}

	vrele(vp);
	return error;
}

MODULE(MODULE_CLASS_EXEC, compat_50_quota, "compat_50,ufs");

static int
compat_50_quota_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
        	return syscall_establish(NULL, vfs_syscalls_50_quota_syscalls);
	case MODULE_CMD_FINI:
        	return syscall_disestablish(NULL, vfs_syscalls_50_quota_syscalls);
	default:
		return ENOTTY;
	}
}
