/* $NetBSD: conf.c,v 1.4.4.2 2019/06/10 22:09:56 christos Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 */

#include "efiboot.h"
#include "efifile.h"
#include "efiblock.h"

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/dosfs.h>
#include <lib/libsa/tftp.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/net.h>
#include <lib/libsa/dev_net.h>

struct devsw devsw[] = {
	{ "efifile", efi_file_strategy, efi_file_open, efi_file_close, noioctl },
	{ "efiblock", efi_block_strategy, efi_block_open, efi_block_close, noioctl },
	{ "net", net_strategy, net_open, net_close, noioctl },
};
int ndevs = __arraycount(devsw);

struct netif_driver *netif_drivers[] = {
	&efinetif,
};
int n_netif_drivers = __arraycount(netif_drivers);

struct fs_ops file_system[] = {
	FS_OPS(null),
	FS_OPS(ffsv1),
	FS_OPS(ffsv2),
	FS_OPS(dosfs),
};
int nfsys = __arraycount(file_system);

struct fs_ops null_fs_ops = FS_OPS(null);
struct fs_ops tftp_fs_ops = FS_OPS(tftp);
struct fs_ops nfs_fs_ops = FS_OPS(nfs);
