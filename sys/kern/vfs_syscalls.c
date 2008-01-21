/*	$NetBSD: vfs_syscalls.c,v 1.223.2.7 2008/01/21 09:46:33 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls.c,v 1.223.2.7 2008/01/21 09:46:33 yamt Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_fileassoc.h"
#include "fss.h"
#include "veriexec.h"

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
#include <sys/kmem.h>
#include <sys/dirent.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>
#include <sys/ktrace.h>
#ifdef FILEASSOC
#include <sys/fileassoc.h>
#endif /* FILEASSOC */
#include <sys/verified_exec.h>
#include <sys/kauth.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#ifdef COMPAT_30
#include "opt_nfsserver.h"
#include <nfs/rpcv2.h>
#endif
#include <nfs/nfsproto.h>
#ifdef COMPAT_30
#include <nfs/nfs.h>
#include <nfs/nfs_var.h>
#endif

#if NFSS > 0
#include <dev/fssvar.h>
#endif

MALLOC_DEFINE(M_MOUNT, "mount", "vfs mount struct");

static int change_dir(struct nameidata *, struct lwp *);
static int change_flags(struct vnode *, u_long, struct lwp *);
static int change_mode(struct vnode *, int, struct lwp *l);
static int change_owner(struct vnode *, uid_t, gid_t, struct lwp *, int);

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

static int
mount_update(struct lwp *l, struct vnode *vp, const char *path, int flags,
    void *data, size_t *data_len)
{
	struct mount *mp;
	int error = 0, saved_flags;

	mp = vp->v_mount;
	saved_flags = mp->mnt_flag;

	/* We can operate only on VV_ROOT nodes. */
	if ((vp->v_vflag & VV_ROOT) == 0) {
		error = EINVAL;
		goto out;
	}

	/*
	 * We only allow the filesystem to be reloaded if it
	 * is currently mounted read-only.
	 */
	if (flags & MNT_RELOAD && !(mp->mnt_flag & MNT_RDONLY)) {
		error = EOPNOTSUPP;	/* Needs translation */
		goto out;
	}

	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_UPDATE, mp, KAUTH_ARG(flags), data);
	if (error)
		goto out;

	if (vfs_busy(mp, LK_NOWAIT, 0)) {
		error = EPERM;
		goto out;
	}

	mp->mnt_flag &= ~MNT_OP_FLAGS;
	mp->mnt_flag |= flags & (MNT_RELOAD | MNT_FORCE | MNT_UPDATE);

	/*
	 * Set the mount level flags.
	 */
	if (flags & MNT_RDONLY)
		mp->mnt_flag |= MNT_RDONLY;
	else if (mp->mnt_flag & MNT_RDONLY)
		mp->mnt_iflag |= IMNT_WANTRDWR;
	mp->mnt_flag &=
	  ~(MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC | MNT_NOCOREDUMP |
	    MNT_NOATIME | MNT_NODEVMTIME | MNT_SYMPERM | MNT_SOFTDEP);
	mp->mnt_flag |= flags &
	   (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC | MNT_NOCOREDUMP |
	    MNT_NOATIME | MNT_NODEVMTIME | MNT_SYMPERM | MNT_SOFTDEP |
	    MNT_IGNORE);

	error = VFS_MOUNT(mp, path, data, data_len);

#if defined(COMPAT_30) && defined(NFSSERVER)
	if (error && data != NULL) {
		int error2;

		/* Update failed; let's try and see if it was an
		 * export request. */
		error2 = nfs_update_exports_30(mp, path, data, l);

		/* Only update error code if the export request was
		 * understood but some problem occurred while
		 * processing it. */
		if (error2 != EJUSTRETURN)
			error = error2;
	}
#endif
	if (mp->mnt_iflag & IMNT_WANTRDWR)
		mp->mnt_flag &= ~MNT_RDONLY;
	if (error)
		mp->mnt_flag = saved_flags;
	mp->mnt_flag &= ~MNT_OP_FLAGS;
	mp->mnt_iflag &= ~IMNT_WANTRDWR;
	if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0) {
		if (mp->mnt_syncer == NULL)
			error = vfs_allocate_syncvnode(mp);
	} else {
		if (mp->mnt_syncer != NULL)
			vfs_deallocate_syncvnode(mp);
	}
	vfs_unbusy(mp);

 out:
	return (error);
}

static int
mount_get_vfsops(const char *fstype, struct vfsops **vfsops)
{
	char fstypename[sizeof(((struct statvfs *)NULL)->f_fstypename)];
	int error;

	/* Copy file-system type from userspace.  */
	error = copyinstr(fstype, fstypename, sizeof(fstypename), NULL);
	if (error) {
#if defined(COMPAT_09) || defined(COMPAT_43)
		/*
		 * Historically, filesystem types were identified by numbers.
		 * If we get an integer for the filesystem type instead of a
		 * string, we check to see if it matches one of the historic
		 * filesystem types.
		 */
		u_long fsindex = (u_long)fstype;
		if (fsindex >= nmountcompatnames ||
		    mountcompatnames[fsindex] == NULL)
			return ENODEV;
		strlcpy(fstypename, mountcompatnames[fsindex],
		    sizeof(fstypename));
#else
		return error;
#endif
	}

#ifdef	COMPAT_10
	/* Accept `ufs' as an alias for `ffs'. */
	if (strcmp(fstypename, "ufs") == 0)
		fstypename[0] = 'f';
#endif

	if ((*vfsops = vfs_getopsbyname(fstypename)) == NULL)
		return ENODEV;
	return 0;
}

static int
mount_domount(struct lwp *l, struct vnode **vpp, struct vfsops *vfsops,
    const char *path, int flags, void *data, size_t *data_len, u_int recurse)
{
	struct mount *mp = NULL;
	struct vnode *vp = *vpp;
	struct vattr va;
	int error;

	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_NEW, vp, KAUTH_ARG(flags), data);
	if (error)
		return error;

	/* Can't make a non-dir a mount-point (from here anyway). */
	if (vp->v_type != VDIR)
		return ENOTDIR;

	/*
	 * If the user is not root, ensure that they own the directory
	 * onto which we are attempting to mount.
	 */
	if ((error = VOP_GETATTR(vp, &va, l->l_cred)) != 0 ||
	    (va.va_uid != kauth_cred_geteuid(l->l_cred) &&
	    (error = kauth_authorize_generic(l->l_cred,
	    KAUTH_GENERIC_ISSUSER, NULL)) != 0)) {
		return error;
	}

	if (flags & MNT_EXPORTED)
		return EINVAL;

	if ((error = vinvalbuf(vp, V_SAVE, l->l_cred, l, 0, 0)) != 0)
		return error;

	/*
	 * Check if a file-system is not already mounted on this vnode.
	 */
	if (vp->v_mountedhere != NULL)
		return EBUSY;

	mp = malloc(sizeof(*mp), M_MOUNT, M_WAITOK|M_ZERO);

	mp->mnt_op = vfsops;

	TAILQ_INIT(&mp->mnt_vnodelist);
	lockinit(&mp->mnt_lock, PVFS, "vfslock", 0, 0);
	mutex_init(&mp->mnt_mutex, MUTEX_DEFAULT, IPL_NONE);
	(void)vfs_busy(mp, LK_NOWAIT, 0);

	mp->mnt_vnodecovered = vp;
	mp->mnt_stat.f_owner = kauth_cred_geteuid(l->l_cred);
	mp->mnt_unmounter = NULL;
	mount_initspecific(mp);

	/*
	 * The underlying file system may refuse the mount for
	 * various reasons.  Allow the user to force it to happen.
	 *
	 * Set the mount level flags.
	 */
	mp->mnt_flag = flags &
	   (MNT_FORCE | MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC | MNT_NOCOREDUMP |
	    MNT_NOATIME | MNT_NODEVMTIME | MNT_SYMPERM | MNT_SOFTDEP |
	    MNT_IGNORE | MNT_RDONLY);

	error = VFS_MOUNT(mp, path, data, data_len);
	mp->mnt_flag &= ~MNT_OP_FLAGS;

	/*
	 * Put the new filesystem on the mount list after root.
	 */
	cache_purge(vp);
	if (error != 0) {
		vp->v_mountedhere = NULL;
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		vfs_destroy(mp);
		return error;
	}

	mp->mnt_iflag &= ~IMNT_WANTRDWR;
	vp->v_mountedhere = mp;
	mutex_enter(&mountlist_lock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);
    	vn_restorerecurse(vp, recurse);
	VOP_UNLOCK(vp, 0);
	checkdirs(vp);
	if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
		error = vfs_allocate_syncvnode(mp);
	vfs_unbusy(mp);
	(void) VFS_STATVFS(mp, &mp->mnt_stat);
	error = VFS_START(mp, 0);
	if (error)
		vrele(vp);
	*vpp = NULL;
	return error;
}

