/*	$NetBSD: net.c,v 1.7.10.1 2012/07/21 00:04:56 riz Exp $	*/

/*
 * Copyright (C) 1995 Wolfgang Solfrank.
 * Copyright (C) 1995 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 * find interface	- netif_open()
 * BOOTP		- bootp()
 * RPC/mountd		- nfs_mount()
 *
 * The root file handle from mountd is saved in a global
 * for use by the NFS open code (NFS/lookup).
 *
 * Note: this is based in part on sys/arch/sparc/stand/net.c
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libsa/bootp.h>
#include <lib/libsa/bootparam.h>
#include <lib/libsa/nfs.h>

#include <lib/libkern/libkern.h>

#include "ofdev.h"
#include "net.h"


static int net_mountroot_bootparams(void);
static int net_mountroot_bootp(void);

char	rootpath[FNAME_SIZE];

static	int netdev_sock = -1;
static	int open_count;

/*
 * Called by devopen after it sets f->f_dev to our devsw entry.
 * This opens the low-level device and sets f->f_devdata.
 */
int
net_open(struct of_dev *op)
{
	int error = 0;
	
	/*
	 * On first open, do netif open, mount, etc.
	 */
	if (open_count == 0) {
		/* Find network interface. */
		if ((netdev_sock = netif_open(op)) < 0) {
			error = errno;
			goto bad;
		}
	}
	open_count++;
bad:
	if (netdev_sock >= 0 && open_count == 0) {
		netif_close(netdev_sock);
		netdev_sock = -1;
	}
	return error;
}

int
net_close(struct of_dev *op)
{

	/*
	 * On last close, do netif close, etc.
	 */
	if (open_count > 0)
		if (--open_count == 0) {
			netif_close(netdev_sock);
			netdev_sock = -1;
		}
	return 0;
}

static void
net_clear_params(void)
{
	
	myip.s_addr = 0;
	netmask = 0;
	gateip.s_addr = 0;
	*hostname = '\0';
	rootip.s_addr = 0;
	*rootpath = '\0';
}

int
net_mountroot_bootparams(void)
{

	net_clear_params();

	/* Get our IP address.  (rarp.c) */
	if (rarp_getipaddress(netdev_sock) == -1)
		return (errno);
	printf("Using BOOTPARAMS protocol:\n  ip addr=%s\n", inet_ntoa(myip));
	if (bp_whoami(netdev_sock))
		return (errno);
	printf("  hostname=%s\n", hostname);
	if (bp_getfile(netdev_sock, "root", &rootip, rootpath))
		return (errno);

	return (0);
}

int
net_mountroot_bootp(void)
{
	int attempts;

	/* We need a few attempts here as some DHCP servers
	 * require >1 packet and my wireless bridge is always
	 * in learning mode until the 2nd attempt ... */
	for (attempts = 0; attempts < 3; attempts++) {
		net_clear_params();
		bootp(netdev_sock);
		if (myip.s_addr != 0)
			break;
	}
	if (myip.s_addr == 0)
		return(ENOENT);

	printf("Using BOOTP protocol:\n ip addr=%s\n", inet_ntoa(myip));
	if (hostname[0])
		printf("  hostname=%s\n", hostname);
	if (netmask)
		printf("  netmask=%s\n", intoa(netmask));
	if (gateip.s_addr)
		printf("  gateway=%s\n", inet_ntoa(gateip));

	return (0);
}

/*
 * libsa's tftp_open expects a pointer to netdev_sock, i.e. an (int *),
 * in f_devdata, a pointer to which gets handed down from devopen().
 *
 * Do not expect booting via different methods to have the same
 * requirements or semantics.
 *
 * net_tftp_bootp uses net_mountroot_bootp because that incidentially does
 * most of what it needs to do. It of course in no manner actually mounts
 * anything, all that routine actually does is prepare the socket for the
 * necessary net access, and print info for the user.
 */

int
net_tftp_bootp(int **sock)
{

	net_mountroot_bootp();
	if (myip.s_addr == 0)
		return(ENOENT);

	*sock = &netdev_sock;
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

	printf("  root addr=%s\n  path=%s\n", inet_ntoa(rootip), rootpath);

	/* Get the NFS file handle (mount). */
	if (nfs_mount(netdev_sock, rootip, rootpath) != 0)
		return (errno);

	return (0);
}
