/*	$NetBSD: vfs_syscalls.c,v 1.440.2.2 2012/05/23 10:08:13 yamt Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Virtual File System System Calls
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls.c,v 1.440.2.2 2012/05/23 10:08:13 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_fileassoc.h"
#include "veriexec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/dirent.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>
#include <sys/quota.h>
#include <sys/quotactl.h>
#include <sys/ktrace.h>
#ifdef FILEASSOC
#include <sys/fileassoc.h>
#endif /* FILEASSOC */
#include <sys/extattr.h>
#include <sys/verified_exec.h>
#include <sys/kauth.h>
#include <sys/atomic.h>
#include <sys/module.h>
#include <sys/buf.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>
#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfs_var.h>

static int change_flags(struct vnode *, u_long, struct lwp *);
static int change_mode(struct vnode *, int, struct lwp *l);
static int change_owner(struct vnode *, uid_t, gid_t, struct lwp *, int);
static int do_open(lwp_t *, struct pathbuf *, int, int, int *);

/*
 * This table is used to maintain compatibility with 4.3BSD
 * and NetBSD 0.9 mount syscalls - and possibly other systems.
 * Note, the order is important!
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

const int nmountcompatnames = __arraycount(mountcompatnames);

static int
open_setfp(struct lwp *l, file_t *fp, struct vnode *vp, int indx, int flags)
{
	int error;

	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_ops = &vnops;
	fp->f_data = vp;

	if (flags & (O_EXLOCK | O_SHLOCK)) {
		struct flock lf;
		int type;

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
		VOP_UNLOCK(vp);
		error = VOP_ADVLOCK(vp, fp, F_SETLK, &lf, type);
		if (error) {
			(void) vn_close(vp, fp->f_flag, fp->f_cred);
			fd_abort(l->l_proc, fp, indx);
			return error;
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		atomic_or_uint(&fp->f_flag, FHASLOCK);
	}
	if (flags & O_CLOEXEC)
		fd_set_exclose(l, indx, true);
	return 0;
}

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
	 * is currently mounted read-only.  Additionally, we
	 * prevent read-write to read-only downgrades.
	 */
	if ((flags & (MNT_RELOAD | MNT_RDONLY)) != 0 &&
	    (mp->mnt_flag & MNT_RDONLY) == 0 &&
	    (mp->mnt_iflag & IMNT_CAN_RWTORO) == 0) {
		error = EOPNOTSUPP;	/* Needs translation */
		goto out;
	}

	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_UPDATE, mp, KAUTH_ARG(flags), data);
	if (error)
		goto out;

	if (vfs_busy(mp, NULL)) {
		error = EPERM;
		goto out;
	}

	mutex_enter(&mp->mnt_updating);

	mp->mnt_flag &= ~MNT_OP_FLAGS;
	mp->mnt_flag |= flags & MNT_OP_FLAGS;

	/*
	 * Set the mount level flags.
	 */
	if (flags & MNT_RDONLY)
		mp->mnt_flag |= MNT_RDONLY;
	else if (mp->mnt_flag & MNT_RDONLY)
		mp->mnt_iflag |= IMNT_WANTRDWR;
	mp->mnt_flag &= ~MNT_BASIC_FLAGS;
	mp->mnt_flag |= flags & MNT_BASIC_FLAGS;
	error = VFS_MOUNT(mp, path, data, data_len);

	if (error && data != NULL) {
		int error2;

		/*
		 * Update failed; let's try and see if it was an
		 * export request.  For compat with 3.0 and earlier.
		 */
		error2 = vfs_hooks_reexport(mp, path, data);

		/*
		 * Only update error code if the export request was
		 * understood but some problem occurred while
		 * processing it.
		 */
		if (error2 != EJUSTRETURN)
			error = error2;
	}

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
	mutex_exit(&mp->mnt_updating);
	vfs_unbusy(mp, false, NULL);

	if ((error == 0) && !(saved_flags & MNT_EXTATTR) && 
	    (flags & MNT_EXTATTR)) {
		if (VFS_EXTATTRCTL(mp, EXTATTR_CMD_START, 
				   NULL, 0, NULL) != 0) {
			printf("%s: failed to start extattr, error = %d",
			       mp->mnt_stat.f_mntonname, error);
			mp->mnt_flag &= ~MNT_EXTATTR;
		}
	}

	if ((error == 0) && (saved_flags & MNT_EXTATTR) && 
	    !(flags & MNT_EXTATTR)) {
		if (VFS_EXTATTRCTL(mp, EXTATTR_CMD_STOP, 
				   NULL, 0, NULL) != 0) {
			printf("%s: failed to stop extattr, error = %d",
			       mp->mnt_stat.f_mntonname, error);
			mp->mnt_flag |= MNT_RDONLY;
		}
	}
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
	}

	/* Accept `ufs' as an alias for `ffs', for compatibility. */
	if (strcmp(fstypename, "ufs") == 0)
		fstypename[0] = 'f';

	if ((*vfsops = vfs_getopsbyname(fstypename)) != NULL)
		return 0;

	/* If we can autoload a vfs module, try again */
	(void)module_autoload(fstypename, MODULE_CLASS_VFS);

	if ((*vfsops = vfs_getopsbyname(fstypename)) != NULL)
		return 0;

	return ENODEV;
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

	if (vfs_busy(mp, NULL))
		return EPERM;

	mutex_enter(&mp->mnt_updating);
	mp->mnt_flag &= ~MNT_OP_FLAGS;
	mp->mnt_flag |= MNT_GETARGS;
	error = VFS_MOUNT(mp, path, data, data_len);
	mp->mnt_flag &= ~MNT_OP_FLAGS;
	mutex_exit(&mp->mnt_updating);

	vfs_unbusy(mp, false, NULL);
	return (error);
}

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
	void *data_buf = data;
	bool vfsopsrele = false;
	int error;

	/* XXX: The calling convention of this routine is totally bizarre */
	if (vfsops)
		vfsopsrele = true;

	/*
	 * Get vnode to be covered
	 */
	error = namei_simple_user(path, NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0) {
		vp = NULL;
		goto done;
	}

	if (vfsops == NULL) {
		if (flags & (MNT_GETARGS | MNT_UPDATE)) {
			vfsops = vp->v_mount->mnt_op;
		} else {
			/* 'type' is userspace */
			error = mount_get_vfsops(type, &vfsops);
			if (error != 0)
				goto done;
			vfsopsrele = true;
		}
	}

	if (data != NULL && data_seg == UIO_USERSPACE) {
		if (data_len == 0) {
			/* No length supplied, use default for filesystem */
			data_len = vfsops->vfs_min_mount_data;
			if (data_len > VFS_MAX_MOUNT_DATA) {
				error = EINVAL;
				goto done;
			}
			/*
			 * Hopefully a longer buffer won't make copyin() fail.
			 * For compatibility with 3.0 and earlier.
			 */
			if (flags & MNT_UPDATE
			    && data_len < sizeof (struct mnt_export_args30))
				data_len = sizeof (struct mnt_export_args30);
		}
		data_buf = kmem_alloc(data_len, KM_SLEEP);

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
		KASSERT(vfsopsrele == true);
		error = mount_domount(l, &vp, vfsops, path, flags, data_buf,
		    &data_len);
		vfsopsrele = false;
	}

    done:
	if (vfsopsrele)
		vfs_delref(vfsops);
    	if (vp != NULL) {
	    	vrele(vp);
	}
	if (data_buf != data)
		kmem_free(data_buf, data_len);
	return (error);
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
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return error;
	}
	vp = nd.ni_vp;
	pathbuf_destroy(pb);

	mp = vp->v_mount;
	atomic_inc_uint(&mp->mnt_refcnt);
	VOP_UNLOCK(vp);

	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_UNMOUNT, mp, NULL, NULL);
	if (error) {
		vrele(vp);
		vfs_destroy(mp);
		return (error);
	}

	/*
	 * Don't allow unmounting the root file system.
	 */
	if (mp->mnt_flag & MNT_ROOTFS) {
		vrele(vp);
		vfs_destroy(mp);
		return (EINVAL);
	}

	/*
	 * Must be the root of the filesystem
	 */
	if ((vp->v_vflag & VV_ROOT) == 0) {
		vrele(vp);
		vfs_destroy(mp);
		return (EINVAL);
	}

	vrele(vp);
	error = dounmount(mp, SCARG(uap, flags), l);
	vfs_destroy(mp);
	return error;
}

