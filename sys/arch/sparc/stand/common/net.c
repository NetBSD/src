/*	$NetBSD: net.c,v 1.9.74.1 2020/04/25 11:23:57 bouyer Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
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
 *
 * the root file handle from mountd is saved in a global
 * for use by the NFS open code (NFS/lookup).
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libsa/bootparam.h>
#include <lib/libsa/bootp.h>
#include <lib/libsa/nfs.h>

#include <lib/libkern/libkern.h>

#include <sparc/stand/common/promdev.h>

extern char	rootpath[FNAME_SIZE];

int	netdev_sock = -1;
static	int open_count;

static int net_mountroot_bootparams(void);
static int net_mountroot_bootp(void);

/*
 * Called by devopen after it sets f->f_dev to our devsw entry.
 * This opens the low-level device and sets f->f_devdata.
 */
int
net_open(struct promdata *pd)
{
	int error = 0;

	/* On first open, do netif open, mount, etc. */
	if (open_count == 0) {
		/* Find network interface. */
		if ((netdev_sock = netif_open(pd)) < 0) {
			error = errno;
			goto bad;
		}
		if ((error = net_mountroot()) != 0)
			goto bad;
	}
	open_count++;
bad:
	return (error);
}

int
net_close(struct promdata *pd)
{
	/* On last close, do netif close, etc. */
	if (open_count <= 0)
		return (0);

	if (--open_count == 0)
		return (netif_close(netdev_sock));

	return (0);
}

int
net_mountroot_bootparams(void)
{
	printf("Trying BOOTPARAMS protocol... ");

	/* Get our IP address.  (rarp.c) */
	if (rarp_getipaddress(netdev_sock) == -1)
		return (errno);

	printf("ip address: %s", inet_ntoa(myip));

	/* Get our hostname, server IP address. */
	if (bp_whoami(netdev_sock))
		return (errno);

	printf(", hostname: %s\n", hostname);

	/* Get the root pathname. */
	if (bp_getfile(netdev_sock, "root", &rootip, rootpath))
		return (errno);

	return (0);
}

int
net_mountroot_bootp(void)
{
	printf("Trying BOOTP protocol... ");

	bootp(netdev_sock);

	if (myip.s_addr == 0)
		return(ENOENT);

	printf("ip address: %s", inet_ntoa(myip));

	if (hostname[0])
		printf(", hostname: %s", hostname);
	if (netmask)
		printf(", netmask: %s", intoa(netmask));
	if (gateip.s_addr)
		printf(", gateway: %s", inet_ntoa(gateip));
	printf("\n");

	return (0);
}

int
net_mountroot(void)
{
	int error;

#ifdef DEBUG
	printf("net_mountroot\n");
#endif

	/*
	 * Get info for NFS boot: our IP address, our hostname,
	 * server IP address, and our root path on the server.
	 * There are two ways to do this:  The old, Sun way,
	 * and the more modern, BOOTP way. (RFC951, RFC1048)
	 */

	/* Try BOOTP first */
	error = net_mountroot_bootp();
	/* Historically, we've used BOOTPARAMS, so try that next */
	if (error != 0)
		error = net_mountroot_bootparams();
	if (error != 0)
		return (error);

	printf("root addr=%s path=%s\n", inet_ntoa(rootip), rootpath);

	/* Get the NFS file handle (mount). */
	if (nfs_mount(netdev_sock, rootip, rootpath) != 0)
		return (errno);

	return (0);
}
