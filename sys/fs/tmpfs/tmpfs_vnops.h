/*	$NetBSD: tmpfs_vnops.h,v 1.13.72.1 2021/08/01 22:42:37 thorpej Exp $	*/

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FS_TMPFS_TMPFS_VNOPS_H_
#define _FS_TMPFS_TMPFS_VNOPS_H_

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

#include <miscfs/genfs/genfs.h>

/*
 * Declarations for tmpfs_vnops.c.
 */

extern int (**tmpfs_vnodeop_p)(void *);

int	tmpfs_lookup		(void *);
int	tmpfs_create		(void *);
int	tmpfs_mknod		(void *);
int	tmpfs_open		(void *);
int	tmpfs_close		(void *);
int	tmpfs_access		(void *);
int	tmpfs_getattr		(void *);
int	tmpfs_setattr		(void *);
int	tmpfs_read		(void *);
int	tmpfs_write		(void *);
int	tmpfs_fsync		(void *);
int	tmpfs_remove		(void *);
int	tmpfs_link		(void *);
int	tmpfs_rename		(void *);
int	tmpfs_mkdir		(void *);
int	tmpfs_rmdir		(void *);
int	tmpfs_symlink		(void *);
int	tmpfs_readdir		(void *);
int	tmpfs_readlink		(void *);
int	tmpfs_inactive		(void *);
int	tmpfs_reclaim		(void *);
int	tmpfs_print		(void *);
int	tmpfs_pathconf		(void *);
int	tmpfs_advlock		(void *);
int	tmpfs_getpages		(void *);
int	tmpfs_putpages		(void *);
int	tmpfs_whiteout		(void *);

#endif /* _FS_TMPFS_TMPFS_VNOPS_H_ */
