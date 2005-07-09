/*	$NetBSD: vfs_syscalls.c,v 1.224 2005/07/09 01:05:24 thorpej Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_syscalls.c	8.42 (Berkeley) 7/31/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls.c,v 1.224 2005/07/09 01:05:24 thorpej Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_ktrace.h"
#include "fss.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/sysctl.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef VERIFIED_EXEC
#include <sys/verified_exec.h>
#endif

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#if NFSS > 0
#include <dev/fssvar.h>
#endif

MALLOC_DEFINE(M_MOUNT, "mount", "vfs mount struct");

static int change_dir(struct nameidata *, struct proc *);
static int change_flags(struct vnode *, u_long, struct proc *);
static int change_mode(struct vnode *, int, struct proc *p);
static int change_owner(struct vnode *, uid_t, gid_t, struct proc *, int);
static int change_utimes(struct vnode *vp, const struct timeval *,
	       struct proc *p);
static int rename_files(const char *, const char *, struct proc *, int);

void checkdirs(struct vnode *);

int dovfsusermount = 0;

/*
 * Virtual File System System Calls
 */

/*
 * Mount a file system.
 */

#if defined(COMPAT_09) || defined(COMPAT_43)
/*
 * This table is used to maintain compatibility with 4.3BSD
 * and NetBSD 0.9 mount syscalls.  Note, the order is important!
 *
 * Do not modify this table. It should only contain filesystems
 * supported by NetBSD 0.9 and 4.3BSD.
 */
const char * const mountcompatnames[] = {
	NULL,		/* 0 = MOUNT_NONE */
	MOUNT_FFS,	/* 1 = MOUNT_UFS */
	MOUNT_NFS,	/* 2 */
	MOUNT_MFS,	/* 3 */
	MOUNT_MSDOS,	/* 4 */
	MOUNT_CD9660,	/* 5 = MOUNT_ISOFS */
	MOUNT_FDESC,	/* 6 */
	MOUNT_KERNFS,	/* 7 */
	NULL,		/* 8 = MOUNT_DEVFS */
	MOUNT_AFS,	/* 9 */
};
const int nmountcompatnames = sizeof(mountcompatnames) /
    sizeof(mountcompatnames[0]);
#endif /* COMPAT_09 || COMPAT_43 */

/* ARGSUSED */
int
sys_mount(struct lwp *l, void *v, register_t *retval)
{
	struct sys_mount_args /* {
		syscallarg(const char *) type;
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;
	int error, flag = 0;
	char fstypename[MFSNAMELEN];
	struct vattr va;
	struct nameidata nd;
	struct vfsops *vfs;

	/*
	 * if MNT_GETARGS is specified, it should be only flag.
	 */

	if ((SCARG(uap, flags) & MNT_GETARGS) != 0 &&
	    (SCARG(uap, flags) & ~MNT_GETARGS) != 0) {
		return EINVAL;
	}

	if (dovfsusermount == 0 && (SCARG(uap, flags) & MNT_GETARGS) == 0 &&
	    (error = suser(p->p_ucred, &p->p_acflag)))
		return (error);
	/*
	 * Get vnode to be covered
	 */
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	/*
	 * A lookup in VFS_MOUNT might result in an attempt to
	 * lock this vnode again, so make the lock recursive.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY | LK_SETRECURSE);
	if (SCARG(uap, flags) & (MNT_UPDATE | MNT_GETARGS)) {
		if ((vp->v_flag & VROOT) == 0) {
			vput(vp);
			return (EINVAL);
		}
		mp = vp->v_mount;
		flag = mp->mnt_flag;
		vfs = mp->mnt_op;
		/*
		 * We only allow the filesystem to be reloaded if it
		 * is currently mounted read-only.
		 */
		if ((SCARG(uap, flags) & MNT_RELOAD) &&
		    ((mp->mnt_flag & MNT_RDONLY) == 0)) {
			vput(vp);
			return (EOPNOTSUPP);	/* Needs translation */
		}
		/*
		 * In "highly secure" mode, don't let the caller do anything
		 * but downgrade a filesystem from read-write to read-only.
		 * (see also below; MNT_UPDATE or MNT_GETARGS is required.)
		 */
		if (securelevel >= 2 &&
		    SCARG(uap, flags) != MNT_GETARGS &&
		    SCARG(uap, flags) !=
		    (mp->mnt_flag | MNT_RDONLY |
		     MNT_RELOAD | MNT_FORCE | MNT_UPDATE)) {
			vput(vp);
			return (EPERM);
		}
		mp->mnt_flag |= SCARG(uap, flags) &
		    (MNT_RELOAD | MNT_FORCE | MNT_UPDATE | MNT_GETARGS);
		/*
		 * Only root, or the user that did the original mount is
		 * permitted to update it.
		 */
		if ((mp->mnt_flag & MNT_GETARGS) == 0 &&
		    mp->mnt_stat.f_owner != p->p_ucred->cr_uid &&
		    (error = suser(p->p_ucred, &p->p_acflag)) != 0) {
			vput(vp);
			return (error);
		}
		/*
		 * Do not allow NFS export by non-root users. For non-root
		 * users, silently enforce MNT_NOSUID and MNT_NODEV, and
		 * MNT_NOEXEC if mount point is already MNT_NOEXEC.
		 */
		if (p->p_ucred->cr_uid != 0) {
			if (SCARG(uap, flags) & MNT_EXPORTED) {
				vput(vp);
				return (EPERM);
			}
			SCARG(uap, flags) |= MNT_NOSUID | MNT_NODEV;
			if (flag & MNT_NOEXEC)
				SCARG(uap, flags) |= MNT_NOEXEC;
		}
		if (vfs_busy(mp, LK_NOWAIT, 0)) {
			vput(vp);
			return (EPERM);
		}
		goto update;
	} else {
		if (securelevel >= 2) {
			vput(vp);
			return (EPERM);
		}
	}
	/*
	 * If the user is not root, ensure that they own the directory
	 * onto which we are attempting to mount.
	 */
	if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)) != 0 ||
	    (va.va_uid != p->p_ucred->cr_uid &&
		(error = suser(p->p_ucred, &p->p_acflag)) != 0)) {
		vput(vp);
		return (error);
	}
	/*
	 * Do not allow NFS export by non-root users. For non-root users,
	 * silently enforce MNT_NOSUID and MNT_NODEV, and MNT_NOEXEC if the
	 * mount point is already MNT_NOEXEC.
	 */
	if (p->p_ucred->cr_uid != 0) {
		if (SCARG(uap, flags) & MNT_EXPORTED) {
			vput(vp);
			return (EPERM);
		}
		SCARG(uap, flags) |= MNT_NOSUID | MNT_NODEV;
		if (vp->v_mount->mnt_flag & MNT_NOEXEC)
			SCARG(uap, flags) |= MNT_NOEXEC;
	}
	if ((error = vinvalbuf(vp, V_SAVE, p->p_ucred, p, 0, 0)) != 0) {
		vput(vp);
		return (error);
	}
	if (vp->v_type != VDIR) {
		vput(vp);
		return (ENOTDIR);
	}
	error = copyinstr(SCARG(uap, type), fstypename, MFSNAMELEN, NULL);
	if (error) {
#if defined(COMPAT_09) || defined(COMPAT_43)
		/*
		 * Historically filesystem types were identified by number.
		 * If we get an integer for the filesystem type instead of a
		 * string, we check to see if it matches one of the historic
		 * filesystem types.
		 */
		u_long fsindex = (u_long)SCARG(uap, type);
		if (fsindex >= nmountcompatnames ||
		    mountcompatnames[fsindex] == NULL) {
			vput(vp);
			return (ENODEV);
		}
		strncpy(fstypename, mountcompatnames[fsindex], MFSNAMELEN);
#else
		vput(vp);
		return (error);
#endif
	}
#ifdef	COMPAT_10
	/* Accept `ufs' as an alias for `ffs'. */
	if (!strncmp(fstypename, "ufs", MFSNAMELEN))
		strncpy(fstypename, "ffs", MFSNAMELEN);