/*
 * Sync each mounted filesystem.
 */
#ifdef DEBUG
int syncprt = 0;
struct ctldebug debug0 = { "syncprt", &syncprt };
#endif

void
do_sys_sync(struct lwp *l)
{
	struct mount *mp, *nmp;
	int asyncflag;

	mutex_enter(&mountlist_lock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, &nmp)) {
			continue;
		}
		mutex_enter(&mp->mnt_updating);
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			asyncflag = mp->mnt_flag & MNT_ASYNC;
			mp->mnt_flag &= ~MNT_ASYNC;
			VFS_SYNC(mp, MNT_NOWAIT, l->l_cred);
			if (asyncflag)
				 mp->mnt_flag |= MNT_ASYNC;
		}
		mutex_exit(&mp->mnt_updating);
		vfs_unbusy(mp, false, &nmp);
	}
	mutex_exit(&mountlist_lock);
#ifdef DEBUG
	if (syncprt)
		vfs_bufstats();
#endif /* DEBUG */
}

/* ARGSUSED */
int
sys_sync(struct lwp *l, const void *v, register_t *retval)
{
	do_sys_sync(l);
	return (0);
}


/*
 * Access or change filesystem quotas.
 *
 * (this is really 14 different calls bundled into one)
 */

static int
do_sys_quotactl_stat(struct mount *mp, struct quotastat *info_u)
{
	struct quotastat info_k;
	int error;

	/* ensure any padding bytes are cleared */
	memset(&info_k, 0, sizeof(info_k));

	error = vfs_quotactl_stat(mp, &info_k);
	if (error) {
		return error;
	}

	return copyout(&info_k, info_u, sizeof(info_k));
}

static int
do_sys_quotactl_idtypestat(struct mount *mp, int idtype,
    struct quotaidtypestat *info_u)
{
	struct quotaidtypestat info_k;
	int error;

	/* ensure any padding bytes are cleared */
	memset(&info_k, 0, sizeof(info_k));

	error = vfs_quotactl_idtypestat(mp, idtype, &info_k);
	if (error) {
		return error;
	}

	return copyout(&info_k, info_u, sizeof(info_k));
}

static int
do_sys_quotactl_objtypestat(struct mount *mp, int objtype,
    struct quotaobjtypestat *info_u)
{
	struct quotaobjtypestat info_k;
	int error;

	/* ensure any padding bytes are cleared */
	memset(&info_k, 0, sizeof(info_k));

	error = vfs_quotactl_objtypestat(mp, objtype, &info_k);
	if (error) {
		return error;
	}

	return copyout(&info_k, info_u, sizeof(info_k));
}

static int
do_sys_quotactl_get(struct mount *mp, const struct quotakey *key_u,
    struct quotaval *val_u)
{
	struct quotakey key_k;
	struct quotaval val_k;
	int error;

	/* ensure any padding bytes are cleared */
	memset(&val_k, 0, sizeof(val_k));

	error = copyin(key_u, &key_k, sizeof(key_k));
	if (error) {
		return error;
	}

	error = vfs_quotactl_get(mp, &key_k, &val_k);
	if (error) {
		return error;
	}

	return copyout(&val_k, val_u, sizeof(val_k));
}

static int
do_sys_quotactl_put(struct mount *mp, const struct quotakey *key_u,
    const struct quotaval *val_u)
{
	struct quotakey key_k;
	struct quotaval val_k;
	int error;

	error = copyin(key_u, &key_k, sizeof(key_k));
	if (error) {
		return error;
	}

	error = copyin(val_u, &val_k, sizeof(val_k));
	if (error) {
		return error;
	}

	return vfs_quotactl_put(mp, &key_k, &val_k);
}

static int
do_sys_quotactl_delete(struct mount *mp, const struct quotakey *key_u)
{
	struct quotakey key_k;
	int error;

	error = copyin(key_u, &key_k, sizeof(key_k));
	if (error) {
		return error;
	}

	return vfs_quotactl_delete(mp, &key_k);
}

static int
do_sys_quotactl_cursoropen(struct mount *mp, struct quotakcursor *cursor_u)
{
	struct quotakcursor cursor_k;
	int error;

	/* ensure any padding bytes are cleared */
	memset(&cursor_k, 0, sizeof(cursor_k));

	error = vfs_quotactl_cursoropen(mp, &cursor_k);
	if (error) {
		return error;
	}

	return copyout(&cursor_k, cursor_u, sizeof(cursor_k));
}

static int
do_sys_quotactl_cursorclose(struct mount *mp, struct quotakcursor *cursor_u)
{
	struct quotakcursor cursor_k;
	int error;

	error = copyin(cursor_u, &cursor_k, sizeof(cursor_k));
	if (error) {
		return error;
	}

	return vfs_quotactl_cursorclose(mp, &cursor_k);
}

static int
do_sys_quotactl_cursorskipidtype(struct mount *mp,
    struct quotakcursor *cursor_u, int idtype)
{
	struct quotakcursor cursor_k;
	int error;

	error = copyin(cursor_u, &cursor_k, sizeof(cursor_k));
	if (error) {
		return error;
	}

	error = vfs_quotactl_cursorskipidtype(mp, &cursor_k, idtype);
	if (error) {
		return error;
	}

	return copyout(&cursor_k, cursor_u, sizeof(cursor_k));
}

static int
do_sys_quotactl_cursorget(struct mount *mp, struct quotakcursor *cursor_u,
    struct quotakey *keys_u, struct quotaval *vals_u, unsigned maxnum,
    unsigned *ret_u)
{
#define CGET_STACK_MAX 8
	struct quotakcursor cursor_k;
	struct quotakey stackkeys[CGET_STACK_MAX];
	struct quotaval stackvals[CGET_STACK_MAX];
	struct quotakey *keys_k;
	struct quotaval *vals_k;
	unsigned ret_k;
	int error;

	if (maxnum > 128) {
		maxnum = 128;
	}

	error = copyin(cursor_u, &cursor_k, sizeof(cursor_k));
	if (error) {
		return error;
	}

	if (maxnum <= CGET_STACK_MAX) {
		keys_k = stackkeys;
		vals_k = stackvals;
		/* ensure any padding bytes are cleared */
		memset(keys_k, 0, maxnum * sizeof(keys_k[0]));
		memset(vals_k, 0, maxnum * sizeof(vals_k[0]));
	} else {
		keys_k = kmem_zalloc(maxnum * sizeof(keys_k[0]), KM_SLEEP);
		vals_k = kmem_zalloc(maxnum * sizeof(vals_k[0]), KM_SLEEP);
	}

	error = vfs_quotactl_cursorget(mp, &cursor_k, keys_k, vals_k, maxnum,
				       &ret_k);
	if (error) {
		goto fail;
	}

	error = copyout(keys_k, keys_u, ret_k * sizeof(keys_k[0]));
	if (error) {
		goto fail;
	}

	error = copyout(vals_k, vals_u, ret_k * sizeof(vals_k[0]));
	if (error) {
		goto fail;
	}

	error = copyout(&ret_k, ret_u, sizeof(ret_k));
	if (error) {
		goto fail;
	}

	/* do last to maximize the chance of being able to recover a failure */
	error = copyout(&cursor_k, cursor_u, sizeof(cursor_k));

fail:
	if (keys_k != stackkeys) {
		kmem_free(keys_k, maxnum * sizeof(keys_k[0]));
	}
	if (vals_k != stackvals) {
		kmem_free(vals_k, maxnum * sizeof(vals_k[0]));
	}
	return error;
}

