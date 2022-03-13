/*	$NetBSD: vfs_vnops.c,v 1.225 2022/03/13 13:52:53 riastradh Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vfs_vnops.c,v 1.225 2022/03/13 13:52:53 riastradh Exp $");

#include "veriexec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/poll.h>
#include <sys/kauth.h>
#include <sys/syslog.h>
#include <sys/fstrans.h>
#include <sys/atomic.h>
#include <sys/filedesc.h>
#include <sys/wapbl.h>
#include <sys/mman.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_device.h>

#ifdef UNION
#include <fs/union/union.h>
#endif

#ifndef COMPAT_ZERODEV
#define COMPAT_ZERODEV(dev)	(0)
#endif

int (*vn_union_readdir_hook) (struct vnode **, struct file *, struct lwp *);

#include <sys/verified_exec.h>

static int vn_read(file_t *fp, off_t *offset, struct uio *uio,
	    kauth_cred_t cred, int flags);
static int vn_write(file_t *fp, off_t *offset, struct uio *uio,
	    kauth_cred_t cred, int flags);
static int vn_closefile(file_t *fp);
static int vn_poll(file_t *fp, int events);
static int vn_fcntl(file_t *fp, u_int com, void *data);
static int vn_statfile(file_t *fp, struct stat *sb);
static int vn_ioctl(file_t *fp, u_long com, void *data);
static int vn_mmap(struct file *, off_t *, size_t, int, int *, int *,
		   struct uvm_object **, int *);
static int vn_seek(struct file *, off_t, int, off_t *, int);

const struct fileops vnops = {
	.fo_name = "vn",
	.fo_read = vn_read,
	.fo_write = vn_write,
	.fo_ioctl = vn_ioctl,
	.fo_fcntl = vn_fcntl,
	.fo_poll = vn_poll,
	.fo_stat = vn_statfile,
	.fo_close = vn_closefile,
	.fo_kqfilter = vn_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = vn_mmap,
	.fo_seek = vn_seek,
};

/*
 * Common code for vnode open operations.
 * Check permissions, and call the VOP_OPEN or VOP_CREATE routine.
 *
 * at_dvp is the directory for openat(), if any.
 * pb is the path.
 * nmode is additional namei flags, restricted to TRYEMULROOT and NOCHROOT.
 * fmode is the open flags, converted from O_* to F*
 * cmode is the creation file permissions.
 *
 * XXX shouldn't cmode be mode_t?
 *
 * On success produces either a vnode in *ret_vp, or if that is NULL,
 * a file descriptor number in ret_fd.
 *
 * The caller may pass NULL for ret_fd (and ret_domove), in which case
 * EOPNOTSUPP will be produced in the cases that would otherwise return
 * a file descriptor.
 *
 * Note that callers that want no-follow behavior should pass
 * O_NOFOLLOW in fmode. Neither FOLLOW nor NOFOLLOW in nmode is
 * honored.
 */
int
vn_open(struct vnode *at_dvp, struct pathbuf *pb,
	int nmode, int fmode, int cmode,
	struct vnode **ret_vp, bool *ret_domove, int *ret_fd)
{
	struct nameidata nd;
	struct vnode *vp = NULL;
	struct lwp *l = curlwp;
	kauth_cred_t cred = l->l_cred;
	struct vattr va;
	int error;
	const char *pathstring;

	KASSERT((nmode & (TRYEMULROOT | NOCHROOT)) == nmode);

	KASSERT(ret_vp != NULL);
	KASSERT((ret_domove == NULL) == (ret_fd == NULL));

	if ((fmode & (O_CREAT | O_DIRECTORY)) == (O_CREAT | O_DIRECTORY))
		return EINVAL;

	NDINIT(&nd, LOOKUP, nmode, pb);
	if (at_dvp != NULL)
		NDAT(&nd, at_dvp);

	nd.ni_cnd.cn_flags &= TRYEMULROOT | NOCHROOT;

	if (fmode & O_CREAT) {
		nd.ni_cnd.cn_nameiop = CREATE;
		nd.ni_cnd.cn_flags |= LOCKPARENT | LOCKLEAF;
		if ((fmode & O_EXCL) == 0 &&
		    ((fmode & O_NOFOLLOW) == 0))
			nd.ni_cnd.cn_flags |= FOLLOW;
		if ((fmode & O_EXCL) == 0)
			nd.ni_cnd.cn_flags |= NONEXCLHACK;
	} else {
		nd.ni_cnd.cn_nameiop = LOOKUP;
		nd.ni_cnd.cn_flags |= LOCKLEAF;
		if ((fmode & O_NOFOLLOW) == 0)
			nd.ni_cnd.cn_flags |= FOLLOW;
	}

