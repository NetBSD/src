/* $NetBSD: conf.c,v 1.2 2003/08/09 08:01:43 igy Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.2 2003/08/09 08:01:43 igy Exp $");

#include <netinet/in.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/netif.h>
#include <lib/libsa/tftp.h>

#include "extern.h"

struct devsw	devsw[] = {
	{
		.dv_name	= "flash",
		.dv_strategy	= flash_strategy,
		.dv_open	= flash_open,
		.dv_close	= flash_close,
		.dv_ioctl	= flash_ioctl,
	},
	{
		.dv_name	= "net",
		.dv_strategy	= net_strategy,
		.dv_open	= net_open,
		.dv_close	= net_close,
		.dv_ioctl	= net_ioctl,
	},
};
int	ndevs = sizeof devsw / sizeof devsw[0];

struct fs_ops	file_system[] = {
	{
		.open		= tftp_open,
		.close		= tftp_close,
		.read		= tftp_read,
		.write		= tftp_write,
		.seek		= tftp_seek,
		.stat		= tftp_stat,
	}
};
int	nfsys = sizeof file_system / sizeof file_system[0];

struct netif_driver	*netif_drivers[] = {
	&cs_driver,
};
int n_netif_drivers = sizeof netif_drivers / sizeof netif_drivers[0];