static int
do_sys_quotactl_cursoratend(struct mount *mp, struct quotakcursor *cursor_u,
    int *ret_u)
{
	struct quotakcursor cursor_k;
	int ret_k;
	int error;

	error = copyin(cursor_u, &cursor_k, sizeof(cursor_k));
	if (error) {
		return error;
	}

	error = vfs_quotactl_cursoratend(mp, &cursor_k, &ret_k);
	if (error) {
		return error;
	}

	error = copyout(&ret_k, ret_u, sizeof(ret_k));
	if (error) {
		return error;
	}

	return copyout(&cursor_k, cursor_u, sizeof(cursor_k));
}

static int
do_sys_quotactl_cursorrewind(struct mount *mp, struct quotakcursor *cursor_u)
{
	struct quotakcursor cursor_k;
	int error;

	error = copyin(cursor_u, &cursor_k, sizeof(cursor_k));
	if (error) {
		return error;
	}

	error = vfs_quotactl_cursorrewind(mp, &cursor_k);
	if (error) {
		return error;
	}

	return copyout(&cursor_k, cursor_u, sizeof(cursor_k));
}

static int
do_sys_quotactl_quotaon(struct mount *mp, int idtype, const char *path_u)
{
	char *path_k;
	int error;

	/* XXX this should probably be a struct pathbuf */
	path_k = PNBUF_GET();
	error = copyin(path_u, path_k, PATH_MAX);
	if (error) {
		PNBUF_PUT(path_k);
		return error;
	}

	error = vfs_quotactl_quotaon(mp, idtype, path_k);

	PNBUF_PUT(path_k);
	return error;
}

static int
do_sys_quotactl_quotaoff(struct mount *mp, int idtype)
{
	return vfs_quotactl_quotaoff(mp, idtype);
}

int
do_sys_quotactl(const char *path_u, const struct quotactl_args *args)
{
	struct mount *mp;
	struct vnode *vp;
	int error;

	error = namei_simple_user(path_u, NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;

	switch (args->qc_op) {
	    case QUOTACTL_STAT:
		error = do_sys_quotactl_stat(mp, args->u.stat.qc_info);
		break;
	    case QUOTACTL_IDTYPESTAT:
		error = do_sys_quotactl_idtypestat(mp,
				args->u.idtypestat.qc_idtype,
				args->u.idtypestat.qc_info);
		break;
	    case QUOTACTL_OBJTYPESTAT:
		error = do_sys_quotactl_objtypestat(mp,
				args->u.objtypestat.qc_objtype,
				args->u.objtypestat.qc_info);
		break;
	    case QUOTACTL_GET:
		error = do_sys_quotactl_get(mp,
				args->u.get.qc_key,
				args->u.get.qc_val);
		break;
	    case QUOTACTL_PUT:
		error = do_sys_quotactl_put(mp,
				args->u.put.qc_key,
				args->u.put.qc_val);
		break;
	    case QUOTACTL_DELETE:
		error = do_sys_quotactl_delete(mp, args->u.delete.qc_key);
		break;
	    case QUOTACTL_CURSOROPEN:
		error = do_sys_quotactl_cursoropen(mp,
				args->u.cursoropen.qc_cursor);
		break;
	    case QUOTACTL_CURSORCLOSE:
		error = do_sys_quotactl_cursorclose(mp,
				args->u.cursorclose.qc_cursor);
		break;
	    case QUOTACTL_CURSORSKIPIDTYPE:
		error = do_sys_quotactl_cursorskipidtype(mp,
				args->u.cursorskipidtype.qc_cursor,
				args->u.cursorskipidtype.qc_idtype);
		break;
	    case QUOTACTL_CURSORGET:
		error = do_sys_quotactl_cursorget(mp,
				args->u.cursorget.qc_cursor,
				args->u.cursorget.qc_keys,
				args->u.cursorget.qc_vals,
				args->u.cursorget.qc_maxnum,
				args->u.cursorget.qc_ret);
		break;
	    case QUOTACTL_CURSORATEND:
		error = do_sys_quotactl_cursoratend(mp,
				args->u.cursoratend.qc_cursor,
				args->u.cursoratend.qc_ret);
		break;
	    case QUOTACTL_CURSORREWIND:
		error = do_sys_quotactl_cursorrewind(mp,
				args->u.cursorrewind.qc_cursor);
		break;
	    case QUOTACTL_QUOTAON:
		error = do_sys_quotactl_quotaon(mp,
				args->u.quotaon.qc_idtype,
				args->u.quotaon.qc_quotafile);
		break;
	    case QUOTACTL_QUOTAOFF:
		error = do_sys_quotactl_quotaoff(mp,
				args->u.quotaoff.qc_idtype);
		break;
	    default:
		error = EINVAL;
		break;
	}

	vrele(vp);
	return error;
}

/* ARGSUSED */
int
sys___quotactl(struct lwp *l, const struct sys___quotactl_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct quotactl_args *) args;
	} */
	struct quotactl_args args;
	int error;

	error = copyin(SCARG(uap, args), &args, sizeof(args));
	if (error) {
		return error;
	}

	return do_sys_quotactl(SCARG(uap, path), &args);
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
		char c;
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
		if (len != 1) {
			/*
			 * for mount points that are below our root, we can see
			 * them, so we fix up the pathname and return them. The
			 * rest we cannot see, so we don't allow viewing the
			 * data.
			 */
			if (strncmp(bp, sp->f_mntonname, len) == 0 &&
			    ((c = sp->f_mntonname[len]) == '/' || c == '\0')) {
				(void)strlcpy(sp->f_mntonname,
				    c == '\0' ? "/" : &sp->f_mntonname[len],
				    sizeof(sp->f_mntonname));
			} else {
				if (root)
					(void)strlcpy(sp->f_mntonname, "/",
					    sizeof(sp->f_mntonname));
				else
					error = EPERM;
			}
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
	struct vnode *vp;

	error = namei_simple_user(path, NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return error;
	mp = vp->v_mount;
	error = dostatvfs(mp, sb, l, flags, 1);
	vrele(vp);
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
	file_t *fp;
	struct mount *mp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(fd, &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	error = dostatvfs(mp, sb, curlwp, flags, 1);
	fd_putfile(fd);
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
		if (vfs_busy(mp, &nmp)) {
			continue;
		}
		if (sfsp && count < maxcount) {
			error = dostatvfs(mp, sb, l, flags, 0);
			if (error) {
				vfs_unbusy(mp, false, &nmp);
				error = 0;
				continue;
			}
			error = copyfn(sb, sfsp, entry_sz);
			if (error) {
				vfs_unbusy(mp, false, NULL);
				goto out;
			}
			sfsp = (char *)sfsp + entry_sz;
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
		    sb, l, flags, 1);
		if (error != 0)
			goto out;
		if (sfsp) {
			error = copyfn(sb, sfsp, entry_sz);
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
	struct cwdinfo *cwdi;
	struct vnode *vp, *tdp;
	struct mount *mp;
	file_t *fp;
	int error, fd;

	/* fd_getvnode() will use the descriptor for us */
	fd = SCARG(uap, fd);
	if ((error = fd_getvnode(fd, &fp)) != 0)
		return (error);
	vp = fp->f_data;

	vref(vp);
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
		error = vfs_busy(mp, NULL);
		vput(vp);
		if (error != 0)
			goto out;
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp, false, NULL);
		if (error)
			goto out;
		vp = tdp;
	}
	VOP_UNLOCK(vp);

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
	fd_putfile(fd);
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
	struct vnode	*vp;
	file_t	*fp;
	int		 error, fd = SCARG(uap, fd);

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_CHROOT,
 	    KAUTH_REQ_SYSTEM_CHROOT_FCHROOT, NULL, NULL, NULL)) != 0)
		return error;
	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(fd, &fp)) != 0)
		return error;
	vp = fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, l->l_cred);
	VOP_UNLOCK(vp);
	if (error)
		goto out;
	vref(vp);

	change_root(p->p_cwdi, vp, l);

 out:
	fd_putfile(fd);
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
	struct vnode *vp;

	if ((error = chdir_lookup(SCARG(uap, path), UIO_USERSPACE,
				  &vp, l)) != 0)
		return (error);
	cwdi = p->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	vrele(cwdi->cwdi_cdir);
	cwdi->cwdi_cdir = vp;
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
	int error;
	struct vnode *vp;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_CHROOT,
	    KAUTH_REQ_SYSTEM_CHROOT_CHROOT, NULL, NULL, NULL)) != 0)
		return (error);
	if ((error = chdir_lookup(SCARG(uap, path), UIO_USERSPACE,
				  &vp, l)) != 0)
		return (error);

	change_root(p->p_cwdi, vp, l);

	return (0);
}