#endif
	if ((vfs = vfs_getopsbyname(fstypename)) == NULL) {
		vput(vp);
		return (ENODEV);
	}
	if (vp->v_mountedhere != NULL) {
		vput(vp);
		return (EBUSY);
	}

	/*
	 * Allocate and initialize the file system.
	 */
	mp = (struct mount *)malloc((u_long)sizeof(struct mount),
		M_MOUNT, M_WAITOK);
	memset((char *)mp, 0, (u_long)sizeof(struct mount));
	lockinit(&mp->mnt_lock, PVFS, "vfslock", 0, 0);
	simple_lock_init(&mp->mnt_slock);
	(void)vfs_busy(mp, LK_NOWAIT, 0);
	mp->mnt_op = vfs;
	vfs->vfs_refcount++;
	mp->mnt_vnodecovered = vp;
	mp->mnt_stat.f_owner = p->p_ucred->cr_uid;
	mp->mnt_unmounter = NULL;
	mp->mnt_leaf = mp;

	/*
	 * The underlying file system may refuse the mount for
	 * various reasons.  Allow the user to force it to happen.
	 */
	mp->mnt_flag |= SCARG(uap, flags) & MNT_FORCE;
 update:
	if ((SCARG(uap, flags) & MNT_GETARGS) == 0) {
		/*
		 * Set the mount level flags.
		 */
		if (SCARG(uap, flags) & MNT_RDONLY)
			mp->mnt_flag |= MNT_RDONLY;
		else if (mp->mnt_flag & MNT_RDONLY)
			mp->mnt_iflag |= IMNT_WANTRDWR;
		mp->mnt_flag &=
		  ~(MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
		    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC | MNT_NOCOREDUMP |
		    MNT_NOATIME | MNT_NODEVMTIME | MNT_SYMPERM | MNT_SOFTDEP |
		    MNT_MAGICLINKS);
		mp->mnt_flag |= SCARG(uap, flags) &
		   (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
		    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC | MNT_NOCOREDUMP |
		    MNT_NOATIME | MNT_NODEVMTIME | MNT_SYMPERM | MNT_SOFTDEP |
		    MNT_IGNORE | MNT_MAGICLINKS);
	}
	/*
	 * Mount the filesystem.
	 */
	error = VFS_MOUNT(mp, SCARG(uap, path), SCARG(uap, data), &nd, p);
	if (mp->mnt_flag & (MNT_UPDATE | MNT_GETARGS)) {
		if (mp->mnt_iflag & IMNT_WANTRDWR)
			mp->mnt_flag &= ~MNT_RDONLY;
		if (error)
			mp->mnt_flag = flag;
		mp->mnt_flag &=~
		    (MNT_RELOAD | MNT_FORCE | MNT_UPDATE | MNT_GETARGS);
		mp->mnt_iflag &=~ IMNT_WANTRDWR;
		if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0) {
			if (mp->mnt_syncer == NULL)
				error = vfs_allocate_syncvnode(mp);
		} else {
			if (mp->mnt_syncer != NULL)
				vfs_deallocate_syncvnode(mp);
		}
		vfs_unbusy(mp);
		VOP_UNLOCK(vp, 0);
		vrele(vp);
		return (error);
	}
	/*
	 * Put the new filesystem on the mount list after root.
	 */
	cache_purge(vp);
	if (!error) {
		mp->mnt_flag &=~
		    (MNT_RELOAD | MNT_FORCE | MNT_UPDATE | MNT_GETARGS);
		mp->mnt_iflag &=~ IMNT_WANTRDWR;
		vp->v_mountedhere = mp;
		simple_lock(&mountlist_slock);
		CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
		simple_unlock(&mountlist_slock);
		checkdirs(vp);
		VOP_UNLOCK(vp, 0);
		if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
			error = vfs_allocate_syncvnode(mp);
		vfs_unbusy(mp);
		(void) VFS_STATVFS(mp, &mp->mnt_stat, p);
		if ((error = VFS_START(mp, 0, p)))
			vrele(vp);
	} else {
		vp->v_mountedhere = (struct mount *)0;
		vfs->vfs_refcount--;
		vfs_unbusy(mp);
		free(mp, M_MOUNT);
		vput(vp);
	}
	return (error);
}

/*
 * Scan all active processes to see if any of them have a current
 * or root directory onto which the new filesystem has just been
 * mounted. If so, replace them with the new mount point.
 */
void
checkdirs(struct vnode *olddp)
{
	struct cwdinfo *cwdi;
	struct vnode *newdp;
	struct proc *p;

	if (olddp->v_usecount == 1)
		return;
	if (VFS_ROOT(olddp->v_mountedhere, &newdp))
		panic("mount: lost mount");
	proclist_lock_read();
	PROCLIST_FOREACH(p, &allproc) {
		cwdi = p->p_cwdi;
		if (!cwdi)
			continue;
		if (cwdi->cwdi_cdir == olddp) {
			vrele(cwdi->cwdi_cdir);
			VREF(newdp);
			cwdi->cwdi_cdir = newdp;
		}
		if (cwdi->cwdi_rdir == olddp) {
			vrele(cwdi->cwdi_rdir);
			VREF(newdp);
			cwdi->cwdi_rdir = newdp;
		}
	}
	proclist_unlock_read();
	if (rootvnode == olddp) {
		vrele(rootvnode);
		VREF(newdp);
		rootvnode = newdp;
	}
	vput(newdp);
}

/*
 * Unmount a file system.
 *
 * Note: unmount takes a path to the vnode mounted on as argument,
 * not special file (as before).
 */
/* ARGSUSED */
int
sys_unmount(struct lwp *l, void *v, register_t *retval)
{
	struct sys_unmount_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	mp = vp->v_mount;

	/*
	 * Only root, or the user that did the original mount is
	 * permitted to unmount this filesystem.
	 */
	if ((mp->mnt_stat.f_owner != p->p_ucred->cr_uid) &&
	    (error = suser(p->p_ucred, &p->p_acflag)) != 0) {
		vput(vp);
		return (error);
	}

	/*
	 * Don't allow unmounting the root file system.
	 */
	if (mp->mnt_flag & MNT_ROOTFS) {
		vput(vp);
		return (EINVAL);
	}

	/*
	 * Must be the root of the filesystem
	 */
	if ((vp->v_flag & VROOT) == 0) {
		vput(vp);
		return (EINVAL);
	}
	vput(vp);

	/*
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 */
	lockmgr(&syncer_lock, LK_EXCLUSIVE, NULL);

	if (vfs_busy(mp, 0, 0)) {
		lockmgr(&syncer_lock, LK_RELEASE, NULL);
		return (EBUSY);
	}

	return (dounmount(mp, SCARG(uap, flags), p));
}

/*
 * Do the actual file system unmount. File system is assumed to have been
 * marked busy by the caller.
 */
