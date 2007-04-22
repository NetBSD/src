/*	$NetBSD: svr4_fcntl.c,v 1.57 2007/04/22 08:29:59 dsl Exp $	 */

/*-
 * Copyright (c) 1994, 1997 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_fcntl.c,v 1.57 2007/04/22 08:29:59 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_fcntl.h>

static u_long svr4_to_bsd_cmd __P((u_long));
static int svr4_to_bsd_flags __P((int));
static int bsd_to_svr4_flags __P((int));
static void bsd_to_svr4_flock __P((struct flock *, struct svr4_flock *));
static void svr4_to_bsd_flock __P((struct svr4_flock *, struct flock *));
static void bsd_to_svr4_flock64 __P((struct flock *, struct svr4_flock64 *));
static void svr4_to_bsd_flock64 __P((struct svr4_flock64 *, struct flock *));
static int fd_revoke __P((struct lwp *, int, register_t *));
static int fd_truncate __P((struct lwp *, int, struct flock *, register_t *));

static u_long
svr4_to_bsd_cmd(cmd)
	u_long	cmd;
{
	switch (cmd) {
	case SVR4_F_DUPFD:
		return F_DUPFD;
	case SVR4_F_GETFD:
		return F_GETFD;
	case SVR4_F_SETFD:
		return F_SETFD;
	case SVR4_F_GETFL:
		return F_GETFL;
	case SVR4_F_SETFL:
		return F_SETFL;
	case SVR4_F_GETLK:
		return F_GETLK;
	case SVR4_F_SETLK:
		return F_SETLK;
	case SVR4_F_SETLKW:
		return F_SETLKW;
	default:
		return -1;
	}
}


static int
svr4_to_bsd_flags(l)
	int	l;
{
	int	r = 0;
	r |= (l & SVR4_O_RDONLY) ? O_RDONLY : 0;
	r |= (l & SVR4_O_WRONLY) ? O_WRONLY : 0;
	r |= (l & SVR4_O_RDWR) ? O_RDWR : 0;
	r |= (l & SVR4_O_NDELAY) ? O_NDELAY : 0;
	r |= (l & SVR4_O_APPEND) ? O_APPEND : 0;
	r |= (l & SVR4_O_SYNC) ? O_FSYNC : 0;
	r |= (l & SVR4_O_RSYNC) ? O_RSYNC : 0;
	r |= (l & SVR4_O_DSYNC) ? O_DSYNC : 0;
	r |= (l & SVR4_O_NONBLOCK) ? O_NONBLOCK : 0;
	r |= (l & SVR4_O_PRIV) ? O_EXLOCK : 0;
	r |= (l & SVR4_O_CREAT) ? O_CREAT : 0;
	r |= (l & SVR4_O_TRUNC) ? O_TRUNC : 0;
	r |= (l & SVR4_O_EXCL) ? O_EXCL : 0;
	r |= (l & SVR4_O_NOCTTY) ? O_NOCTTY : 0;
	return r;
}


static int
bsd_to_svr4_flags(l)
	int	l;
{
	int	r = 0;
	r |= (l & O_RDONLY) ? SVR4_O_RDONLY : 0;
	r |= (l & O_WRONLY) ? SVR4_O_WRONLY : 0;
	r |= (l & O_RDWR) ? SVR4_O_RDWR : 0;
	r |= (l & O_NDELAY) ? SVR4_O_NDELAY : 0;
	r |= (l & O_APPEND) ? SVR4_O_APPEND : 0;
	r |= (l & O_FSYNC) ? SVR4_O_SYNC : 0;
	r |= (l & O_RSYNC) ? SVR4_O_RSYNC : 0;
	r |= (l & O_DSYNC) ? SVR4_O_DSYNC : 0;
	r |= (l & O_NONBLOCK) ? SVR4_O_NONBLOCK : 0;
	r |= (l & O_EXLOCK) ? SVR4_O_PRIV : 0;
	r |= (l & O_CREAT) ? SVR4_O_CREAT : 0;
	r |= (l & O_TRUNC) ? SVR4_O_TRUNC : 0;
	r |= (l & O_EXCL) ? SVR4_O_EXCL : 0;
	r |= (l & O_NOCTTY) ? SVR4_O_NOCTTY : 0;
	return r;
}


static void
bsd_to_svr4_flock(iflp, oflp)
	struct flock		*iflp;
	struct svr4_flock	*oflp;
{
	switch (iflp->l_type) {
	case F_RDLCK:
		oflp->l_type = SVR4_F_RDLCK;
		break;
	case F_WRLCK:
		oflp->l_type = SVR4_F_WRLCK;
		break;
	case F_UNLCK:
		oflp->l_type = SVR4_F_UNLCK;
		break;
	default:
		oflp->l_type = -1;
		break;
	}

	oflp->l_whence = (short) iflp->l_whence;
	oflp->l_start = (svr4_off_t) iflp->l_start;
	oflp->l_len = (svr4_off_t) iflp->l_len;
	oflp->l_sysid = 0;
	oflp->l_pid = (svr4_pid_t) iflp->l_pid;
}


static void
svr4_to_bsd_flock(iflp, oflp)
	struct svr4_flock	*iflp;
	struct flock		*oflp;
{
	switch (iflp->l_type) {
	case SVR4_F_RDLCK:
		oflp->l_type = F_RDLCK;
		break;
	case SVR4_F_WRLCK:
		oflp->l_type = F_WRLCK;
		break;
	case SVR4_F_UNLCK:
		oflp->l_type = F_UNLCK;
		break;
	default:
		oflp->l_type = -1;
		break;
	}

	oflp->l_whence = iflp->l_whence;
	oflp->l_start = (off_t) iflp->l_start;
	oflp->l_len = (off_t) iflp->l_len;
	oflp->l_pid = (pid_t) iflp->l_pid;

}

static void
bsd_to_svr4_flock64(iflp, oflp)
	struct flock		*iflp;
	struct svr4_flock64	*oflp;
{
	switch (iflp->l_type) {
	case F_RDLCK:
		oflp->l_type = SVR4_F_RDLCK;
		break;
	case F_WRLCK:
		oflp->l_type = SVR4_F_WRLCK;
		break;
	case F_UNLCK:
		oflp->l_type = SVR4_F_UNLCK;
		break;
	default:
		oflp->l_type = -1;
		break;
	}

	oflp->l_whence = (short) iflp->l_whence;
	oflp->l_start = (svr4_off64_t) iflp->l_start;
	oflp->l_len = (svr4_off64_t) iflp->l_len;
	oflp->l_sysid = 0;
	oflp->l_pid = (svr4_pid_t) iflp->l_pid;
}


static void
svr4_to_bsd_flock64(iflp, oflp)
	struct svr4_flock64	*iflp;
	struct flock		*oflp;
{
	switch (iflp->l_type) {
	case SVR4_F_RDLCK:
		oflp->l_type = F_RDLCK;
		break;
	case SVR4_F_WRLCK:
		oflp->l_type = F_WRLCK;
		break;
	case SVR4_F_UNLCK:
		oflp->l_type = F_UNLCK;
		break;
	default:
		oflp->l_type = -1;
		break;
	}

	oflp->l_whence = iflp->l_whence;
	oflp->l_start = (off_t) iflp->l_start;
	oflp->l_len = (off_t) iflp->l_len;
	oflp->l_pid = (pid_t) iflp->l_pid;

}


static int
fd_revoke(struct lwp *l, int fd, register_t *retval)
{
	struct filedesc *fdp = l->l_proc->p_fd;
	struct file *fp;
	struct vnode *vp;
	struct vattr vattr;
	int error;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return EBADF;

	simple_unlock(&fp->f_slock);
	if (fp->f_type != DTYPE_VNODE)
		return EINVAL;

	vp = (struct vnode *) fp->f_data;

	if (vp->v_type != VCHR && vp->v_type != VBLK) {
		error = EINVAL;
		goto out;
	}

	if ((error = VOP_GETATTR(vp, &vattr, l->l_cred, l)) != 0)
		goto out;

	if (kauth_cred_geteuid(l->l_cred) != vattr.va_uid &&
	    (error = kauth_authorize_generic(l->l_cred,
	    KAUTH_GENERIC_ISSUSER, NULL)) != 0)
		goto out;

	if (vp->v_usecount > 1 || (vp->v_flag & VALIASED))
		VOP_REVOKE(vp, REVOKEALL);
out:
	vrele(vp);
	return error;
}


static int
fd_truncate(l, fd, flp, retval)
	struct lwp *l;
	int fd;
	struct flock *flp;
	register_t *retval;
{
	struct filedesc *fdp = l->l_proc->p_fd;
	struct file *fp;
	off_t start, length;
	struct vnode *vp;
	struct vattr vattr;
	int error;
	struct sys_ftruncate_args ft;

	/*
	 * We only support truncating the file.
	 */
	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return EBADF;

	simple_unlock(&fp->f_slock);
	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO)
		return ESPIPE;

	if ((error = VOP_GETATTR(vp, &vattr, l->l_cred, l)) != 0)
		return error;

	length = vattr.va_size;

	switch (flp->l_whence) {
	case SEEK_CUR:
		start = fp->f_offset + flp->l_start;
		break;

	case SEEK_END:
		start = flp->l_start + length;
		break;

	case SEEK_SET:
		start = flp->l_start;
		break;

	default:
		return EINVAL;
	}

	if (start + flp->l_len < length) {
		/* We don't support free'ing in the middle of the file */
		return EINVAL;
	}

	SCARG(&ft, fd) = fd;
	SCARG(&ft, length) = start;

	return sys_ftruncate(l, &ft, retval);
}


