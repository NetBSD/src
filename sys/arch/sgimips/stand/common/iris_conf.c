/*	$NetBSD: iris_conf.c,v 1.1.6.2 2019/06/10 22:06:44 christos Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/lfs.h>

#include "../common/disk.h"

#define diskioctl /*(()(struct open_file*, u_long, void*))*/0

struct devsw devsw[] = {
	{ "dksc", diskstrategy, diskopen, diskclose, diskioctl },
};

int	ndevs = __arraycount(devsw);

struct fs_ops file_system[] = {
	FS_OPS(ffsv1),
	FS_OPS(ffsv2),
	FS_OPS(lfsv1),
	FS_OPS(lfsv2),
};

int nfsys = __arraycount(file_system);
