/*	$NetBSD: svr4_32_ioctl.c,v 1.15.18.2 2007/12/26 21:39:17 ad Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_ioctl.c,v 1.15.18.2 2007/12/26 21:39:17 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <net/if.h>
#include <sys/malloc.h>

#include <sys/syscallargs.h>

#include <compat/sys/socket.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_util.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <compat/svr4_32/svr4_32_lwp.h>
#include <compat/svr4_32/svr4_32_ucontext.h>
#include <compat/svr4_32/svr4_32_syscallargs.h>
#include <compat/svr4_32/svr4_32_stropts.h>
#include <compat/svr4_32/svr4_32_ioctl.h>
#include <compat/svr4_32/svr4_32_termios.h>
#include <compat/svr4/svr4_ttold.h>
#include <compat/svr4/svr4_filio.h>
#include <compat/svr4_32/svr4_32_sockio.h>


#ifdef DEBUG_SVR4
static void svr4_32_decode_cmd(netbsd32_u_long, char *, char *, int *, int *);
/*
 * Decode an ioctl command symbolically
 */
static void
svr4_32_decode_cmd(cmd, dir, c, num, argsiz)
	netbsd32_u_long	  cmd;
	char		 *dir, *c;
	int		 *num, *argsiz;
{
	if (cmd & SVR4_IOC_VOID)
		*dir++ = 'V';
	if (cmd & SVR4_IOC_IN)
		*dir++ = 'R';
	if (cmd & SVR4_IOC_OUT)
		*dir++ = 'W';
	*dir = '\0';
	if (cmd & SVR4_IOC_INOUT)
		*argsiz = (cmd >> 16) & 0xff;
	else
		*argsiz = -1;

	*c = (cmd >> 8) & 0xff;
	*num = cmd & 0xff;
}
#endif

int
svr4_32_sys_ioctl(struct lwp *l, const struct svr4_32_sys_ioctl_args *uap, register_t *retval)
{
	struct proc 	*p = l->l_proc;
	struct file	*fp;
	struct filedesc	*fdp;
	u_long		 cmd;
	int (*fun)(struct file *, struct lwp *, register_t *,
			int, u_long, void *);
#ifdef DEBUG_SVR4
	char		 dir[4];
	char		 c;
	int		 num;
	int		 argsiz;

	svr4_32_decode_cmd(SCARG(uap, com), dir, &c, &num, &argsiz);

	uprintf("svr4_32_ioctl(%d, _IO%s(%c, %d, %d), %#x);\n", SCARG(uap, fd),
	    dir, c, num, argsiz, SCARG(uap, data));
#endif
	fdp = p->p_fd;
	cmd = SCARG(uap, com);

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return EBADF;

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return EBADF;

	switch (cmd & 0xff00) {
	case SVR4_tIOC:
		fun = svr4_ttold_ioctl;
		break;

	case SVR4_TIOC:
		fun = svr4_32_term_ioctl;
		break;

	case SVR4_STR:
		fun = svr4_32_stream_ioctl;
		break;

	case SVR4_FIOC:
		fun = svr4_fil_ioctl;
		break;

	case SVR4_SIOC:
		fun = svr4_32_sock_ioctl;
		break;

	case SVR4_XIOC:
		/* We do not support those */
		return EINVAL;

	default:
		DPRINTF(("Unimplemented ioctl %lx\n", cmd));
		return 0;	/* XXX: really ENOSYS */
	}
	return (*fun)(fp, l, retval, SCARG(uap, fd), cmd,
		      SCARG_P32(uap, data));
}
