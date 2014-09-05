/*	$NetBSD: netbsd32_compat_20.c,v 1.34 2014/09/05 09:21:54 matt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_20.c,v 1.34 2014/09/05 09:21:54 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ktrace.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

static inline void compat_20_netbsd32_from_statvfs(struct statvfs *,
    struct netbsd32_statfs *);

static inline void
compat_20_netbsd32_from_statvfs(struct statvfs *sbp, struct netbsd32_statfs *sb32p)
{
	sb32p->f_flags = sbp->f_flag;
	sb32p->f_bsize = (netbsd32_long)sbp->f_bsize;
	sb32p->f_iosize = (netbsd32_long)sbp->f_iosize;
	sb32p->f_blocks = (netbsd32_long)sbp->f_blocks;
	sb32p->f_bfree = (netbsd32_long)sbp->f_bfree;
	sb32p->f_bavail = (netbsd32_long)sbp->f_bavail;
	sb32p->f_files = (netbsd32_long)sbp->f_files;
	sb32p->f_ffree = (netbsd32_long)sbp->f_ffree;
	sb32p->f_fsid = sbp->f_fsidx;
	sb32p->f_owner = sbp->f_owner;
	sb32p->f_spare[0] = 0;
	sb32p->f_spare[1] = 0;
	sb32p->f_spare[2] = 0;
	sb32p->f_spare[3] = 0;
	(void)memcpy(sb32p->f_fstypename, sbp->f_fstypename,
	    sizeof(sb32p->f_fstypename));
	(void)memcpy(sb32p->f_mntonname, sbp->f_mntonname,
	    sizeof(sb32p->f_mntonname));
	(void)memcpy(sb32p->f_mntfromname, sbp->f_mntfromname,
	    sizeof(sb32p->f_mntfromname));
}

int
compat_20_netbsd32_getfsstat(struct lwp *l, const struct compat_20_netbsd32_getfsstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_statfsp_t) buf;
		syscallarg(netbsd32_long) bufsize;
		syscallarg(int) flags;
	} */
	int root = 0;
	struct proc *p = l->l_proc;
	struct mount *mp, *nmp;
	struct statvfs *sb;
	struct netbsd32_statfs sb32;
	void *sfsp;
	size_t count, maxcount;
	int error = 0;

	sb = STATVFSBUF_GET();
	maxcount = SCARG(uap, bufsize) / sizeof(struct netbsd32_statfs);
	sfsp = SCARG_P32(uap, buf);
	mutex_enter(&mountlist_lock);
	count = 0;
	for (mp = TAILQ_FIRST(&mountlist); mp != NULL; mp = nmp) {
		if (vfs_busy(mp, &nmp)) {
			continue;
		}
		if (sfsp && count < maxcount) {
			error = dostatvfs(mp, sb, l, SCARG(uap, flags), 0);
			if (error) {
				vfs_unbusy(mp, false, &nmp);
				error = 0;
				continue;
			}
			compat_20_netbsd32_from_statvfs(sb, &sb32);
			error = copyout(&sb32, sfsp, sizeof(sb32));
			if (error) {
				vfs_unbusy(mp, false, NULL);
				goto out;
			}
			sfsp = (char *)sfsp + sizeof(sb32);
			root |= strcmp(sb->f_mntonname, "/") == 0;
		}
		count++;
		vfs_unbusy(mp, false, &nmp);
	}
	mutex_exit(&mountlist_lock);

	if (root == 0 && p->p_cwdi->cwdi_rdir) {
		/*
		 * fake a root entry
		 */
		error = dostatvfs(p->p_cwdi->cwdi_rdir->v_mount,
		    sb, l, SCARG(uap, flags), 1);
		if (error != 0)
			goto out;
		if (sfsp) {
			compat_20_netbsd32_from_statvfs(sb, &sb32);
			error = copyout(&sb32, sfsp, sizeof(sb32));
			if (error != 0)
				goto out;
		}
		count++;
	}

	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
out:
	STATVFSBUF_PUT(sb);
	return error;
}

int
compat_20_netbsd32_statfs(struct lwp *l, const struct compat_20_netbsd32_statfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statfsp_t) buf;
	} */
	struct mount *mp;
	struct statvfs *sb;
	struct netbsd32_statfs s32;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG_P32(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	sb = &mp->mnt_stat;
	vrele(vp);
	if ((error = dostatvfs(mp, sb, l, 0, 0)) != 0)
		return (error);
	compat_20_netbsd32_from_statvfs(sb, &s32);
	return copyout(&s32, SCARG_P32(uap, buf), sizeof(s32));
}

int
compat_20_netbsd32_fstatfs(struct lwp *l, const struct compat_20_netbsd32_fstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statfsp_t) buf;
	} */
	file_t *fp;
	struct mount *mp;
	struct statvfs *sb;
	struct netbsd32_statfs s32;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = fp->f_vnode->v_mount;
	sb = &mp->mnt_stat;
	if ((error = dostatvfs(mp, sb, l, 0, 0)) != 0)
		goto out;
	compat_20_netbsd32_from_statvfs(sb, &s32);
	error = copyout(&s32, SCARG_P32(uap, buf), sizeof(s32));
 out:
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
compat_20_netbsd32_fhstatfs(struct lwp *l, const struct compat_20_netbsd32_fhstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(struct statvfs *) buf;
	} */
	struct compat_30_sys_fhstatvfs1_args ua;

	NETBSD32TOP_UAP(fhp, const struct compat_30_fhandle);
	NETBSD32TOP_UAP(buf, struct statvfs);
#ifdef notyet
	NETBSD32TOP_UAP(flags, int);
#endif
	return (compat_30_sys_fhstatvfs1(l, &ua, retval));
}
