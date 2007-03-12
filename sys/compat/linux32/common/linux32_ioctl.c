/*	$NetBSD: linux32_ioctl.c,v 1.4.6.1 2007/03/12 05:52:29 rmind Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux32_ioctl.c,v 1.4.6.1 2007/03/12 05:52:29 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ucred.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_ioctl.h>
#include <compat/linux32/common/linux32_termios.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/linux32_syscallargs.h>

extern int linux_ioctl_socket(struct lwp *, 
    struct linux_sys_ioctl_args *, register_t *);

int
linux32_sys_ioctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) com;
		syscallarg(netbsd32_charp) data;
	} */ *uap = v;
	int group;
	int error;

	group = LINUX32_IOCGROUP((int)SCARG(uap, com));

#ifdef DEBUG_LINUX
	printf("linux32_sys_ioctl(%d, 0x%x/\'%c\', %p)\n", SCARG(uap, fd),
	    SCARG(uap, com), (char)group, NETBSD32PTR64(SCARG(uap, data)));
#endif

	switch(group) {
	case 'T':
		error = linux32_ioctl_termios(l, uap, retval);
		break;
	case 0x89: {
		struct linux_sys_ioctl_args cup;

		SCARG(&cup, fd) = SCARG(uap, fd);
		SCARG(&cup, com) = (u_long)SCARG(uap, com);
		SCARG(&cup, data) = (void *)(u_long)SCARG(uap, data);
		error = linux_ioctl_socket(l, &cup, retval);
		break;
	}
	default:
		printf("Not yet implemented ioctl group \'%c\'\n", group);
		error = EINVAL;
		break;
	}

	if (error == EPASSTHROUGH)
		error = EINVAL;

	return error;
}
