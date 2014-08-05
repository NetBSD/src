/* $NetBSD: dev_net.c,v 1.3 2014/08/05 17:55:20 joerg Exp $ */

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

#include <machine/bootinfo.h>

#include "globals.h"

static int netdev_sock = -1;
static int netdev_opens;

int
net_open(struct open_file *f, ...)

{
	va_list ap;
	char *file, *proto;
	int error;
	extern struct btinfo_bootpath bi_path;
	
	va_start(ap, f);
	file = va_arg(ap, char *);
	proto = va_arg(ap, char *);
	va_end(ap);

	if (netdev_opens > 0)
		return 0;

	if ((netdev_sock = netif_open(NULL)) < 0)
		return ENXIO;	/* never fails indeed */

	error = 0;
	bootp(netdev_sock);	/* send DHCP request */
	if (myip.s_addr == 0) {
		error = ENOENT;	/* IP address was not found */
		goto bad;
	}

	if (file[0] != '\0') 
		snprintf(bootfile, sizeof(bootfile), "%s", file);
	else if (bootfile[0] == '\0')
		snprintf(bootfile, sizeof(bootfile), "netbsd");

	if (strcmp(proto, "nfs") == 0
	    && (error = nfs_mount(netdev_sock, rootip, rootpath)) != 0)
		goto bad;

	snprintf(bi_path.bootpath, sizeof(bi_path.bootpath), "%s", bootfile);
	f->f_devdata = &netdev_sock;
	netdev_opens++;
	return 0;
 bad:
	netif_close(netdev_sock);
	netdev_sock = -1;
	return error;
}

int
net_close(struct open_file *f)
	
{
	f->f_devdata = NULL;
	if (--netdev_opens > 0)
		return 0;
	netif_close(netdev_sock);
	netdev_sock = -1;
	return 0;
}

int
net_strategy(void *devdata, int rw, daddr_t dblk, size_t size,
	void *p, size_t *rsize)
{

	return EIO;
}
