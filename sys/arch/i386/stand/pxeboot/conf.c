/*	$NetBSD: conf.c,v 1.1.4.2 2002/02/28 04:10:29 nathanw Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <stand.h>
#ifdef SUPPORT_NFS
#include <nfs.h>
#endif
#ifdef SUPPORT_TFTP
#include <tftp.h>
#endif
#include <dev_net.h>

#include "pxeboot.h"

#ifdef SUPPORT_NFS
struct fs_ops file_system_nfs = {
	nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat
};
#endif

#ifdef SUPPORT_TFTP
struct fs_ops file_system_tftp = {
	tftp_open, tftp_close, tftp_read, tftp_write, tftp_seek, tftp_stat
};
#endif

struct pxeboot_fstab pxeboot_fstab[] = {
#ifdef SUPPORT_NFS
	{ "nfs", &file_system_nfs },
#endif
#ifdef SUPPORT_TFTP
	{ "tftp", &file_system_tftp },
#endif
};
int npxeboot_fstab = sizeof(pxeboot_fstab) / sizeof(pxeboot_fstab[0]);

/* Filled in in devopen(). */
struct fs_ops file_system[1];
int nfsys = 1;

struct devsw devsw[] = {
	{ "net", net_strategy, net_open, net_close, net_ioctl },
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

extern struct netif_driver pxe_netif_driver;

struct netif_driver *netif_drivers[] = {
	&pxe_netif_driver,
};
int n_netif_drivers = sizeof(netif_drivers) / sizeof(netif_drivers[0]);
