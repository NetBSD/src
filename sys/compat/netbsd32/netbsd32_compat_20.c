/*	$NetBSD: netbsd32_compat_20.c,v 1.9.2.3 2007/05/27 14:35:17 ad Exp $	*/

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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_20.c,v 1.9.2.3 2007/05/27 14:35:17 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
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
compat_20_netbsd32_from_statvfs(sbp, sb32p)
	struct statvfs *sbp;
	struct netbsd32_statfs *sb32p;
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
#if 1
	/* May as well do the whole batch in one go */
	memcpy(sb32p->f_fstypename, sbp->f_fstypename, MFSNAMELEN+MNAMELEN+MNAMELEN);
#else
	/* If we want to be careful */
	memcpy(sb32p->f_fstypename, sbp->f_fstypename, MFSNAMELEN);
	memcpy(sb32p->f_mntonname, sbp->f_mntonname, MNAMELEN);
	memcpy(sb32p->f_mntfromname, sbp->f_mntfromname, MNAMELEN);
#endif
}

int
compat_20_netbsd32_getfsstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_20_netbsd32_getfsstat_args /* {
		syscallarg(netbsd32_statfsp_t) buf;
		syscallarg(netbsd32_long) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	struct mount *mp, *nmp;
	struct statvfs *sp;
	struct netbsd32_statfs sb32;
	void *sfsp;
	long count, maxcount, error;

	maxcount = SCARG(uap, bufsize) / sizeof(struct netbsd32_statfs);
	sfsp = (void *)NETBSD32PTR64(SCARG(uap, buf));
	mutex_enter(&mountlist_lock);
	count = 0;
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_lock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		if (sfsp && count < maxcount) {
			sp = &mp->mnt_stat;
			/*
			 * If MNT_NOWAIT or MNT_LAZY is specified, do not
			 * refresh the fsstat cache. MNT_WAIT or MNT_LAZY
			 * overrides MNT_NOWAIT.
			 */
			if (SCARG(uap, flags) != MNT_NOWAIT &&
			    SCARG(uap, flags) != MNT_LAZY &&
			    (SCARG(uap, flags) == MNT_WAIT ||
			     SCARG(uap, flags) == 0) &&
			    (error = VFS_STATVFS(mp, sp, l)) != 0) {
				mutex_enter(&mountlist_lock);
				nmp = mp->mnt_list.cqe_next;
				vfs_unbusy(mp);
				continue;
			}
			sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
			compat_20_netbsd32_from_statvfs(sp, &sb32);
			error = copyout(&sb32, sfsp, sizeof(sb32));
			if (error) {
				vfs_unbusy(mp);
				return (error);
			}
			sfsp = (char *)sfsp + sizeof(sb32);
		}
		count++;
		mutex_enter(&mountlist_lock);
		nmp = mp->mnt_list.cqe_next;
		vfs_unbusy(mp);
	}
	mutex_exit(&mountlist_lock);
	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return (0);
}

int
compat_20_netbsd32_statfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_20_netbsd32_statfs_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statfsp_t) buf;
	} */ *uap = v;
	struct mount *mp;
	struct statvfs *sp;
	struct netbsd32_statfs s32;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE, SCARG_P32(uap, path), l);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATVFS(mp, sp, l)) != 0)
		return (error);
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	compat_20_netbsd32_from_statvfs(sp, &s32);
	return copyout(&s32, SCARG_P32(uap, buf), sizeof(s32));
}

int
compat_20_netbsd32_fstatfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_20_netbsd32_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statfsp_t) buf;
	} */ *uap = v;
	struct file *fp;
	struct mount *mp;
	struct statvfs *sp;
	struct netbsd32_statfs s32;
	int error;
	struct proc *p = l->l_proc;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp, l)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	compat_20_netbsd32_from_statvfs(sp, &s32);
	error = copyout(&s32, SCARG_P32(uap, buf), sizeof(s32));
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
compat_20_netbsd32_fhstatfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_20_netbsd32_fhstatfs_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(struct statvfs *) buf;
	} */ *uap = v;
	struct compat_30_sys_fhstatvfs1_args ua;

	NETBSD32TOP_UAP(fhp, const struct compat_30_fhandle);
	NETBSD32TOP_UAP(buf, struct statvfs);
#ifdef notyet
	NETBSD32TOP_UAP(flags, int);
#endif
	return (compat_30_sys_fhstatvfs1(l, &ua, retval));
}