static int
mount_getargs(struct lwp *l, struct vnode *vp, const char *path, int flags,
    void *data, size_t *data_len)
{
	struct mount *mp;
	int error;

	/* If MNT_GETARGS is specified, it should be the only flag. */
	if (flags & ~MNT_GETARGS)
		return EINVAL;

	mp = vp->v_mount;

	/* XXX: probably some notion of "can see" here if we want isolation. */ 
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_GET, mp, data, NULL);
	if (error)
		return error;

	if ((vp->v_vflag & VV_ROOT) == 0)
		return EINVAL;

	if (vfs_busy(mp, LK_NOWAIT, 0))
		return EPERM;

	mp->mnt_flag &= ~MNT_OP_FLAGS;
	mp->mnt_flag |= MNT_GETARGS;
	error = VFS_MOUNT(mp, path, data, data_len);
	mp->mnt_flag &= ~MNT_OP_FLAGS;

	vfs_unbusy(mp);
	return (error);
}

#ifdef COMPAT_40
/* ARGSUSED */
int
compat_40_sys_mount(struct lwp *l, const struct compat_40_sys_mount_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) type;
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
	} */
	register_t dummy;

	return do_sys_mount(l, NULL, SCARG(uap, type), SCARG(uap, path),
	    SCARG(uap, flags), SCARG(uap, data), UIO_USERSPACE, 0, &dummy);
}
#endif

int
sys___mount50(struct lwp *l, const struct sys___mount50_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) type;
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
		syscallarg(size_t) data_len;
	} */

	return do_sys_mount(l, NULL, SCARG(uap, type), SCARG(uap, path),
	    SCARG(uap, flags), SCARG(uap, data), UIO_USERSPACE,
	    SCARG(uap, data_len), retval);
}

int
do_sys_mount(struct lwp *l, struct vfsops *vfsops, const char *type,
    const char *path, int flags, void *data, enum uio_seg data_seg,
    size_t data_len, register_t *retval)
{
	struct vnode *vp;
	struct nameidata nd;
	void *data_buf = data;
	u_int recurse;
	int error;

	/*
	 * Get vnode to be covered
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE, path);
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;

	/*
	 * A lookup in VFS_MOUNT might result in an attempt to
	 * lock this vnode again, so make the lock recursive.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	recurse = vn_setrecurse(vp);
  
	if (vfsops == NULL) {
		if (flags & (MNT_GETARGS | MNT_UPDATE))
			vfsops = vp->v_mount->mnt_op;
		else {
			/* 'type' is userspace */
			error = mount_get_vfsops(type, &vfsops);
			if (error != 0)
				goto done;
		}
	}

	if (data != NULL && data_seg == UIO_USERSPACE) {
		if (data_len == 0) {
			/* No length supplied, use default for filesystem */
			data_len = vfsops->vfs_min_mount_data;
			if (data_len > VFS_MAX_MOUNT_DATA) {
				/* maybe a force loaded old LKM */
				error = EINVAL;
				goto done;
			}
#ifdef COMPAT_30
			/* Hopefully a longer buffer won't make copyin() fail */
			if (flags & MNT_UPDATE
			    && data_len < sizeof (struct mnt_export_args30))
				data_len = sizeof (struct mnt_export_args30);
#endif
		}
		data_buf = malloc(data_len, M_TEMP, M_WAITOK);

		/* NFS needs the buffer even for mnt_getargs .... */
		error = copyin(data, data_buf, data_len);
		if (error != 0)
			goto done;
	}

	if (flags & MNT_GETARGS) {
		if (data_len == 0) {
			error = EINVAL;
			goto done;
		}
		error = mount_getargs(l, vp, path, flags, data_buf, &data_len);
		if (error != 0)
			goto done;
		if (data_seg == UIO_USERSPACE)
			error = copyout(data_buf, data, data_len);
		*retval = data_len;
	} else if (flags & MNT_UPDATE) {
		error = mount_update(l, vp, path, flags, data_buf, &data_len);
	} else {
		/* Locking is handled internally in mount_domount(). */
		error = mount_domount(l, &vp, vfsops, path, flags, data_buf,
		    &data_len, recurse);
	}

    done:
    	if (vp != NULL) {
	    	vn_restorerecurse(vp, recurse);
	    	vput(vp);
	}
	if (data_buf != data)
		free(data_buf, M_TEMP);
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
	mutex_enter(&proclist_lock);
	PROCLIST_FOREACH(p, &allproc) {
		cwdi = p->p_cwdi;
		if (!cwdi)
			continue;
		rw_enter(&cwdi->cwdi_lock, RW_WRITER);
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
		rw_exit(&cwdi->cwdi_lock);
	}
	mutex_exit(&proclist_lock);
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
sys_unmount(struct lwp *l, const struct sys_unmount_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */
	struct vnode *vp;
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	mp = vp->v_mount;

	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_UNMOUNT, mp, NULL, NULL);
	if (error) {
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
	if ((vp->v_vflag & VV_ROOT) == 0) {
		vput(vp);
		return (EINVAL);
	}
	vput(vp);

	/*
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 */
	mutex_enter(&syncer_mutex);

	if (vfs_busy(mp, 0, 0)) {
		mutex_exit(&syncer_mutex);
		return (EBUSY);
	}

	return (dounmount(mp, SCARG(uap, flags), l));
}

/*
 * Do the actual file system unmount. File system is assumed to have been
 * marked busy by the caller.
 */