int
svr4_sys_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_open_args	*uap = v;
	int			error;
	struct sys_open_args	cup;

	SCARG(&cup, flags) = svr4_to_bsd_flags(SCARG(uap, flags));

	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, mode) = SCARG(uap, mode);
	error = sys_open(l, &cup, retval);

	if (error)
		return error;

	/* XXXAD locking */

	if (!(SCARG(&cup, flags) & O_NOCTTY) && SESS_LEADER(l->l_proc) &&
	    !(l->l_proc->p_lflag & PL_CONTROLT)) {
		struct filedesc	*fdp = l->l_proc->p_fd;
		struct file	*fp;

		fp = fd_getfile(fdp, *retval);
		simple_unlock(&fp->f_slock);

		/* ignore any error, just give it a try */
		if (fp != NULL && fp->f_type == DTYPE_VNODE)
			(fp->f_ops->fo_ioctl) (fp, TIOCSCTTY, (void *) 0, l);
	}
	return 0;
}


int
svr4_sys_open64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return svr4_sys_open(l, v, retval);
}


int
svr4_sys_creat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_creat_args *uap = v;
	struct sys_open_args cup;

	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, mode) = SCARG(uap, mode);
	SCARG(&cup, flags) = O_WRONLY | O_CREAT | O_TRUNC;

	return sys_open(l, &cup, retval);
}


