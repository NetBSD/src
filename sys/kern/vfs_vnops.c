/*	$NetBSD: vfs_vnops.c,v 1.135.2.4 2007/04/13 15:49:49 ad Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)vfs_vnops.c	8.14 (Berkeley) 6/15/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_vnops.c,v 1.135.2.4 2007/04/13 15:49:49 ad Exp $");

#include "fs_union.h"
#include "veriexec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/poll.h>
#include <sys/kauth.h>
#include <sys/syslog.h>

#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_readahead.h>

#ifdef UNION
#include <fs/union/union.h>
#endif

#if defined(LKM) || defined(UNION)
int (*vn_union_readdir_hook) (struct vnode **, struct file *, struct lwp *);
#endif

#if NVERIEXEC > 0
#include <sys/verified_exec.h>
#endif /* NVERIEXEC > 0 */

static int vn_read(struct file *fp, off_t *offset, struct uio *uio,
	    kauth_cred_t cred, int flags);
static int vn_write(struct file *fp, off_t *offset, struct uio *uio,
	    kauth_cred_t cred, int flags);
static int vn_closefile(struct file *fp, struct lwp *l);
static int vn_poll(struct file *fp, int events, struct lwp *l);
static int vn_fcntl(struct file *fp, u_int com, void *data, struct lwp *l);
static int vn_statfile(struct file *fp, struct stat *sb, struct lwp *l);
static int vn_ioctl(struct file *fp, u_long com, void *data, struct lwp *l);

const struct fileops vnops = {
	vn_read, vn_write, vn_ioctl, vn_fcntl, vn_poll,
	vn_statfile, vn_closefile, vn_kqfilter
};

/*
 * Common code for vnode open operations.
 * Check permissions, and call the VOP_OPEN or VOP_CREATE routine.
 */
int
vn_open(struct nameidata *ndp, int fmode, int cmode)
{
	struct vnode *vp;
	struct lwp *l = ndp->ni_cnd.cn_lwp;
	kauth_cred_t cred = l->l_cred;
	struct vattr va;
	int error;
	pathname_t pn = NULL;

	if (fmode & O_CREAT) {
		ndp->ni_cnd.cn_nameiop = CREATE;
		ndp->ni_cnd.cn_flags = LOCKPARENT | LOCKLEAF;
		if ((fmode & O_EXCL) == 0 &&
		    ((fmode & O_NOFOLLOW) == 0))
			ndp->ni_cnd.cn_flags |= FOLLOW;
	} else {
		ndp->ni_cnd.cn_nameiop = LOOKUP;
		ndp->ni_cnd.cn_flags = LOCKLEAF;
		if ((fmode & O_NOFOLLOW) == 0)
			ndp->ni_cnd.cn_flags |= FOLLOW;
	}
#if NVERIEXEC > 0
	error = pathname_get(ndp->ni_dirp, ndp->ni_segflg, &pn);
	if (error)
		goto bad2;
	ndp->ni_dirp = pathname_path(pn);
	ndp->ni_segflg = UIO_SYSSPACE;
#endif /* NVERIEXEC > 0 */
	error = namei(ndp);
	if (error)
		goto bad2;

	vp = ndp->ni_vp;

#if NVERIEXEC > 0
	error = veriexec_openchk(l, ndp->ni_vp, ndp->ni_dirp, fmode);
	if (error)
		goto bad;
#endif /* NVERIEXEC > 0 */

	if (fmode & O_CREAT) {
		if (ndp->ni_vp == NULL) {
			VATTR_NULL(&va);
			va.va_type = VREG;
			va.va_mode = cmode;
			if (fmode & O_EXCL)
				 va.va_vaflags |= VA_EXCLUSIVE;
			VOP_LEASE(ndp->ni_dvp, l, cred, LEASE_WRITE);
			error = VOP_CREATE(ndp->ni_dvp, &ndp->ni_vp,
					   &ndp->ni_cnd, &va);
			if (error)
				goto bad2;
			fmode &= ~O_TRUNC;
			vp = ndp->ni_vp;
		} else {
			VOP_ABORTOP(ndp->ni_dvp, &ndp->ni_cnd);
			if (ndp->ni_dvp == ndp->ni_vp)
				vrele(ndp->ni_dvp);
			else
				vput(ndp->ni_dvp);
			ndp->ni_dvp = NULL;
			vp = ndp->ni_vp;
			if (fmode & O_EXCL) {
				error = EEXIST;
				goto bad;
			}
			fmode &= ~O_CREAT;
		}
	} else {
		vp = ndp->ni_vp;
	}
	if (vp->v_type == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	if (ndp->ni_vp->v_type == VLNK) {
		error = EFTYPE;
		goto bad;
	}

	if ((fmode & O_CREAT) == 0) {
		if (fmode & FREAD) {
			if ((error = VOP_ACCESS(vp, VREAD, cred, l)) != 0)
				goto bad;
		}

		if (fmode & (FWRITE | O_TRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto bad;
			}
			if ((error = vn_writechk(vp)) != 0 ||
			    (error = VOP_ACCESS(vp, VWRITE, cred, l)) != 0)
				goto bad;
		}
	}

	if (fmode & O_TRUNC) {
		VOP_UNLOCK(vp, 0);			/* XXX */

		VOP_LEASE(vp, l, cred, LEASE_WRITE);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);	/* XXX */
		VATTR_NULL(&va);
		va.va_size = 0;
		error = VOP_SETATTR(vp, &va, cred, l);
		if (error != 0)
			goto bad;
	}
	if ((error = VOP_OPEN(vp, fmode, cred, l)) != 0)
		goto bad;
	if (vp->v_type == VREG &&
	    uvn_attach(vp, fmode & FWRITE ? VM_PROT_WRITE : 0) == NULL) {
		error = EIO;
		goto bad;
	}
	if (fmode & FWRITE) {
		mutex_enter(&vp->v_interlock);
		vp->v_writecount++;
		mutex_exit(&vp->v_interlock);
	}

bad:
	if (error)
		vput(vp);

bad2:
	pathname_put(pn);

	return (error);
}