int
dounmount(struct mount *mp, int flags, struct lwp *l)
{
	struct vnode *coveredvp;
	int error;
	int async;
	int used_syncer;

#if NVERIEXEC > 0
	error = veriexec_unmountchk(mp);
	if (error)
		return (error);
#endif /* NVERIEXEC > 0 */

	mutex_enter(&mountlist_lock);
	vfs_unbusy(mp);
	used_syncer = (mp->mnt_syncer != NULL);

	/*
	 * XXX Syncer must be frozen when we get here.  This should really
	 * be done on a per-mountpoint basis, but especially the softdep
	 * code possibly called from the syncer doesn't exactly work on a
	 * per-mountpoint basis, so the softdep code would become a maze
	 * of vfs_busy() calls.
	 *
	 * The caller of dounmount() must acquire syncer_mutex because
	 * the syncer itself acquires locks in syncer_mutex -> vfs_busy
	 * order, and we must preserve that order to avoid deadlock.
	 *
	 * So, if the file system did not use the syncer, now is
	 * the time to release the syncer_mutex.
	 */
	if (used_syncer == 0)
		mutex_exit(&syncer_mutex);

	mp->mnt_iflag |= IMNT_UNMOUNT;
	mp->mnt_unmounter = l;
	lockmgr(&mp->mnt_lock, LK_DRAIN | LK_INTERLOCK, &mountlist_lock);

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
			error = VFS_SYNC(mp, MNT_WAIT, l->l_cred);
	}
	if (error == 0 || (flags & MNT_FORCE))
		error = VFS_UNMOUNT(mp, flags);
	if (error) {
		if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
			(void) vfs_allocate_syncvnode(mp);
		mutex_enter(&mountlist_lock);
		mp->mnt_iflag &= ~IMNT_UNMOUNT;
		mp->mnt_unmounter = NULL;
		mp->mnt_flag |= async;
		lockmgr(&mp->mnt_lock, LK_RELEASE | LK_INTERLOCK | LK_REENABLE,
		    &mountlist_lock);
		if (used_syncer)
			mutex_exit(&syncer_mutex);
		mutex_enter(&mp->mnt_mutex);
		while (mp->mnt_wcnt > 0) {
			wakeup(mp);
			mtsleep(&mp->mnt_wcnt, PVFS, "mntwcnt1",
				0, &mp->mnt_mutex);
		}
		mutex_exit(&mp->mnt_mutex);
		return (error);
	}
	vfs_scrubvnlist(mp);
	mutex_enter(&mountlist_lock);
	CIRCLEQ_REMOVE(&mountlist, mp, mnt_list);
	if ((coveredvp = mp->mnt_vnodecovered) != NULLVP)
		coveredvp->v_mountedhere = NULL;
	if (TAILQ_FIRST(&mp->mnt_vnodelist) != NULL)
		panic("unmount: dangling vnode");
	mp->mnt_iflag |= IMNT_GONE;
	lockmgr(&mp->mnt_lock, LK_RELEASE | LK_INTERLOCK, &mountlist_lock);
	if (coveredvp != NULLVP)
		vrele(coveredvp);
	if (used_syncer)
		mutex_exit(&syncer_mutex);
	mutex_enter(&mp->mnt_mutex);
	while (mp->mnt_wcnt > 0) {
		wakeup(mp);
		mtsleep(&mp->mnt_wcnt, PVFS, "mntwcnt2", 0, &mp->mnt_mutex);
	}
	mutex_exit(&mp->mnt_mutex);
	vfs_hooks_unmount(mp);
	vfs_delref(mp->mnt_op);
	vfs_destroy(mp);
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
sys_sync(struct lwp *l, const void *v, register_t *retval)
{
	struct mount *mp, *nmp;
	int asyncflag;

	if (l == NULL)
		l = &lwp0;

	mutex_enter(&mountlist_lock);
	for (mp = mountlist.cqh_last; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_lock)) {
			nmp = mp->mnt_list.cqe_prev;
			continue;
		}
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			asyncflag = mp->mnt_flag & MNT_ASYNC;
			mp->mnt_flag &= ~MNT_ASYNC;
			VFS_SYNC(mp, MNT_NOWAIT, l->l_cred);
			if (asyncflag)
				 mp->mnt_flag |= MNT_ASYNC;
		}
		mutex_enter(&mountlist_lock);
		nmp = mp->mnt_list.cqe_prev;
		vfs_unbusy(mp);

	}
	mutex_exit(&mountlist_lock);
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
sys_quotactl(struct lwp *l, const struct sys_quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(void *) arg;
	} */
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	error = VFS_QUOTACTL(mp, SCARG(uap, cmd), SCARG(uap, uid),
	    SCARG(uap, arg));
	vrele(nd.ni_vp);
	return (error);
}

int
dostatvfs(struct mount *mp, struct statvfs *sp, struct lwp *l, int flags,
    int root)
{
	struct cwdinfo *cwdi = l->l_proc->p_cwdi;
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
	if ((error = VFS_STATVFS(mp, sp)) != 0) {
		return error;
	}

	if (cwdi->cwdi_rdir == NULL)
		(void)memcpy(&mp->mnt_stat, sp, sizeof(mp->mnt_stat));
done:
	if (cwdi->cwdi_rdir != NULL) {
		size_t len;
		char *bp;
		char *path = PNBUF_GET();

		bp = path + MAXPATHLEN;
		*--bp = '\0';
		rw_enter(&cwdi->cwdi_lock, RW_READER);
		error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp, path,
		    MAXPATHLEN / 2, 0, l);
		rw_exit(&cwdi->cwdi_lock);
		if (error) {
			PNBUF_PUT(path);
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
		PNBUF_PUT(path);
	}
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	return error;
}

/*
 * Get filesystem statistics by path.
 */
int
do_sys_pstatvfs(struct lwp *l, const char *path, int flags, struct statvfs *sb)
{
	struct mount *mp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE, path);
	if ((error = namei(&nd)) != 0)
		return error;
	mp = nd.ni_vp->v_mount;
	error = dostatvfs(mp, sb, l, flags, 1);
	vrele(nd.ni_vp);
	return error;
}

/* ARGSUSED */
int
sys_statvfs1(struct lwp *l, const struct sys_statvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct statvfs *) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG(uap, path), SCARG(uap, flags), sb);
	if (error == 0)
		error = copyout(sb, SCARG(uap, buf), sizeof(*sb));
	STATVFSBUF_PUT(sb);
	return error;
}

/*
 * Get filesystem statistics by fd.
 */
int
do_sys_fstatvfs(struct lwp *l, int fd, int flags, struct statvfs *sb)
{
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mount *mp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, fd, &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	error = dostatvfs(mp, sb, l, flags, 1);
	FILE_UNUSE(fp, l);
	return error;
}

/* ARGSUSED */
int
sys_fstatvfs1(struct lwp *l, const struct sys_fstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct statvfs *) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), SCARG(uap, flags), sb);
	if (error == 0)
		error = copyout(sb, SCARG(uap, buf), sizeof(*sb));
	STATVFSBUF_PUT(sb);
	return error;
}


/*
 * Get statistics on all filesystems.
 */
int
do_sys_getvfsstat(struct lwp *l, void *sfsp, size_t bufsize, int flags,
    int (*copyfn)(const void *, void *, size_t), size_t entry_sz,
    register_t *retval)
{
	int root = 0;
	struct proc *p = l->l_proc;
	struct mount *mp, *nmp;
	struct statvfs *sb;
	size_t count, maxcount;
	int error = 0;

	sb = STATVFSBUF_GET();
	maxcount = bufsize / entry_sz;
	mutex_enter(&mountlist_lock);
	count = 0;
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_lock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		if (sfsp && count < maxcount) {
			error = dostatvfs(mp, sb, l, flags, 0);
			if (error) {
				mutex_enter(&mountlist_lock);
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				vfs_unbusy(mp);
				continue;
			}
			error = copyfn(sb, sfsp, entry_sz);
			if (error) {
				vfs_unbusy(mp);
				goto out;
			}
			sfsp = (char *)sfsp + entry_sz;
			root |= strcmp(sb->f_mntonname, "/") == 0;
		}
		count++;
		mutex_enter(&mountlist_lock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}

	mutex_exit(&mountlist_lock);
	if (root == 0 && p->p_cwdi->cwdi_rdir) {
		/*
		 * fake a root entry
		 */
		error = dostatvfs(p->p_cwdi->cwdi_rdir->v_mount,
		    sb, l, flags, 1);
		if (error != 0)
			goto out;
		if (sfsp)
			error = copyfn(sb, sfsp, entry_sz);
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
sys_getvfsstat(struct lwp *l, const struct sys_getvfsstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct statvfs *) buf;
		syscallarg(size_t) bufsize;
		syscallarg(int) flags;
	} */

	return do_sys_getvfsstat(l, SCARG(uap, buf), SCARG(uap, bufsize),
	    SCARG(uap, flags), copyout, sizeof (struct statvfs), retval);
}

/*
 * Change current working directory to a given file descriptor.
 */
/* ARGSUSED */
int
sys_fchdir(struct lwp *l, const struct sys_fchdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct cwdinfo *cwdi;
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
		error = VOP_ACCESS(vp, VEXEC, l->l_cred);
	if (error) {
		vput(vp);
		goto out;
	}
	while ((mp = vp->v_mountedhere) != NULL) {
		if (vfs_busy(mp, 0, 0))
			continue;

		vput(vp);
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp);
		if (error)
			goto out;
		vp = tdp;
	}
	VOP_UNLOCK(vp, 0);

	/*
	 * Disallow changing to a directory not under the process's
	 * current root directory (if there is one).
	 */
	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	if (cwdi->cwdi_rdir && !vn_isunder(vp, NULL, l)) {
		vrele(vp);
		error = EPERM;	/* operation not permitted */
	} else {
		vrele(cwdi->cwdi_cdir);
		cwdi->cwdi_cdir = vp;
	}
	rw_exit(&cwdi->cwdi_lock);

 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Change this process's notion of the root directory to a given file
 * descriptor.
 */
