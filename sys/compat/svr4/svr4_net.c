/*	$NetBSD: svr4_net.c,v 1.1 1994/11/14 06:13:17 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * Emulate /dev/{udp,tcp,...}
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>


#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_ioctl.h>

/*
 * Device minor numbers
 */
enum {
	dev_arp		= 26,
	dev_icmp	= 27,
	dev_ip		= 28,
	dev_tcp		= 35,
	dev_udp		= 36
};


static struct	fileops svr4_netops =
    { soo_read, soo_write, soo_ioctl, soo_select, soo_close };

/*
 * Used by new config, but we don't need it.
 */
int
svr4_netattach(n)
	int n;
{
	return 0;
}


int
svr4_netopen(dev, flag, mode, p, fp)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
	struct file *fp;
{
	int type;
	int protocol;
	struct socket *so;
	int error;

	DPRINTF(("netopen("));

	if (fp == NULL)
		return ENODEV;

	switch (minor(dev)) {
	case dev_udp:
		type = SOCK_DGRAM;
		protocol = IPPROTO_UDP;
		DPRINTF(("udp, "));
		break;

	case dev_tcp:
		type = SOCK_STREAM;
		protocol = IPPROTO_TCP;
		DPRINTF(("tcp, "));
		break;

	case dev_ip:
		type = SOCK_RAW;
		protocol = IPPROTO_IP;
		DPRINTF(("ip, "));
		break;

	case dev_icmp:
		type = SOCK_RAW;
		protocol = IPPROTO_ICMP;
		DPRINTF(("icmp, "));
		break;

	default:
		DPRINTF(("%d);\n", minor(dev)));
		return EOPNOTSUPP;
	}


	if ((error = socreate(AF_INET, &so, type, protocol)) != 0) {
		DPRINTF(("socreate error %d\n", error));
		return error;
	}

	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &svr4_netops;
	fp->f_data = (caddr_t)so;
	DPRINTF(("ok);\n"));

	return 0;
}
