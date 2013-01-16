/*	$NetBSD: conf.c,v 1.1.2.3 2013/01/16 05:33:08 yamt Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/dev_net.h>
#include <netinet/in.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/lfs.h>
#include <lib/libsa/cd9660.h>
#include <lib/libsa/ustarfs.h>

#include "libx68k.h"

struct devsw devsw[] = {
	{ "nfs", net_strategy, net_open, net_close, net_ioctl },
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

const struct devspec devspec[] = {
	{ "nfs", 0, 1, 1 },
	{ 0,    0, 0, 0 },
};

struct fs_ops file_system[] = {
	FS_OPS(nfs),
};
int nfsys = sizeof(file_system) / sizeof(file_system[0]);

struct open_file files[SOPEN_MAX];

extern struct netif_driver ne_netif_driver;

struct netif_driver *netif_drivers[] = {
	&ne_netif_driver,
};
int n_netif_drivers = sizeof(netif_drivers) / sizeof(netif_drivers[0]);