/*
 * Check for write permissions on the specified vnode.
 * Prototype text segments cannot be written.
 */
int
vn_writechk(struct vnode *vp)
{

	/*
	 * If the vnode is in use as a process's text,
	 * we can't allow writing.
	 */
	if (vp->v_flag & VTEXT)
		return (ETXTBSY);
	return (0);
}

/*
 * Mark a vnode as having executable mappings.
 */
void
vn_markexec(struct vnode *vp)
{

	KASSERT(mutex_owned(&vp->v_interlock));

	if ((vp->v_flag & VEXECMAP) == 0) {
		/* XXXSMP should be atomic */
		uvmexp.filepages -= vp->v_uobj.uo_npages;
		uvmexp.execpages += vp->v_uobj.uo_npages;
	}
	vp->v_flag |= VEXECMAP;
}

/*
 * Mark a vnode as being the text of a process.
 * Fail if the vnode is currently writable.
 */
int
vn_marktext(struct vnode *vp)
{

	mutex_enter(&vp->v_interlock);
	if (vp->v_writecount != 0) {
		KASSERT((vp->v_flag & VTEXT) == 0);
		mutex_exit(&vp->v_interlock);
		return (ETXTBSY);
	}
	vp->v_flag |= VTEXT;
	vn_markexec(vp);
	mutex_exit(&vp->v_interlock);
	return (0);
}

/*
 * Vnode close call
 *
 * Note: takes an unlocked vnode, while VOP_CLOSE takes a locked node.
 */
int
vn_close(struct vnode *vp, int flags, kauth_cred_t cred, struct lwp *l)
{
	int error;

	mutex_enter(&vp->v_interlock);
	if (flags & FWRITE)
		vp->v_writecount--;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY | LK_INTERLOCK);
	error = VOP_CLOSE(vp, flags, cred, l);
	vput(vp);
	return (error);
}

/*
 * Package up an I/O request on a vnode into a uio and do it.
 */
int
vn_rdwr(enum uio_rw rw, struct vnode *vp, void *base, int len, off_t offset,
    enum uio_seg segflg, int ioflg, kauth_cred_t cred, size_t *aresid,
    struct lwp *l)
{
	struct uio auio;
	struct iovec aiov;
	int error;

	if ((ioflg & IO_NODELOCKED) == 0) {
		if (rw == UIO_READ) {
			vn_lock(vp, LK_SHARED | LK_RETRY);
		} else /* UIO_WRITE */ {
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		}
	}
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_resid = len;
	auio.uio_offset = offset;
	auio.uio_rw = rw;
	if (segflg == UIO_SYSSPACE) {
		UIO_SETUP_SYSSPACE(&auio);
	} else {
		auio.uio_vmspace = l->l_proc->p_vmspace;
	}
	if (rw == UIO_READ) {
		error = VOP_READ(vp, &auio, ioflg, cred);
	} else {
		error = VOP_WRITE(vp, &auio, ioflg, cred);
	}
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid && error == 0)
			error = EIO;
	if ((ioflg & IO_NODELOCKED) == 0) {
		VOP_UNLOCK(vp, 0);
	}
	return (error);
}

