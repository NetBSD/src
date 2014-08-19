/*	$NetBSD: linux_ioctl.c,v 1.56.12.2 2014/08/20 00:03:32 tls Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_ioctl.c,v 1.56.12.2 2014/08/20 00:03:32 tls Exp $");

#if defined(_KERNEL_OPT)
#include "sequencer.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/sockio.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/ossaudio/ossaudio.h>
#define LINUX_TO_OSS(v) ((const void *)(v))	/* do nothing, same ioctl() encoding */

#include <dev/ic/mfiio.h>
#define LINUX_MEGARAID_CMD	_LINUX_IOWR('M', 1, struct mfi_ioc_packet)
#define LINUX_MEGARAID_GET_AEN	_LINUX_IOW('M', 3, struct mfi_ioc_aen)

/*
 * Most ioctl command are just converted to their NetBSD values,
 * and passed on. The ones that take structure pointers and (flag)
 * values need some massaging.
 */
int
linux_sys_ioctl(struct lwp *l, const struct linux_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(void *) data;
	} */
	int error;

	switch (LINUX_IOCGROUP(SCARG(uap, com))) {
	case 'M':
		switch(SCARG(uap, com)) {
		case LINUX_MEGARAID_CMD:
		case LINUX_MEGARAID_GET_AEN:
		{
			struct sys_ioctl_args ua;
			u_long com = 0;
			if (SCARG(uap, com) & IOC_IN)
				com |= IOC_OUT;
			if (SCARG(uap, com) & IOC_OUT)
				com |= IOC_IN;
			SCARG(&ua, fd) = SCARG(uap, fd);
			SCARG(&ua, com) = SCARG(uap, com);
			SCARG(&ua, com) &= ~IOC_DIRMASK;
			SCARG(&ua, com) |= com;
			SCARG(&ua, data) = SCARG(uap, data);
			error = sys_ioctl(l, (const void *)&ua, retval);
			break;
		}
		default:
			error = oss_ioctl_mixer(l, LINUX_TO_OSS(uap), retval);
			break;
		}
		break;
	case 'Q':
		error = oss_ioctl_sequencer(l, LINUX_TO_OSS(uap), retval);
		break;
	case 'P':
		error = oss_ioctl_audio(l, LINUX_TO_OSS(uap), retval);
		break;
	case 'V':	/* video4linux2 */
	case 'd':	/* drm */
	{
		struct sys_ioctl_args ua;
		u_long com = 0;
		if (SCARG(uap, com) & IOC_IN)
			com |= IOC_OUT;
		if (SCARG(uap, com) & IOC_OUT)
			com |= IOC_IN;
		SCARG(&ua, fd) = SCARG(uap, fd);
		SCARG(&ua, com) = SCARG(uap, com);
		SCARG(&ua, com) &= ~IOC_DIRMASK;
		SCARG(&ua, com) |= com;
		SCARG(&ua, data) = SCARG(uap, data);
		error = sys_ioctl(l, (const void *)&ua, retval);
		break;
	}
	case 'r': /* VFAT ioctls; not yet supported */
		error = ENOSYS;
		break;
	case 'S':
		error = linux_ioctl_cdrom(l, uap, retval);
		break;
	case 't':
	case 'f':
		error = linux_ioctl_termios(l, uap, retval);
		break;
	case 'm':
		error = linux_ioctl_mtio(l, uap, retval);
		break;
	case 'T':
	{
#if NSEQUENCER > 0
/* XXX XAX 2x check this. */
		/*
		 * Both termios and the MIDI sequencer use 'T' to identify
		 * the ioctl, so we have to differentiate them in another
		 * way.  We do it by indexing in the cdevsw with the major
		 * device number and check if that is the sequencer entry.
		 */
		bool is_sequencer = false;
		struct file *fp;
		struct vnode *vp;
		struct vattr va;
		extern const struct cdevsw sequencer_cdevsw;

		if ((fp = fd_getfile(SCARG(uap, fd))) == NULL)
			return EBADF;
		if (fp->f_type == DTYPE_VNODE &&
		    (vp = (struct vnode *)fp->f_data) != NULL &&
		    vp->v_type == VCHR) {
			vn_lock(vp, LK_SHARED | LK_RETRY);
			error = VOP_GETATTR(vp, &va, l->l_cred);
			VOP_UNLOCK(vp);
			if (error == 0 &&
			    cdevsw_lookup(va.va_rdev) == &sequencer_cdevsw)
				is_sequencer = true;
		}
		if (is_sequencer) {
			error = oss_ioctl_sequencer(l, (const void *)LINUX_TO_OSS(uap),
						   retval);
		}
		else {
			error = linux_ioctl_termios(l, uap, retval);
		}
		fd_putfile(SCARG(uap, fd));
#else
		error = linux_ioctl_termios(l, uap, retval);
#endif
	}
		break;
	case '"':
		error = linux_ioctl_sg(l, uap, retval);
		break;
	case 0x89:
		error = linux_ioctl_socket(l, uap, retval);
		break;
	case 0x03:
		error = linux_ioctl_hdio(l, uap, retval);
		break;
	case 0x02:
		error = linux_ioctl_fdio(l, uap, retval);
		break;
	case 0x12:
		error = linux_ioctl_blkio(l, uap, retval);
		break;
	default:
		error = linux_machdepioctl(l, uap, retval);
		break;
	}
	if (error == EPASSTHROUGH) {
		/*
		 * linux returns EINVAL or ENOTTY for not supported ioctls.
		 */ 
		error = EINVAL;
	}

	return error;
}
