/*	$NetBSD: svr4_ioctl.c,v 1.2 1994/06/29 06:30:33 cgd Exp $	*/

/*
 * Copyright (c) 1994 Christos Zoulas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

struct svr4_ioctl_args {
	int	fd;
	int	cmd;
	caddr_t	data;
};

#define	SVR4_IOC_VOID	0x20000000
#define	SVR4_IOC_OUT	0x40000000
#define	SVR4_IOC_IN	0x80000000
#define	SVR4_IOC_INOUT	(SVR4_IOC_IN|SVR4_IOC_OUT)

#ifdef DEBUG_SVR4
/*
 * Decode an ioctl command symbolically
 */
static void
svr4_decode_cmd(cmd, dir, c, num, argsiz)
	int cmd;
	char **dir, *c;
	int *num, *argsiz;
{
	*dir = "";
	if (cmd & SVR4_IOC_VOID)
		*dir = "V";
	if (cmd & SVR4_IOC_INOUT)
		*dir = "RW";
	if (cmd & SVR4_IOC_OUT)
		*dir = "W";
	if (cmd & SVR4_IOC_IN)
		*dir = "R";
	if (cmd & (SVR4_IOC_INOUT|SVR4_IOC_VOID))
		*argsiz = (cmd >> 16) & 0xff;
	else
		*argsiz = -1;

	*c = (cmd >> 8) & 0xff;
	*num = cmd & 0xff;
}
#endif

int
svr4_ioctl(p, uap, retval)
	register struct proc *p;
	register struct svr4_ioctl_args *uap;
	int *retval;
{
	char *dir;
	char c;
	int  num;
	int  argsiz;
#ifdef DEBUG_SVR4
	svr4_decode_cmd(uap->cmd, &dir, &c, &num, &argsiz);
	printf("svr4_ioctl(%d, _IO%s(%c, %d, %d))\n", uap->fd,
	       dir, c, num, argsiz);
#endif
	return ENOSYS;
}