/*
 * Common routine for chroot and fchroot.
 * NB: callers need to properly authorize the change root operation.
 */
void
change_root(struct cwdinfo *cwdi, struct vnode *vp, struct lwp *l)
{

	rw_enter(&cwdi->cwdi_lock, RW_WRITER);
	if (cwdi->cwdi_rdir != NULL)
		vrele(cwdi->cwdi_rdir);
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
		vref(vp);
		cwdi->cwdi_cdir = vp;
	}
	rw_exit(&cwdi->cwdi_lock);
}

/*
 * Common routine for chroot and chdir.
 * XXX "where" should be enum uio_seg
 */
int
chdir_lookup(const char *path, int where, struct vnode **vpp, struct lwp *l)
{
	struct pathbuf *pb;
	struct nameidata nd;
	int error;

	error = pathbuf_maybe_copyin(path, where, &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return error;
	}
	*vpp = nd.ni_vp;
	pathbuf_destroy(pb);

	if ((*vpp)->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(*vpp, VEXEC, l->l_cred);

	if (error)
		vput(*vpp);
	else
		VOP_UNLOCK(*vpp);
	return (error);
}

/*
 * Internals of sys_open - path has already been converted into a pathbuf
 * (so we can easily reuse this function from other parts of the kernel,
 * like posix_spawn post-processing).
 */
static int
do_open(lwp_t *l, struct pathbuf *pb, int open_flags, int open_mode, int *fd)
{
	struct proc *p = l->l_proc;
	struct cwdinfo *cwdi = p->p_cwdi;
	file_t *fp;
	struct vnode *vp;
	int flags, cmode;
	int indx, error;
	struct nameidata nd;

	flags = FFLAGS(open_flags);
	if ((flags & (FREAD | FWRITE)) == 0)
		return EINVAL;

	if ((error = fd_allocfile(&fp, &indx)) != 0) {
		return error;
	}

	/* We're going to read cwdi->cwdi_cmask unlocked here. */
	cmode = ((open_mode &~ cwdi->cwdi_cmask) & ALLPERMS) &~ S_ISTXT;
	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, pb);
	l->l_dupfd = -indx - 1;			/* XXX check for fdopen */
	if ((error = vn_open(&nd, flags, cmode)) != 0) {
		fd_abort(p, fp, indx);
		if ((error == EDUPFD || error == EMOVEFD) &&
		    l->l_dupfd >= 0 &&			/* XXX from fdopen */
		    (error =
			fd_dupopen(l->l_dupfd, &indx, flags, error)) == 0) {
			*fd = indx;
			return 0;
		}
		if (error == ERESTART)
			error = EINTR;
		return error;
	}

	l->l_dupfd = 0;
	vp = nd.ni_vp;

	if ((error = open_setfp(l, fp, vp, indx, flags)))
		return error;

	VOP_UNLOCK(vp);
	*fd = indx;
	fd_affix(p, fp, indx);
	return 0;
}

int
fd_open(const char *path, int open_flags, int open_mode, int *fd)
{
	struct pathbuf *pb;
	int error, oflags;

	oflags = FFLAGS(open_flags);
	if ((oflags & (FREAD | FWRITE)) == 0)
		return EINVAL;

	pb = pathbuf_create(path);
	if (pb == NULL)
		return ENOMEM;

	error = do_open(curlwp, pb, open_flags, open_mode, fd);
	pathbuf_destroy(pb);

	return error;
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
	struct pathbuf *pb;
	int result, flags, error;

	flags = FFLAGS(SCARG(uap, flags));
	if ((flags & (FREAD | FWRITE)) == 0)
		return EINVAL;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error)
		return error;

	error = do_open(l, pb, SCARG(uap, flags), SCARG(uap, mode), &result);
	pathbuf_destroy(pb);

	*retval = result;
	return error;
}

int
sys_openat(struct lwp *l, const struct sys_openat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */

	return ENOSYS;
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
	struct pathbuf *pb;
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

	error = pathbuf_copyin(SCARG(uap, fname), &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	error = namei(&nd);
	if (error) {
		pathbuf_destroy(pb);
		return error;
	}
	vp = nd.ni_vp;
	pathbuf_destroy(pb);

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
	file_t *fp;
	struct vnode *vp = NULL;
	kauth_cred_t cred = l->l_cred;
	file_t *nfp;
	int indx, error = 0;
	struct vattr va;
	fhandle_t *fh;
	int flags;
	proc_t *p;

	p = curproc;

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
	if ((error = fd_allocfile(&nfp, &indx)) != 0)
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
		VOP_UNLOCK(vp);			/* XXX */
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);   /* XXX */
		vattr_null(&va);
		va.va_size = 0;
		error = VOP_SETATTR(vp, &va, cred);
		if (error)
			goto bad;
	}
	if ((error = VOP_OPEN(vp, flags, cred)) != 0)
		goto bad;
	if (flags & FWRITE) {
		mutex_enter(vp->v_interlock);
		vp->v_writecount++;
		mutex_exit(vp->v_interlock);
	}

	/* done with modified vn_open, now finish what sys_open does. */
	if ((error = open_setfp(l, fp, vp, indx, flags)))
		return error;

	VOP_UNLOCK(vp);
	*retval = indx;
	fd_affix(p, fp, indx);
	vfs_copyinfh_free(fh);
	return (0);

bad:
	fd_abort(p, fp, indx);
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

	error = vn_stat(vp, sb);
	vput(vp);
	return error;
}