int
sys_fchroot(struct lwp *l, const struct sys_fchroot_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct cwdinfo *cwdi;
	struct vnode	*vp;
	struct file	*fp;
	int		 error;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_CHROOT,
 	    KAUTH_REQ_SYSTEM_CHROOT_FCHROOT, NULL, NULL, NULL)) != 0)
		return error;
	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(fdp, SCARG(uap, fd), &fp)) != 0)
		return error;
	vp = (struct vnode *) fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, l->l_cred);
	VOP_UNLOCK(vp, 0);
	if (error)
		goto out;
	VREF(vp);

	/*
	 * Prevent escaping from chroot by putting the root under
	 * the working directory.  Silently chdir to / if we aren't
	 * already there.
	 */
	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	if (!vn_isunder(cwdi->cwdi_cdir, vp, l)) {
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
	rw_exit(&cwdi->cwdi_lock);

 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Change current working directory (``.'').
 */
/* ARGSUSED */
int
sys_chdir(struct lwp *l, const struct sys_chdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = change_dir(&nd, l)) != 0)
		return (error);
	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	vrele(cwdi->cwdi_cdir);
	cwdi->cwdi_cdir = nd.ni_vp;
	rw_exit(&cwdi->cwdi_lock);
	return (0);
}

/*
 * Change notion of root (``/'') directory.
 */
/* ARGSUSED */
int
sys_chroot(struct lwp *l, const struct sys_chroot_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi;
	struct vnode *vp;
	int error;
	struct nameidata nd;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_CHROOT,
	    KAUTH_REQ_SYSTEM_CHROOT_CHROOT, NULL, NULL, NULL)) != 0)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = change_dir(&nd, l)) != 0)
		return (error);

	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	if (cwdi->cwdi_rdir != NULL)
		vrele(cwdi->cwdi_rdir);
	vp = nd.ni_vp;
	cwdi->cwdi_rdir = vp;

	/*
	 * Prevent escaping from chroot by putting the root under
	 * the working directory.  Silently chdir to / if we aren't
	 * already there.
	 */
	if (!vn_isunder(cwdi->cwdi_cdir, vp, l)) {
		/*
		 * XXX would be more failsafe to change directory to a
		 * deadfs node here instead
		 */
		vrele(cwdi->cwdi_cdir);
		VREF(vp);
		cwdi->cwdi_cdir = vp;
	}
	rw_exit(&cwdi->cwdi_lock);

	return (0);
}

/*
 * Common routine for chroot and chdir.
 */
static int
change_dir(struct nameidata *ndp, struct lwp *l)
{
	struct vnode *vp;
	int error;

	if ((error = namei(ndp)) != 0)
		return (error);
	vp = ndp->ni_vp;
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, l->l_cred);

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
sys_open(struct lwp *l, const struct sys_open_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */
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
	if ((error = falloc(l, &fp, &indx)) != 0)
		return (error);
	/* We're going to read cwdi->cwdi_cmask unlocked here. */
	cmode = ((SCARG(uap, mode) &~ cwdi->cwdi_cmask) & ALLPERMS) &~ S_ISTXT;
	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	l->l_dupfd = -indx - 1;			/* XXX check for fdopen */
	if ((error = vn_open(&nd, flags, cmode)) != 0) {
		rw_enter(&fdp->fd_lock, RW_WRITER);
		FILE_UNUSE(fp, l);
		fdp->fd_ofiles[indx] = NULL;
		rw_exit(&fdp->fd_lock);
		ffree(fp);
		if ((error == EDUPFD || error == EMOVEFD) &&
		    l->l_dupfd >= 0 &&			/* XXX from fdopen */
		    (error =
			dupfdopen(l, indx, l->l_dupfd, flags, error)) == 0) {
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
			(void) vn_close(vp, fp->f_flag, fp->f_cred, l);
			FILE_UNUSE(fp, l);
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
	FILE_UNUSE(fp, l);
	return (0);
}

static void
vfs__fhfree(fhandle_t *fhp)
{
	size_t fhsize;

	if (fhp == NULL) {
		return;
	}
	fhsize = FHANDLE_SIZE(fhp);
	kmem_free(fhp, fhsize);
}

/*
 * vfs_composefh: compose a filehandle.
 */

int
vfs_composefh(struct vnode *vp, fhandle_t *fhp, size_t *fh_size)
{
	struct mount *mp;
	struct fid *fidp;
	int error;
	size_t needfhsize;
	size_t fidsize;

	mp = vp->v_mount;
	fidp = NULL;
	if (*fh_size < FHANDLE_SIZE_MIN) {
		fidsize = 0;
	} else {
		fidsize = *fh_size - offsetof(fhandle_t, fh_fid);
		if (fhp != NULL) {
			memset(fhp, 0, *fh_size);
			fhp->fh_fsid = mp->mnt_stat.f_fsidx;
			fidp = &fhp->fh_fid;
		}
	}
	error = VFS_VPTOFH(vp, fidp, &fidsize);
	needfhsize = FHANDLE_SIZE_FROM_FILEID_SIZE(fidsize);
	if (error == 0 && *fh_size < needfhsize) {
		error = E2BIG;
	}
	*fh_size = needfhsize;
	return error;
}

int
vfs_composefh_alloc(struct vnode *vp, fhandle_t **fhpp)
{
	struct mount *mp;
	fhandle_t *fhp;
	size_t fhsize;
	size_t fidsize;
	int error;

	*fhpp = NULL;
	mp = vp->v_mount;
	fidsize = 0;
	error = VFS_VPTOFH(vp, NULL, &fidsize);
	KASSERT(error != 0);
	if (error != E2BIG) {
		goto out;
	}
	fhsize = FHANDLE_SIZE_FROM_FILEID_SIZE(fidsize);
	fhp = kmem_zalloc(fhsize, KM_SLEEP);
	if (fhp == NULL) {
		error = ENOMEM;
		goto out;
	}
	fhp->fh_fsid = mp->mnt_stat.f_fsidx;
	error = VFS_VPTOFH(vp, &fhp->fh_fid, &fidsize);
	if (error == 0) {
		KASSERT((FHANDLE_SIZE(fhp) == fhsize &&
		    FHANDLE_FILEID(fhp)->fid_len == fidsize));
		*fhpp = fhp;
	} else {
		kmem_free(fhp, fhsize);
	}
out:
	return error;
}

void
vfs_composefh_free(fhandle_t *fhp)
{

	vfs__fhfree(fhp);
}

/*
 * vfs_fhtovp: lookup a vnode by a filehandle.
 */

int
vfs_fhtovp(fhandle_t *fhp, struct vnode **vpp)
{
	struct mount *mp;
	int error;

	*vpp = NULL;
	mp = vfs_getvfs(FHANDLE_FSID(fhp));
	if (mp == NULL) {
		error = ESTALE;
		goto out;
	}
	if (mp->mnt_op->vfs_fhtovp == NULL) {
		error = EOPNOTSUPP;
		goto out;
	}
	error = VFS_FHTOVP(mp, FHANDLE_FILEID(fhp), vpp);
out:
	return error;
}

/*
 * vfs_copyinfh_alloc: allocate and copyin a filehandle, given
 * the needed size.
 */

int
vfs_copyinfh_alloc(const void *ufhp, size_t fhsize, fhandle_t **fhpp)
{
	fhandle_t *fhp;
	int error;

	*fhpp = NULL;
	if (fhsize > FHANDLE_SIZE_MAX) {
		return EINVAL;
	}
	if (fhsize < FHANDLE_SIZE_MIN) {
		return EINVAL;
	}
again:
	fhp = kmem_alloc(fhsize, KM_SLEEP);
	if (fhp == NULL) {
		return ENOMEM;
	}
	error = copyin(ufhp, fhp, fhsize);
	if (error == 0) {
		/* XXX this check shouldn't be here */
		if (FHANDLE_SIZE(fhp) == fhsize) {
			*fhpp = fhp;
			return 0;
		} else if (fhsize == NFSX_V2FH && FHANDLE_SIZE(fhp) < fhsize) {
			/*
			 * a kludge for nfsv2 padded handles.
			 */
			size_t sz;

			sz = FHANDLE_SIZE(fhp);
			kmem_free(fhp, fhsize);
			fhsize = sz;
			goto again;
		} else {
			/*
			 * userland told us wrong size.
			 */
		    	error = EINVAL;
		}
	}
	kmem_free(fhp, fhsize);
	return error;
}

void
vfs_copyinfh_free(fhandle_t *fhp)
{

	vfs__fhfree(fhp);
}

/*
 * Get file handle system call
 */
int
sys___getfh30(struct lwp *l, const struct sys___getfh30_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) fname;
		syscallarg(fhandle_t *) fhp;
		syscallarg(size_t *) fh_size;
	} */
	struct vnode *vp;
	fhandle_t *fh;
	int error;
	struct nameidata nd;
	size_t sz;
	size_t usz;

	/*
	 * Must be super user
	 */
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL);
	if (error)
		return (error);
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, fname));
	error = namei(&nd);
	if (error)
		return (error);
	vp = nd.ni_vp;
	error = vfs_composefh_alloc(vp, &fh);
	vput(vp);
	if (error != 0) {
		goto out;
	}
	error = copyin(SCARG(uap, fh_size), &usz, sizeof(size_t));
	if (error != 0) {
		goto out;
	}
	sz = FHANDLE_SIZE(fh);
	error = copyout(&sz, SCARG(uap, fh_size), sizeof(size_t));
	if (error != 0) {
		goto out;
	}
	if (usz >= sz) {
		error = copyout(fh, SCARG(uap, fhp), sz);
	} else {
		error = E2BIG;
	}
