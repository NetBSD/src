/*	$NetBSD: darwin_ioctl.c,v 1.7.4.1 2008/01/02 21:51:46 bouyer Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_ioctl.c,v 1.7.4.1 2008/01/02 21:51:46 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/proc.h>

#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_ioctl.h>
#include <compat/darwin/darwin_syscallargs.h>

static int vtype_to_dtype(int);

int
darwin_sys_ioctl(struct lwp *l, const struct darwin_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(void *) data;
	} */
	struct sys_ioctl_args cup;
	int error;

	switch (SCARG(uap, com)) {
	case DARWIN_FIODTYPE: { /* Get file d_type */
		struct proc *p = l->l_proc;
		struct file *fp;
		struct vnode *vp;
		int *data = SCARG(uap, data);
		int type;

		/* getvnode() will use the descriptor for us */
		if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
			return (error);

		vp = fp->f_data;
		type = vtype_to_dtype(vp->v_type);
		FILE_UNUSE(fp, l);

		error = copyout(&type, data, sizeof(*data));

		return error;
		break;
	}

	default:
		/* Try native ioctl */
		break;
	}

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, com) = SCARG(uap, com);
	SCARG(&cup, data) = SCARG(uap, data);

	error = sys_ioctl(l, &cup, retval);

	return error;
}

static int
vtype_to_dtype(int dtype)
{
	switch (dtype) {
	case VNON:
		return DT_UNKNOWN;
		break;
	case VREG:
		return DT_REG;
		break;
	case VDIR:
		return DT_DIR;
		break;
	case VBLK:
		return DT_BLK;
		break;
	case VCHR:
		return DT_CHR;
		break;
	case VLNK:
		return DT_LNK;
		break;
	case VSOCK:
		return DT_SOCK;
		break;
	case VFIFO:
		return DT_FIFO;
		break;
	case VBAD:
		return DT_WHT;
		break;
	default:
		break;
	}

	return DT_UNKNOWN;
}