/* ARGSUSED */
int
sys___fhstat50(struct lwp *l, const struct sys___fhstat50_args *uap, register_t *retval)
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
sys___mknod50(struct lwp *l, const struct sys___mknod50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
		syscallarg(dev_t) dev;
	} */
	return do_sys_mknod(l, SCARG(uap, path), SCARG(uap, mode),
	    SCARG(uap, dev), retval, UIO_USERSPACE);
}

int
sys_mknodat(struct lwp *l, const struct sys_mknodat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
		syscallarg(uint32_t) dev;
	} */

	return ENOSYS;
}

int
do_sys_mknod(struct lwp *l, const char *pathname, mode_t mode, dev_t dev,
    register_t *retval, enum uio_seg seg)
{
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct vattr vattr;
	int error, optype;
	struct pathbuf *pb;
	struct nameidata nd;
	const char *pathstring;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MKNOD,
	    0, NULL, NULL, NULL)) != 0)
		return (error);

	optype = VOP_MKNOD_DESCOFFSET;

	error = pathbuf_maybe_copyin(pathname, seg, &pb);
	if (error) {
		return error;
	}
	pathstring = pathbuf_stringcopy_get(pb);
	if (pathstring == NULL) {
		pathbuf_destroy(pb);
		return ENOMEM;
	}

	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0)
		goto out;
	vp = nd.ni_vp;

	if (vp != NULL)
		error = EEXIST;
	else {
		vattr_null(&vattr);
		/* We will read cwdi->cwdi_cmask unlocked. */
		vattr.va_mode = (mode & ALLPERMS) &~ p->p_cwdi->cwdi_cmask;
		vattr.va_rdev = dev;

		switch (mode & S_IFMT) {
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
			error = veriexec_openchk(l, nd.ni_vp, pathstring,
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
	if (error == 0 && optype == VOP_MKNOD_DESCOFFSET
	    && vattr.va_rdev == VNOVAL)
		error = EINVAL;
	if (!error) {
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
	pathbuf_stringcopy_put(pb, pathstring);
	pathbuf_destroy(pb);
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
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return error;
	}
	if (nd.ni_vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		pathbuf_destroy(pb);
		return (EEXIST);
	}
	vattr_null(&vattr);
	vattr.va_type = VFIFO;
	/* We will read cwdi->cwdi_cmask unlocked. */
	vattr.va_mode = (SCARG(uap, mode) & ALLPERMS) &~ p->p_cwdi->cwdi_cmask;
	error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (error == 0)
		vput(nd.ni_vp);
	pathbuf_destroy(pb);
	return (error);
}

int
sys_mkfifoat(struct lwp *l, const struct sys_mkfifoat_args *uap, 
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */

	return ENOSYS;
}
/*
 * Make a hard file link.
 */
/* ARGSUSED */
static int
do_sys_link(struct lwp *l, const char *path, const char *link, 
	    int follow, register_t *retval) 
{
	struct vnode *vp;
	struct pathbuf *linkpb;
	struct nameidata nd;
	namei_simple_flags_t namei_simple_flags;
	int error;

	if (follow)
		namei_simple_flags = NSM_FOLLOW_TRYEMULROOT;
	else
		namei_simple_flags =  NSM_NOFOLLOW_TRYEMULROOT;

	error = namei_simple_user(path, namei_simple_flags, &vp);
	if (error != 0)
		return (error);
	error = pathbuf_copyin(link, &linkpb);
	if (error) {
		goto out1;
	}
	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, linkpb);
	if ((error = namei(&nd)) != 0)
		goto out2;
	if (nd.ni_vp) {
		error = EEXIST;
		goto abortop;
	}
	/* Prevent hard links on directories. */
	if (vp->v_type == VDIR) {
		error = EPERM;
		goto abortop;
	}
	/* Prevent cross-mount operation. */
	if (nd.ni_dvp->v_mount != vp->v_mount) {
		error = EXDEV;
		goto abortop;
	}
	error = VOP_LINK(nd.ni_dvp, vp, &nd.ni_cnd);
out2:
	pathbuf_destroy(linkpb);
out1:
	vrele(vp);
	return (error);
abortop:
	VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	if (nd.ni_dvp == nd.ni_vp)
		vrele(nd.ni_dvp);
	else
		vput(nd.ni_dvp);
	if (nd.ni_vp != NULL)
		vrele(nd.ni_vp);
	goto out2;
}

int
sys_link(struct lwp *l, const struct sys_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */
	const char *path = SCARG(uap, path);
	const char *link = SCARG(uap, link);

	return do_sys_link(l, path, link, 1, retval);
}

int
sys_linkat(struct lwp *l, const struct sys_linkat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd1;
		syscallarg(const char *) name1;
		syscallarg(int) fd2;
		syscallarg(const char *) name2;
		syscallarg(int) flags;
	} */
	const char *name1 = SCARG(uap, name1);
	const char *name2 = SCARG(uap, name2);
	int follow;

	/*
	 * Specified fd1 and fd2 are not yet implemented
	 */ 
	if ((SCARG(uap, fd1) != AT_FDCWD) || (SCARG(uap, fd2) != AT_FDCWD))
		return ENOSYS;

	follow = SCARG(uap, flags) & AT_SYMLINK_FOLLOW;

	return do_sys_link(l, name1, name2, follow, retval);
}


int
do_sys_symlink(const char *patharg, const char *link, enum uio_seg seg)
{
	struct proc *p = curproc;
	struct vattr vattr;
	char *path;
	int error;
	struct pathbuf *linkpb;
	struct nameidata nd;

	path = PNBUF_GET();
	if (seg == UIO_USERSPACE) {
		if ((error = copyinstr(patharg, path, MAXPATHLEN, NULL)) != 0)
			goto out1;
		if ((error = pathbuf_copyin(link, &linkpb)) != 0)
			goto out1;
	} else {
		KASSERT(strlen(patharg) < MAXPATHLEN);
		strcpy(path, patharg);
		linkpb = pathbuf_create(link);
		if (linkpb == NULL) {
			error = ENOMEM;
			goto out1;
		}
	}
	ktrkuser("symlink-target", path, strlen(path));

	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, linkpb);
	if ((error = namei(&nd)) != 0)
		goto out2;
	if (nd.ni_vp) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		error = EEXIST;
		goto out2;
	}
	vattr_null(&vattr);
	vattr.va_type = VLNK;
	/* We will read cwdi->cwdi_cmask unlocked. */
	vattr.va_mode = ACCESSPERMS &~ p->p_cwdi->cwdi_cmask;
	error = VOP_SYMLINK(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr, path);
	if (error == 0)
		vput(nd.ni_vp);
out2:
	pathbuf_destroy(linkpb);
out1:
	PNBUF_PUT(path);
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

	return do_sys_symlink(SCARG(uap, path), SCARG(uap, link),
	    UIO_USERSPACE);
}

int
sys_symlinkat(struct lwp *l, const struct sys_symlinkat_args *uap, 
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */

	return ENOSYS;
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
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}

	NDINIT(&nd, DELETE, LOCKPARENT | DOWHITEOUT | TRYEMULROOT, pb);
	error = namei(&nd);
	if (error) {
		pathbuf_destroy(pb);
		return (error);
	}

	if (nd.ni_vp != NULLVP || !(nd.ni_cnd.cn_flags & ISWHITEOUT)) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == nd.ni_vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		if (nd.ni_vp)
			vrele(nd.ni_vp);
		pathbuf_destroy(pb);
		return (EEXIST);
	}
	if ((error = VOP_WHITEOUT(nd.ni_dvp, &nd.ni_cnd, DELETE)) != 0)
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	vput(nd.ni_dvp);
	pathbuf_destroy(pb);
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
sys_unlinkat(struct lwp *l, const struct sys_unlinkat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
	} */

	return ENOSYS;
}