int
svr4_sys_creat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return svr4_sys_creat(l, v, retval);
}


int
svr4_sys_llseek(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_llseek_args *uap = v;
	struct sys_lseek_args ap;

	SCARG(&ap, fd) = SCARG(uap, fd);

#if BYTE_ORDER == BIG_ENDIAN
	SCARG(&ap, offset) = (((long long) SCARG(uap, offset1)) << 32) |
		SCARG(uap, offset2);
#else
	SCARG(&ap, offset) = (((long long) SCARG(uap, offset2)) << 32) |
		SCARG(uap, offset1);
#endif
	SCARG(&ap, whence) = SCARG(uap, whence);

	return sys_lseek(l, &ap, retval);
}

int
svr4_sys_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_access_args *uap = v;
	struct sys_access_args cup;

	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, flags) = SCARG(uap, flags);

	return sys_access(l, &cup, retval);
}


int
svr4_sys_pread(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_pread_args *uap = v;
	struct sys_pread_args pra;

	/*
	 * Just translate the args structure and call the NetBSD
	 * pread(2) system call (offset type is 64-bit in NetBSD).
	 */
	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, offset) = SCARG(uap, off);

	return (sys_pread(l, &pra, retval));
}


int
svr4_sys_pread64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	struct svr4_sys_pread64_args *uap = v;
	struct sys_pread_args pra;

	/*
	 * Just translate the args structure and call the NetBSD
	 * pread(2) system call (offset type is 64-bit in NetBSD).
	 */
	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, offset) = SCARG(uap, off);

	return (sys_pread(l, &pra, retval));
}


int
svr4_sys_pwrite(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_pwrite_args *uap = v;
	struct sys_pwrite_args pwa;

	/*
	 * Just translate the args structure and call the NetBSD
	 * pwrite(2) system call (offset type is 64-bit in NetBSD).
	 */
	SCARG(&pwa, fd) = SCARG(uap, fd);
	SCARG(&pwa, buf) = SCARG(uap, buf);
	SCARG(&pwa, nbyte) = SCARG(uap, nbyte);
	SCARG(&pwa, offset) = SCARG(uap, off);

	return (sys_pwrite(l, &pwa, retval));
}


int
svr4_sys_pwrite64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_pwrite64_args *uap = v;
	struct sys_pwrite_args pwa;

	/*
	 * Just translate the args structure and call the NetBSD
	 * pwrite(2) system call (offset type is 64-bit in NetBSD).
	 */
	SCARG(&pwa, fd) = SCARG(uap, fd);
	SCARG(&pwa, buf) = SCARG(uap, buf);
	SCARG(&pwa, nbyte) = SCARG(uap, nbyte);
	SCARG(&pwa, offset) = SCARG(uap, off);

	return (sys_pwrite(l, &pwa, retval));
}


