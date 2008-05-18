/*	$NetBSD: irix_ioctl.c,v 1.19.2.1 2008/05/18 12:33:11 yamt Exp $ */

/*-
 * Copyright (c) 2002, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
__KERNEL_RCSID(0, "$NetBSD: irix_ioctl.c,v 1.19.2.1 2008/05/18 12:33:11 yamt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/vnode.h>
#include <sys/types.h>
#include <sys/syscallargs.h>
#include <sys/conf.h>

#include <miscfs/specfs/specdev.h>

#include <compat/common/compat_util.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_syscall.h>
#include <compat/svr4/svr4_syscallargs.h>

#include <compat/irix/irix_ioctl.h>
#include <compat/irix/irix_usema.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_types.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_syscallargs.h>

int
irix_sys_ioctl(struct lwp *l, const struct irix_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(void *) data;
	} */
	extern const struct cdevsw irix_usema_cdevsw;
	u_long	cmd;
	void *data;
	file_t *fp;
	struct vnode *vp;
	struct vattr vattr;
	struct irix_ioctl_usrdata iiu;
	int error, val, fd;

	/*
	 * This duplicates 6 lines from svr4_sys_ioctl()
	 * It would be nice to merge it.
	 */
	fd = SCARG(uap, fd);
	cmd = SCARG(uap, com);
	data = SCARG(uap, data);

	if ((fp = fd_getfile(fd)) == NULL)
		return EBADF;

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		fd_putfile(fd);
		return EBADF;
	}

	/*
	 * A special hook for /dev/usemaclone ioctls. Some of the ioctl
	 * commands need to set the return value, which is normally
	 * impossible in the file methods and lower. We do the job by
	 * copying the retval address and the data argument to a
	 * struct irix_ioctl_usrdata. The data argument
	 * is set to the address of the structure, and the underlying
	 * code will be able to retreive both data and the retval address
	 * from the struct irix_ioctl_usrdata.
	 *
	 * We also bypass the checks in sys_ioctl() because theses ioctl
	 * are defined _IO but really are _IOR. XXX need security review.
	 */
	if ((cmd & IRIX_UIOC_MASK) == IRIX_UIOC) {
		if (fp->f_type != DTYPE_VNODE) {
			fd_putfile(fd);
			return ENOTTY;
		}
		vp = fp->f_data;
		if (vp->v_type != VCHR ||
		    cdevsw_lookup(vp->v_rdev) != &irix_usema_cdevsw ||
		    minor(vp->v_rdev) != IRIX_USEMACLNDEV_MINOR) {
			error = ENOTTY;
			goto out;
		}

		iiu.iiu_data = data;
		iiu.iiu_retval = retval;

		error = (*fp->f_ops->fo_ioctl)(fp, cmd, &iiu);
out:
		fd_putfile(fd);
		return error;
	}

	switch (cmd) {
	case IRIX_SIOCNREAD: /* number of bytes to read */
		error = (*(fp->f_ops->fo_ioctl))(fp, FIONREAD,
		    SCARG(uap, data));
		fd_putfile(fd);
		return error;

	case IRIX_MTIOCGETBLKSIZE: /* get tape block size in 512B units */
		if (fp->f_type != DTYPE_VNODE) {
			fd_putfile(fd);
			return ENOSYS;
		}

		vp = fp->f_data;

		switch (vp->v_type) {
		case VREG:
		case VLNK:
		case VDIR:
			error = ENOTTY;
			break;
		case VCHR:
		case VFIFO:
			error = EINVAL;
			break;
		case VBLK:
			error = VOP_GETATTR(vp, &vattr, l->l_cred);
			if (error == 0) {
				val = vattr.va_blocksize / 512;
				error = copyout(&val, data, sizeof(int));
			}

		default:
			error = ENOSYS;
			break;
		}

		fd_putfile(fd);
		return error;

	default: /* Fallback to the standard SVR4 ioctl's */
		fd_putfile(fd);
		return svr4_sys_ioctl(l, (const void *)uap, retval);
	}
}