out:
	vfs_composefh_free(fh);
	return (error);
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */

int
dofhopen(struct lwp *l, const void *ufhp, size_t fhsize, int oflags,
    register_t *retval)
{
	struct filedesc *fdp = l->l_proc->p_fd;
	struct file *fp;
	struct vnode *vp = NULL;
	kauth_cred_t cred = l->l_cred;
	struct file *nfp;
	int type, indx, error=0;
	struct flock lf;
	struct vattr va;
	fhandle_t *fh;
	int flags;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL)))
		return (error);

	flags = FFLAGS(oflags);
	if ((flags & (FREAD | FWRITE)) == 0)
		return (EINVAL);
	if ((flags & O_CREAT))
		return (EINVAL);
	/* falloc() will use the file descriptor for us */
	if ((error = falloc(l, &nfp, &indx)) != 0)
		return (error);
	fp = nfp;
	error = vfs_copyinfh_alloc(ufhp, fhsize, &fh);
	if (error != 0) {
		goto bad;
	}
	error = vfs_fhtovp(fh, &vp);
	if (error != 0) {
		goto bad;
	}

	/* Now do an effective vn_open */

	if (vp->v_type == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	error = vn_openchk(vp, cred, flags);
	if (error != 0)
		goto bad;
	if (flags & O_TRUNC) {
		VOP_UNLOCK(vp, 0);			/* XXX */
		VOP_LEASE(vp, cred, LEASE_WRITE);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);   /* XXX */
		VATTR_NULL(&va);
		va.va_size = 0;
		error = VOP_SETATTR(vp, &va, cred);
		if (error)
			goto bad;
	}
	if ((error = VOP_OPEN(vp, flags, cred)) != 0)
		goto bad;
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
			(void) vn_close(vp, fp->f_flag, fp->f_cred, l);
			FILE_UNUSE(fp, l);
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
	FILE_UNUSE(fp, l);
	vfs_copyinfh_free(fh);
	return (0);

bad:
	FILE_UNUSE(fp, l);
	ffree(fp);
	fdremove(fdp, indx);
	if (vp != NULL)
		vput(vp);
	vfs_copyinfh_free(fh);
	return (error);
}

int
sys___fhopen40(struct lwp *l, const struct sys___fhopen40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) fhp;
		syscallarg(size_t) fh_size;
		syscallarg(int) flags;
	} */

	return dofhopen(l, SCARG(uap, fhp), SCARG(uap, fh_size),
	    SCARG(uap, flags), retval);
}

int
do_fhstat(struct lwp *l, const void *ufhp, size_t fhsize, struct stat *sb)
{
	int error;
	fhandle_t *fh;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL)))
		return (error);

	error = vfs_copyinfh_alloc(ufhp, fhsize, &fh);
	if (error != 0)
		return error;

	error = vfs_fhtovp(fh, &vp);
	vfs_copyinfh_free(fh);
	if (error != 0)
		return error;

	error = vn_stat(vp, sb, l);
	vput(vp);
	return error;
}


/* ARGSUSED */
int
sys___fhstat40(struct lwp *l, const struct sys___fhstat40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) fhp;
		syscallarg(size_t) fh_size;
		syscallarg(struct stat *) sb;
	} */
	struct stat sb;
	int error;

	error = do_fhstat(l, SCARG(uap, fhp), SCARG(uap, fh_size), &sb);
	if (error)
		return error;
	return copyout(&sb, SCARG(uap, sb), sizeof(sb));
}

int
do_fhstatvfs(struct lwp *l, const void *ufhp, size_t fhsize, struct statvfs *sb,
    int flags)
{
	fhandle_t *fh;
	struct mount *mp;
	struct vnode *vp;
	int error;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL)))
		return error;

	error = vfs_copyinfh_alloc(ufhp, fhsize, &fh);
	if (error != 0)
		return error;

	error = vfs_fhtovp(fh, &vp);
	vfs_copyinfh_free(fh);
	if (error != 0)
		return error;

	mp = vp->v_mount;
	error = dostatvfs(mp, sb, l, flags, 1);
	vput(vp);
	return error;
}

/* ARGSUSED */
int
sys___fhstatvfs140(struct lwp *l, const struct sys___fhstatvfs140_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) fhp;
		syscallarg(size_t) fh_size;
		syscallarg(struct statvfs *) buf;
		syscallarg(int)	flags;
	} */
	struct statvfs *sb = STATVFSBUF_GET();
	int error;

	error = do_fhstatvfs(l, SCARG(uap, fhp), SCARG(uap, fh_size), sb,
	    SCARG(uap, flags));
	if (error == 0)
		error = copyout(sb, SCARG(uap, buf), sizeof(*sb));
	STATVFSBUF_PUT(sb);
	return error;
}

/*
 * Create a special file.
 */
/* ARGSUSED */
int
sys_mknod(struct lwp *l, const struct sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct vattr vattr;
	int error, optype;
	struct nameidata nd;
	char *path;
	const char *cpath;
	enum uio_seg seg = UIO_USERSPACE;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MKNOD,
	    0, NULL, NULL, NULL)) != 0)
		return (error);

	optype = VOP_MKNOD_DESCOFFSET;

	VERIEXEC_PATH_GET(SCARG(uap, path), seg, cpath, path);
	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, seg, cpath);

	if ((error = namei(&nd)) != 0)
		goto out;
	vp = nd.ni_vp;
	if (vp != NULL)
		error = EEXIST;
	else {
		VATTR_NULL(&vattr);
		/* We will read cwdi->cwdi_cmask unlocked. */
		vattr.va_mode =
		    (SCARG(uap, mode) & ALLPERMS) &~ p->p_cwdi->cwdi_cmask;
		vattr.va_rdev = SCARG(uap, dev);

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
			optype = VOP_WHITEOUT_DESCOFFSET;
			break;
		case S_IFREG:
#if NVERIEXEC > 0
			error = veriexec_openchk(l, nd.ni_vp, nd.ni_dirp,
			    O_CREAT);
#endif /* NVERIEXEC > 0 */
			vattr.va_type = VREG;
			vattr.va_rdev = VNOVAL;
			optype = VOP_CREATE_DESCOFFSET;
			break;
		default:
			error = EINVAL;
			break;
		}
	}
	if (!error) {
		VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
		switch (optype) {
		case VOP_WHITEOUT_DESCOFFSET:
			error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, CREATE);
			if (error)
				VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
			vput(nd.ni_dvp);
			break;

		case VOP_MKNOD_DESCOFFSET:
			error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp,
						&nd.ni_cnd, &vattr);
			if (error == 0)
				vput(nd.ni_vp);
			break;

		case VOP_CREATE_DESCOFFSET:
			error = VOP_CREATE(nd.ni_dvp, &nd.ni_vp,
						&nd.ni_cnd, &vattr);
			if (error == 0)
				vput(nd.ni_vp);
			break;
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
out:
	VERIEXEC_PATH_PUT(path);
	return (error);
}

