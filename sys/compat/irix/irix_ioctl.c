/*	$NetBSD: irix_ioctl.c,v 1.2 2002/05/22 05:14:01 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: irix_ioctl.c,v 1.2 2002/05/22 05:14:01 manu Exp $");

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
irix_sys_ioctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	u_long	cmd;
	caddr_t data;
	struct file *fp;
	struct filedesc *fdp;
	struct vnode *vp;
	struct irix_ioctl_usrdata iiu;
	struct irix_ioctl_usrdata *iiup;
	caddr_t sg = stackgap_init(p, 0);
	int error;

	/* 
	 * This duplicates 6 lines from svr4_sys_ioctl() 
	 * It would be nice to merge it.
	 */
	fdp = p->p_fd;
	cmd = SCARG(uap, com);
	data = SCARG(uap, data);
	
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return EBADF;

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return EBADF;

	/*
	 * A special hook for /dev/usemaclone ioctls. Some of the ioctl
	 * commands need to set the return value, which is normally
	 * impossible in the file methods and lower. We do the job by
	 * copying the retval address and the data argument to a 
	 * struct irix_ioctl_usrdata in the stackgap. The data argument
	 * is set to the address of the structure, and the underlying 
	 * code will be able to retreive both data and the retval address
	 * by fecthing the struct irix_ioctl_usrdata.
	 *
	 * We also bypass the checks in sys_ioctl() because theses ioctl
	 * are defined _IO but really are _IOR. XXX need security review.
	 */
	if ((cmd & IRIX_UIOC_MASK) == IRIX_UIOC) {
		FILE_USE(fp);
		vp = (struct vnode*)fp->f_data;
		if (vp->v_type != VCHR || vp->v_rdev != irix_usemaclonedev) {
			error = ENOTTY;
			goto out;
		}

		iiup = stackgap_alloc(p, &sg, sizeof(iiu));	
		iiu.iiu_data = data;
		iiu.iiu_retval = retval;
		data = (caddr_t)iiup;
		if ((error = copyout(&iiu, iiup, sizeof(iiu))) != 0) 
			goto out;

		error = (*fp->f_ops->fo_ioctl)(fp, cmd, data, p);
out:
		FILE_UNUSE(fp, p);
		return error;
	}

	switch (cmd) {
	case IRIX_SIOCNREAD: /* number of bytes to read */
		return (*(fp->f_ops->fo_ioctl))(fp, FIONREAD, 
		    SCARG(uap, data), p);
		break;	

	default: /* Fallback to the standard SVR4 ioctl's */
		error = svr4_sys_ioctl(p, v, retval);
		break;
	}

	return error;
}
