/* $NetBSD: dev_net.c,v 1.7.6.1 2010/05/30 05:17:05 rmind Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/param.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/bootp.h>
#include <lib/libsa/nfs.h>

#include <lib/libkern/libkern.h>

#include "globals.h"

static int netdev_sock = -1;
static int netdev_opens;

int
net_open(struct open_file *f, ...)
{
	int error = 0;
	
	if (netdev_opens == 0) {
		if ((netdev_sock = netif_open(NULL)) < 0) {
			error = errno;
			goto bad;
		}

		/* send DHCP request */
		bootp(netdev_sock);

		/* IP address was not found */
		if (myip.s_addr == 0) {
			error = ENOENT;
			goto bad;
		}

		if (bootfile[0] == '\0')
			strcpy(bootfile, "netbsd");

		if (nfs_mount(netdev_sock, rootip, rootpath) != 0) {
			error = errno;
			goto bad;
		}
	}
	netdev_opens++;
bad:
	return (error);
}

int
net_close(struct open_file *f)
	
{
	if (--netdev_opens > 0)
		return (0);
	netif_close(netdev_sock);
	netdev_sock = -1;
	return (0);
}

int
net_strategy(void *d, int f, daddr_t b, size_t s, void *buf, size_t *r)
{

	return (EIO);
}