int
vn_readdir(struct file *fp, char *bf, int segflg, u_int count, int *done,
    struct lwp *l, off_t **cookies, int *ncookies)
{
	struct vnode *vp = (struct vnode *)fp->f_data;
	struct iovec aiov;
	struct uio auio;
	int error, eofflag;

	/* Limit the size on any kernel buffers used by VOP_READDIR */
	count = min(MAXBSIZE, count);

unionread:
	if (vp->v_type != VDIR)
		return (EINVAL);
	aiov.iov_base = bf;
	aiov.iov_len = count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	if (segflg == UIO_SYSSPACE) {
		UIO_SETUP_SYSSPACE(&auio);
	} else {
		KASSERT(l == curlwp);
		auio.uio_vmspace = l->l_proc->p_vmspace;
	}
	auio.uio_resid = count;
	vn_lock(vp, LK_SHARED | LK_RETRY);
	auio.uio_offset = fp->f_offset;
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, cookies,
		    ncookies);
	mutex_enter(&fp->f_lock);
	fp->f_offset = auio.uio_offset;
	mutex_exit(&fp->f_lock);
	VOP_UNLOCK(vp, 0);
	if (error)
		return (error);

#if defined(UNION) || defined(LKM)
	if (count == auio.uio_resid && vn_union_readdir_hook) {
		struct vnode *ovp = vp;

		error = (*vn_union_readdir_hook)(&vp, fp, l);
		if (error)
			return (error);
		if (vp != ovp)
			goto unionread;
	}
#endif /* UNION || LKM */

	if (count == auio.uio_resid && (vp->v_flag & VROOT) &&
	    (vp->v_mount->mnt_flag & MNT_UNION)) {
		struct vnode *tvp = vp;
		vp = vp->v_mount->mnt_vnodecovered;
		VREF(vp);
		mutex_enter(&fp->f_lock);
		fp->f_data = vp;
		fp->f_offset = 0;
		mutex_exit(&fp->f_lock);
		vrele(tvp);
		goto unionread;
	}
	*done = count - auio.uio_resid;
	return error;
}

/*
 * File table vnode read routine.
 */
