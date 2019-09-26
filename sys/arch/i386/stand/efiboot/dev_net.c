/*	$NetBSD: dev_net.c,v 1.3 2019/09/26 12:21:03 nonaka Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

/*
 * This module implements a "raw device" interface suitable for
 * use by the stand-alone I/O library NFS code.  This interface
 * does not support any "block" access, and exists only for the
 * purpose of initializing the network interface, getting boot
 * parameters, and performing the NFS mount.
 *
 * At open time, this does:
 *
 * find interface      - netif_open()
 * RARP for IP address - rarp_getipaddress()
 * RPC/bootparams      - callrpc(d, RPC_BOOTPARAMS, ...)
 * RPC/mountd          - nfs_mount(sock, ip, path)
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libkern/libkern.h>

#include "stand.h"
#include "net.h"
#include "netif.h"
#include "nfs.h"
#include "bootparam.h"
#include "dev_net.h"
#ifdef SUPPORT_BOOTP
#include "bootp.h"
#endif

static int netdev_sock = -1;
static int netdev_opens;

static int net_getparams(int);

/*
 * Called by devopen after it sets f->f_dev to our devsw entry.
 * This opens the low-level device and sets f->f_devdata.
 * This is declared with variable arguments...
 */
int
net_open(struct open_file *f, ...)
{
	va_list ap;
	struct devdesc *dev;
	int error = 0;

	va_start(ap, f);
	dev = va_arg(ap, struct devdesc *);
	va_end(ap);

#ifdef	NETIF_DEBUG
	if (debug)
		printf("%s\n", dev->devname);
#endif

	/* On first open, do netif open, mount, etc. */
	if (netdev_opens == 0) {
		/* Find network interface. */
		if (netdev_sock < 0) {
			netdev_sock = netif_open(dev);
			if (netdev_sock < 0) {
				printf("netif_open() failed\n");
				return ENXIO;
			}
#ifdef NETIF_DEBUG
			if (debug)
				printf("netif_open() succeeded\n");
#endif
		}
		if (rootip.s_addr == 0) {
			/* Get root IP address, and path, etc. */
			error = net_getparams(netdev_sock);
			if (error) {
				/* getparams makes its own noise */
				netif_close(netdev_sock);
				netdev_sock = -1;
				return error;
			}
		}
	}
	netdev_opens++;
	f->f_devdata = &netdev_sock;
	return error;
}

int
net_close(struct open_file *f)
{

#ifdef	NETIF_DEBUG
	if (debug)
		printf("net_close: opens=%d\n", netdev_opens);
#endif

	/* On last close, do netif close, etc. */
	f->f_devdata = NULL;
	/* Extra close call? */
	if (netdev_opens <= 0)
		return 0;
	netdev_opens--;
	/* Not last close? */
	if (netdev_opens > 0)
		return 0;
	rootip.s_addr = 0;
	if (netdev_sock >= 0) {
#ifdef NETIF_DEBUG
		if (debug)
			printf("%s: calling netif_close()\n", __func__);
#endif
		netif_close(netdev_sock);
		netdev_sock = -1;
	}
	return 0;
}

int
net_ioctl(struct open_file *f, u_long cmd, void *data)
{

	return EIO;
}

int
net_strategy(void *devdata, int rw, daddr_t blk, size_t size, void *buf,
	size_t *rsize)
{

	return EIO;
}


#ifdef	SUPPORT_BOOTP
int try_bootp;
#endif

static int
net_getparams(int sock)
{
	char buf[MAXHOSTNAMELEN];
	n_long smask;

#ifdef	SUPPORT_BOOTP
	/*
	 * Try to get boot info using BOOTP.  If we succeed, then
	 * the server IP address, gateway, and root path will all
	 * be initialized.  If any remain uninitialized, we will
	 * use RARP and RPC/bootparam (the Sun way) to get them.
	 */
	if (try_bootp)
		bootp(sock);
	if (myip.s_addr != 0)
		return 0;
#ifdef NETIF_DEBUG
	if (debug)
		printf("BOOTP failed, trying RARP/RPC...\n");
#endif
#endif

	/*
	 * Use RARP to get our IP address.  This also sets our
	 * netmask to the "natural" default for our address.
	 */
	if (rarp_getipaddress(sock)) {
		printf("RARP failed\n");
		return EIO;
	}
#ifdef NETIF_DEBUG
	if (debug)
		printf("client addr: %s\n", inet_ntoa(myip));
#endif

	/* Get our hostname, server IP address, gateway. */
	if (bp_whoami(sock)) {
		printf("bootparam/whoami RPC failed\n");
		return EIO;
	}
#ifdef NETIF_DEBUG
	if (debug)
		printf("client name: %s\n", hostname);
#endif

	/*
	 * Ignore the gateway from whoami (unreliable).
	 * Use the "gateway" parameter instead.
	 */
	smask = 0;
	gateip.s_addr = 0;
	if (bp_getfile(sock, "gateway", &gateip, buf)) {
		printf("%s: gateway bootparam missing\n", __func__);
	} else {
		/* Got it!  Parse the netmask. */
		smask = inet_addr(buf);
	}
	if (smask) {
		netmask = smask;
#ifdef NETIF_DEBUG
		if (debug)
			printf("subnet mask: %s\n", intoa(netmask));
#endif
	}
#ifdef NETIF_DEBUG
	if (debug)
		if (gateip.s_addr)
			printf("net gateway: %s\n", inet_ntoa(gateip));
#endif

	/* Get the root server and pathname. */
	if (bp_getfile(sock, "root", &rootip, rootpath)) {
		printf("bootparam/getfile RPC failed\n");
		return EIO;
	}

#ifdef NETIF_DEBUG
	if (debug) {
		printf("server addr: %s\n", inet_ntoa(rootip));
		printf("server path: %s\n", rootpath);
	}
#endif

	return 0;
}
