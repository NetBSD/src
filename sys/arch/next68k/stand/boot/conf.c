/*	$NetBSD: conf.c,v 1.4 2003/08/07 16:28:57 agc Exp $	*/

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
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <ufs.h>
#include <nfs.h>
#include <netif.h>
#include <dev_net.h>

/*
 * Device configuration
 */

extern int	sdstrategy __P((void *, int, daddr_t, size_t,
				void *, size_t *));
extern int	sdopen __P((struct open_file *, ...));
extern int	sdclose __P((struct open_file *));
#define	sdioctl	noioctl

/* ### now from libsa
extern int	enstrategy __P((void *, int, daddr_t, size_t,
				void *, size_t *));
extern int	enopen __P((struct open_file *, ...));
extern int	enclose __P((struct open_file *));
#define	enioctl	noioctl
*/

struct devsw devsw[] = {
	{ "sd",	sdstrategy,	sdopen,	sdclose,	sdioctl },
	{ "en",	net_strategy,	net_open, net_close,	net_ioctl },
	{ "tp",	net_strategy,	net_open, net_close,	net_ioctl },
	{ "xe",	net_strategy,	net_open, net_close,	net_ioctl },
#if 0
	{ "fd",	nullsys,	nodev,	nullsys,	noioctl },
	{ "od",	nullsys,	nodev,	nullsys,	noioctl },
	{ "tp",	nullsys,	nodev,	nullsys,	noioctl },
#endif
};
int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

/*
 * Filesystem configuration
 */
struct fs_ops file_system[] = {
	{ ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat },
	{ nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
};

int nfsys = sizeof(file_system) / sizeof(file_system[0]);

extern struct netif_driver en_driver;

struct netif_driver *netif_drivers[] = {
	&en_driver,
};
int n_netif_drivers = sizeof(netif_drivers) / sizeof(netif_drivers[0]);
