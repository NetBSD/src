/*	$NetBSD: conf.c,v 1.4 2003/02/23 23:23:10 simonb Exp $	*/

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

int ndevs = (sizeof (devsw) / sizeof (devsw[0]));

const struct devspec devspec[] = {
	{ "sd", 0, 7 },
	{ "cd", 1, 7 },
	{ "fd", 2, 3 },
	{ 0, 0, 0 }
};

struct fs_ops file_system[] = {
    { ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek,
	ufs_stat },
    { lfsv1_open, lfsv1_close, lfsv1_read, lfsv1_write, lfsv1_seek,
	lfsv1_stat },
    { lfsv2_open, lfsv2_close, lfsv2_read, lfsv2_write, lfsv2_seek,
	lfsv2_stat },
    { cd9660_open, cd9660_close, cd9660_read, cd9660_write, cd9660_seek,
	cd9660_stat },
    { ustarfs_open, ustarfs_close, ustarfs_read, ustarfs_write, ustarfs_seek,
	ustarfs_stat },
};
 
int nfsys = sizeof(file_system)/sizeof(struct fs_ops);

struct open_file files[SOPEN_MAX];
