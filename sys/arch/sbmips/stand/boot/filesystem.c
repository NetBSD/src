/*	$NetBSD: filesystem.c,v 1.3 2005/06/23 19:14:24 junyoung Exp $	*/

/*
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	filesystem.c
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/ustarfs.h>
#include <lib/libsa/cd9660.h>
#include <lib/libsa/lfs.h>

struct fs_ops file_system[] = {
    { ustarfs_open, ustarfs_close, ustarfs_read, ustarfs_write, ustarfs_seek,
	ustarfs_stat },	/* this one can work from tape, so put it first */
    { ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek,
	ufs_stat },
    { cd9660_open, cd9660_close, cd9660_read, cd9660_write, cd9660_seek,
	cd9660_stat },
    { lfsv1_open, lfsv1_close, lfsv1_read, lfsv1_write, lfsv1_seek,
	lfsv1_stat },
    { lfsv2_open, lfsv2_close, lfsv2_read, lfsv2_write, lfsv2_seek,
	lfsv2_stat },
};

int nfsys = sizeof(file_system)/sizeof(struct fs_ops);