	pathstring = pathbuf_stringcopy_get(nd.ni_pathbuf);
	if (pathstring == NULL) {
		return ENOMEM;
	}

	/*
	 * When this "interface" was exposed to do_open() it used
	 * to initialize l_dupfd to -newfd-1 (thus passing in the
	 * new file handle number to use)... but nothing in the
	 * kernel uses that value. So just send 0.
	 */
	l->l_dupfd = 0;

	error = namei(&nd);
	if (error)
		goto out;

	vp = nd.ni_vp;

#if NVERIEXEC > 0
	error = veriexec_openchk(l, nd.ni_vp, pathstring, fmode);
	if (error) {
		/* We have to release the locks ourselves */
		/*
		 * 20210604 dholland passing NONEXCLHACK means we can
		 * get ni_dvp == NULL back if ni_vp exists, and we should
		 * treat that like the non-O_CREAT case.
		 */
		if ((fmode & O_CREAT) != 0 && nd.ni_dvp != NULL) {
			if (vp == NULL) {
				vput(nd.ni_dvp);
			} else {
				VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
				if (nd.ni_dvp == nd.ni_vp)
					vrele(nd.ni_dvp);
				else
					vput(nd.ni_dvp);
				nd.ni_dvp = NULL;
				vput(vp);
			}
		} else {
			vput(vp);
		}
		goto out;
	}
#endif /* NVERIEXEC > 0 */

	/*
	 * 20210604 dholland ditto
	 */
	if ((fmode & O_CREAT) != 0 && nd.ni_dvp != NULL) {
		if (nd.ni_vp == NULL) {
			vattr_null(&va);
			va.va_type = VREG;
			va.va_mode = cmode;
			if (fmode & O_EXCL)
				 va.va_vaflags |= VA_EXCLUSIVE;
			error = VOP_CREATE(nd.ni_dvp, &nd.ni_vp,
					   &nd.ni_cnd, &va);
			if (error) {
				vput(nd.ni_dvp);
				goto out;
			}
			fmode &= ~O_TRUNC;
			vp = nd.ni_vp;
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			vput(nd.ni_dvp);
		} else {
			VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
			if (nd.ni_dvp == nd.ni_vp)
				vrele(nd.ni_dvp);
			else
				vput(nd.ni_dvp);
			nd.ni_dvp = NULL;
			vp = nd.ni_vp;
			if (fmode & O_EXCL) {
				error = EEXIST;
				goto bad;
			}
			fmode &= ~O_CREAT;
		}
	} else if ((fmode & O_CREAT) != 0) {
		/*
		 * 20210606 dholland passing NONEXCLHACK means this
		 * case exists; it is the same as the following one
		 * but also needs to do things in the second (exists)
		 * half of the following block. (Besides handle
		 * ni_dvp, anyway.)
		 */
		vp = nd.ni_vp;
		KASSERT((fmode & O_EXCL) == 0);
		fmode &= ~O_CREAT;
	} else {
		vp = nd.ni_vp;
	}
	if (vp->v_type == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	if (nd.ni_vp->v_type == VLNK) {
		error = EFTYPE;
		goto bad;
	}

	if ((fmode & O_CREAT) == 0) {
		error = vn_openchk(vp, cred, fmode);
		if (error != 0)
			goto bad;
	}

	if (fmode & O_TRUNC) {
		vattr_null(&va);
		va.va_size = 0;
		error = VOP_SETATTR(vp, &va, cred);
		if (error != 0)
			goto bad;
	}
	if ((error = VOP_OPEN(vp, fmode, cred)) != 0)
		goto bad;
	if (fmode & FWRITE) {
		mutex_enter(vp->v_interlock);
		vp->v_writecount++;
		mutex_exit(vp->v_interlock);
	}

bad:
	if (error)
		vput(vp);
out:
	pathbuf_stringcopy_put(nd.ni_pathbuf, pathstring);

	switch (error) {
	case EDUPFD:
	case EMOVEFD:
		/* if the caller isn't prepared to handle fds, fail for them */
		if (ret_fd == NULL) {
			error = EOPNOTSUPP;
			break;
		}
		*ret_vp = NULL;
		*ret_domove = error == EMOVEFD;
		*ret_fd = l->l_dupfd;
		error = 0;
		break;
	case 0:
		*ret_vp = vp;
		break;
	}
	l->l_dupfd = 0;
	return error;
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
	if (vp->v_iflag & VI_TEXT)
		return (ETXTBSY);
	return (0);
}

int
vn_openchk(struct vnode *vp, kauth_cred_t cred, int fflags)
{
	int permbits = 0;
	int error;

	if (vp->v_type == VNON || vp->v_type == VBAD)
		return ENXIO;

	if ((fflags & O_DIRECTORY) != 0 && vp->v_type != VDIR)
		return ENOTDIR;

	if ((fflags & O_REGULAR) != 0 && vp->v_type != VREG)
		return EFTYPE;

	if ((fflags & FREAD) != 0) {
		permbits = VREAD;
	}
	if ((fflags & FEXEC) != 0) {
		permbits |= VEXEC;
	}
	if ((fflags & (FWRITE | O_TRUNC)) != 0) {
		permbits |= VWRITE;
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto bad;
		}
		error = vn_writechk(vp);
		if (error != 0)
			goto bad;
	}
	error = VOP_ACCESS(vp, permbits, cred);
bad:
	return error;
}