int
do_sys_unlink(const char *arg, enum uio_seg seg)
{
	struct vnode *vp;
	int error;
	struct pathbuf *pb;
	struct nameidata nd;
	const char *pathstring;

	error = pathbuf_maybe_copyin(arg, seg, &pb);
	if (error) {
		return error;
	}
	pathstring = pathbuf_stringcopy_get(pb);
	if (pathstring == NULL) {
		pathbuf_destroy(pb);
		return ENOMEM;
	}

	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF | TRYEMULROOT, pb);
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
	if ((error = veriexec_removechk(curlwp, nd.ni_vp, pathstring)) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vput(vp);
		goto out;
	}
#endif /* NVERIEXEC > 0 */
	
#ifdef FILEASSOC
	(void)fileassoc_file_delete(vp);
#endif /* FILEASSOC */
	error = VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
out:
	pathbuf_stringcopy_put(pb, pathstring);
	pathbuf_destroy(pb);
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
	kauth_cred_t cred = l->l_cred;
	file_t *fp;
	struct vnode *vp;
	struct vattr vattr;
	off_t newoff;
	int error, fd;

	fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	vp = fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO) {
		error = ESPIPE;
		goto out;
	}

	switch (SCARG(uap, whence)) {
	case SEEK_CUR:
		newoff = fp->f_offset + SCARG(uap, offset);
		break;
	case SEEK_END:
		vn_lock(vp, LK_SHARED | LK_RETRY);
		error = VOP_GETATTR(vp, &vattr, cred);
		VOP_UNLOCK(vp);
		if (error) {
			goto out;
		}
		newoff = SCARG(uap, offset) + vattr.va_size;
		break;
	case SEEK_SET:
		newoff = SCARG(uap, offset);
		break;
	default:
		error = EINVAL;
		goto out;
	}
	if ((error = VOP_SEEK(vp, fp->f_offset, newoff, cred)) == 0) {
		*(off_t *)retval = fp->f_offset = newoff;
	}
 out:
 	fd_putfile(fd);
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
	file_t *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FREAD) == 0) {
		fd_putfile(fd);
		return (EBADF);
	}

	vp = fp->f_data;
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
	fd_putfile(fd);
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
	file_t *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	if ((fp->f_flag & FWRITE) == 0) {
		fd_putfile(fd);
		return (EBADF);
	}

	vp = fp->f_data;
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
	fd_putfile(fd);
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
	struct pathbuf *pb;
	struct nameidata nd;

	CTASSERT(F_OK == 0);
	if ((SCARG(uap, flags) & ~(R_OK | W_OK | X_OK)) != 0) {
		/* nonsense flags */
		return EINVAL;
	}

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);

	/* Override default credentials */
	cred = kauth_cred_dup(l->l_cred);
	kauth_cred_seteuid(cred, kauth_cred_getuid(l->l_cred));
	kauth_cred_setegid(cred, kauth_cred_getgid(l->l_cred));
	nd.ni_cnd.cn_cred = cred;

	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		goto out;
	}
	vp = nd.ni_vp;
	pathbuf_destroy(pb);

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

int
sys_faccessat(struct lwp *l, const struct sys_faccessat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) amode;
		syscallarg(int) flag;
	} */

	return ENOSYS;
}

/*
 * Common code for all sys_stat functions, including compat versions.
 */
int
do_sys_stat(const char *userpath, unsigned int nd_flags, struct stat *sb)
{
	int error;
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(userpath, &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, nd_flags | LOCKLEAF | TRYEMULROOT, pb);
	error = namei(&nd);
	if (error != 0) {
		pathbuf_destroy(pb);
		return error;
	}
	error = vn_stat(nd.ni_vp, sb);
	vput(nd.ni_vp);
	pathbuf_destroy(pb);
	return error;
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
sys___stat50(struct lwp *l, const struct sys___stat50_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */
	struct stat sb;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;
	return copyout(&sb, SCARG(uap, ub), sizeof(sb));
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
sys___lstat50(struct lwp *l, const struct sys___lstat50_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
	} */
	struct stat sb;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;
	return copyout(&sb, SCARG(uap, ub), sizeof(sb));
}

int
sys_fstatat(struct lwp *l, const struct sys_fstatat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(struct stat *) ub;
		syscallarg(int) flag;
	} */

	return ENOSYS;
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
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return (error);
	}
	error = VOP_PATHCONF(nd.ni_vp, SCARG(uap, name), retval);
	vput(nd.ni_vp);
	pathbuf_destroy(pb);
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
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, NOFOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return error;
	}
	vp = nd.ni_vp;
	pathbuf_destroy(pb);
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

int
sys_readlinkat(struct lwp *l, const struct sys_readlinkat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(char *) buf;
		syscallarg(size_t) count;
	} */

	return ENOSYS;
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

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
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
	struct vnode *vp;
	file_t *fp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = fp->f_data;
	error = change_flags(vp, SCARG(uap, flags), l);
	VOP_UNLOCK(vp);
	fd_putfile(SCARG(uap, fd));
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

	error = namei_simple_user(SCARG(uap, path),
				NSM_NOFOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
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

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	vattr_null(&vattr);
	vattr.va_flags = flags;
	error = VOP_SETATTR(vp, &vattr, l->l_cred);

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
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);

	error = change_mode(vp, SCARG(uap, mode), l);

	vrele(vp);
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
	file_t *fp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	error = change_mode(fp->f_data, SCARG(uap, mode), l);
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
sys_fchmodat(struct lwp *l, const struct sys_fchmodat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarg(int) flag;
	} */

	return ENOSYS;
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
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_NOFOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);

	error = change_mode(vp, SCARG(uap, mode), l);

	vrele(vp);
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

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vattr_null(&vattr);
	vattr.va_mode = mode & ALLPERMS;
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
	VOP_UNLOCK(vp);
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
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);

	error = change_owner(vp, SCARG(uap, uid), SCARG(uap, gid), l, 0);

	vrele(vp);
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
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);

	error = change_owner(vp, SCARG(uap, uid), SCARG(uap, gid), l, 1);

	vrele(vp);
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
	int error;
	file_t *fp;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	error = change_owner(fp->f_data, SCARG(uap, uid), SCARG(uap, gid),
	    l, 0);
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
sys_fchownat(struct lwp *l, const struct sys_fchownat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
		syscallarg(int) flag;
	} */

	return ENOSYS;
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
	int error;
	file_t *fp;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	error = change_owner(fp->f_data, SCARG(uap, uid), SCARG(uap, gid),
	    l, 1);
	fd_putfile(SCARG(uap, fd));
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
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_NOFOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);

	error = change_owner(vp, SCARG(uap, uid), SCARG(uap, gid), l, 0);

	vrele(vp);
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
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_NOFOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);

	error = change_owner(vp, SCARG(uap, uid), SCARG(uap, gid), l, 1);

	vrele(vp);
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
		if (vattr.va_mode & S_ISUID) {
			if (kauth_authorize_vnode(l->l_cred,
			    KAUTH_VNODE_RETAIN_SUID, vp, NULL, EPERM) != 0)
				newmode &= ~S_ISUID;
		}
		if (vattr.va_mode & S_ISGID) {
			if (kauth_authorize_vnode(l->l_cred,
			    KAUTH_VNODE_RETAIN_SGID, vp, NULL, EPERM) != 0)
				newmode &= ~S_ISGID;
		}
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

	vattr_null(&vattr);
	vattr.va_uid = CHANGED(uid) ? uid : (uid_t)VNOVAL;
	vattr.va_gid = CHANGED(gid) ? gid : (gid_t)VNOVAL;
	vattr.va_mode = newmode;
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
#undef CHANGED

