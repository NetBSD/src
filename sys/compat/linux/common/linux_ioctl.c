/*	$NetBSD: linux_ioctl.c,v 1.25.2.4 2002/05/29 21:32:39 nathanw Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_ioctl.c,v 1.25.2.4 2002/05/29 21:32:39 nathanw Exp $");

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
	struct proc *p = l->l_proc;

	switch (LINUX_IOCGROUP(SCARG(uap, com))) {
	case 'M':
		return oss_ioctl_mixer(p, LINUX_TO_OSS(v), retval);
	case 'Q':
		return oss_ioctl_sequencer(p, LINUX_TO_OSS(v), retval);
	case 'P':
		return oss_ioctl_audio(p, LINUX_TO_OSS(v), retval);
	case 'S':
		return linux_ioctl_cdrom(p, uap, retval);
	case 't':
	case 'f':
		return linux_ioctl_termios(p, uap, retval);
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
		extern int sequencerioctl 
			__P((dev_t, u_long, caddr_t, int, struct proc *));

		fdp = p->p_fd;
		if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
			return EBADF;
		if (fp->f_type == DTYPE_VNODE &&
		    (vp = (struct vnode *)fp->f_data) != NULL &&
		    vp->v_type == VCHR &&
		    VOP_GETATTR(vp, &va, p->p_ucred, p) == 0 &&
		    major(va.va_rdev) < nchrdev &&
		    cdevsw[major(va.va_rdev)].d_ioctl == &sequencerioctl)
			return oss_ioctl_sequencer(p, (void*)LINUX_TO_OSS(uap),
						   retval);
		else
#endif
			return linux_ioctl_termios(p, uap, retval);
	}
	case 0x89:
		return linux_ioctl_socket(p, uap, retval);
	case 0x03:
		return linux_ioctl_hdio(p, uap, retval);
	case 0x02:
		return linux_ioctl_fdio(p, uap, retval);
	case 0x12:
		return linux_ioctl_blkio(p, uap, retval);
	default:
		return linux_machdepioctl(p, uap, retval);
	}
}