int
svr4_sys_fcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_fcntl_args	*uap = v;
	struct proc *p = l->l_proc;
	int				error;
	struct sys_fcntl_args		fa;

	SCARG(&fa, fd) = SCARG(uap, fd);
	SCARG(&fa, cmd) = svr4_to_bsd_cmd(SCARG(uap, cmd));

	switch (SCARG(&fa, cmd)) {
	case F_DUPFD:
	case F_GETFD:
	case F_SETFD:
		SCARG(&fa, arg) = SCARG(uap, arg);
		return sys_fcntl(l, &fa, retval);

	case F_GETFL:
		SCARG(&fa, arg) = SCARG(uap, arg);
		error = sys_fcntl(l, &fa, retval);
		if (error)
			return error;
		*retval = bsd_to_svr4_flags(*retval);
		return error;

	case F_SETFL:
		{
			/*
			 * we must save the O_ASYNC flag, as that is
			 * handled by ioctl(_, I_SETSIG, _) emulation.
			 */
			register_t flags;
			int cmd;

			cmd = SCARG(&fa, cmd); /* save it for a while */

			SCARG(&fa, cmd) = F_GETFL;
			if ((error = sys_fcntl(l, &fa, &flags)) != 0)
				return error;
			flags &= O_ASYNC;
			flags |= svr4_to_bsd_flags((u_long) SCARG(uap, arg));
			SCARG(&fa, cmd) = cmd;
			SCARG(&fa, arg) = (void *) flags;
			return sys_fcntl(l, &fa, retval);
		}

	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		{
			struct svr4_flock	 ifl;
			struct flock		*flp, fl;
			void *sg = stackgap_init(p, 0);

			flp = stackgap_alloc(p, &sg, sizeof(struct flock));
			SCARG(&fa, arg) = (void *) flp;

			error = copyin(SCARG(uap, arg), &ifl, sizeof ifl);
			if (error)
				return error;

			svr4_to_bsd_flock(&ifl, &fl);

			error = copyout(&fl, flp, sizeof fl);
			if (error)
				return error;

			error = sys_fcntl(l, &fa, retval);
			if (error || SCARG(&fa, cmd) != F_GETLK)
				return error;

			error = copyin(flp, &fl, sizeof fl);
			if (error)
				return error;

			bsd_to_svr4_flock(&fl, &ifl);

			return copyout(&ifl, SCARG(uap, arg), sizeof ifl);
		}
	case -1:
		switch (SCARG(uap, cmd)) {
		case SVR4_F_DUP2FD:
			{
				struct sys_dup2_args du;

				SCARG(&du, from) = SCARG(uap, fd);
				SCARG(&du, to) = (int)(u_long)SCARG(uap, arg);
				error = sys_dup2(l, &du, retval);
				if (error)
					return error;
				*retval = SCARG(&du, to);
				return 0;
			}

		case SVR4_F_FREESP:
			{
				struct svr4_flock	 ifl;
				struct flock		 fl;

				error = copyin(SCARG(uap, arg), &ifl,
				    sizeof ifl);
				if (error)
					return error;
				svr4_to_bsd_flock(&ifl, &fl);
				return fd_truncate(l, SCARG(uap, fd), &fl,
				    retval);
			}

		case SVR4_F_GETLK64:
		case SVR4_F_SETLK64:
		case SVR4_F_SETLKW64:
			{
				struct svr4_flock64	 ifl;
				struct flock		*flp, fl;
				void *sg = stackgap_init(p, 0);

				flp = stackgap_alloc(p, &sg,
				    sizeof(struct flock));
				SCARG(&fa, arg) = (void *) flp;

				error = copyin(SCARG(uap, arg), &ifl,
				    sizeof ifl);
				if (error)
					return error;

				svr4_to_bsd_flock64(&ifl, &fl);

				error = copyout(&fl, flp, sizeof fl);
				if (error)
					return error;

				error = sys_fcntl(l, &fa, retval);
				if (error || SCARG(&fa, cmd) != F_GETLK)
					return error;

				error = copyin(flp, &fl, sizeof fl);
				if (error)
					return error;

				bsd_to_svr4_flock64(&fl, &ifl);

				return copyout(&ifl, SCARG(uap, arg),
				    sizeof ifl);
			}

		case SVR4_F_FREESP64:
			{
				struct svr4_flock64	 ifl;
				struct flock		 fl;

				error = copyin(SCARG(uap, arg), &ifl,
				    sizeof ifl);
				if (error)
					return error;
				svr4_to_bsd_flock64(&ifl, &fl);
				return fd_truncate(l, SCARG(uap, fd), &fl,
				    retval);
			}

		case SVR4_F_REVOKE:
			return fd_revoke(l, SCARG(uap, fd), retval);

		default:
			return ENOSYS;
		}

	default:
		return ENOSYS;
	}
}