/*
 * Create a named pipe.
 */
/* ARGSUSED */
int
sys_mkfifo(struct lwp *l, const struct sys_mkfifo_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */
	struct proc *p = l->l_proc;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
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
	VATTR_NULL(&vattr);
	vattr.va_type = VFIFO;
	/* We will read cwdi->cwdi_cmask unlocked. */
	vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_cwdi->cwdi_cmask;
	VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
	error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (error == 0)
		vput(nd.ni_vp);
	return (error);
}

/*
 * Make a hard file link.
 */
/* ARGSUSED */
int
sys_link(struct lwp *l, const struct sys_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */
	struct vnode *vp;
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, link));
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
	VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	error = VOP_LINK(nd.ni_dvp, vp, &nd.ni_cnd);
out:
	vrele(vp);
	return (error);
}

/*
 * Make a symbolic link.
 */
/* ARGSUSED */
int
sys_symlink(struct lwp *l, const struct sys_symlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */
	struct proc *p = l->l_proc;
	struct vattr vattr;
	char *path;
	int error;
	struct nameidata nd;

	path = PNBUF_GET();
	error = copyinstr(SCARG(uap, path), path, MAXPATHLEN, NULL);
	if (error)
		goto out;
	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, link));
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
	VATTR_NULL(&vattr);
	vattr.va_type = VLNK;
	/* We will read cwdi->cwdi_cmask unlocked. */
	vattr.va_mode = ACCESSPERMS &~ p->p_cwdi->cwdi_cmask;
	VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
	error = VOP_SYMLINK(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr, path);
	if (error == 0)
		vput(nd.ni_vp);
out:
	PNBUF_PUT(path);
	return (error);
}

/*
 * Delete a whiteout from the filesystem.
 */
/* ARGSUSED */
int
sys_undelete(struct lwp *l, const struct sys_undelete_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, DELETE, LOCKPARENT | DOWHITEOUT | TRYEMULROOT,
	    UIO_USERSPACE, SCARG(uap, path));
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
	VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
	if ((error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, DELETE)) != 0)
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	vput(nd.ni_dvp);
	return (error);
}

/*
 * Delete a name from the filesystem.
 */
/* ARGSUSED */
int
sys_unlink(struct lwp *l, const struct sys_unlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */

	return do_sys_unlink(SCARG(uap, path), UIO_USERSPACE);
}

int
do_sys_unlink(const char *arg, enum uio_seg seg)
{
	struct vnode *vp;
	int error;
	struct nameidata nd;
	kauth_cred_t cred;
	char *path;
	const char *cpath;

	VERIEXEC_PATH_GET(arg, seg, cpath, path);
	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF | TRYEMULROOT, seg, cpath);

	if ((error = namei(&nd)) != 0)
		goto out;
	vp = nd.ni_vp;

	/*
	 * The root of a mounted filesystem cannot be deleted.
	 */
	if (vp->v_vflag & VV_ROOT) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		error = EBUSY;
		goto out;
	}

#if NVERIEXEC > 0
	/* Handle remove requests for veriexec entries. */
	if ((error = veriexec_removechk(curlwp, nd.ni_vp, nd.ni_dirp)) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		goto out;
	}
#endif /* NVERIEXEC > 0 */
	
	cred = kauth_cred_get();
	VOP_LEASE(nd.ni_dvp, cred, LEASE_WRITE);
	VOP_LEASE(vp, cred, LEASE_WRITE);
#ifdef FILEASSOC
	(void)fileassoc_file_delete(vp);
#endif /* FILEASSOC */
	error = VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
out:
	VERIEXEC_PATH_PUT(path);
	return (error);
}

/*
 * Reposition read/write file offset.
 */
int
sys_lseek(struct lwp *l, const struct sys_lseek_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} */
	struct proc *p = l->l_proc;
	kauth_cred_t cred = l->l_cred;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	struct vattr vattr;
	off_t newoff;
	int error;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		FILE_UNLOCK(fp);
		goto out;
	}

	switch (SCARG(uap, whence)) {
	case SEEK_CUR:
		newoff = fp->f_offset + SCARG(uap, offset);
		FILE_USE(fp);
		break;
	case SEEK_END:
		FILE_USE(fp);
		error = VOP_GETATTR(vp, &vattr, cred);
		if (error) {
			FILE_UNUSE(fp, l);
			goto out;
		}
		newoff = SCARG(uap, offset) + vattr.va_size;
		break;
	case SEEK_SET:
		FILE_USE(fp);
		newoff = SCARG(uap, offset);
		break;
	default:
		FILE_UNLOCK(fp);
		error = EINVAL;
		goto out;
	}
	if ((error = VOP_SEEK(vp, fp->f_offset, newoff, cred)) == 0) {
		FILE_LOCK(fp);
		*(off_t *)retval = fp->f_offset = newoff;
		FILE_UNLOCK(fp);
	}
	FILE_UNUSE(fp, l);
 out:
	return (error);
}

/*
 * Positional read system call.
 */
int
sys_pread(struct lwp *l, const struct sys_pread_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		FILE_UNLOCK(fp);
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
	return (dofileread(fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &offset, 0, retval));

 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Positional scatter read system call.
 */
int
sys_preadv(struct lwp *l, const struct sys_preadv_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(off_t) offset;
	} */
	off_t offset = SCARG(uap, offset);

	return do_filereadv(SCARG(uap, fd), SCARG(uap, iovp),
	    SCARG(uap, iovcnt), &offset, 0, retval);
}

/*
 * Positional write system call.
 */
int
sys_pwrite(struct lwp *l, const struct sys_pwrite_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} */
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		FILE_UNLOCK(fp);
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
	return (dofilewrite(fd, fp, SCARG(uap, buf), SCARG(uap, nbyte),
	    &offset, 0, retval));

 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Positional gather write system call.
 */
int
sys_pwritev(struct lwp *l, const struct sys_pwritev_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct iovec *) iovp;
		syscallarg(int) iovcnt;
		syscallarg(off_t) offset;
	} */
	off_t offset = SCARG(uap, offset);

	return do_filewritev(SCARG(uap, fd), SCARG(uap, iovp),
	    SCARG(uap, iovcnt), &offset, 0, retval);
}

/*
 * Check access permissions.
 */
int
sys_access(struct lwp *l, const struct sys_access_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */
	kauth_cred_t cred;
	struct vnode *vp;
	int error, flags;
	struct nameidata nd;

	cred = kauth_cred_dup(l->l_cred);
	kauth_cred_seteuid(cred, kauth_cred_getuid(l->l_cred));
	kauth_cred_setegid(cred, kauth_cred_getgid(l->l_cred));
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
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

		error = VOP_ACCESS(vp, flags, cred);
		if (!error && (flags & VWRITE))
			error = vn_writechk(vp);
	}
	vput(vp);
out:
	kauth_cred_free(cred);
	return (error);
}

/*
 * Common code for all sys_stat functions, including compat versions.
 */
