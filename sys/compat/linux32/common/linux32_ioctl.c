/*	$NetBSD: linux32_ioctl.c,v 1.13 2009/08/18 02:02:58 christos Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_ioctl.c,v 1.13 2009/08/18 02:02:58 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/ioctl.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_ioctl.h>
#include <compat/linux32/common/linux32_termios.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/linux32_syscallargs.h>

#include <compat/ossaudio/ossaudio.h>
#include <compat/ossaudio/ossaudiovar.h>

int
linux32_sys_ioctl(struct lwp *l, const struct linux32_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) com;
		syscallarg(netbsd32_charp) data;
	} */
	struct oss_sys_ioctl_args ossuap;
	int group;
	int error;

	group = LINUX32_IOCGROUP((int)SCARG(uap, com));

#ifdef DEBUG_LINUX
	printf("linux32_sys_ioctl(%d, 0x%x/\'%c\', %p)\n", SCARG(uap, fd),
	    SCARG(uap, com), (char)group, SCARG_P32(uap, data));
#endif

	switch(group) {
	case 'T':
		error = linux32_ioctl_termios(l, uap, retval);
		break;
	case 'M':
	case 'Q':
	case 'P':
		SCARG(&ossuap, fd) = SCARG(uap, fd);
		SCARG(&ossuap, com) = (u_long)SCARG(uap, com);
		SCARG(&ossuap, data) = SCARG_P32(uap, data);
		switch (group) {
		case 'M':
			error = oss_ioctl_mixer(l, &ossuap, retval);
			break;
		case 'Q':
			error = oss_ioctl_sequencer(l, &ossuap, retval);
			break;
		case 'P':
			error = oss_ioctl_audio(l, &ossuap, retval);
			break;
		default:
			error = EINVAL; /* shutup gcc */
			break;
		}
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
		SCARG(&ua, data) = SCARG_P32(uap, data);
		error = sys_ioctl(l, (const void *)&ua, retval);
		break;
	}
	case 0x89:
		error = linux32_ioctl_socket(l, uap, retval);
		break;
	default:
		printf("Not yet implemented ioctl group \'%c\'\n", group);
		error = EINVAL;
		break;
	}

	if (error == EPASSTHROUGH)
		error = EINVAL;

	return error;
}
