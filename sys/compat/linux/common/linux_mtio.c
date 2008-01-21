/* $NetBSD: linux_mtio.c,v 1.1.10.5 2008/01/21 09:41:26 yamt Exp $ */

/*
 * Copyright (c) 2005 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_mtio.c,v 1.1.10.5 2008/01/21 09:41:26 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/proc.h>

#include <sys/mtio.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_mtio.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

static const struct mtop_mapping {
	short	lop;
	short	op;
} mtop_map[] = {
	{ LINUX_MTFSF,		MTFSF },
	{ LINUX_MTBSF,		MTBSF },
	{ LINUX_MTFSR,		MTFSR },
	{ LINUX_MTBSR,		MTBSR },
	{ LINUX_MTWEOF,		MTWEOF },
	{ LINUX_MTREW,		MTREW },
	{ LINUX_MTOFFL,		MTOFFL },
	{ LINUX_MTNOP,		MTNOP },
	{ LINUX_MTRETEN,	MTRETEN },
	{ LINUX_MTEOM,		MTEOM },
	{ LINUX_MTERASE,	MTERASE },
	{ LINUX_MTSETBLK,	MTSETBSIZ },
	{ LINUX_MTSETDENSITY,	MTSETDNSTY },
	{ LINUX_MTCOMPRESSION,	MTCMPRESS },
	{ -1, -1 }
};

int
linux_ioctl_mtio(struct lwp *l, const struct linux_sys_ioctl_args *uap,
    register_t *retval)
{
	struct file *fp;
	struct filedesc *fdp;
	u_long com = SCARG(uap, com);
	int i, error = 0;
	int (*ioctlf)(struct file *, u_long, void *, struct lwp *);
	struct linux_mtop lmtop;
	struct linux_mtget lmtget;
	struct mtop mt;

        fdp = l->l_proc->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return EBADF;

	FILE_USE(fp);
	ioctlf = fp->f_ops->fo_ioctl;

	*retval = 0;
	switch (com) {
	case LINUX_MTIOCTOP:
		error = copyin(SCARG(uap, data), &lmtop, sizeof lmtop);
		for (i = 0; mtop_map[i].lop >= 0; i++) {
			if (mtop_map[i].lop == lmtop.mt_op)
				break;
		}

		if (mtop_map[i].lop == -1) {
			error = EINVAL;
			break;
		}
		
		mt.mt_op = mtop_map[i].op;
		mt.mt_count = lmtop.mt_count;
		error = ioctlf(fp, MTIOCTOP, (void *)&mt, l);
		break;
	case LINUX_MTIOCGET:
		lmtget.mt_type = LINUX_MT_ISUNKNOWN;
		lmtget.mt_resid = 0;
		lmtget.mt_dsreg = 0;
		lmtget.mt_gstat = 0;
		lmtget.mt_erreg = 0;
		lmtget.mt_fileno = 0;
		lmtget.mt_blkno = 0;
                error = copyout(&lmtget, SCARG(uap, data), sizeof lmtget);
		break;
	case LINUX_MTIOCPOS:
	default:
		printf("linux_mtio unsupported ioctl 0x%lx\n", com);
		error = ENODEV;
		break;
	}

	FILE_UNUSE(fp, l);

	return error;
}