int
dounmount(struct mount *mp, int flags, struct proc *p)
{
	struct vnode *coveredvp;
	int error;
	int async;
	int used_syncer;

	simple_lock(&mountlist_slock);
	vfs_unbusy(mp);
	used_syncer = (mp->mnt_syncer != NULL);

	/*
	 * XXX Syncer must be frozen when we get here.  This should really
	 * be done on a per-mountpoint basis, but especially the softdep
	 * code possibly called from the syncer doens't exactly work on a
	 * per-mountpoint basis, so the softdep code would become a maze
	 * of vfs_busy() calls.
	 *
	 * The caller of dounmount() must acquire syncer_lock because
	 * the syncer itself acquires locks in syncer_lock -> vfs_busy
	 * order, and we must preserve that order to avoid deadlock.
	 *
	 * So, if the file system did not use the syncer, now is
	 * the time to release the syncer_lock.
	 */
	if (used_syncer == 0)
		lockmgr(&syncer_lock, LK_RELEASE, NULL);

	mp->mnt_iflag |= IMNT_UNMOUNT;
	mp->mnt_unmounter = p;
	lockmgr(&mp->mnt_lock, LK_DRAIN | LK_INTERLOCK, &mountlist_slock);
	vn_start_write(NULL, &mp, V_WAIT);

	if (mp->mnt_flag & MNT_EXPUBLIC)
		vfs_setpublicfs(NULL, NULL, NULL);
	async = mp->mnt_flag & MNT_ASYNC;
	mp->mnt_flag &= ~MNT_ASYNC;
	cache_purgevfs(mp);	/* remove cache entries for this file sys */
	if (mp->mnt_syncer != NULL)
		vfs_deallocate_syncvnode(mp);
	error = 0;
	if ((mp->mnt_flag & MNT_RDONLY) == 0) {
#if NFSS > 0
		error = fss_umount_hook(mp, (flags & MNT_FORCE));
#endif
		if (error == 0)
			error = VFS_SYNC(mp, MNT_WAIT, p->p_ucred, p);
	}
	if (error == 0 || (flags & MNT_FORCE))
		error = VFS_UNMOUNT(mp, flags, p);
	vn_finished_write(mp, 0);
	simple_lock(&mountlist_slock);
	if (error) {
		if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
			(void) vfs_allocate_syncvnode(mp);
		mp->mnt_iflag &= ~IMNT_UNMOUNT;
		mp->mnt_unmounter = NULL;
		mp->mnt_flag |= async;
		lockmgr(&mp->mnt_lock, LK_RELEASE | LK_INTERLOCK | LK_REENABLE,
		    &mountlist_slock);
		if (used_syncer)
			lockmgr(&syncer_lock, LK_RELEASE, NULL);
		simple_lock(&mp->mnt_slock);
		while (mp->mnt_wcnt > 0) {
			wakeup(mp);
			ltsleep(&mp->mnt_wcnt, PVFS, "mntwcnt1",
				0, &mp->mnt_slock);
		}
		simple_unlock(&mp->mnt_slock);
		return (error);
	}
	CIRCLEQ_REMOVE(&mountlist, mp, mnt_list);
	if ((coveredvp = mp->mnt_vnodecovered) != NULLVP) {
		coveredvp->v_mountedhere = NULL;
		vrele(coveredvp);
	}
	mp->mnt_op->vfs_refcount--;
	if (LIST_FIRST(&mp->mnt_vnodelist) != NULL)
		panic("unmount: dangling vnode");
	mp->mnt_iflag |= IMNT_GONE;
	lockmgr(&mp->mnt_lock, LK_RELEASE | LK_INTERLOCK, &mountlist_slock);
	if (used_syncer)
		lockmgr(&syncer_lock, LK_RELEASE, NULL);
	simple_lock(&mp->mnt_slock);
	while (mp->mnt_wcnt > 0) {
		wakeup(mp);
		ltsleep(&mp->mnt_wcnt, PVFS, "mntwcnt2", 0, &mp->mnt_slock);
	}
	simple_unlock(&mp->mnt_slock);
	free(mp, M_MOUNT);
	return (0);
}

/*
 * Sync each mounted filesystem.
 */
#ifdef DEBUG
int syncprt = 0;
struct ctldebug debug0 = { "syncprt", &syncprt };
#endif

/* ARGSUSED */
int
sys_sync(struct lwp *l, void *v, register_t *retval)
{
	struct mount *mp, *nmp;
	int asyncflag;
	struct proc *p = l == NULL ? &proc0 : l->l_proc;

	simple_lock(&mountlist_slock);
	for (mp = mountlist.cqh_last; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_prev;
			continue;
		}
		if ((mp->mnt_flag & MNT_RDONLY) == 0 &&
		    vn_start_write(NULL, &mp, V_NOWAIT) == 0) {
			asyncflag = mp->mnt_flag & MNT_ASYNC;
			mp->mnt_flag &= ~MNT_ASYNC;
			VFS_SYNC(mp, MNT_NOWAIT, p->p_ucred, p);
			if (asyncflag)
				 mp->mnt_flag |= MNT_ASYNC;
			vn_finished_write(mp, 0);
		}
		simple_lock(&mountlist_slock);
		nmp = mp->mnt_list.cqe_prev;
		vfs_unbusy(mp);

	}
	simple_unlock(&mountlist_slock);
#ifdef DEBUG
	if (syncprt)
		vfs_bufstats();
#endif /* DEBUG */
	return (0);
}

/*
 * Change filesystem quotas.
 */
/* ARGSUSED */
int
sys_quotactl(struct lwp *l, void *v, register_t *retval)
{
	struct sys_quotactl_args /* {
		syscallarg(const char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(caddr_t) arg;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_start_write(nd.ni_vp, &mp, V_WAIT | V_PCATCH);
	vrele(nd.ni_vp);
	if (error)
		return (error);
	error = VFS_QUOTACTL(mp, SCARG(uap, cmd), SCARG(uap, uid),
	    SCARG(uap, arg), p);
	vn_finished_write(mp, 0);
	return (error);
}

int
dostatvfs(struct mount *mp, struct statvfs *sp, struct proc *p, int flags,
    int root)
{
	struct cwdinfo *cwdi = p->p_cwdi;
	int error = 0;

	/*
	 * If MNT_NOWAIT or MNT_LAZY is specified, do not
	 * refresh the fsstat cache. MNT_WAIT or MNT_LAZY
	 * overrides MNT_NOWAIT.
	 */
	if (flags == MNT_NOWAIT	|| flags == MNT_LAZY ||
	    (flags != MNT_WAIT && flags != 0)) {
		memcpy(sp, &mp->mnt_stat, sizeof(*sp));
		goto done;
	}

	/* Get the filesystem stats now */
	memset(sp, 0, sizeof(*sp));
	if ((error = VFS_STATVFS(mp, sp, p)) != 0) {
		return error;
	}

	if (cwdi->cwdi_rdir == NULL)
		(void)memcpy(&mp->mnt_stat, sp, sizeof(mp->mnt_stat));
done:
	if (cwdi->cwdi_rdir != NULL) {
		size_t len;
		char *bp;
		char *path = malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
		if (!path)
			return ENOMEM;

		bp = path + MAXPATHLEN;
		*--bp = '\0';
		error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp, path,
		    MAXPATHLEN / 2, 0, p);
		if (error) {
			free(path, M_TEMP);
			return error;
		}
		len = strlen(bp);
		/*
		 * for mount points that are below our root, we can see
		 * them, so we fix up the pathname and return them. The
		 * rest we cannot see, so we don't allow viewing the
		 * data.
		 */
		if (strncmp(bp, sp->f_mntonname, len) == 0) {
			strlcpy(sp->f_mntonname, &sp->f_mntonname[len],
			    sizeof(sp->f_mntonname));
			if (sp->f_mntonname[0] == '\0')
				(void)strlcpy(sp->f_mntonname, "/",
				    sizeof(sp->f_mntonname));
		} else {
			if (root)
				(void)strlcpy(sp->f_mntonname, "/",
				    sizeof(sp->f_mntonname));
			else
				error = EPERM;
		}
		free(path, M_TEMP);
	}
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	return error;
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
sys_statvfs1(struct lwp *l, void *v, register_t *retval)
{
	struct sys_statvfs1_args /* {
		syscallarg(const char *) path;
		syscallarg(struct statvfs *) buf;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct statvfs sbuf;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return error;
	mp = nd.ni_vp->v_mount;
	vrele(nd.ni_vp);
	if ((error = dostatvfs(mp, &sbuf, p, SCARG(uap, flags), 1)) != 0)
		return error;
	return copyout(&sbuf, SCARG(uap, buf), sizeof(sbuf));
}

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
sys_fstatvfs1(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fstatvfs1_args /* {
		syscallarg(int) fd;
		syscallarg(struct statvfs *) buf;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mount *mp;
	struct statvfs sbuf;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	if ((error = dostatvfs(mp, &sbuf, p, SCARG(uap, flags), 1)) != 0)
		goto out;
	error = copyout(&sbuf, SCARG(uap, buf), sizeof(sbuf));
 out:
	FILE_UNUSE(fp, p);
	return error;
}


/*
 * Get statistics on all filesystems.
 */
int
sys_getvfsstat(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getvfsstat_args /* {
		syscallarg(struct statvfs *) buf;
		syscallarg(size_t) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	int root = 0;
	struct proc *p = l->l_proc;
	struct mount *mp, *nmp;
	struct statvfs sbuf;
	struct statvfs *sfsp;
	size_t count, maxcount;
	int error = 0;

	maxcount = SCARG(uap, bufsize) / sizeof(struct statvfs);
	sfsp = SCARG(uap, buf);
	simple_lock(&mountlist_slock);
	count = 0;
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		if (sfsp && count < maxcount) {
			error = dostatvfs(mp, &sbuf, p, SCARG(uap, flags), 0);
			if (error) {
				simple_lock(&mountlist_slock);
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				vfs_unbusy(mp);
				continue;
			}
			error = copyout(&sbuf, sfsp, sizeof(*sfsp));
			if (error) {
				vfs_unbusy(mp);
				return (error);
			}
			sfsp++;
			root |= strcmp(sbuf.f_mntonname, "/") == 0;
		}
		count++;
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
	if (root == 0 && p->p_cwdi->cwdi_rdir) {
		/*
		 * fake a root entry
		 */
		if ((error = dostatvfs(p->p_cwdi->cwdi_rdir->v_mount, &sbuf, p,
		    SCARG(uap, flags), 1)) != 0)
			return error;
		if (sfsp)
			error = copyout(&sbuf, sfsp, sizeof(*sfsp));
		count++;
	}
	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return error;
}

