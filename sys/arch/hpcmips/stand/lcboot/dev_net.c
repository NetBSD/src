/* $NetBSD: dev_net.c,v 1.2 2003/08/09 11:37:57 igy Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
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
 *
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dev_net.c,v 1.2 2003/08/09 11:37:57 igy Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/bootp.h>
#include <lib/libkern/libkern.h>
#include <machine/stdarg.h>

#include "extern.h"

extern struct in_addr	servip;

static int	netdev_sock = -1;

int
net_strategy(void *devdata, int rw, daddr_t blk,
	     size_t size, void *buf , size_t *rsize)
{
	return EIO;
}

int
net_open(struct open_file *f, ...)
{
	char		*fname;
	char		**file;
	struct iodesc	*s;
	va_list		ap;

	va_start(ap, f);
	fname = va_arg(ap, char *);
	file = va_arg(ap, char **);
	va_end(ap);

	f->f_devdata = &netdev_sock;
	netdev_sock = netif_open(NULL);

	bootfile[0] = '\0';
	if (bootopts.b_flags & B_F_USE_BOOTP) {
		s = socktodesc(netdev_sock);
		bootp(netdev_sock);
		if (fname[0] != '\0')
			strlcpy(bootfile, fname, sizeof bootfile);
	} else {
		s = socktodesc(netdev_sock);

		servip = s->destip = bootopts.b_remote_ip;
		myip = s->myip = bootopts.b_local_ip;
		netmask = bootopts.b_netmask;
		gateip = bootopts.b_gate_ip;

		if (fname[0] == '\0') {
			printf("no boot filename\n");
			netif_close(netdev_sock);
			return EIO;
		}
		strlcpy(bootfile, fname, sizeof bootfile);
	}

	*file = bootfile;

	return 0;
}

int
net_close(struct open_file *f)
{
	int	sock;

	sock = *((int *) f->f_devdata);
	netif_close(sock);
	f->f_devdata = NULL;
	return 0;
}

int
net_ioctl(struct open_file *f, u_long cmd, void *data)
{
	return EIO;
}
