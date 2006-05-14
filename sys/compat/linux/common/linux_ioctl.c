/*	$NetBSD: linux_ioctl.c,v 1.44 2006/05/14 21:24:50 elad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_ioctl.c,v 1.44 2006/05/14 21:24:50 elad Exp $");

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

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ioctl.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/ossaudio/ossaudio.h>
#define LINUX_TO_OSS(v) (v)	/* do nothing, same ioctl() encoding */

/*
 * Most ioctl command are just converted to their NetBSD values,
 * and passed on. The ones that take structure pointers and (flag)
 * values need some massaging. This is done the usual way by
 * allocating stackgap memory, letting the actual ioctl call do its
 * work there and converting back the data afterwards.
 */
int
linux_sys_ioctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	int error;

	switch (LINUX_IOCGROUP(SCARG(uap, com))) {
	case 'M':
		error = oss_ioctl_mixer(l, LINUX_TO_OSS(v), retval);
		break;
	case 'Q':
		error = oss_ioctl_sequencer(l, LINUX_TO_OSS(v), retval);
		break;
	case 'P':
		error = oss_ioctl_audio(l, LINUX_TO_OSS(v), retval);
		break;
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
		 * Both termios and the MIDI sequncer use 'T' to identify
		 * the ioctl, so we have to differentiate them in another
		 * way.  We do it by indexing in the cdevsw with the major
		 * device number and check if that is the sequencer entry.
		 */
		struct file *fp;
		struct filedesc *fdp;
		struct vnode *vp;
		struct vattr va;
		extern const struct cdevsw sequencer_cdevsw;
		struct proc *p = l->l_proc;

		fdp = p->p_fd;
		if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
			return EBADF;
		FILE_USE(fp);
		if (fp->f_type == DTYPE_VNODE &&
		    (vp = (struct vnode *)fp->f_data) != NULL &&
		    vp->v_type == VCHR &&
		    VOP_GETATTR(vp, &va, p->p_cred, l) == 0 &&
		    cdevsw_lookup(va.va_rdev) == &sequencer_cdevsw) {
			error = oss_ioctl_sequencer(l, (void*)LINUX_TO_OSS(uap),
						   retval);
		}
		else {
			error = linux_ioctl_termios(l, uap, retval);
		}
		FILE_UNUSE(fp, l);
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