/*
 * Change current working directory to a given file descriptor.
 */
/* ARGSUSED */
int
sys_fchdir(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fchdir_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct cwdinfo *cwdi = p->p_cwdi;
	struct vnode *vp, *tdp;
	struct mount *mp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(fdp, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;

	VREF(vp);
	vn_lock(vp,  LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	while (!error && (mp = vp->v_mountedhere) != NULL) {
		if (vfs_busy(mp, 0, 0))
			continue;
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp);
		if (error)
			break;
		vput(vp);
		vp = tdp;
	}
	if (error) {
		vput(vp);
		goto out;
	}
	VOP_UNLOCK(vp, 0);

	/*
	 * Disallow changing to a directory not under the process's
	 * current root directory (if there is one).
	 */
	if (cwdi->cwdi_rdir && !vn_isunder(vp, NULL, p)) {
		vrele(vp);
		error = EPERM;	/* operation not permitted */
		goto out;
	}

	vrele(cwdi->cwdi_cdir);
	cwdi->cwdi_cdir = vp;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Change this process's notion of the root directory to a given file
 * descriptor.
 */
int
sys_fchroot(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fchroot_args *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct cwdinfo *cwdi = p->p_cwdi;
	struct vnode	*vp;
	struct file	*fp;
	int		 error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return error;
	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(fdp, SCARG(uap, fd), &fp)) != 0)
		return error;
	vp = (struct vnode *) fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	VOP_UNLOCK(vp, 0);
	if (error)
		goto out;
	VREF(vp);

	/*
	 * Prevent escaping from chroot by putting the root under
	 * the working directory.  Silently chdir to / if we aren't
	 * already there.
	 */
	if (!vn_isunder(cwdi->cwdi_cdir, vp, p)) {
		/*
		 * XXX would be more failsafe to change directory to a
		 * deadfs node here instead
		 */
		vrele(cwdi->cwdi_cdir);
		VREF(vp);
		cwdi->cwdi_cdir = vp;
	}

	if (cwdi->cwdi_rdir != NULL)
		vrele(cwdi->cwdi_rdir);
	cwdi->cwdi_rdir = vp;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Change current working directory (``.'').
 */
/* ARGSUSED */
int
sys_chdir(struct lwp *l, void *v, register_t *retval)
{
	struct sys_chdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi = p->p_cwdi;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = change_dir(&nd, p)) != 0)
		return (error);
	vrele(cwdi->cwdi_cdir);
	cwdi->cwdi_cdir = nd.ni_vp;
	return (0);
}

/*
 * Change notion of root (``/'') directory.
 */
/* ARGSUSED */
int
sys_chroot(struct lwp *l, void *v, register_t *retval)
{
	struct sys_chroot_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi = p->p_cwdi;
	struct vnode *vp;
	int error;
	struct nameidata nd;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = change_dir(&nd, p)) != 0)
		return (error);
	if (cwdi->cwdi_rdir != NULL)
		vrele(cwdi->cwdi_rdir);
	vp = nd.ni_vp;
	cwdi->cwdi_rdir = vp;

	/*
	 * Prevent escaping from chroot by putting the root under
	 * the working directory.  Silently chdir to / if we aren't
	 * already there.
	 */
	if (!vn_isunder(cwdi->cwdi_cdir, vp, p)) {
		/*
		 * XXX would be more failsafe to change directory to a
		 * deadfs node here instead
		 */
		vrele(cwdi->cwdi_cdir);
		VREF(vp);
		cwdi->cwdi_cdir = vp;
	}

	return (0);
}

/*
 * Common routine for chroot and chdir.
 */
static int
change_dir(struct nameidata *ndp, struct proc *p)
{
	struct vnode *vp;
	int error;

	if ((error = namei(ndp)) != 0)
		return (error);
	vp = ndp->ni_vp;
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);

	if (error)
		vput(vp);
	else
		VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
sys_open(struct lwp *l, void *v, register_t *retval)
{
	struct sys_open_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi = p->p_cwdi;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	int flags, cmode;
	int type, indx, error;
	struct flock lf;
	struct nameidata nd;

	flags = FFLAGS(SCARG(uap, flags));
	if ((flags & (FREAD | FWRITE)) == 0)
		return (EINVAL);
	/* falloc() will use the file descriptor for us */
	if ((error = falloc(p, &fp, &indx)) != 0)
		return (error);
	cmode = ((SCARG(uap, mode) &~ cwdi->cwdi_cmask) & ALLPERMS) &~ S_ISTXT;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	l->l_dupfd = -indx - 1;			/* XXX check for fdopen */
	if ((error = vn_open(&nd, flags, cmode)) != 0) {
		FILE_UNUSE(fp, p);
		fdp->fd_ofiles[indx] = NULL;
		ffree(fp);
		if ((error == EDUPFD || error == EMOVEFD) &&
		    l->l_dupfd >= 0 &&			/* XXX from fdopen */
		    (error =
			dupfdopen(p, indx, l->l_dupfd, flags, error)) == 0) {
			*retval = indx;
			return (0);
		}
		if (error == ERESTART)
			error = EINTR;
		fdremove(fdp, indx);
		return (error);
	}
	l->l_dupfd = 0;
	vp = nd.ni_vp;
	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;
	if (flags & (O_EXLOCK | O_SHLOCK)) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		if (flags & O_EXLOCK)
			lf.l_type = F_WRLCK;
		else
			lf.l_type = F_RDLCK;
		type = F_FLOCK;
		if ((flags & FNONBLOCK) == 0)
			type |= F_WAIT;
		VOP_UNLOCK(vp, 0);
		error = VOP_ADVLOCK(vp, fp, F_SETLK, &lf, type);
		if (error) {
			(void) vn_close(vp, fp->f_flag, fp->f_cred, p);
			FILE_UNUSE(fp, p);
			ffree(fp);
			fdremove(fdp, indx);
			return (error);
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		fp->f_flag |= FHASLOCK;
	}
	VOP_UNLOCK(vp, 0);
	*retval = indx;
	FILE_SET_MATURE(fp);
	FILE_UNUSE(fp, p);
	return (0);
}

/*
 * Get file handle system call
 */