int
do_sys_stat(struct lwp *l, const char *path, unsigned int nd_flags,
    struct stat *sb)
{
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, nd_flags | LOCKLEAF | TRYEMULROOT,
	    UIO_USERSPACE, path);
	error = namei(&nd);
	if (error != 0)
		return error;
	error = vn_stat(nd.ni_vp, sb, l);
	vput(nd.ni_vp);
	return error;
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
sys___stat30(struct lwp *l, const struct sys___stat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;
	return copyout(&sb, SCARG(uap, ub), sizeof(sb));
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
sys___lstat30(struct lwp *l, const struct sys___lstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;
	return copyout(&sb, SCARG(uap, ub), sizeof(sb));
}

/*
 * Get configurable pathname variables.
 */
/* ARGSUSED */
int
sys_pathconf(struct lwp *l, const struct sys_pathconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) name;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
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
sys_readlink(struct lwp *l, const struct sys_readlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */
	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	if (vp->v_type != VLNK)
		error = EINVAL;
	else if (!(vp->v_mount->mnt_flag & MNT_SYMPERM) ||
	    (error = VOP_ACCESS(vp, VREAD, l->l_cred)) == 0) {
		aiov.iov_base = SCARG(uap, buf);
		aiov.iov_len = SCARG(uap, count);
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		KASSERT(l == curlwp);
		auio.uio_vmspace = l->l_proc->p_vmspace;
		auio.uio_resid = SCARG(uap, count);
		error = VOP_READLINK(vp, &auio, l->l_cred);
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
sys_chflags(struct lwp *l, const struct sys_chflags_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(u_long) flags;
	} */
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	error = change_flags(vp, SCARG(uap, flags), l);
	vput(vp);
	return (error);
}

/*
 * Change flags of a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchflags(struct lwp *l, const struct sys_fchflags_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) flags;
	} */
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	error = change_flags(vp, SCARG(uap, flags), l);
	VOP_UNLOCK(vp, 0);
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Change flags of a file given a path name; this version does
 * not follow links.
 */
int
sys_lchflags(struct lwp *l, const struct sys_lchflags_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(u_long) flags;
	} */
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	error = change_flags(vp, SCARG(uap, flags), l);
	vput(vp);
	return (error);
}

/*
 * Common routine to change flags of a file.
 */
int
change_flags(struct vnode *vp, u_long flags, struct lwp *l)
{
	struct vattr vattr;
	int error;

	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	/*
	 * Non-superusers cannot change the flags on devices, even if they
	 * own them.
	 */
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER, NULL)) {
		if ((error = VOP_GETATTR(vp, &vattr, l->l_cred)) != 0)
			goto out;
		if (vattr.va_type == VCHR || vattr.va_type == VBLK) {
			error = EINVAL;
			goto out;
		}
	}
	VATTR_NULL(&vattr);
	vattr.va_flags = flags;
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
out:
	return (error);
}

/*
 * Change mode of a file given path name; this version follows links.
 */
/* ARGSUSED */
int
sys_chmod(struct lwp *l, const struct sys_chmod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_mode(nd.ni_vp, SCARG(uap, mode), l);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Change mode of a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchmod(struct lwp *l, const struct sys_fchmod_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) mode;
	} */
	struct proc *p = l->l_proc;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_mode((struct vnode *)fp->f_data, SCARG(uap, mode), l);
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Change mode of a file given path name; this version does not follow links.
 */
/* ARGSUSED */
int
sys_lchmod(struct lwp *l, const struct sys_lchmod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_mode(nd.ni_vp, SCARG(uap, mode), l);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set mode given a vnode.
 */
static int
change_mode(struct vnode *vp, int mode, struct lwp *l)
{
	struct vattr vattr;
	int error;

	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	VATTR_NULL(&vattr);
	vattr.va_mode = mode & ALLPERMS;
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
	VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * Set ownership given a path name; this version follows links.
 */
/* ARGSUSED */
int
sys_chown(struct lwp *l, const struct sys_chown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), l, 0);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set ownership given a path name; this version follows links.
 * Provides POSIX semantics.
 */
/* ARGSUSED */
int
sys___posix_chown(struct lwp *l, const struct sys___posix_chown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), l, 1);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set ownership given a file descriptor.
 */
/* ARGSUSED */
int
sys_fchown(struct lwp *l, const struct sys_fchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct proc *p = l->l_proc;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_owner((struct vnode *)fp->f_data, SCARG(uap, uid),
	    SCARG(uap, gid), l, 0);
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Set ownership given a file descriptor, providing POSIX/XPG semantics.
 */
/* ARGSUSED */
int
sys___posix_fchown(struct lwp *l, const struct sys___posix_fchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct proc *p = l->l_proc;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_owner((struct vnode *)fp->f_data, SCARG(uap, uid),
	    SCARG(uap, gid), l, 1);
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Set ownership given a path name; this version does not follow links.
 */
/* ARGSUSED */
int
sys_lchown(struct lwp *l, const struct sys_lchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), l, 0);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Set ownership given a path name; this version does not follow links.
 * Provides POSIX/XPG semantics.
 */
/* ARGSUSED */
int
sys___posix_lchown(struct lwp *l, const struct sys___posix_lchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_owner(nd.ni_vp, SCARG(uap, uid), SCARG(uap, gid), l, 1);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set ownership given a vnode.
 */
static int
change_owner(struct vnode *vp, uid_t uid, gid_t gid, struct lwp *l,
    int posix_semantics)
{
	struct vattr vattr;
	mode_t newmode;
	int error;

	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if ((error = VOP_GETATTR(vp, &vattr, l->l_cred)) != 0)
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
		if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
				      NULL) != 0)
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
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
#undef CHANGED

out:
	VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * Set the access and modification times given a path name; this
 * version follows links.
 */
/* ARGSUSED */
int
sys_utimes(struct lwp *l, const struct sys_utimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval *) tptr;
	} */

	return do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
			SCARG(uap, tptr), UIO_USERSPACE);
}

/*
 * Set the access and modification times given a file descriptor.
 */
/* ARGSUSED */
int
sys_futimes(struct lwp *l, const struct sys_futimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct timeval *) tptr;
	} */
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(l->l_proc->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = do_sys_utimes(l, fp->f_data, NULL, 0, 
			SCARG(uap, tptr), UIO_USERSPACE);

	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Set the access and modification times given a path name; this
 * version does not follow links.
 */
int
sys_lutimes(struct lwp *l, const struct sys_lutimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval *) tptr;
	} */

	return do_sys_utimes(l, NULL, SCARG(uap, path), NOFOLLOW,
			SCARG(uap, tptr), UIO_USERSPACE);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
int
do_sys_utimes(struct lwp *l, struct vnode *vp, const char *path, int flag,
    const struct timeval *tptr, enum uio_seg seg)
{
	struct vattr vattr;
	struct nameidata nd;
	int error;

	VATTR_NULL(&vattr);
	if (tptr == NULL) {
		nanotime(&vattr.va_atime);
		vattr.va_mtime = vattr.va_atime;
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		struct timeval tv[2];

		if (seg != UIO_SYSSPACE) {
			error = copyin(tptr, &tv, sizeof (tv));
			if (error != 0)
				return error;
			tptr = tv;
		}
		TIMEVAL_TO_TIMESPEC(tptr, &vattr.va_atime);
		TIMEVAL_TO_TIMESPEC(tptr + 1, &vattr.va_mtime);
	}

	if (vp == NULL) {
		NDINIT(&nd, LOOKUP, flag | TRYEMULROOT, UIO_USERSPACE, path);
		if ((error = namei(&nd)) != 0)
			return (error);
		vp = nd.ni_vp;
	} else
		nd.ni_vp = NULL;

	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
	VOP_UNLOCK(vp, 0);

	if (nd.ni_vp != NULL)
		vrele(nd.ni_vp);

	return (error);
}

/*
 * Truncate a file given its path name.
 */
/* ARGSUSED */
int
sys_truncate(struct lwp *l, const struct sys_truncate_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0 &&
	    (error = VOP_ACCESS(vp, VWRITE, l->l_cred)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, l->l_cred);
	}
	vput(vp);
	return (error);
}