/*
 * Mark a vnode as having executable mappings.
 */
void
vn_markexec(struct vnode *vp)
{

	if ((vp->v_iflag & VI_EXECMAP) != 0) {
		/* Safe unlocked, as long as caller holds a reference. */
		return;
	}

	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	mutex_enter(vp->v_interlock);
	if ((vp->v_iflag & VI_EXECMAP) == 0) {
		cpu_count(CPU_COUNT_EXECPAGES, vp->v_uobj.uo_npages);
		vp->v_iflag |= VI_EXECMAP;
	}
	mutex_exit(vp->v_interlock);
	rw_exit(vp->v_uobj.vmobjlock);
}

/*
 * Mark a vnode as being the text of a process.
 * Fail if the vnode is currently writable.
 */
int
vn_marktext(struct vnode *vp)
{

	if ((vp->v_iflag & (VI_TEXT|VI_EXECMAP)) == (VI_TEXT|VI_EXECMAP)) {
		/* Safe unlocked, as long as caller holds a reference. */
		return (0);
	}

	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	mutex_enter(vp->v_interlock);
	if (vp->v_writecount != 0) {
		KASSERT((vp->v_iflag & VI_TEXT) == 0);
		mutex_exit(vp->v_interlock);
		rw_exit(vp->v_uobj.vmobjlock);
		return (ETXTBSY);
	}
	if ((vp->v_iflag & VI_EXECMAP) == 0) {
		cpu_count(CPU_COUNT_EXECPAGES, vp->v_uobj.uo_npages);
	}
	vp->v_iflag |= (VI_TEXT | VI_EXECMAP);
	mutex_exit(vp->v_interlock);
	rw_exit(vp->v_uobj.vmobjlock);
	return (0);
}

/*
 * Vnode close call
 *
 * Note: takes an unlocked vnode, while VOP_CLOSE takes a locked node.
 */
int
vn_close(struct vnode *vp, int flags, kauth_cred_t cred)
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (flags & FWRITE) {
		mutex_enter(vp->v_interlock);
		KASSERT(vp->v_writecount > 0);
		vp->v_writecount--;
		mutex_exit(vp->v_interlock);
	}
	error = VOP_CLOSE(vp, flags, cred);
	vput(vp);
	return (error);
}

static int
enforce_rlimit_fsize(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct lwp *l = curlwp;
	off_t testoff;

	if (uio->uio_rw != UIO_WRITE || vp->v_type != VREG)
		return 0;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	if (ioflag & IO_APPEND)
		testoff = vp->v_size;
	else
		testoff = uio->uio_offset;

	if (testoff + uio->uio_resid >
	    l->l_proc->p_rlimit[RLIMIT_FSIZE].rlim_cur) {
		mutex_enter(&proc_lock);
		psignal(l->l_proc, SIGXFSZ);
		mutex_exit(&proc_lock);
		return EFBIG;
	}

	return 0;
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

	if ((error = enforce_rlimit_fsize(vp, &auio, ioflg)) != 0)
		goto out;

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

 out:
	if ((ioflg & IO_NODELOCKED) == 0) {
		VOP_UNLOCK(vp);
	}
	return (error);
}

