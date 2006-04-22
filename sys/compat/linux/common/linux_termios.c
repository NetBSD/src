/*	$NetBSD: linux_termios.c,v 1.24.6.1 2006/04/22 11:38:13 simonb Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
__KERNEL_RCSID(0, "$NetBSD: linux_termios.c,v 1.24.6.1 2006/04/22 11:38:13 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ptm.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/termios.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_termios.h>

#include <compat/linux/linux_syscallargs.h>

int
linux_ioctl_termios(l, uap, retval)
	struct lwp *l;
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	struct linux_termio tmplt;
	struct linux_termios tmplts;
	struct termios tmpbts;
	int idat;
	struct sys_ioctl_args ia;
	int error;
	char tioclinux;
	int (*bsdioctl)(struct file *, u_long, void *, struct lwp *);

	fdp = l->l_proc->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	FILE_USE(fp);

	bsdioctl = fp->f_ops->fo_ioctl;
	com = SCARG(uap, com);
	retval[0] = 0;

	switch (com & 0xffff) {
	case LINUX_TCGETS:
		error = (*bsdioctl)(fp, TIOCGETA, (caddr_t)&tmpbts, l);
		if (error)
			goto out;
		bsd_termios_to_linux_termios(&tmpbts, &tmplts);
		error = copyout(&tmplts, SCARG(uap, data), sizeof tmplts);
		goto out;
	case LINUX_TCSETS:
	case LINUX_TCSETSW:
	case LINUX_TCSETSF:
		/*
		 * First fill in all fields, so that we keep the current
		 * values for fields that Linux doesn't know about.
		 */
		error = (*bsdioctl)(fp, TIOCGETA, (caddr_t)&tmpbts, l);
		if (error)
			goto out;
		error = copyin(SCARG(uap, data), &tmplts, sizeof tmplts);
		if (error)
			goto out;
		linux_termios_to_bsd_termios(&tmplts, &tmpbts);
		switch (com) {
		case LINUX_TCSETS:
			com = TIOCSETA;
			break;
		case LINUX_TCSETSW:
			com = TIOCSETAW;
			break;
		case LINUX_TCSETSF:
			com = TIOCSETAF;
			break;
		}
		error = (*bsdioctl)(fp, com, (caddr_t)&tmpbts, l);
		goto out;
	case LINUX_TCGETA:
		error = (*bsdioctl)(fp, TIOCGETA, (caddr_t)&tmpbts, l);
		if (error)
			goto out;
		bsd_termios_to_linux_termio(&tmpbts, &tmplt);
		error = copyout(&tmplt, SCARG(uap, data), sizeof tmplt);
		goto out;
	case LINUX_TCSETA:
	case LINUX_TCSETAW:
	case LINUX_TCSETAF:
		/*
		 * First fill in all fields, so that we keep the current
		 * values for fields that Linux doesn't know about.
		 */
		error = (*bsdioctl)(fp, TIOCGETA, (caddr_t)&tmpbts, l);
		if (error)
			goto out;
		error = copyin(SCARG(uap, data), &tmplt, sizeof tmplt);
		if (error)
			goto out;
		linux_termio_to_bsd_termios(&tmplt, &tmpbts);
		switch (com) {
		case LINUX_TCSETA:
			com = TIOCSETA;
			break;
		case LINUX_TCSETAW:
			com = TIOCSETAW;
			break;
		case LINUX_TCSETAF:
			com = TIOCSETAF;
			break;
		}
		error = (*bsdioctl)(fp, com, (caddr_t)&tmpbts, l);
		goto out;
	case LINUX_TCFLSH:
		switch((u_long)SCARG(uap, data)) {
		case 0:
			idat = FREAD;
			break;
		case 1:
			idat = FWRITE;
			break;
		case 2:
			idat = 0;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		error = (*bsdioctl)(fp, TIOCFLUSH, (caddr_t)&idat, l);
		goto out;
	case LINUX_TIOCGETD:
		error = (*bsdioctl)(fp, TIOCGETD, (caddr_t)&idat, l);
		if (error)
			goto out;
		switch (idat) {
		case TTYDISC:
			idat = LINUX_N_TTY;
			break;
		case SLIPDISC:
			idat = LINUX_N_SLIP;
			break;
		case PPPDISC:
			idat = LINUX_N_PPP;
			break;
		case STRIPDISC:
			idat = LINUX_N_STRIP;
			break;
		/*
		 * Linux does not have the tablet line discipline.
		 */
		case TABLDISC:
		default:
			idat = -1;	/* XXX What should this be? */
			break;
		}
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		goto out;
	case LINUX_TIOCSETD:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		switch (idat) {
		case LINUX_N_TTY:
			idat = TTYDISC;
			break;
		case LINUX_N_SLIP:
			idat = SLIPDISC;
			break;
		case LINUX_N_PPP:
			idat = PPPDISC;
			break;
		case LINUX_N_STRIP:
			idat = STRIPDISC;
			break;
		/*
		 * We can't handle the mouse line discipline Linux has.
		 */
		case LINUX_N_MOUSE:
		case LINUX_N_AX25:
		case LINUX_N_X25:
		case LINUX_N_6PACK:
		default:
			error = EINVAL;
			goto out;
		}
		error = (*bsdioctl)(fp, TIOCSETD, (caddr_t)&idat, l);
		goto out;
	case LINUX_TIOCLINUX:
		error = copyin(SCARG(uap, data), &tioclinux, sizeof tioclinux);
		if (error != 0)
			goto out;
		switch (tioclinux) {
		case LINUX_TIOCLINUX_KERNMSG:
			/*
			 * XXX needed to not fail for some things. Could
			 * try to use TIOCCONS, but the char argument
			 * specifies the VT #, not an fd.
			 */
			error = 0;
			goto out;
		case LINUX_TIOCLINUX_COPY:
		case LINUX_TIOCLINUX_PASTE:
		case LINUX_TIOCLINUX_UNBLANK:
		case LINUX_TIOCLINUX_LOADLUT:
		case LINUX_TIOCLINUX_READSHIFT:
		case LINUX_TIOCLINUX_READMOUSE:
		case LINUX_TIOCLINUX_VESABLANK:
		case LINUX_TIOCLINUX_CURCONS:	/* could use VT_GETACTIVE */
			error = EINVAL;
			goto out;
		}
		break;
	case LINUX_TIOCGWINSZ:
		SCARG(&ia, com) = TIOCGWINSZ;
		break;
	case LINUX_TIOCSWINSZ:
		SCARG(&ia, com) = TIOCSWINSZ;
		break;
	case LINUX_TIOCGPGRP:
		SCARG(&ia, com) = TIOCGPGRP;
		break;
	case LINUX_TIOCSPGRP:
		SCARG(&ia, com) = TIOCSPGRP;
		break;
	case LINUX_FIONREAD:
		SCARG(&ia, com) = FIONREAD;
		break;
	case LINUX_FIONBIO:
		SCARG(&ia, com) = FIONBIO;
		break;
	case LINUX_FIOASYNC:
		SCARG(&ia, com) = FIOASYNC;
		break;
	case LINUX_TIOCEXCL:
		SCARG(&ia, com) = TIOCEXCL;
		break;
	case LINUX_TIOCNXCL:
		SCARG(&ia, com) = TIOCNXCL;
		break;
	case LINUX_TIOCCONS:
		SCARG(&ia, com) = TIOCCONS;
		break;
	case LINUX_TIOCNOTTY:
		SCARG(&ia, com) = TIOCNOTTY;
		break;
	case LINUX_TCSBRK:
		SCARG(&ia, com) = SCARG(uap, data) ? TIOCDRAIN : TIOCSBRK;
		break;
	case LINUX_TIOCMGET:
		SCARG(&ia, com) = TIOCMGET;
		break;
	case LINUX_TIOCMSET:
		SCARG(&ia, com) = TIOCMSET;
		break;
	case LINUX_TIOCMBIC:
		SCARG(&ia, com) = TIOCMBIC;
		break;
	case LINUX_TIOCMBIS:
		SCARG(&ia, com) = TIOCMBIS;
		break;
#ifdef LINUX_TIOCGPTN
	case LINUX_TIOCGPTN:
#ifndef NO_DEV_PTM
		{
			caddr_t sg = stackgap_init(l->l_proc, 0);
			struct ptmget ptm, *ptmp = stackgap_alloc(l->l_proc, &sg,
				sizeof(*ptmp));

			SCARG(&ia, fd) = SCARG(uap, fd);
			SCARG(&ia, com) = TIOCPTSNAME;
			SCARG(&ia, data) = ptmp;

			if ((error = sys_ioctl(curlwp, &ia, retval)) != 0)
				goto out;

			if ((error = copyin(ptmp, &ptm, sizeof(ptm))) != 0)
				printf("copyin %d\n", error);

			error = copyout(&ptm.sfd, SCARG(uap, data),
			    sizeof(ptm.sfd));
			goto out;
		}
#endif /* NO_DEV_PTM */
#endif /* LINUX_TIOCGPTN */
	default:
		error = EINVAL;
		goto out;
	}

	SCARG(&ia, fd) = SCARG(uap, fd);
	SCARG(&ia, data) = SCARG(uap, data);
	/* XXX NJWLWP */
	error = sys_ioctl(curlwp, &ia, retval);
out:
	FILE_UNUSE(fp, l);
	return error;
}