out:
	VOP_UNLOCK(vp);
	return (error);
}

/*
 * Set the access and modification times given a path name; this
 * version follows links.
 */
/* ARGSUSED */
int
sys___utimes50(struct lwp *l, const struct sys___utimes50_args *uap,
    register_t *retval)
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
sys___futimes50(struct lwp *l, const struct sys___futimes50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct timeval *) tptr;
	} */
	int error;
	file_t *fp;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	error = do_sys_utimes(l, fp->f_data, NULL, 0, SCARG(uap, tptr),
	    UIO_USERSPACE);
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
sys_futimens(struct lwp *l, const struct sys_futimens_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct timespec *) tptr;
	} */
	int error;
	file_t *fp;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	error = do_sys_utimens(l, fp->f_data, NULL, 0, SCARG(uap, tptr),
	    UIO_USERSPACE);
	fd_putfile(SCARG(uap, fd));
	return (error);
}

/*
 * Set the access and modification times given a path name; this
 * version does not follow links.
 */
int
sys___lutimes50(struct lwp *l, const struct sys___lutimes50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval *) tptr;
	} */

	return do_sys_utimes(l, NULL, SCARG(uap, path), NOFOLLOW,
	    SCARG(uap, tptr), UIO_USERSPACE);
}

int
sys_utimensat(struct lwp *l, const struct sys_utimensat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(const struct timespec *) tptr;
		syscallarg(int) flag;
	} */
	int follow;
	const struct timespec *tptr;

	/*
	 * Specified fd is not yet implemented
	 */ 
	if (SCARG(uap, fd) != AT_FDCWD)
		return ENOSYS;

	tptr = SCARG(uap, tptr);
	follow = (SCARG(uap, flag) & AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;

	return do_sys_utimens(l, NULL, SCARG(uap, path), follow,
	    tptr, UIO_USERSPACE);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
int
do_sys_utimens(struct lwp *l, struct vnode *vp, const char *path, int flag,
    const struct timespec *tptr, enum uio_seg seg)
{
	struct vattr vattr;
	int error, dorele = 0;
	namei_simple_flags_t sflags;

	bool vanull, setbirthtime;
	struct timespec ts[2];

	/* 
	 * I have checked all callers and they pass either FOLLOW,
	 * NOFOLLOW, or 0 (when they don't pass a path), and NOFOLLOW
	 * is 0. More to the point, they don't pass anything else.
	 * Let's keep it that way at least until the namei interfaces
	 * are fully sanitized.
	 */
	KASSERT(flag == NOFOLLOW || flag == FOLLOW);
	sflags = (flag == FOLLOW) ? 
		NSM_FOLLOW_TRYEMULROOT : NSM_NOFOLLOW_TRYEMULROOT;

	if (tptr == NULL) {
		vanull = true;
		nanotime(&ts[0]);
		ts[1] = ts[0];
	} else {
		vanull = false;
		if (seg != UIO_SYSSPACE) {
			error = copyin(tptr, ts, sizeof (ts));
			if (error != 0)
				return error;
		} else {
			ts[0] = tptr[0];
			ts[1] = tptr[1];
		}
	}

	if (ts[0].tv_nsec == UTIME_NOW) {
		nanotime(&ts[0]);
		if (ts[1].tv_nsec == UTIME_NOW) {
			vanull = true;
			ts[1] = ts[0];
		}
	} else if (ts[1].tv_nsec == UTIME_NOW)
		nanotime(&ts[1]);

	if (vp == NULL) {
		/* note: SEG describes TPTR, not PATH; PATH is always user */
		error = namei_simple_user(path, sflags, &vp);
		if (error != 0)
			return error;
		dorele = 1;
	}

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	setbirthtime = (VOP_GETATTR(vp, &vattr, l->l_cred) == 0 &&
	    timespeccmp(&ts[1], &vattr.va_birthtime, <));
	vattr_null(&vattr);

	if (ts[0].tv_nsec != UTIME_OMIT)
		vattr.va_atime = ts[0];

	if (ts[1].tv_nsec != UTIME_OMIT) {
		vattr.va_mtime = ts[1];
		if (setbirthtime)
			vattr.va_birthtime = ts[1];
	}

	if (vanull)
		vattr.va_vaflags |= VA_UTIMES_NULL;
	error = VOP_SETATTR(vp, &vattr, l->l_cred);
	VOP_UNLOCK(vp);

	if (dorele != 0)
		vrele(vp);

	return error;
}

int
do_sys_utimes(struct lwp *l, struct vnode *vp, const char *path, int flag,
    const struct timeval *tptr, enum uio_seg seg)
{
	struct timespec ts[2];
	struct timespec *tsptr = NULL;
	int error;
	
	if (tptr != NULL) {
		struct timeval tv[2];

		if (seg != UIO_SYSSPACE) {
			error = copyin(tptr, tv, sizeof (tv));
			if (error != 0)
				return error;
			tptr = tv;
		}

		if ((tv[0].tv_usec == UTIME_NOW) || 
		    (tv[0].tv_usec == UTIME_OMIT))
			ts[0].tv_nsec = tv[0].tv_usec;
		else
			TIMEVAL_TO_TIMESPEC(&tptr[0], &ts[0]);

		if ((tv[1].tv_usec == UTIME_NOW) || 
		    (tv[1].tv_usec == UTIME_OMIT))
			ts[1].tv_nsec = tv[1].tv_usec;
		else
			TIMEVAL_TO_TIMESPEC(&tptr[1], &ts[1]);

		tsptr = &ts[0];	
	}

	return do_sys_utimens(l, vp, path, flag, tsptr, UIO_SYSSPACE);
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

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0 &&
	    (error = VOP_ACCESS(vp, VWRITE, l->l_cred)) == 0) {
		vattr_null(&vattr);
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
	struct vattr vattr;
	struct vnode *vp;
	file_t *fp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FWRITE) == 0) {
		error = EINVAL;
		goto out;
	}
	vp = fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (vp->v_type == VDIR)
		error = EISDIR;
	else if ((error = vn_writechk(vp)) == 0) {
		vattr_null(&vattr);
		vattr.va_size = SCARG(uap, length);
		error = VOP_SETATTR(vp, &vattr, fp->f_cred);
	}
	VOP_UNLOCK(vp);
 out:
	fd_putfile(SCARG(uap, fd));
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
	struct vnode *vp;
	file_t *fp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	vp = fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT, 0, 0);
	VOP_UNLOCK(vp);
	fd_putfile(SCARG(uap, fd));
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
	struct vnode *vp;
	file_t *fp;
	int flags, nflags;
	off_t s, e, len;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
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
	/* If length == 0, we do the whole file, and s = e = 0 will do that */
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

	vp = fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, nflags, s, e);
	VOP_UNLOCK(vp);
out:
	fd_putfile(SCARG(uap, fd));
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
	struct vnode *vp;
	file_t *fp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FWRITE) == 0) {
		fd_putfile(SCARG(uap, fd));
		return (EBADF);
	}
	vp = fp->f_data;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, FSYNC_WAIT|FSYNC_DATAONLY, 0, 0);
	VOP_UNLOCK(vp);
	fd_putfile(SCARG(uap, fd));
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

