/*	$NetBSD: linux_fdio.c,v 1.1.2.2 2000/12/13 15:49:49 bouyer Exp $	*/

/*
 * Copyright (c) 2000 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/disklabel.h>

#include <sys/fdio.h>

#include <sys/syscallargs.h>

#include <dev/isa/fdreg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_fdio.h>

#include <compat/linux/linux_syscallargs.h>

int
linux_ioctl_fdio(struct proc *p, struct linux_sys_ioctl_args *uap,
		 register_t *retval)
{
	struct filedesc *fdp;
	struct file *fp;
	int error;
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));
	u_long com;
	struct fdformat_parms fparams;
	struct linux_floppy_struct lflop;
	struct linux_floppy_drive_struct ldrive;

	com = (u_long)SCARG(uap, data);

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	com = SCARG(uap, com);
	ioctlf = fp->f_ops->fo_ioctl;

	retval[0] = error = 0;

	switch (com) {
	case LINUX_FDMSGON:
	case LINUX_FDMSGOFF:
	case LINUX_FDTWADDLE:
	case LINUX_FDCLRPRM:
		/* whatever you say */
		break;
	case LINUX_FDPOLLDRVSTAT:
		/*
		 * Just fill in some innocent defaults.
		 */
		memset(&ldrive, 0, sizeof ldrive);
		ldrive.fd_ref = 1;
		ldrive.maxblock = 2;
		ldrive.maxtrack = ldrive.track = 1;
		ldrive.flags = LINUX_FD_DISK_WRITABLE;
		error = copyout(&ldrive, SCARG(uap, data), sizeof ldrive);
		break;
	case LINUX_FDGETPRM:
		error = ioctlf(fp, FDIOCGETFORMAT, (caddr_t)&fparams, p);
		if (error != 0)
			break;
		lflop.size = fparams.ncyl * fparams.nspt * fparams.ntrk;
		lflop.sect = fparams.nspt;
		lflop.head = fparams.ntrk;
		lflop.track = fparams.ncyl;
		lflop.stretch = fparams.stepspercyl == 2 ? 1 : 0;
		lflop.fmt_gap = fparams.gaplen;
		/*
		 * XXXX Use knowledge of floppy for these fields.
		 * FDIOCGETFORMAT does not provide enough information.
		 * It would be better to have FDIOCGETFORMAT return more
		 * information.
		 */
		switch (fparams.xfer_rate) {
		case 500 * 1024:
			lflop.spec1 = 0xcf;
			lflop.gap = 0x1b;
			lflop.rate = FDC_500KBPS;
			break;
		case 300 * 1024:
			lflop.spec1 = 0xdf;
			lflop.gap = 0x23;
			lflop.rate = FDC_300KBPS;
			break;
		case 250 * 1024:
			lflop.spec1 = 0xdf;
			lflop.gap = 0x2a;
			lflop.rate = FDC_250KBPS;
			break;
		}
		error = copyout(&lflop, SCARG(uap, data), sizeof lflop);
		break;
	case LINUX_FDSETPRM:
		/*
		 * Should use FDIOCSETFORMAT here, iff its interface
		 * is extended.
		 */
	case LINUX_FDDEFPRM:
	case LINUX_FDFMTBEG:
	case LINUX_FDFMTTRK:
	case LINUX_FDFMTEND:
	case LINUX_FDSETEMSGTRESH:
	case LINUX_FDFLUSH:
	case LINUX_FDSETMAXERRS:
	case LINUX_FDGETMAXERRS:
	case LINUX_FDGETDRVTYP:
	case LINUX_FDSETDRVPRM:
	case LINUX_FDGETDRVPRM:
	case LINUX_FDGETDRVSTAT:
	case LINUX_FDRESET:
	case LINUX_FDGETFDCSTAT:
	case LINUX_FDWERRORCLR:
	case LINUX_FDWERRORGET:
	case LINUX_FDRAWCMD:
	case LINUX_FDEJECT:
	default:
		error = EINVAL;
	}

	FILE_UNUSE(fp, p);

	return 0;
}