/*
 * Truncate a file given a file descriptor.
 */
/* ARGSUSED */
int
sys_ftruncate(struct lwp *l, const struct sys_ftruncate_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */
	struct proc *p = l->l_proc;
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
	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0) {
		VATTR_NULL(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, fp->f_cred);
	}
	VOP_UNLOCK(vp, 0);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Sync an open file.
 */
/* ARGSUSED */
int
sys_fsync(struct lwp *l, const struct sys_fsync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT, 0, 0);
	if (error == 0 && bioopsp != NULL &&
	    vp->v_mount && (vp->v_mount->mnt_flag & MNT_SOFTDEP))
		(*bioopsp->io_fsync)(vp, 0);
	VOP_UNLOCK(vp, 0);
	FILE_UNUSE(fp, l);
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
sys_fsync_range(struct lwp *l, const struct sys_fsync_range_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) flags;
		syscallarg(off_t) start;
		syscallarg(off_t) length;
	} */
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
		error = EBADF;
		goto out;
	}

	flags = SCARG(uap, flags);
	if (((flags & (FDATASYNC | FFILESYNC)) == 0) ||
	    ((~flags & (FDATASYNC | FFILESYNC)) == 0)) {
		error = EINVAL;
		goto out;
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
			error = EINVAL;
			goto out;
		}
	} else {
		e = 0;
		s = 0;
	}

	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, nflags, s, e);

	if (error == 0 && bioopsp != NULL &&
	    vp->v_mount && (vp->v_mount->mnt_flag & MNT_SOFTDEP))
		(*bioopsp->io_fsync)(vp, nflags);

	VOP_UNLOCK(vp, 0);
out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Sync the data of an open file.
 */
/* ARGSUSED */
int
sys_fdatasync(struct lwp *l, const struct sys_fdatasync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct file *fp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FWRITE) == 0) {
		FILE_UNUSE(fp, l);
		return (EBADF);
	}
	vp = (struct vnode *)fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT|FSYNC_DATAONLY, 0, 0);
	VOP_UNLOCK(vp, 0);
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Rename files, (standard) BSD semantics frontend.
 */
/* ARGSUSED */
int
sys_rename(struct lwp *l, const struct sys_rename_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */

	return (do_sys_rename(SCARG(uap, from), SCARG(uap, to), UIO_USERSPACE, 0));
}

/*
 * Rename files, POSIX semantics frontend.
 */
/* ARGSUSED */
int
sys___posix_rename(struct lwp *l, const struct sys___posix_rename_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */

	return (do_sys_rename(SCARG(uap, from), SCARG(uap, to), UIO_USERSPACE, 1));
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
int
do_sys_rename(const char *from, const char *to, enum uio_seg seg, int retain)
{
	struct vnode *tvp, *fvp, *tdvp;
	struct nameidata fromnd, tond;
	struct lwp *l = curlwp;
	struct proc *p;
	int error;

	NDINIT(&fromnd, DELETE, LOCKPARENT | SAVESTART | TRYEMULROOT,
	    seg, from);
	if ((error = namei(&fromnd)) != 0)
		return (error);
	if (fromnd.ni_dvp != fromnd.ni_vp)
		VOP_UNLOCK(fromnd.ni_dvp, 0);
	fvp = fromnd.ni_vp;
	NDINIT(&tond, RENAME,
	    LOCKPARENT | LOCKLEAF | NOCACHE | SAVESTART | TRYEMULROOT
	      | (fvp->v_type == VDIR ? CREATEDIR : 0),
	    seg, to);
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

#if NVERIEXEC > 0
	if (!error) {
		char *f1, *f2;

		f1 = malloc(fromnd.ni_cnd.cn_namelen + 1, M_TEMP, M_WAITOK);
		strlcpy(f1, fromnd.ni_cnd.cn_nameptr, fromnd.ni_cnd.cn_namelen);

		f2 = malloc(tond.ni_cnd.cn_namelen + 1, M_TEMP, M_WAITOK);
		strlcpy(f2, tond.ni_cnd.cn_nameptr, tond.ni_cnd.cn_namelen);

		error = veriexec_renamechk(l, fvp, f1, tvp, f2);

		free(f1, M_TEMP);
		free(f2, M_TEMP);
	}
#endif /* NVERIEXEC > 0 */

out:
	p = l->l_proc;
	if (!error) {
		VOP_LEASE(tdvp, l->l_cred, LEASE_WRITE);
		if (fromnd.ni_dvp != tdvp)
			VOP_LEASE(fromnd.ni_dvp, l->l_cred, LEASE_WRITE);
		if (tvp) {
			VOP_LEASE(tvp, l->l_cred, LEASE_WRITE);
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
sys_mkdir(struct lwp *l, const struct sys_mkdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct nameidata nd;

	NDINIT(&nd, CREATE, LOCKPARENT | CREATEDIR | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
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
	VATTR_NULL(&vattr);
	vattr.va_type = VDIR;
	/* We will read cwdi->cwdi_cmask unlocked. */
	vattr.va_mode =
	    (SCARG(uap, mode) & ACCESSPERMS) &~ p->p_cwdi->cwdi_cmask;
	VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
	error = VOP_MKDIR(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (!error)
		vput(nd.ni_vp);
	return (error);
}

/*
 * Remove a directory file.
 */
/* ARGSUSED */
int
sys_rmdir(struct lwp *l, const struct sys_rmdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
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
	if (vp->v_vflag & VV_ROOT) {
		error = EBUSY;
		goto out;
	}
	VOP_LEASE(nd.ni_dvp, l->l_cred, LEASE_WRITE);
	VOP_LEASE(vp, l->l_cred, LEASE_WRITE);
	error = VOP_RMDIR(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
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
sys___getdents30(struct lwp *l, const struct sys___getdents30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */
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
			SCARG(uap, count), &done, l, 0, 0);
	ktrgenio(SCARG(uap, fd), UIO_READ, SCARG(uap, buf), done, error);
	*retval = done;
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/*
 * Set the mode mask for creation of filesystem nodes.
 */
int
sys_umask(struct lwp *l, const struct sys_umask_args *uap, register_t *retval)
{
	/* {
		syscallarg(mode_t) newmask;
	} */
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi;

	/*
	 * cwdi->cwdi_cmask will be read unlocked elsewhere.  What's
	 * important is that we serialize changes to the mask.  The
	 * rw_exit() will issue a write memory barrier on our behalf,
	 * and force the changes out to other CPUs (as it must use an
	 * atomic operation, draining the local CPU's store buffers).
	 */
	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	*retval = cwdi->cwdi_cmask;
	cwdi->cwdi_cmask = SCARG(uap, newmask) & ALLPERMS;
	rw_exit(&cwdi->cwdi_lock);

	return (0);
}

int
dorevoke(struct vnode *vp, kauth_cred_t cred)
{
	struct vattr vattr;
	int error;
	bool revoke;

	if ((error = VOP_GETATTR(vp, &vattr, cred)) != 0)
		goto out;
	if (kauth_cred_geteuid(cred) != vattr.va_uid &&
	    (error = kauth_authorize_generic(cred,
	    KAUTH_GENERIC_ISSUSER, NULL)) != 0)
		goto out;
	mutex_enter(&vp->v_interlock);
	revoke = (vp->v_usecount > 1 || (vp->v_iflag & (VI_ALIASED|VI_LAYER)));
	mutex_exit(&vp->v_interlock);
	if (revoke)
		VOP_REVOKE(vp, REVOKEALL);

 out:
	return (error);
}

/*
 * Void all references to file by ripping underlying filesystem
 * away from vnode.
 */
/* ARGSUSED */
int
sys_revoke(struct lwp *l, const struct sys_revoke_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	struct vnode *vp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));
	if ((error = namei(&nd)) != 0)
		return (error);
	vp = nd.ni_vp;
	error = dorevoke(vp, l->l_cred);
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