static int
vn_read(struct file *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct vnode *vp = (struct vnode *)fp->f_data;
	int count, error, ioflag;
	struct lwp *l = curlwp;

	VOP_LEASE(vp, l, cred, LEASE_READ);
	mutex_enter(&fp->f_lock);
	ioflag = IO_ADV_ENCODE(fp->f_advice);
	if (fp->f_flag & FNONBLOCK)
		ioflag |= IO_NDELAY;
	if ((fp->f_flag & (FFSYNC | FRSYNC)) == (FFSYNC | FRSYNC))
		ioflag |= IO_SYNC;
	if (fp->f_flag & FALTIO)
		ioflag |= IO_ALTSEMANTICS;
	if (fp->f_flag & FDIRECT)
		ioflag |= IO_DIRECT;
	mutex_exit(&fp->f_lock);
	vn_lock(vp, LK_SHARED | LK_RETRY);
	uio->uio_offset = *offset;
	count = uio->uio_resid;
	error = VOP_READ(vp, uio, ioflag, cred);
	if (flags & FOF_UPDATE_OFFSET)
		*offset += count - uio->uio_resid;
	VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * File table vnode write routine.
 */
static int
vn_write(struct file *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct vnode *vp = (struct vnode *)fp->f_data;
	int count, error, ioflag = IO_UNIT;
	struct lwp *l = curlwp;

	mutex_enter(&fp->f_lock);
	if (vp->v_type == VREG && (fp->f_flag & O_APPEND))
		ioflag |= IO_APPEND;
	if (fp->f_flag & FNONBLOCK)
		ioflag |= IO_NDELAY;
	if (fp->f_flag & FFSYNC ||
	    (vp->v_mount && (vp->v_mount->mnt_flag & MNT_SYNCHRONOUS)))
		ioflag |= IO_SYNC;
	else if (fp->f_flag & FDSYNC)
		ioflag |= IO_DSYNC;
	if (fp->f_flag & FALTIO)
		ioflag |= IO_ALTSEMANTICS;
	if (fp->f_flag & FDIRECT)
		ioflag |= IO_DIRECT;
	mutex_exit(&fp->f_lock);
	VOP_LEASE(vp, l, cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	uio->uio_offset = *offset;
	count = uio->uio_resid;
	error = VOP_WRITE(vp, uio, ioflag, cred);
	if (flags & FOF_UPDATE_OFFSET) {
		if (ioflag & IO_APPEND)
			*offset = uio->uio_offset;
		else
			*offset += count - uio->uio_resid;
	}
	VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * File table vnode stat routine.
 */
static int
vn_statfile(struct file *fp, struct stat *sb, struct lwp *l)
{
	struct vnode *vp = (struct vnode *)fp->f_data;

	return vn_stat(vp, sb, l);
}

int
vn_stat(struct vnode *vp, struct stat *sb, struct lwp *l)
{
	struct vattr va;
	int error;
	mode_t mode;

	error = VOP_GETATTR(vp, &va, l->l_cred, l);
	if (error)
		return (error);
	/*
	 * Copy from vattr table
	 */
	sb->st_dev = va.va_fsid;
	sb->st_ino = va.va_fileid;
	mode = va.va_mode;
	switch (vp->v_type) {
	case VREG:
		mode |= S_IFREG;
		break;
	case VDIR:
		mode |= S_IFDIR;
		break;
	case VBLK:
		mode |= S_IFBLK;
		break;
	case VCHR:
		mode |= S_IFCHR;
		break;
	case VLNK:
		mode |= S_IFLNK;
		break;
	case VSOCK:
		mode |= S_IFSOCK;
		break;
	case VFIFO:
		mode |= S_IFIFO;
		break;
	default:
		return (EBADF);
	};
	sb->st_mode = mode;
	sb->st_nlink = va.va_nlink;
	sb->st_uid = va.va_uid;
	sb->st_gid = va.va_gid;
	sb->st_rdev = va.va_rdev;
	sb->st_size = va.va_size;
	sb->st_atimespec = va.va_atime;
	sb->st_mtimespec = va.va_mtime;
	sb->st_ctimespec = va.va_ctime;
	sb->st_birthtimespec = va.va_birthtime;
	sb->st_blksize = va.va_blocksize;
	sb->st_flags = va.va_flags;
	sb->st_gen = 0;
	sb->st_blocks = va.va_bytes / S_BLKSIZE;
	return (0);
}

/*
 * File table vnode fcntl routine.
 */
static int
vn_fcntl(struct file *fp, u_int com, void *data, struct lwp *l)
{
	struct vnode *vp = ((struct vnode *)fp->f_data);
	int error;

	error = VOP_FCNTL(vp, com, data, fp->f_flag, l->l_cred, l);
	return (error);
}

/*
 * File table vnode ioctl routine.
 */
static int
vn_ioctl(struct file *fp, u_long com, void *data, struct lwp *l)
{
	struct vnode *vp = ((struct vnode *)fp->f_data), *ovp;
	struct proc *p = l->l_proc;
	struct vattr vattr;
	int error;

	switch (vp->v_type) {

	case VREG:
	case VDIR:
		if (com == FIONREAD) {
			error = VOP_GETATTR(vp, &vattr, l->l_cred, l);
			if (error)
				return (error);
			*(int *)data = vattr.va_size - fp->f_offset;
			return (0);
		}
		if ((com == FIONWRITE) || (com == FIONSPACE)) {
			/*
			 * Files don't have send queues, so there never
			 * are any bytes in them, nor is there any
			 * open space in them.
			 */
			*(int *)data = 0;
			return (0);
		}
		if (com == FIOGETBMAP) {
			daddr_t *block;

			if (*(daddr_t *)data < 0)
				return (EINVAL);
			block = (daddr_t *)data;
			return (VOP_BMAP(vp, *block, NULL, block, NULL));
		}
		if (com == OFIOGETBMAP) {
			daddr_t ibn, obn;

			if (*(int32_t *)data < 0)
				return (EINVAL);
			ibn = (daddr_t)*(int32_t *)data;
			error = VOP_BMAP(vp, ibn, NULL, &obn, NULL);
			*(int32_t *)data = (int32_t)obn;
			return error;
		}
		if (com == FIONBIO || com == FIOASYNC)	/* XXX */
			return (0);			/* XXX */
		/* fall into ... */
	case VFIFO:
	case VCHR:
	case VBLK:
		error = VOP_IOCTL(vp, com, data, fp->f_flag,
		    l->l_cred, l);
		if (error == 0 && com == TIOCSCTTY) {
			VREF(vp);
			mutex_enter(&proclist_lock);
			ovp = p->p_session->s_ttyvp;
			p->p_session->s_ttyvp = vp;
			mutex_exit(&proclist_lock);
			if (ovp != NULL)
				vrele(ovp);
		}
		return (error);

	default:
		return (EPASSTHROUGH);
	}
}

/*
 * File table vnode poll routine.
 */
static int
vn_poll(struct file *fp, int events, struct lwp *l)
{

	return (VOP_POLL(((struct vnode *)fp->f_data), events, l));
}

/*
 * File table vnode kqfilter routine.
 */
int
vn_kqfilter(struct file *fp, struct knote *kn)
{

	return (VOP_KQFILTER((struct vnode *)fp->f_data, kn));
}

/*
 * Check that the vnode is still valid, and if so
 * acquire requested lock.
 */
int
vn_lock(struct vnode *vp, int flags)
{
	int error;

#if 0
	KASSERT(vp->v_usecount > 0 || (flags & LK_INTERLOCK) != 0
	    || (vp->v_flag & VONWORKLST) != 0);
#endif
	KASSERT((flags &
	    ~(LK_INTERLOCK|LK_SHARED|LK_EXCLUSIVE|LK_DRAIN|LK_NOWAIT|LK_RETRY|
	    LK_SETRECURSE|LK_CANRECURSE))
	    == 0);

	do {
		if ((flags & LK_INTERLOCK) == 0)
			mutex_enter(&vp->v_interlock);
		if (vp->v_flag & VXLOCK) {
			if (flags & LK_NOWAIT) {
				mutex_exit(&vp->v_interlock);
				return EBUSY;
			}
			vwait(vp, VXLOCK);
			mutex_exit(&vp->v_interlock);
			error = ENOENT;
		} else {
			error = VOP_LOCK(vp,
			    (flags & ~LK_RETRY) | LK_INTERLOCK);
			if (error == 0 || error == EDEADLK || error == EBUSY)
				return (error);
		}
		flags &= ~LK_INTERLOCK;
	} while (flags & LK_RETRY);
	return (error);
}

/*
 * File table vnode close routine.
 */
static int
vn_closefile(struct file *fp, struct lwp *l)
{

	return (vn_close(((struct vnode *)fp->f_data), fp->f_flag,
		fp->f_cred, l));
}

/*
 * Enable LK_CANRECURSE on lock. Return prior status.
 */
u_int
vn_setrecurse(struct vnode *vp)
{
	struct lock *lkp = &vp->v_lock;
	u_int retval;

	mutex_enter(&lkp->lk_interlock);
	retval = lkp->lk_flags & LK_CANRECURSE;
	lkp->lk_flags |= LK_CANRECURSE;
	mutex_exit(&lkp->lk_interlock);

	return retval;
}

/*
 * Called when done with locksetrecurse.
 */
void
vn_restorerecurse(struct vnode *vp, u_int flags)
{
	struct lock *lkp = &vp->v_lock;

	mutex_enter(&lkp->lk_interlock);
	lkp->lk_flags &= ~LK_CANRECURSE;
	lkp->lk_flags |= flags;
	mutex_exit(&lkp->lk_interlock);
}

int
vn_cow_establish(struct vnode *vp,
    int (*func)(void *, struct buf *), void *cookie)
{
	struct spec_cow_entry *e;

	MALLOC(e, struct spec_cow_entry *, sizeof(struct spec_cow_entry),
	    M_DEVBUF, M_WAITOK);
	e->ce_func = func;
	e->ce_cookie = cookie;

	mutex_enter(&vp->v_spec_cow_lock);
	vp->v_spec_cow_req++;
	while (vp->v_spec_cow_count > 0)
		mtsleep(&vp->v_spec_cow_req, PRIBIO, "cowlist", 0,
		    &vp->v_spec_cow_lock);

	SLIST_INSERT_HEAD(&vp->v_spec_cow_head, e, ce_list);

	vp->v_spec_cow_req--;
	if (vp->v_spec_cow_req == 0)
		wakeup(&vp->v_spec_cow_req);
	mutex_exit(&vp->v_spec_cow_lock);

	return 0;
}

int
vn_cow_disestablish(struct vnode *vp,
    int (*func)(void *, struct buf *), void *cookie)
{
	struct spec_cow_entry *e;

	mutex_enter(&vp->v_spec_cow_lock);
	vp->v_spec_cow_req++;
	while (vp->v_spec_cow_count > 0)
		mtsleep(&vp->v_spec_cow_req, PRIBIO, "cowlist", 0,
		    &vp->v_spec_cow_lock);

	SLIST_FOREACH(e, &vp->v_spec_cow_head, ce_list)
		if (e->ce_func == func && e->ce_cookie == cookie) {
			SLIST_REMOVE(&vp->v_spec_cow_head, e,
			    spec_cow_entry, ce_list);
			FREE(e, M_DEVBUF);
			break;
		}

	vp->v_spec_cow_req--;
	if (vp->v_spec_cow_req == 0)
		wakeup(&vp->v_spec_cow_req);
	mutex_exit(&vp->v_spec_cow_lock);

	return e ? 0 : EINVAL;
}

/*
 * Simplified in-kernel wrapper calls for extended attribute access.
 * Both calls pass in a NULL credential, authorizing a "kernel" access.
 * Set IO_NODELOCKED in ioflg if the vnode is already locked.
 */
int
vn_extattr_get(struct vnode *vp, int ioflg, int attrnamespace,
    const char *attrname, size_t *buflen, void *bf, struct lwp *l)
{
	struct uio auio;
	struct iovec aiov;
	int error;

	aiov.iov_len = *buflen;
	aiov.iov_base = bf;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_offset = 0;
	auio.uio_resid = *buflen;
	UIO_SETUP_SYSSPACE(&auio);

	if ((ioflg & IO_NODELOCKED) == 0)
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	error = VOP_GETEXTATTR(vp, attrnamespace, attrname, &auio, NULL, NULL,
	    l);

	if ((ioflg & IO_NODELOCKED) == 0)
		VOP_UNLOCK(vp, 0);

	if (error == 0)
		*buflen = *buflen - auio.uio_resid;

	return (error);
}

/*
 * XXX Failure mode if partially written?
 */
int
vn_extattr_set(struct vnode *vp, int ioflg, int attrnamespace,
    const char *attrname, size_t buflen, const void *bf, struct lwp *l)
{
	struct uio auio;
	struct iovec aiov;
	int error;

	aiov.iov_len = buflen;
	aiov.iov_base = __UNCONST(bf);		/* XXXUNCONST kills const */

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_WRITE;
	auio.uio_offset = 0;
	auio.uio_resid = buflen;
	UIO_SETUP_SYSSPACE(&auio);

	if ((ioflg & IO_NODELOCKED) == 0) {
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}

	error = VOP_SETEXTATTR(vp, attrnamespace, attrname, &auio, NULL, l);

	if ((ioflg & IO_NODELOCKED) == 0) {
		VOP_UNLOCK(vp, 0);
	}

	return (error);
}

int
vn_extattr_rm(struct vnode *vp, int ioflg, int attrnamespace,
    const char *attrname, struct lwp *l)
{
	int error;

	if ((ioflg & IO_NODELOCKED) == 0) {
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}

	error = VOP_DELETEEXTATTR(vp, attrnamespace, attrname, NULL, l);
	if (error == EOPNOTSUPP)
		error = VOP_SETEXTATTR(vp, attrnamespace, attrname, NULL,
		    NULL, l);

	if ((ioflg & IO_NODELOCKED) == 0) {
		VOP_UNLOCK(vp, 0);
	}

	return (error);
}

void
vn_ra_allocctx(struct vnode *vp)
{
	struct uvm_ractx *ra = NULL;

	if (vp->v_type != VREG) {
		return;
	}
	if (vp->v_ractx != NULL) {
		return;
	}
	mutex_enter(&vp->v_interlock);
	if (vp->v_ractx == NULL) {
		mutex_exit(&vp->v_interlock);
		ra = uvm_ra_allocctx();
		mutex_enter(&vp->v_interlock);
		if (ra != NULL && vp->v_ractx == NULL) {
			vp->v_ractx = ra;
			ra = NULL;
		}
	}
	mutex_exit(&vp->v_interlock);
	if (ra != NULL) {
		uvm_ra_freectx(ra);
	}
}
