/*	$NetBSD: conf.c,v 1.4 2014/01/11 08:08:23 tsutsui Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)conf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/dev_net.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/ufs.h>

#include <machine/bootinfo.h>
#include <luna68k/stand/boot/samachdep.h>

#define xxstrategy	\
	(int (*)(void *, int, daddr_t, size_t, void *, size_t *))nullsys
#define xxopen		(int (*)(struct open_file *, ...))nodev
#define xxclose		(int (*)(struct open_file *))nullsys

/*
 * Device configuration
 */
#ifndef SUPPORT_ETHERNET
#define	netstrategy	xxstrategy
#define	netopen		xxopen
#define	netclose	xxclose
#else
#define	netstrategy	net_strategy
#define	netopen		net_open
#define	netclose	net_close
#endif
#define	netioctl	noioctl

#ifndef SUPPORT_DISK
#define	sdstrategy	xxstrategy
#define	sdopen		xxopen
#define	sdclose		xxclose
#endif
#define	sdioctl		noioctl

/*
 * Note: "le" isn't a major offset.
 */
struct devsw devsw[] = {
	{ "sd",	sdstrategy,	sdopen,	sdclose,	sdioctl },
	{ "le",	netstrategy,	netopen, netclose,	netioctl },
};
int	ndevs = __arraycount(devsw);

/* XXX: These indices must sync with the above devsw */
const int dev2adpt[] = {
	LUNA68K_BOOTADPT_SPC,
	LUNA68K_BOOTADPT_LANCE,
};

#ifdef SUPPORT_ETHERNET
extern struct netif_driver le_netif_driver;
struct netif_driver *netif_drivers[] = {
	&le_netif_driver,
};
int	n_netif_drivers = __arraycount(netif_drivers);
#endif

/*
 * Filesystem configuration
 */
#ifdef SUPPORT_DISK
struct fs_ops file_system_disk[] = {
	FS_OPS(ffsv1),
	FS_OPS(ffsv2)
};
int	nfsys_disk = __arraycount(file_system_disk);
#endif
#ifdef SUPPORT_ETHERNET
struct fs_ops file_system_nfs[] = { FS_OPS(nfs) };
#endif

#define MAX_NFSYS	5
struct fs_ops file_system[MAX_NFSYS];
int	nfsys = 1;		/* we always know which one we want */
