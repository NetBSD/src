/*	$NetBSD: irix_fcntl.c,v 1.7.2.3 2002/06/20 16:41:01 gehenna Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: irix_fcntl.c,v 1.7.2.3 2002/06/20 16:41:01 gehenna Exp $");

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_fcntl.h>
#include <compat/irix/irix_usema.h>
#include <compat/irix/irix_syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_fcntl.h>
#include <compat/svr4/svr4_syscallargs.h>

static int fd_truncate __P((struct proc *, int, int, off_t, register_t *));
static int bsd_to_irix_fcntl_flags __P((int));
static int irix_to_bsd_fcntl_flags __P((int));

int
irix_sys_lseek64(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/* 
	 * Note: we have an alignement problem here. If pad2, pad3 and pad4 
	 * are removed, lseek64 will break, because whence will be wrong.
	 */
	struct irix_sys_lseek64_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad1;
		syscallarg(irix_off64_t) offset;
		syscallarg(int) whence;
		syscallarg(int) pad2;
		syscallarg(int) pad3;
		syscallarg(int) pad4;
	} */ *uap = v;
	struct sys_lseek_args cup;

#ifdef DEBUG_IRIX
	printf("irix_sys_lseek64(): fd = %d, pad1 = 0x%08x, offset = 0x%llx\n",
	    SCARG(uap, fd), SCARG(uap, pad1), SCARG(uap, offset));
	printf("whence = 0x%08x, pad2 = 0x%08x, pad3 = 0x%08x, pad4 = 0x%08x\n",
	    SCARG(uap, whence), SCARG(uap, pad2), SCARG(uap, pad3), 
	    SCARG(uap, pad4));
#endif
	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, pad) = 0;
	SCARG(&cup, offset) = SCARG(uap, offset);
	SCARG(&cup, whence) = SCARG(uap, whence);

	return sys_lseek(p, (void *)&cup, retval);
}

int
irix_sys_fcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(char *)arg;
	} */ *uap = v;
	struct svr4_sys_fcntl_args cup;
	int cmd;
	int error;

	cmd = SCARG(uap, cmd);
	switch (cmd) {
	case SVR4_F_FREESP:
	case SVR4_F_ALLOCSP: {
		struct svr4_flock fl;

		if ((error = copyin(SCARG(uap, arg), &fl, sizeof(fl))) != 0)
			return error;

		return fd_truncate(p, SCARG(uap, fd), 
		    fl.l_whence, fl.l_start, retval);
		break;
	}

	case SVR4_F_FREESP64:
	case IRIX_F_ALLOCSP64: {
		struct svr4_flock64 fl;

		if ((error = copyin(SCARG(uap, arg), &fl, sizeof(fl))) != 0)
			return error;

		return fd_truncate(p, SCARG(uap, fd), 
		    fl.l_whence, fl.l_start, retval);
		break;
	}

	case IRIX_F_SETBSDLKW:
		cmd = SVR4_F_SETLKW;
		break;

	case IRIX_F_SETBSDLK:
		cmd = SVR4_F_SETLK;
		break;

	case IRIX_F_GETFL:
		SCARG(&cup, fd) = SCARG(uap, fd);
		SCARG(&cup, cmd) = F_GETFL;
		SCARG(&cup, arg) = SCARG(uap, arg);
		if ((error = sys_fcntl(p, &cup, retval)) != 0)
			return error;
		*retval = bsd_to_irix_fcntl_flags(*retval);
		return 0;
		break;
		
	case IRIX_F_SETFL:
		/* 
		 * All unsupported flags are silently ignored 
		 * except FDIRECT taht will return EINVAL
		 */
		if ((int)SCARG(uap, arg) & IRIX_FDIRECT)
			return EINVAL;

		SCARG(&cup, fd) = SCARG(uap, fd);
		SCARG(&cup, arg) = 
		    (char *)irix_to_bsd_fcntl_flags((int)SCARG(uap, arg));
		SCARG(&cup, cmd) = F_SETFL;
		return sys_fcntl(p, &cup, retval);
		break;

	case SVR4_F_DUPFD:
	case SVR4_F_GETFD:
	case SVR4_F_SETFD:
	case SVR4_F_SETLK:
	case SVR4_F_SETLKW:
	case SVR4_F_CHKFL:
	case SVR4_F_GETLK:
	case SVR4_F_RSETLK:
	case SVR4_F_RGETLK:
	case SVR4_F_RSETLKW:
	case SVR4_F_GETOWN:
	case SVR4_F_SETOWN:
	case SVR4_F_GETLK64:
	case SVR4_F_SETLK64:
	case SVR4_F_SETLKW64:
		break;

	case IRIX_F_CHKLK:
	case IRIX_F_CHKLKW:
	case IRIX_F_CLNLK:
	case IRIX_F_DIOINFO:
	case IRIX_F_FSGETXATTR:
	case IRIX_F_FSSETXATTR:
	case IRIX_F_GETBMAP:
	case IRIX_F_FSSETDM:
	case IRIX_F_RESVSP:
	case IRIX_F_UNRESVSP:
	case IRIX_F_RESVSP64:
	case IRIX_F_UNRESVSP64:
	case IRIX_F_GETBMAPA:
	case IRIX_F_FSGETXATTRA:
	case IRIX_F_SETBIOSIZE:
	case IRIX_F_GETBIOSIZE:
	case IRIX_F_GETOPS:
	case IRIX_F_DMAPI:
	case IRIX_F_FSYNC:
	case IRIX_F_FSYNC64:
	case IRIX_F_GETBDSATTR:
	case IRIX_F_SETBDSATTR:
	case IRIX_F_GETBMAPX:
	case IRIX_F_SETPRIO:
	case IRIX_F_GETPRIO:
	default:
		printf("Warning: unimplemented IRIX fcntl() command %d\n", 
		    cmd);
		return EINVAL;
		break;
	}

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, cmd) = cmd;
	SCARG(&cup, arg) = SCARG(uap, arg);
	return svr4_sys_fcntl(p, &cup, retval);
}
	