int
sys_renameat(struct lwp *l, const struct sys_renameat_args *uap, 
    register_t *retval)
{
	/* {
		syscallarg(int) fromfd;
		syscallarg(const char *) from;
		syscallarg(int) tofd;
		syscallarg(const char *) to;
	} */

	return ENOSYS;
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
	struct pathbuf *frompb, *topb;
	struct nameidata fromnd, tond;
	struct mount *fs;
	int error;

	error = pathbuf_maybe_copyin(from, seg, &frompb);
	if (error) {
		return error;
	}
	error = pathbuf_maybe_copyin(to, seg, &topb);
	if (error) {
		pathbuf_destroy(frompb);
		return error;
	}

	NDINIT(&fromnd, DELETE, LOCKPARENT | TRYEMULROOT | INRENAME,
	    frompb);
	if ((error = namei(&fromnd)) != 0) {
		pathbuf_destroy(frompb);
		pathbuf_destroy(topb);
		return (error);
	}
	if (fromnd.ni_dvp != fromnd.ni_vp)
		VOP_UNLOCK(fromnd.ni_dvp);
	fvp = fromnd.ni_vp;

	fs = fvp->v_mount;
	error = VFS_RENAMELOCK_ENTER(fs);
	if (error) {
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
		goto out1;
	}

	/*
	 * close, partially, yet another race - ideally we should only
	 * go as far as getting fromnd.ni_dvp before getting the per-fs
	 * lock, and then continue to get fromnd.ni_vp, but we can't do
	 * that with namei as it stands.
	 *
	 * This still won't prevent rmdir from nuking fromnd.ni_vp
	 * under us. The real fix is to get the locks in the right
	 * order and do the lookups in the right places, but that's a
	 * major rototill.
	 *
	 * Note: this logic (as well as this whole function) is cloned
	 * in nfs_serv.c. Proceed accordingly.
	 */
	vrele(fvp);
	if ((fromnd.ni_cnd.cn_namelen == 1 && 
	     fromnd.ni_cnd.cn_nameptr[0] == '.') ||
	    (fromnd.ni_cnd.cn_namelen == 2 && 
	     fromnd.ni_cnd.cn_nameptr[0] == '.' &&
	     fromnd.ni_cnd.cn_nameptr[1] == '.')) {
		error = EINVAL;
		VFS_RENAMELOCK_EXIT(fs);
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		goto out1;
	}
	vn_lock(fromnd.ni_dvp, LK_EXCLUSIVE | LK_RETRY);
	error = relookup(fromnd.ni_dvp, &fromnd.ni_vp, &fromnd.ni_cnd, 0);
	if (error) {
		VOP_UNLOCK(fromnd.ni_dvp);
		VFS_RENAMELOCK_EXIT(fs);
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		goto out1;
	}
	VOP_UNLOCK(fromnd.ni_vp);
	if (fromnd.ni_dvp != fromnd.ni_vp)
		VOP_UNLOCK(fromnd.ni_dvp);
	fvp = fromnd.ni_vp;

	NDINIT(&tond, RENAME,
	    LOCKPARENT | LOCKLEAF | NOCACHE | TRYEMULROOT
	      | INRENAME | (fvp->v_type == VDIR ? CREATEDIR : 0),
	    topb);
	if ((error = namei(&tond)) != 0) {
		VFS_RENAMELOCK_EXIT(fs);
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
		    !memcmp(fromnd.ni_cnd.cn_nameptr, tond.ni_cnd.cn_nameptr,
		          fromnd.ni_cnd.cn_namelen))
			error = -1;
	}
	/*
	 * Prevent cross-mount operation.
	 */
	if (error == 0) {
		if (tond.ni_dvp->v_mount != fromnd.ni_dvp->v_mount) {
			error = EXDEV;
		}
	}
#if NVERIEXEC > 0
	if (!error) {
		char *f1, *f2;
		size_t f1_len;
		size_t f2_len;

		f1_len = fromnd.ni_cnd.cn_namelen + 1;
		f1 = kmem_alloc(f1_len, KM_SLEEP);
		strlcpy(f1, fromnd.ni_cnd.cn_nameptr, f1_len);

		f2_len = tond.ni_cnd.cn_namelen + 1;
		f2 = kmem_alloc(f2_len, KM_SLEEP);
		strlcpy(f2, tond.ni_cnd.cn_nameptr, f2_len);

		error = veriexec_renamechk(curlwp, fvp, f1, tvp, f2);

		kmem_free(f1, f1_len);
		kmem_free(f2, f2_len);
	}
#endif /* NVERIEXEC > 0 */

out:
	if (!error) {
		error = VOP_RENAME(fromnd.ni_dvp, fromnd.ni_vp, &fromnd.ni_cnd,
				   tond.ni_dvp, tond.ni_vp, &tond.ni_cnd);
		VFS_RENAMELOCK_EXIT(fs);
	} else {
		VOP_ABORTOP(tond.ni_dvp, &tond.ni_cnd);
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VFS_RENAMELOCK_EXIT(fs);
		VOP_ABORTOP(fromnd.ni_dvp, &fromnd.ni_cnd);
		vrele(fromnd.ni_dvp);
		vrele(fvp);
	}
out1:
	pathbuf_destroy(frompb);
	pathbuf_destroy(topb);
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

	return do_sys_mkdir(SCARG(uap, path), SCARG(uap, mode), UIO_USERSPACE);
}

int
sys_mkdirat(struct lwp *l, const struct sys_mkdirat_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */

	return ENOSYS;
}


int
do_sys_mkdir(const char *path, mode_t mode, enum uio_seg seg)
{
	struct proc *p = curlwp->l_proc;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct pathbuf *pb;
	struct nameidata nd;

	/* XXX bollocks, should pass in a pathbuf */
	error = pathbuf_maybe_copyin(path, seg, &pb);
	if (error) {
		return error;
	}

	NDINIT(&nd, CREATE, LOCKPARENT | CREATEDIR | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return (error);
	}
	vp = nd.ni_vp;
	if (vp != NULL) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp)
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(vp);
		pathbuf_destroy(pb);
		return (EEXIST);
	}
	vattr_null(&vattr);
	vattr.va_type = VDIR;
	/* We will read cwdi->cwdi_cmask unlocked. */
	vattr.va_mode = (mode & ACCESSPERMS) &~ p->p_cwdi->cwdi_cmask;
	error = VOP_MKDIR(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	if (!error)
		vput(nd.ni_vp);
	pathbuf_destroy(pb);
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
	struct pathbuf *pb;
	struct nameidata nd;

	error = pathbuf_copyin(SCARG(uap, path), &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF | TRYEMULROOT, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return error;
	}
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
	if ((vp->v_vflag & VV_ROOT) != 0 || vp->v_mountedhere != NULL) {
		error = EBUSY;
		goto out;
	}
	error = VOP_RMDIR(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	pathbuf_destroy(pb);
	return (error);

out:
	VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
	if (nd.ni_dvp == vp)
		vrele(nd.ni_dvp);
	else
		vput(nd.ni_dvp);
	vput(vp);
	pathbuf_destroy(pb);
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
	file_t *fp;
	int error, done;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
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
	fd_putfile(SCARG(uap, fd));
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
	int error, fs_decision;

	vn_lock(vp, LK_SHARED | LK_RETRY);
	error = VOP_GETATTR(vp, &vattr, cred);
	VOP_UNLOCK(vp);
	if (error != 0)
		return error;
	fs_decision = (kauth_cred_geteuid(cred) == vattr.va_uid) ? 0 : EPERM;
	error = kauth_authorize_vnode(cred, KAUTH_VNODE_REVOKE, vp, NULL,
	    fs_decision);
	if (!error)
		VOP_REVOKE(vp, REVOKEALL);
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

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	error = dorevoke(vp, l->l_cred);
	vrele(vp);
	return (error);
}