int
sys_getfh(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getfh_args /* {
		syscallarg(char *) fname;
		syscallarg(fhandle_t *) fhp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	fhandle_t fh;
	int error;
	struct nameidata nd;

	/*
	 * Must be super user
	 */
	error = suser(p->p_ucred, &p->p_acflag);
	if (error)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, fname), p);
	error = namei(&nd);
	if (error)
		return (error);
	vp = nd.ni_vp;
	memset(&fh, 0, sizeof(fh));
	fh.fh_fsid = vp->v_mount->mnt_stat.f_fsidx;
	error = VFS_VPTOFH(vp, &fh.fh_fid);
	vput(vp);
	if (error)
		return (error);
	error = copyout(&fh, (caddr_t)SCARG(uap, fhp), sizeof (fh));
	return (error);
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
sys_fhopen(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fhopen_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp = NULL;
	struct mount *mp;
	struct ucred *cred = p->p_ucred;
	int flags;
	struct file *nfp;
	int type, indx, error=0;
	struct flock lf;
	struct vattr va;
	fhandle_t fh;

	/*
	 * Must be super user
	 */
	if ((error = suser(p->p_ucred, &p->p_acflag)))
		return (error);

	flags = FFLAGS(SCARG(uap, flags));
	if ((flags & (FREAD | FWRITE)) == 0)
		return (EINVAL);
	if ((flags & O_CREAT))
		return (EINVAL);
	/* falloc() will use the file descriptor for us */
	if ((error = falloc(p, &nfp, &indx)) != 0)
		return (error);
	fp = nfp;
	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fhandle_t))) != 0)
		goto bad;

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL) {
		error = ESTALE;
		goto bad;
	}

	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)) != 0) {
		vp = NULL;	/* most likely unnecessary sanity for bad: */
		goto bad;
	}

	/* Now do an effective vn_open */

	if (vp->v_type == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	if (flags & FREAD) {
		if ((error = VOP_ACCESS(vp, VREAD, cred, p)) != 0)
			goto bad;
	}
	if (flags & (FWRITE | O_TRUNC)) {
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto bad;
		}
		if ((error = vn_writechk(vp)) != 0 ||
		    (error = VOP_ACCESS(vp, VWRITE, cred, p)) != 0)
			goto bad;
	}
	if (flags & O_TRUNC) {
		if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0)
			goto bad;
		VOP_UNLOCK(vp, 0);			/* XXX */
		VOP_LEASE(vp, p, cred, LEASE_WRITE);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);   /* XXX */
		VATTR_NULL(&va);
		va.va_size = 0;
		error = VOP_SETATTR(vp, &va, cred, p);
		vn_finished_write(mp, 0);
		if (error)
			goto bad;
	}
	if ((error = VOP_OPEN(vp, flags, cred, p)) != 0)
		goto bad;
	if (vp->v_type == VREG &&
	    uvn_attach(vp, flags & FWRITE ? VM_PROT_WRITE : 0) == NULL) {
		error = EIO;
		goto bad;
	}
	if (flags & FWRITE)
		vp->v_writecount++;

	/* done with modified vn_open, now finish what sys_open does. */

	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;
	if (flags & (O_EXLOCK | O_SHLOCK)) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		if (flags & O_EXLOCK)
			lf.l_type = F_WRLCK;
		else
			lf.l_type = F_RDLCK;
		type = F_FLOCK;
		if ((flags & FNONBLOCK) == 0)
			type |= F_WAIT;
		VOP_UNLOCK(vp, 0);
		error = VOP_ADVLOCK(vp, fp, F_SETLK, &lf, type);
		if (error) {
			(void) vn_close(vp, fp->f_flag, fp->f_cred, p);
			FILE_UNUSE(fp, p);
			ffree(fp);
			fdremove(fdp, indx);
			return (error);
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		fp->f_flag |= FHASLOCK;
	}
	VOP_UNLOCK(vp, 0);
	*retval = indx;
	FILE_SET_MATURE(fp);
	FILE_UNUSE(fp, p);
	return (0);

bad:
	FILE_UNUSE(fp, p);
	ffree(fp);
	fdremove(fdp, indx);
	if (vp != NULL)
		vput(vp);
	return (error);
}

/* ARGSUSED */
int
sys_fhstat(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fhstat_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(struct stat *) sb;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat sb;
	int error;
	fhandle_t fh;
	struct mount *mp;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = suser(p->p_ucred, &p->p_acflag)))
		return (error);

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fhandle_t))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)))
		return (error);
	error = vn_stat(vp, &sb, p);
	vput(vp);
	if (error)
		return (error);
	error = copyout(&sb, SCARG(uap, sb), sizeof(sb));
	return (error);
}

/* ARGSUSED */
int
sys_fhstatvfs1(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fhstatvfs1_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(struct statvfs *) buf;
		syscallarg(int)	flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct statvfs sbuf;
	fhandle_t fh;
	struct mount *mp;
	struct vnode *vp;
	int error;

	/*
	 * Must be super user
	 */
	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return error;

	if ((error = copyin(SCARG(uap, fhp), &fh, sizeof(fhandle_t))) != 0)
		return error;

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return ESTALE;
	if ((error = VFS_FHTOVP(mp, &fh.fh_fid, &vp)))
		return error;

	mp = vp->v_mount;
	if ((error = dostatvfs(mp, &sbuf, p, SCARG(uap, flags), 1)) != 0) {
		vput(vp);
		return error;
	}
	vput(vp);
	return copyout(&sbuf, SCARG(uap, buf), sizeof(sbuf));
}

/*
 * Create a special file.
 */
/* ARGSUSED */
int
sys_mknod(struct lwp *l, void *v, register_t *retval)
{
	struct sys_mknod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;
	struct vattr vattr;
	int error;
	int whiteout = 0;
	struct nameidata nd;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
restart:
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp != NULL)
		error = EEXIST;
	else {
		VATTR_NULL(&vattr);
		vattr.va_mode =
		    (SCARG(uap, mode) & ALLPERMS) &~ p->p_cwdi->cwdi_cmask;
		vattr.va_rdev = SCARG(uap, dev);
		whiteout = 0;

		switch (SCARG(uap, mode) & S_IFMT) {
		case S_IFMT:	/* used by badsect to flag bad sectors */
			vattr.va_type = VBAD;
			break;
		case S_IFCHR:
			vattr.va_type = VCHR;
			break;
		case S_IFBLK:
			vattr.va_type = VBLK;
			break;
		case S_IFWHT:
			whiteout = 1;
			break;
		default:
			error = EINVAL;
			break;
		}
	}
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (vp)
			vrele(vp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	if (!error) {
		VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
		if (whiteout) {
			error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, CREATE);
			if (error)
				VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
			vput(nd.ni_dvp);
		} else {
			error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp,
						&nd.ni_cnd, &vattr);
			if (error == 0)
				vput(nd.ni_vp);
		}
	} else {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (vp)
			vrele(vp);
	}
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Create a named pipe.
 */
/* ARGSUSED */
int
sys_mkfifo(struct lwp *l, void *v, register_t *retval)
{
	struct sys_mkfifo_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

restart:
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	if (nd.ni_vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		return (EEXIST);
	}
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (nd.ni_vp)
			vrele(nd.ni_vp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VFIFO;
	vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_cwdi->cwdi_cmask;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (error == 0)
		vput(nd.ni_vp);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Make a hard file link.
 */
/* ARGSUSED */
int
sys_link(struct lwp *l, void *v, register_t *retval)
{
	struct sys_link_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0) {
		vrele(vp);
		return (error);
	}
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, link), p);
	if ((error = namei(&nd)) != 0)
		goto out;
	if (nd.ni_vp) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		error = EEXIST;
		goto out;
	}
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_LINK(nd.ni_dvp, vp, &nd.ni_cnd);
out:
	vrele(vp);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Make a symbolic link.
 */
/* ARGSUSED */
int
sys_symlink(struct lwp *l, void *v, register_t *retval)
{
	struct sys_symlink_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vattr vattr;
	char *path;
	int error;
	struct nameidata nd;

	path = PNBUF_GET();
	error = copyinstr(SCARG(uap, path), path, MAXPATHLEN, NULL);
	if (error)
		goto out;
restart:
	NDINIT(&nd, CREATE, LOCKPARENT, UIO_USERSPACE, SCARG(uap, link), p);
	if ((error = namei(&nd)) != 0)
		goto out;
	if (nd.ni_vp) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		error = EEXIST;
		goto out;
	}
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	VATTR_NULL(&vattr);
	vattr.va_mode = ACCESSPERMS &~ p->p_cwdi->cwdi_cmask;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_SYMLINK(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr, path);
	if (error == 0)
		vput(nd.ni_vp);
	vn_finished_write(mp, 0);