static int
fd_truncate(p, fd, whence, start, retval)
	struct proc *p;
	int fd;
	int whence;
	off_t start;
	register_t *retval;
{	
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	struct vattr vattr;
	struct sys_ftruncate_args ft;
	int error;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return EBADF;

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE || vp->v_type == VFIFO)
		return ESPIPE;

	switch (whence) {
	case SEEK_CUR:
		SCARG(&ft, length) = fp->f_offset + start;
		break;

	case SEEK_END:
		if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)) != 0)
			return error;
		SCARG(&ft, length) = vattr.va_size + start;
		break;

	case SEEK_SET:
		SCARG(&ft, length) = start;
		break;

	default:
		return EINVAL;
		break;
	}

	SCARG(&ft, fd) = fd;
	return sys_ftruncate(p, &ft, retval);
}

int
irix_sys_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_open_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	extern const struct cdevsw irix_usema_cdevsw;
	int error;
	int fd;
	struct file *fp;
	struct vnode *vp;
	struct vnode *nvp;

	if ((error = svr4_sys_open(p, v, retval)) != 0)
		return error;

	fd = (int)*retval;
	if ((fp = fd_getfile(p->p_fd, fd)) == NULL)
		return EBADF;

	FILE_USE(fp);

	vp = (struct vnode *)fp->f_data;

	/* 
	 * A special hook for the usemaclone driver: we need to clone
	 * the vnode, because the driver method need a context for each
	 * usemaclone open instance, and because we need to overload 
	 * some vnode operations like setattr.
	 * The original vnode is stored in the v_data field of the cloned
	 * vnode.
	 */
	if (vp->v_type == VCHR &&
	    cdevsw_lookup(vp->v_rdev) == &irix_usema_cdevsw &&
	    minor(vp->v_rdev) == IRIX_USEMACLNDEV_MINOR) {
		if ((error = getnewvnode(VCHR, vp->v_mount, 
		    irix_usema_vnodeop_p, 
		    (struct vnode **)&fp->f_data)) != 0) {
			(void) vn_close(vp, fp->f_flag, fp->f_cred, p);
			FILE_UNUSE(fp, p);
			ffree(fp);
			fdremove(p->p_fd, fd);
			return error;
		}
		
		nvp = (struct vnode *)fp->f_data;

		if (SCARG(uap, flags) & O_RDWR || SCARG(uap, flags) & O_WRONLY)
			nvp->v_writecount++;

		nvp->v_type = VCHR;
		nvp->v_specinfo = (void *)malloc(sizeof(struct specinfo), 
		    M_VNODE, M_WAITOK|M_ZERO);
		nvp->v_rdev = vp->v_rdev;
		nvp->v_specmountpoint = vp->v_specmountpoint;

		nvp->v_data = (void *)vp;
		vref(vp);
	}
	FILE_UNUSE(fp, p);

	return 0;
}

static int 
irix_to_bsd_fcntl_flags(flags)
	int flags;
{
	int ret = 0;

	if (flags & IRIX_FNDELAY) ret |= FNDELAY;
	if (flags & IRIX_FAPPEND) ret |= FAPPEND;
	if (flags & IRIX_FSYNC) ret |= FFSYNC;
	if (flags & IRIX_FDSYNC) ret |= FDSYNC;
	if (flags & IRIX_FASYNC) ret |= FASYNC;
	if (flags & IRIX_FRSYNC) ret |= FRSYNC;
	if (flags & IRIX_FNONBLOCK) ret |= FNONBLOCK;
	if (flags & IRIX_FLARGEFILE)
		printf("Warning: ignored fcntl IRIX_FLARGEFILE flag");
	if (flags & IRIX_FDIRECT)
		printf("Warning: ignored fcntl IRIX_FDIRECT flag");
	if (flags & IRIX_FBULK)
		printf("Warning: ignored fcntl IRIX_FBULK flag");
	if (flags & IRIX_FLCINVAL)
		printf("Warning: ignored fcntl IRIX_FLCINVAL flag");
	if (flags & IRIX_FLCFLUSH)
		printf("Warning: ignored fcntl IRIX_FLCFLUSH flag");

	return ret;
}

static int 
bsd_to_irix_fcntl_flags(flags)
	int flags;
{
	int ret = 0;

	if (flags & FNDELAY) ret |= IRIX_FNDELAY;
	if (flags & FAPPEND) ret |= IRIX_FAPPEND;
	if (flags & FFSYNC) ret |= IRIX_FSYNC;
	if (flags & FDSYNC) ret |= IRIX_FDSYNC;
	if (flags & FRSYNC) ret |= IRIX_FRSYNC;
	if (flags & FNONBLOCK) ret |= IRIX_FNONBLOCK;
	if (flags & FASYNC) ret |= IRIX_FASYNC;

	return ret;
}