int
vn_readdir(file_t *fp, char *bf, int segflg, u_int count, int *done,
    struct lwp *l, off_t **cookies, int *ncookies)
{
	struct vnode *vp = fp->f_vnode;
	struct iovec aiov;
	struct uio auio;
	int error, eofflag;

	/* Limit the size on any kernel buffers used by VOP_READDIR */
	count = uimin(MAXBSIZE, count);

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
	VOP_UNLOCK(vp);
	if (error)
		return (error);

	if (count == auio.uio_resid && vn_union_readdir_hook) {
		struct vnode *ovp = vp;

		error = (*vn_union_readdir_hook)(&vp, fp, l);
		if (error)
			return (error);
		if (vp != ovp)
			goto unionread;
	}

	if (count == auio.uio_resid && (vp->v_vflag & VV_ROOT) &&
	    (vp->v_mount->mnt_flag & MNT_UNION)) {
		struct vnode *tvp = vp;
		vp = vp->v_mount->mnt_vnodecovered;
		vref(vp);
		mutex_enter(&fp->f_lock);
		fp->f_vnode = vp;
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
vn_read(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct vnode *vp = fp->f_vnode;
	int error, ioflag, fflag;
	size_t count;

	ioflag = IO_ADV_ENCODE(fp->f_advice);
	fflag = fp->f_flag;
	if (fflag & FNONBLOCK)
		ioflag |= IO_NDELAY;
	if ((fflag & (FFSYNC | FRSYNC)) == (FFSYNC | FRSYNC))
		ioflag |= IO_SYNC;
	if (fflag & FALTIO)
		ioflag |= IO_ALTSEMANTICS;
	if (fflag & FDIRECT)
		ioflag |= IO_DIRECT;
	vn_lock(vp, LK_SHARED | LK_RETRY);
	uio->uio_offset = *offset;
	count = uio->uio_resid;
	error = VOP_READ(vp, uio, ioflag, cred);
	if (flags & FOF_UPDATE_OFFSET)
		*offset += count - uio->uio_resid;
	VOP_UNLOCK(vp);
	return (error);
}

/*
 * File table vnode write routine.
 */
static int
vn_write(file_t *fp, off_t *offset, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct vnode *vp = fp->f_vnode;
	int error, ioflag, fflag;
	size_t count;

	ioflag = IO_ADV_ENCODE(fp->f_advice) | IO_UNIT;
	fflag = fp->f_flag;
	if (vp->v_type == VREG && (fflag & O_APPEND))
		ioflag |= IO_APPEND;
	if (fflag & FNONBLOCK)
		ioflag |= IO_NDELAY;
	if (fflag & FFSYNC ||
	    (vp->v_mount && (vp->v_mount->mnt_flag & MNT_SYNCHRONOUS)))
		ioflag |= IO_SYNC;
	else if (fflag & FDSYNC)
		ioflag |= IO_DSYNC;
	if (fflag & FALTIO)
		ioflag |= IO_ALTSEMANTICS;
	if (fflag & FDIRECT)
		ioflag |= IO_DIRECT;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	uio->uio_offset = *offset;
	count = uio->uio_resid;

	if ((error = enforce_rlimit_fsize(vp, uio, ioflag)) != 0)
		goto out;

	error = VOP_WRITE(vp, uio, ioflag, cred);

	if (flags & FOF_UPDATE_OFFSET) {
		if (ioflag & IO_APPEND) {
			/*
			 * SUSv3 describes behaviour for count = 0 as following:
			 * "Before any action ... is taken, and if nbyte is zero
			 * and the file is a regular file, the write() function
			 * ... in the absence of errors ... shall return zero
			 * and have no other results."
			 */ 
			if (count)
				*offset = uio->uio_offset;
		} else
			*offset += count - uio->uio_resid;
	}

 out:
	VOP_UNLOCK(vp);
	return (error);
}

/*
 * File table vnode stat routine.
 */
static int
vn_statfile(file_t *fp, struct stat *sb)
{
	struct vnode *vp = fp->f_vnode;
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = vn_stat(vp, sb);
	VOP_UNLOCK(vp);
	return error;
}

int
vn_stat(struct vnode *vp, struct stat *sb)
{
	struct vattr va;
	int error;
	mode_t mode;

	memset(&va, 0, sizeof(va));
	error = VOP_GETATTR(vp, &va, kauth_cred_get());
	if (error)
		return (error);
	/*
	 * Copy from vattr table
	 */
	memset(sb, 0, sizeof(*sb));
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
	}
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
vn_fcntl(file_t *fp, u_int com, void *data)
{
	struct vnode *vp = fp->f_vnode;
	int error;

	error = VOP_FCNTL(vp, com, data, fp->f_flag, kauth_cred_get());
	return (error);
}

/*
 * File table vnode ioctl routine.
 */
static int
vn_ioctl(file_t *fp, u_long com, void *data)
{
	struct vnode *vp = fp->f_vnode, *ovp;
	struct vattr vattr;
	int error;

	switch (vp->v_type) {

	case VREG:
	case VDIR:
		if (com == FIONREAD) {
			vn_lock(vp, LK_SHARED | LK_RETRY);
			error = VOP_GETATTR(vp, &vattr, kauth_cred_get());
			if (error == 0)
				*(int *)data = vattr.va_size - fp->f_offset;
			VOP_UNLOCK(vp);
			if (error)
				return (error);
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
			vn_lock(vp, LK_SHARED | LK_RETRY);
			error = VOP_BMAP(vp, *block, NULL, block, NULL);
			VOP_UNLOCK(vp);
			return error;
		}
		if (com == OFIOGETBMAP) {
			daddr_t ibn, obn;

			if (*(int32_t *)data < 0)
				return (EINVAL);
			ibn = (daddr_t)*(int32_t *)data;
			vn_lock(vp, LK_SHARED | LK_RETRY);
			error = VOP_BMAP(vp, ibn, NULL, &obn, NULL);
			VOP_UNLOCK(vp);
			*(int32_t *)data = (int32_t)obn;
			return error;
		}
		if (com == FIONBIO || com == FIOASYNC)	/* XXX */
			return (0);			/* XXX */
		/* FALLTHROUGH */
	case VFIFO:
	case VCHR:
	case VBLK:
		error = VOP_IOCTL(vp, com, data, fp->f_flag,
		    kauth_cred_get());
		if (error == 0 && com == TIOCSCTTY) {
			vref(vp);
			mutex_enter(&proc_lock);
			ovp = curproc->p_session->s_ttyvp;
			curproc->p_session->s_ttyvp = vp;
			mutex_exit(&proc_lock);
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
vn_poll(file_t *fp, int events)
{

	return (VOP_POLL(fp->f_vnode, events));
}

/*
 * File table vnode kqfilter routine.
 */
int
vn_kqfilter(file_t *fp, struct knote *kn)
{

	return (VOP_KQFILTER(fp->f_vnode, kn));
}

static int
vn_mmap(struct file *fp, off_t *offp, size_t size, int prot, int *flagsp,
	int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct uvm_object *uobj;
	struct vnode *vp;
	struct vattr va;
	struct lwp *l;
	vm_prot_t maxprot;
	off_t off;
	int error, flags;
	bool needwritemap;

	l = curlwp;

	off = *offp;
	flags = *flagsp;
	maxprot = VM_PROT_EXECUTE;

	vp = fp->f_vnode;
	if (vp->v_type != VREG && vp->v_type != VCHR &&
	    vp->v_type != VBLK) {
		/* only REG/CHR/BLK support mmap */
		return ENODEV;
	}
	if (vp->v_type != VCHR && off < 0) {
		return EINVAL;
	}
	if (vp->v_type != VCHR && (off_t)(off + size) < off) {
		/* no offset wrapping */
		return EOVERFLOW;
	}

	/* special case: catch SunOS style /dev/zero */
	if (vp->v_type == VCHR &&
	    (vp->v_rdev == zerodev || COMPAT_ZERODEV(vp->v_rdev))) {
		*uobjp = NULL;
		*maxprotp = VM_PROT_ALL;
		return 0;
	}

	/*
	 * Old programs may not select a specific sharing type, so
	 * default to an appropriate one.
	 *
	 * XXX: how does MAP_ANON fit in the picture?
	 */
	if ((flags & (MAP_SHARED|MAP_PRIVATE)) == 0) {
#if defined(DEBUG)
		struct proc *p = l->l_proc;
		printf("WARNING: defaulted mmap() share type to "
		       "%s (pid %d command %s)\n", vp->v_type == VCHR ?
		       "MAP_SHARED" : "MAP_PRIVATE", p->p_pid,
		       p->p_comm);
#endif
		if (vp->v_type == VCHR)
			flags |= MAP_SHARED;	/* for a device */
		else
			flags |= MAP_PRIVATE;	/* for a file */
	}

	/*
	 * MAP_PRIVATE device mappings don't make sense (and aren't
	 * supported anyway).  However, some programs rely on this,
	 * so just change it to MAP_SHARED.
	 */
	if (vp->v_type == VCHR && (flags & MAP_PRIVATE) != 0) {
		flags = (flags & ~MAP_PRIVATE) | MAP_SHARED;
	}

	/*
	 * now check protection
	 */

	/* check read access */
	if (fp->f_flag & FREAD)
		maxprot |= VM_PROT_READ;
	else if (prot & PROT_READ) {
		return EACCES;
	}

	/* check write access, shared case first */
	if (flags & MAP_SHARED) {
		/*
		 * if the file is writable, only add PROT_WRITE to
		 * maxprot if the file is not immutable, append-only.
		 * otherwise, if we have asked for PROT_WRITE, return
		 * EPERM.
		 */
		if (fp->f_flag & FWRITE) {
			vn_lock(vp, LK_SHARED | LK_RETRY);
			error = VOP_GETATTR(vp, &va, l->l_cred);
			VOP_UNLOCK(vp);
			if (error) {
				return error;
			}
			if ((va.va_flags &
			     (SF_SNAPSHOT|IMMUTABLE|APPEND)) == 0)
				maxprot |= VM_PROT_WRITE;
			else if (prot & PROT_WRITE) {
				return EPERM;
			}
		} else if (prot & PROT_WRITE) {
			return EACCES;
		}
	} else {
		/* MAP_PRIVATE mappings can always write to */
		maxprot |= VM_PROT_WRITE;
	}

	/*
	 * Don't allow mmap for EXEC if the file system
	 * is mounted NOEXEC.
	 */
	if ((prot & PROT_EXEC) != 0 &&
	    (vp->v_mount->mnt_flag & MNT_NOEXEC) != 0) {
		return EACCES;
	}

	if (vp->v_type != VCHR) {
		error = VOP_MMAP(vp, prot, curlwp->l_cred);
		if (error) {
			return error;
		}
		vref(vp);
		uobj = &vp->v_uobj;

		/*
		 * If the vnode is being mapped with PROT_EXEC,
		 * then mark it as text.
		 */
		if (prot & PROT_EXEC) {
			vn_markexec(vp);
		}
	} else {
		int i = maxprot;

		/*
		 * XXX Some devices don't like to be mapped with
		 * XXX PROT_EXEC or PROT_WRITE, but we don't really
		 * XXX have a better way of handling this, right now
		 */
		do {
			uobj = udv_attach(vp->v_rdev,
					  (flags & MAP_SHARED) ? i :
					  (i & ~VM_PROT_WRITE), off, size);
			i--;
		} while ((uobj == NULL) && (i > 0));
		if (uobj == NULL) {
			return EINVAL;
		}
		*advicep = UVM_ADV_RANDOM;
	}

	/*
	 * Set vnode flags to indicate the new kinds of mapping.
	 * We take the vnode lock in exclusive mode here to serialize
	 * with direct I/O.
	 *
	 * Safe to check for these flag values without a lock, as
	 * long as a reference to the vnode is held.
	 */
	needwritemap = (vp->v_iflag & VI_WRMAP) == 0 &&
		(flags & MAP_SHARED) != 0 &&
		(maxprot & VM_PROT_WRITE) != 0;
	if ((vp->v_vflag & VV_MAPPED) == 0 || needwritemap) {
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		vp->v_vflag |= VV_MAPPED;
		if (needwritemap) {
			rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
			mutex_enter(vp->v_interlock);
			vp->v_iflag |= VI_WRMAP;
			mutex_exit(vp->v_interlock);
			rw_exit(vp->v_uobj.vmobjlock);
		}
		VOP_UNLOCK(vp);
	}

#if NVERIEXEC > 0

	/*
	 * Check if the file can be executed indirectly.
	 *
	 * XXX: This gives false warnings about "Incorrect access type"
	 * XXX: if the mapping is not executable. Harmless, but will be
	 * XXX: fixed as part of other changes.
	 */
	if (veriexec_verify(l, vp, "(mmap)", VERIEXEC_INDIRECT,
			    NULL)) {

		/*
		 * Don't allow executable mappings if we can't
		 * indirectly execute the file.
		 */
		if (prot & VM_PROT_EXECUTE) {
			return EPERM;
		}

		/*
		 * Strip the executable bit from 'maxprot' to make sure
		 * it can't be made executable later.
		 */
		maxprot &= ~VM_PROT_EXECUTE;
	}
#endif /* NVERIEXEC > 0 */

	*uobjp = uobj;
	*maxprotp = maxprot;
	*flagsp = flags;

	return 0;
}

static int
vn_seek(struct file *fp, off_t delta, int whence, off_t *newoffp,
    int flags)
{
	const off_t OFF_MIN = __type_min(off_t);
	const off_t OFF_MAX = __type_max(off_t);
	kauth_cred_t cred = fp->f_cred;
	off_t oldoff, newoff;
	struct vnode *vp = fp->f_vnode;
	struct vattr vattr;
	int error;

	if (vp->v_type == VFIFO)
		return ESPIPE;

	vn_lock(vp, LK_SHARED | LK_RETRY);

	/* Compute the old and new offsets.  */
	oldoff = fp->f_offset;
	switch (whence) {
	case SEEK_CUR:
		if (delta > 0) {
			if (oldoff > 0 && delta > OFF_MAX - oldoff) {
				newoff = OFF_MAX;
				break;
			}
		} else {
			if (oldoff < 0 && delta < OFF_MIN - oldoff) {
				newoff = OFF_MIN;
				break;
			}
		}
		newoff = oldoff + delta;
		break;
	case SEEK_END:
		error = VOP_GETATTR(vp, &vattr, cred);
		if (error)
			goto out;
		if (vattr.va_size > OFF_MAX ||
		    delta > OFF_MAX - (off_t)vattr.va_size) {
			newoff = OFF_MAX;
			break;
		}
		newoff = delta + vattr.va_size;
		break;
	case SEEK_SET:
		newoff = delta;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	/* Pass the proposed change to the file system to audit.  */
	error = VOP_SEEK(vp, oldoff, newoff, cred);
	if (error)
		goto out;

	/* Success!  */
	if (newoffp)
		*newoffp = newoff;
	if (flags & FOF_UPDATE_OFFSET)
		fp->f_offset = newoff;
	error = 0;

out:	VOP_UNLOCK(vp);
	return error;
}

/*
 * Check that the vnode is still valid, and if so
 * acquire requested lock.
 */
int
vn_lock(struct vnode *vp, int flags)
{
	struct lwp *l;
	int error;

#if 0
	KASSERT(vrefcnt(vp) > 0 || (vp->v_iflag & VI_ONWORKLST) != 0);
#endif
	KASSERT((flags & ~(LK_SHARED|LK_EXCLUSIVE|LK_NOWAIT|LK_RETRY|
	    LK_UPGRADE|LK_DOWNGRADE)) == 0);
	KASSERT((flags & LK_NOWAIT) != 0 || !mutex_owned(vp->v_interlock));

#ifdef DIAGNOSTIC
	if (wapbl_vphaswapbl(vp))
		WAPBL_JUNLOCK_ASSERT(wapbl_vptomp(vp));
#endif

	/* Get a more useful report for lockstat. */
	l = curlwp;
	KASSERT(l->l_rwcallsite == 0);
	l->l_rwcallsite = (uintptr_t)__builtin_return_address(0);	

	error = VOP_LOCK(vp, flags);
	if ((flags & LK_RETRY) != 0 && error == ENOENT)
		error = VOP_LOCK(vp, flags);

	l->l_rwcallsite = 0;

	KASSERT((flags & LK_RETRY) == 0 || (flags & LK_NOWAIT) != 0 ||
	    error == 0);

	return error;
}

/*
 * File table vnode close routine.
 */
static int
vn_closefile(file_t *fp)
{

	return vn_close(fp->f_vnode, fp->f_flag, fp->f_cred);
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

	error = VOP_GETEXTATTR(vp, attrnamespace, attrname, &auio, NULL,
	    NOCRED);

	if ((ioflg & IO_NODELOCKED) == 0)
		VOP_UNLOCK(vp);

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

	error = VOP_SETEXTATTR(vp, attrnamespace, attrname, &auio, NOCRED);

	if ((ioflg & IO_NODELOCKED) == 0) {
		VOP_UNLOCK(vp);
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

	error = VOP_DELETEEXTATTR(vp, attrnamespace, attrname, NOCRED);
	if (error == EOPNOTSUPP)
		error = VOP_SETEXTATTR(vp, attrnamespace, attrname, NULL,
		    NOCRED);

	if ((ioflg & IO_NODELOCKED) == 0) {
		VOP_UNLOCK(vp);
	}

	return (error);
}

int
vn_fifo_bypass(void *v)
{
	struct vop_generic_args *ap = v;

	return VOCALL(fifo_vnodeop_p, ap->a_desc->vdesc_offset, v);
}

/*
 * Open block device by device number
 */
int
vn_bdev_open(dev_t dev, struct vnode **vpp, struct lwp *l)
{
	int     error;

	if ((error = bdevvp(dev, vpp)) != 0)
		return error;

	if ((error = VOP_OPEN(*vpp, FREAD | FWRITE, l->l_cred)) != 0) {
		vrele(*vpp);
		return error;
	}
	mutex_enter((*vpp)->v_interlock);
	(*vpp)->v_writecount++;
	mutex_exit((*vpp)->v_interlock);

	return 0;
}

/*
 * Lookup the provided name in the filesystem.  If the file exists,
 * is a valid block device, and isn't being used by anyone else,
 * set *vpp to the file's vnode.
 */
int
vn_bdev_openpath(struct pathbuf *pb, struct vnode **vpp, struct lwp *l)
{
	struct vnode *vp;
	dev_t dev;
	enum vtype vt;
	int     error;

	error = vn_open(NULL, pb, 0, FREAD | FWRITE, 0, &vp, NULL, NULL);
	if (error != 0)
		return error;

	dev = vp->v_rdev;
	vt = vp->v_type;

	VOP_UNLOCK(vp);
	(void) vn_close(vp, FREAD | FWRITE, l->l_cred);

	if (vt != VBLK)
		return ENOTBLK;

	return vn_bdev_open(dev, vpp, l);
}

static long
vn_knote_to_interest(const struct knote *kn)
{
	switch (kn->kn_filter) {
	case EVFILT_READ:
		/*
		 * Writing to the file or changing its attributes can
		 * set the file size, which impacts the readability
		 * filter.
		 *
		 * (No need to set NOTE_EXTEND here; it's only ever
		 * send with other hints; see vnode_if.c.)
		 */
		return NOTE_WRITE | NOTE_ATTRIB;

	case EVFILT_VNODE:
		return kn->kn_sfflags;

	case EVFILT_WRITE:
	default:
		return 0;
	}
}

void
vn_knote_attach(struct vnode *vp, struct knote *kn)
{
	long interest = 0;

	/*
	 * We maintain a bitmask of the kevents that there is interest in,
	 * to minimize the impact of having watchers.  It's silly to have
	 * to traverse vn_klist every time a read or write happens simply
	 * because there is someone interested in knowing when the file
	 * is deleted, for example.
	 */

	mutex_enter(vp->v_interlock);
	SLIST_INSERT_HEAD(&vp->v_klist, kn, kn_selnext);
	SLIST_FOREACH(kn, &vp->v_klist, kn_selnext) {
		interest |= vn_knote_to_interest(kn);
	}
	vp->v_klist_interest = interest;
	mutex_exit(vp->v_interlock);
}

void
vn_knote_detach(struct vnode *vp, struct knote *kn)
{
	int interest = 0;

	/*
	 * We special case removing the head of the list, beacuse:
	 *
	 * 1. It's extremely likely that we're detaching the only
	 *    knote.
	 *
	 * 2. We're already traversing the whole list, so we don't
	 *    want to use the generic SLIST_REMOVE() which would
	 *    traverse it *again*.
	 */

	mutex_enter(vp->v_interlock);
	if (__predict_true(kn == SLIST_FIRST(&vp->v_klist))) {
		SLIST_REMOVE_HEAD(&vp->v_klist, kn_selnext);
		SLIST_FOREACH(kn, &vp->v_klist, kn_selnext) {
			interest |= vn_knote_to_interest(kn);
		}
		vp->v_klist_interest = interest;
	} else {
		struct knote *thiskn, *nextkn, *prevkn = NULL;

		SLIST_FOREACH_SAFE(thiskn, &vp->v_klist, kn_selnext, nextkn) {
			if (thiskn == kn) {
				KASSERT(kn != NULL);
				KASSERT(prevkn != NULL);
				SLIST_REMOVE_AFTER(prevkn, kn_selnext);
				kn = NULL;
			} else {
				interest |= vn_knote_to_interest(thiskn);
				prevkn = thiskn;
			}
		}
		vp->v_klist_interest = interest;
	}
	mutex_exit(vp->v_interlock);
}