out:
	PNBUF_PUT(path);
	return (error);
}

/*
 * Delete a whiteout from the filesystem.
 */
/* ARGSUSED */
int
sys_undelete(struct lwp *l, void *v, register_t *retval)
{
	struct sys_undelete_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct mount *mp;
	struct nameidata nd;

restart:
	NDINIT(&nd, DELETE, LOCKPARENT|DOWHITEOUT, UIO_USERSPACE,
	    SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	if (nd.ni_vp != NULLVP || !(nd.ni_cnd.cn_flags & ISWHITEOUT)) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (nd.ni_vp)
			vrele(nd.ni_vp);
		return (EEXIST);
	}
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	if ((error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, DELETE)) != 0)
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	vput(nd.ni_dvp);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Delete a name from the filesystem.
 */
/* ARGSUSED */
int
sys_unlink(struct lwp *l, void *v, register_t *retval)
{
	struct sys_unlink_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vnode *vp;
	int error;
	struct nameidata nd;

restart:
	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;

	/*
	 * The root of a mounted filesystem cannot be deleted.
	 */
	if (vp->v_flag & VROOT) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		error = EBUSY;
		goto out;
	}

#ifdef VERIFIED_EXEC
	/* Handle remove requests for veriexec entries. */
	if ((error = veriexec_removechk(p, vp, nd.ni_dirp)) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		goto out;
	}
#endif
	
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	vn_finished_write(mp, 0);
out:
	return (error);
}

/*
 * Reposition read/write file offset.
 */
