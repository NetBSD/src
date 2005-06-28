/*	$NetBSD: conf.c,v 1.7 2005/06/28 14:43:26 junyoung Exp $	*/

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
#include <lib/libsa/ufs.h>
#include <lib/libsa/lfs.h>
#include <lib/libsa/cd9660.h>
#include <lib/libsa/ustarfs.h>

#include "libx68k.h"

struct devsw devsw[] = {
	{ "sd",	sdstrategy, sdopen, sdclose, noioctl },
	{ "cd",	cdstrategy, cdopen, cdclose, noioctl },
	{ "fd",	fdstrategy, fdopen, fdclose, noioctl },
	{ 0, 0, 0, 0, 0 }
};

int ndevs = sizeof(devsw) / sizeof(devsw[0]);

const struct devspec devspec[] = {
	{ "sd", 0, 7 },
	{ "cd", 1, 7 },
	{ "fd", 2, 3 },
	{ 0, 0, 0 }
};

struct fs_ops file_system[] = {
	FS_OPS(ufs),
	FS_OPS(lfsv1),
	FS_OPS(lfsv2),
	FS_OPS(cd9660),
	FS_OPS(ustarfs),
};

int nfsys = sizeof(file_system) / sizeof(file_system[0]);

struct open_file files[SOPEN_MAX];