int
sys_lseek(struct lwp *l, void *v, register_t *retval)
{
	struct sys_lseek_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct ucred *cred = p->p_ucred;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	struct vattr vattr;
	off_t newoff;
	int error;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	switch (SCARG(uap, whence)) {
	case SEEK_CUR:
		newoff = fp->f_offset + SCARG(uap, offset);
		break;
	case SEEK_END:
		error = VOP_GETATTR(vp, &vattr, cred, p);
		if (error)
			goto out;
		newoff = SCARG(uap, offset) + vattr.va_size;
		break;
	case SEEK_SET:
		newoff = SCARG(uap, offset);
		break;
	default:
		error = EINVAL;
		goto out;
	}
	if ((error = VOP_SEEK(vp, fp->f_offset, newoff, cred)) != 0)
		goto out;

	*(off_t *)retval = fp->f_offset = newoff;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Positional read system call.
 */
int
sys_pread(struct lwp *l, void *v, register_t *retval)
{
	struct sys_pread_args /* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		goto out;

	/* dofileread() will unuse the descriptor for us */
	return (dofileread(p, fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &offset, 0, retval));

 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Positional scatter read system call.
 */
int
sys_preadv(struct lwp *l, void *v, register_t *retval)
{
	struct sys_preadv_args /* {
		syscallarg(int) fd;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		goto out;

	/* dofilereadv() will unuse the descriptor for us */
	return (dofilereadv(p, fd, fp, SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, 0, retval));

 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Positional write system call.
 */
int
sys_pwrite(struct lwp *l, void *v, register_t *retval)
{
	struct sys_pwrite_args /* {
		syscallarg(int) fd;
		syscallarg(const void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		goto out;

	/* dofilewrite() will unuse the descriptor for us */
	return (dofilewrite(p, fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &offset, 0, retval));

 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Positional gather write system call.
 */
int
sys_pwritev(struct lwp *l, void *v, register_t *retval)
{
	struct sys_pwritev_args /* {
		syscallarg(int) fd;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		goto out;

	/* dofilewritev() will unuse the descriptor for us */
	return (dofilewritev(p, fd, fp, SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, 0, retval));

 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Check access permissions.
 */
int
sys_access(struct lwp *l, void *v, register_t *retval)
{
	struct sys_access_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct ucred *cred;
	struct vnode *vp;
	int error, flags;
	struct nameidata nd;

	cred = crdup(p->p_ucred);
	cred->cr_uid = p->p_cred->p_ruid;
	cred->cr_gid = p->p_cred->p_rgid;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	/* Override default credentials */
	nd.ni_cnd.cn_cred = cred;
	if ((error = namei(&nd)) != 0)
		goto out;
	vp = nd.ni_vp;

	/* Flags == 0 means only check for existence. */
	if (SCARG(uap, flags)) {
		flags = 0;
		if (SCARG(uap, flags) & R_OK)
			flags |= VREAD;
		if (SCARG(uap, flags) & W_OK)
			flags |= VWRITE;
		if (SCARG(uap, flags) & X_OK)
			flags |= VEXEC;

		error = VOP_ACCESS(vp, flags, cred, p);
		if (!error && (flags & VWRITE))
			error = vn_writechk(vp);
	}
	vput(vp);
out:
	crfree(cred);
	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
sys___stat13(struct lwp *l, void *v, register_t *retval)
{
	struct sys___stat13_args /* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat sb;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	error = copyout(&sb, SCARG(uap, ub), sizeof(sb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
sys___lstat13(struct lwp *l, void *v, register_t *retval)
{
	struct sys___lstat13_args /* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat sb;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	error = copyout(&sb, SCARG(uap, ub), sizeof(sb));
	return (error);
}

/*
 * Get configurable pathname variables.
 */
/* ARGSUSED */
int
sys_pathconf(struct lwp *l, void *v, register_t *retval)
{
	struct sys_pathconf_args /* {
		syscallarg(const char *) path;
		syscallarg(int) name;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = VOP_PATHCONF(nd.ni_vp, SCARG(uap, name), retval);
	vput(nd.ni_vp);
	return (error);
}

/*
 * Return target name of a symbolic link.
 */
/* ARGSUSED */
int
sys_readlink(struct lwp *l, void *v, register_t *retval)
{
	struct sys_readlink_args /* {
		syscallarg(const char *) path;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VLNK)
		error = EINVAL;
	else if (!(vp->v_mount->mnt_flag & MNT_SYMPERM) ||
	    (error = VOP_ACCESS(vp, VREAD, p->p_ucred, p)) == 0) {
		aiov.iov_base = SCARG(uap, buf);
		aiov.iov_len = SCARG(uap, count);
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_USERSPACE;
		auio.uio_procp = p;
		auio.uio_resid = SCARG(uap, count);
		error = VOP_READLINK(vp, &auio, p->p_ucred);
	}
	vput(vp);
	*retval = SCARG(uap, count) - auio.uio_resid;
	return (error);
}

/*
 * Change flags of a file given a path name.
 */
/* ARGSUSED */
int
sys_chflags(struct lwp *l, void *v, register_t *retval)
{
	struct sys_chflags_args /* {
		syscallarg(const char *) path;
		syscallarg(u_long) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	error = change_flags(vp, SCARG(uap, flags), p);
	vput(vp);
	return (error);
}

/*
 * Change flags of a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchflags(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fchflags_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	error = change_flags(vp, SCARG(uap, flags), p);
	VOP_UNLOCK(vp, 0);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Change flags of a file given a path name; this version does
 * not follow links.
 */
int
sys_lchflags(struct lwp *l, void *v, register_t *retval)
{
	struct sys_lchflags_args /* {
		syscallarg(const char *) path;
		syscallarg(u_long) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	error = change_flags(vp, SCARG(uap, flags), p);
	vput(vp);
	return (error);
}

/*
 * Common routine to change flags of a file.
 */
int
change_flags(struct vnode *vp, u_long flags, struct proc *p)
{
	struct mount *mp;
	struct vattr vattr;
	int error;

	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0)
		return (error);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	/*
	 * Non-superusers cannot change the flags on devices, even if they
	 * own them.
	 */
	if (suser(p->p_ucred, &p->p_acflag) != 0) {
		if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
			goto out;
		if (vattr.va_type == VCHR || vattr.va_type == VBLK) {
			error = EINVAL;
			goto out;
		}
	}
	VATTR_NULL(&vattr);
	vattr.va_flags = flags;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
out:
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Change mode of a file given path name; this version follows links.
 */
/* ARGSUSED */
int
sys_chmod(struct lwp *l, void *v, register_t *retval)
{
	struct sys_chmod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_mode(nd.ni_vp, SCARG(uap, mode), p);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Change mode of a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchmod(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fchmod_args /* {
		syscallarg(int) fd;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_mode((struct vnode *)fp->f_data, SCARG(uap, mode), p);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Change mode of a file given path name; this version does not follow links.
 */
/* ARGSUSED */
int
sys_lchmod(struct lwp *l, void *v, register_t *retval)
{
	struct sys_lchmod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_mode(nd.ni_vp, SCARG(uap, mode), p);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set mode given a vnode.
 */
static int
change_mode(struct vnode *vp, int mode, struct proc *p)
{
	struct mount *mp;
	struct vattr vattr;
	int error;

	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0)
		return (error);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	VATTR_NULL(&vattr);
	vattr.va_mode = mode & ALLPERMS;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Set ownership given a path name; this version follows links.
 */
/* ARGSUSED */
int
sys_chown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_chown_args /* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), p, 0);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set ownership given a path name; this version follows links.
 * Provides POSIX semantics.
 */
/* ARGSUSED */
int
sys___posix_chown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_chown_args /* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), p, 1);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set ownership given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_owner((struct vnode *)fp->f_data, SCARG(uap, uid),
	    SCARG(uap, gid), p, 0);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Set ownership given a file descriptor, providing POSIX/XPG semantics.
 */
/* ARGSUSED */
int
sys___posix_fchown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_owner((struct vnode *)fp->f_data, SCARG(uap, uid),
	    SCARG(uap, gid), p, 1);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Set ownership given a path name; this version does not follow links.
 */
/* ARGSUSED */
int
sys_lchown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_lchown_args /* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), p, 0);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set ownership given a path name; this version does not follow links.
 * Provides POSIX/XPG semantics.
 */
/* ARGSUSED */
int
sys___posix_lchown(struct lwp *l, void *v, register_t *retval)
{
	struct sys_lchown_args /* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), p, 1);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set ownership given a vnode.
 */
static int
change_owner(struct vnode *vp, uid_t uid, gid_t gid, struct proc *p,
    int posix_semantics)
{
	struct mount *mp;
	struct vattr vattr;
	mode_t newmode;
	int error;

	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0)
		return (error);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
		goto out;

#define CHANGED(x) ((int)(x) != -1)
	newmode = vattr.va_mode;
	if (posix_semantics) {
		/*
		 * POSIX/XPG semantics: if the caller is not the super-user,
		 * clear set-user-id and set-group-id bits.  Both POSIX and
		 * the XPG consider the behaviour for calls by the super-user
		 * implementation-defined; we leave the set-user-id and set-
		 * group-id settings intact in that case.
		 */
		if (suser(p->p_ucred, NULL) != 0)
			newmode &= ~(S_ISUID | S_ISGID);
	} else {
		/*
		 * NetBSD semantics: when changing owner and/or group,
		 * clear the respective bit(s).
		 */
		if (CHANGED(uid))
			newmode &= ~S_ISUID;
		if (CHANGED(gid))
			newmode &= ~S_ISGID;
	}
	/* Update va_mode iff altered. */
	if (vattr.va_mode == newmode)
		newmode = VNOVAL;

	VATTR_NULL(&vattr);
	vattr.va_uid = CHANGED(uid) ? uid : (uid_t)VNOVAL;
	vattr.va_gid = CHANGED(gid) ? gid : (gid_t)VNOVAL;
	vattr.va_mode = newmode;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
#undef CHANGED

out:
	VOP_UNLOCK(vp, 0);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Set the access and modification times given a path name; this
 * version follows links.
 */
/* ARGSUSED */
int
sys_utimes(struct lwp *l, void *v, register_t *retval)
{
	struct sys_utimes_args /* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval *) tptr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes(nd.ni_vp, SCARG(uap, tptr), p);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set the access and modification times given a file descriptor.
 */
/* ARGSUSED */
int
sys_futimes(struct lwp *l, void *v, register_t *retval)
{
	struct sys_futimes_args /* {
		syscallarg(int) fd;
		syscallarg(const struct timeval *) tptr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_utimes((struct vnode *)fp->f_data, SCARG(uap, tptr), p);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Set the access and modification times given a path name; this
 * version does not follow links.
 */
/* ARGSUSED */
int
sys_lutimes(struct lwp *l, void *v, register_t *retval)
{
	struct sys_lutimes_args /* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval *) tptr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes(nd.ni_vp, SCARG(uap, tptr), p);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
static int
change_utimes(struct vnode *vp, const struct timeval *tptr, struct proc *p)
{
	struct timeval tv[2];
	struct mount *mp;
	struct vattr vattr;
	int error;

	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0)
		return (error);
	VATTR_NULL(&vattr);
	if (tptr == NULL) {
		microtime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		error = copyin(tptr, tv, sizeof(tv));
		if (error)
			goto out;
	}
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vattr.va_atime.tv_sec = tv[0].tv_sec;
	vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
	vattr.va_mtime.tv_sec = tv[1].tv_sec;
	vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0);
out:
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Truncate a file given its path name.
 */
/* ARGSUSED */
int
sys_truncate(struct lwp *l, void *v, register_t *retval)
{
	struct sys_truncate_args /* {
		syscallarg(const char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0) {
		vrele(vp);
		return (error);
	}
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0 &&
	    (error = VOP_ACCESS(vp, VWRITE, p->p_ucred, p)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	}
	vput(vp);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Truncate a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_ftruncate(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vattr vattr;
	struct vnode *vp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FWRITE) == 0) {
		error = EINVAL;
		goto out;
	}
	vp = (struct vnode *)fp->f_data;
	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0) {
		FILE_UNUSE(fp, p);
		return (error);
	}
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, fp->f_cred, p);
	}
	VOP_UNLOCK(vp, 0);
	vn_finished_write(mp, 0);
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Sync an open file.
 */
/* ARGSUSED */
int
sys_fsync(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fsync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0) {
		FILE_UNUSE(fp, p);
		return (error);
	}
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT, 0, 0, p);
	if (error == 0 && bioops.io_fsync != NULL &&
	    vp->v_mount && (vp->v_mount->mnt_flag & MNT_SOFTDEP))
		(*bioops.io_fsync)(vp, 0);
	VOP_UNLOCK(vp, 0);
	vn_finished_write(mp, 0);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Sync a range of file data.  API modeled after that found in AIX.
 *
 * FDATASYNC indicates that we need only save enough metadata to be able
 * to re-read the written data.  Note we duplicate AIX's requirement that
 * the file be open for writing.
 */
/* ARGSUSED */
int
sys_fsync_range(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fsync_range_args /* {
		syscallarg(int) fd;
		syscallarg(int) flags;
		syscallarg(off_t) start;
		syscallarg(int) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	int flags, nflags;
	off_t s, e, len;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	if ((fp->f_flag & FWRITE) == 0) {
		FILE_UNUSE(fp, p);
		return (EBADF);
	}

	flags = SCARG(uap, flags);
	if (((flags & (FDATASYNC | FFILESYNC)) == 0) ||
	    ((~flags & (FDATASYNC | FFILESYNC)) == 0)) {
		return (EINVAL);
	}
	/* Now set up the flags for value(s) to pass to VOP_FSYNC() */
	if (flags & FDATASYNC)
		nflags = FSYNC_DATAONLY | FSYNC_WAIT;
	else
		nflags = FSYNC_WAIT;
	if (flags & FDISKSYNC)
		nflags |= FSYNC_CACHE;

	len = SCARG(uap, length);
	/* If length == 0, we do the whole file, and s = l = 0 will do that */
	if (len) {
		s = SCARG(uap, start);
		e = s + len;
		if (e < s) {
			FILE_UNUSE(fp, p);
			return (EINVAL);
		}
	} else {
		e = 0;
		s = 0;
	}

	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, nflags, s, e, p);

	if (error == 0 && bioops.io_fsync != NULL &&
	    vp->v_mount && (vp->v_mount->mnt_flag & MNT_SOFTDEP))
		(*bioops.io_fsync)(vp, nflags);

	VOP_UNLOCK(vp, 0);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Sync the data of an open file.
 */
/* ARGSUSED */
int
sys_fdatasync(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fdatasync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FWRITE) == 0) {
		FILE_UNUSE(fp, p);
		return (EBADF);
	}
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT|FSYNC_DATAONLY, 0, 0, p);
	VOP_UNLOCK(vp, 0);
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Rename files, (standard) BSD semantics frontend.
 */
/* ARGSUSED */
int
sys_rename(struct lwp *l, void *v, register_t *retval)
{
	struct sys_rename_args /* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (rename_files(SCARG(uap, from), SCARG(uap, to), p, 0));
}

/*
 * Rename files, POSIX semantics frontend.
 */
/* ARGSUSED */
int
sys___posix_rename(struct lwp *l, void *v, register_t *retval)
{
	struct sys___posix_rename_args /* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (rename_files(SCARG(uap, from), SCARG(uap, to), p, 1));
}

/*
 * Rename files.  Source and destination must either both be directories,
 * or both not be directories.  If target is a directory, it must be empty.
 * If `from' and `to' refer to the same object, the value of the `retain'
 * argument is used to determine whether `from' will be
 *
 * (retain == 0)	deleted unless `from' and `to' refer to the same
 *			object in the file system's name space (BSD).
 * (retain == 1)	always retained (POSIX).
 */
static int
rename_files(const char *from, const char *to, struct proc *p, int retain)
{
	struct mount *mp = NULL;
	struct vnode *tvp, *fvp, *tdvp;
	struct nameidata fromnd, tond;
	int error;

	NDINIT(&fromnd, DELETE, WANTPARENT | SAVESTART, UIO_USERSPACE,
	    from, p);
	if ((error = namei(&fromnd)) != 0)
		return (error);
	fvp = fromnd.ni_vp;
	error = vn_start_write(fvp, &mp, V_WAIT | V_PCATCH);
	if (error != 0) {
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
		if (fromnd.ni_startdir)
			vrele(fromnd.ni_startdir);
		PNBUF_PUT(fromnd.ni_cnd.cn_pnbuf);
		return (error);
	}
	NDINIT(&tond, RENAME, LOCKPARENT | LOCKLEAF | NOCACHE | SAVESTART |
	    (fvp->v_type == VDIR ? CREATEDIR : 0), UIO_USERSPACE, to, p);
	if ((error = namei(&tond)) != 0) {
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
		goto out1;
	}
	tdvp = tond.ni_dvp;
	tvp = tond.ni_vp;

	if (tvp != NULL) {
		if (fvp->v_type == VDIR && tvp->v_type != VDIR) {
			error = ENOTDIR;
			goto out;
		} else if (fvp->v_type != VDIR && tvp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
	}

	if (fvp == tdvp)
		error = EINVAL;

	/*
	 * Source and destination refer to the same object.
	 */
	if (fvp == tvp) {
		if (retain)
			error = -1;
		else if (fromnd.ni_dvp == tdvp &&
		    fromnd.ni_cnd.cn_namelen == tond.ni_cnd.cn_namelen &&
		    !memcmp(fromnd.ni_cnd.cn_nameptr,
		          tond.ni_cnd.cn_nameptr,
		          fromnd.ni_cnd.cn_namelen))
		error = -1;
	}

out:
	if (!error) {
		VOP_LEASE(tdvp, p, p->p_ucred, LEASE_WRITE);
		if (fromnd.ni_dvp != tdvp)
			VOP_LEASE(fromnd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
		if (tvp) {
			VOP_LEASE(tvp, p, p->p_ucred, LEASE_WRITE);
		}
		error = VOP_RENAME(fromnd.ni_dvp, fromnd.ni_vp, &fromnd.ni_cnd,
				   tond.ni_dvp, tond.ni_vp, &tond.ni_cnd);
	} else {
		VOP_ABORTOP(tond.ni_dvp, &tond.ni_cnd);
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
	}
	vrele(tond.ni_startdir);
	PNBUF_PUT(tond.ni_cnd.cn_pnbuf);
out1:
	vn_finished_write(mp, 0);
	if (fromnd.ni_startdir)
		vrele(fromnd.ni_startdir);
	PNBUF_PUT(fromnd.ni_cnd.cn_pnbuf);
	return (error == -1 ? 0 : error);
}

/*
 * Make a directory file.
 */
/* ARGSUSED */
int
sys_mkdir(struct lwp *l, void *v, register_t *retval)
{
	struct sys_mkdir_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

restart:
	NDINIT(&nd, CREATE, LOCKPARENT | CREATEDIR, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(vp);
		return (EEXIST);
	}
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode =
	    (SCARG(uap, mode) & ACCESSPERMS) &~ p->p_cwdi->cwdi_cmask;
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_MKDIR(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (!error)
		vput(nd.ni_vp);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * Remove a directory file.
 */
/* ARGSUSED */
int
sys_rmdir(struct lwp *l, void *v, register_t *retval)
{
	struct sys_rmdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vnode *vp;
	int error;
	struct nameidata nd;

restart:
	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * No rmdir "." please.
	 */
	if (nd.ni_dvp == vp) {
		error = EINVAL;
		goto out;
	}
	/*
	 * The root of a mounted filesystem cannot be deleted.
	 */
	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}
	if (vn_start_write(nd.ni_dvp, &mp, V_NOWAIT) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		if ((error = vn_start_write(NULL, &mp,
		    V_WAIT | V_SLEEPONLY | V_PCATCH)) != 0)
			return (error);
		goto restart;
	}
	VOP_LEASE(nd.ni_dvp, p, p->p_ucred, LEASE_WRITE);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	error = VOP_RMDIR(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	vn_finished_write(mp, 0);
	return (error);

out:
	VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	if (nd.ni_dvp == vp)
		vrele(nd.ni_dvp);
	else
		vput(nd.ni_dvp);
	vput(vp);
	return (error);
}

/*
 * Read a block of directory entries in a file system independent format.
 */
int
sys_getdents(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	int error, done;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	error = vn_readdir(fp, SCARG(uap, buf), UIO_USERSPACE,
			SCARG(uap, count), &done, p, 0, 0);
#ifdef KTRACE
	if (!error && KTRPOINT(p, KTR_GENIO)) {
		struct iovec iov;
		iov.iov_base = SCARG(uap, buf);
		iov.iov_len = done;
		ktrgenio(p, SCARG(uap, fd), UIO_READ, &iov, done, 0);
	}
#endif
	*retval = done;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Set the mode mask for creation of filesystem nodes.
 */
int
sys_umask(struct lwp *l, void *v, register_t *retval)
{
	struct sys_umask_args /* {
		syscallarg(mode_t) newmask;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi;

	cwdi = p->p_cwdi;
	*retval = cwdi->cwdi_cmask;
	cwdi->cwdi_cmask = SCARG(uap, newmask) & ALLPERMS;
	return (0);
}

/*
 * Void all references to file by ripping underlying filesystem
 * away from vnode.
 */
/* ARGSUSED */
int
sys_revoke(struct lwp *l, void *v, register_t *retval)
{
	struct sys_revoke_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
		goto out;
	if (p->p_ucred->cr_uid != vattr.va_uid &&
	    (error = suser(p->p_ucred, &p->p_acflag)) != 0)
		goto out;
	if ((error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH)) != 0)
		goto out;
	if (vp->v_usecount > 1 || (vp->v_flag & (VALIASED | VLAYER)))
		VOP_REVOKE(vp, REVOKEALL);
	vn_finished_write(mp, 0);
out:
	vrele(vp);
	return (error);
}

/*
 * Convert a user file descriptor to a kernel file entry.
 */
int
getvnode(struct filedesc *fdp, int fd, struct file **fpp)
{
	struct vnode *vp;
	struct file *fp;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if (fp->f_type != DTYPE_VNODE) {
		FILE_UNUSE(fp, NULL);
		return (EINVAL);
	}

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type == VBAD) {
		FILE_UNUSE(fp, NULL);
		return (EBADF);
	}

	*fpp = fp;
	return (0);
}
