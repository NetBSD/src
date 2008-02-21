/* 	$NetBSD: devfs_vnops.h,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _FS_DEVFS_DEVFS_VNOPS_H_
#define _FS_DEVFS_DEVFS_VNOPS_H_

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

#include <miscfs/genfs/genfs.h>

/* --------------------------------------------------------------------- */

/*
 * Declarations for devfs_vnops.c.
 */

extern int (**devfs_vnodeop_p)(void *);

int	devfs_lookup		(void *);
int	devfs_create		(void *);
int	devfs_mknod		(void *);
int	devfs_open		(void *);
int	devfs_close		(void *);
int	devfs_access		(void *);
int	devfs_getattr		(void *);
int	devfs_setattr		(void *);
int	devfs_read		(void *);
int	devfs_write		(void *);
int	devfs_fcntl		(void *);
#define	devfs_ioctl		genfs_enoioctl
#define	devfs_poll		genfs_poll
#define	devfs_kqfilter		genfs_kqfilter
#define	devfs_revoke		genfs_revoke
#define	devfs_mmap		genfs_mmap
int	devfs_fsync		(void *);
#define	devfs_seek		genfs_seek
int	devfs_remove		(void *);
int	devfs_link		(void *);
int	devfs_rename		(void *);
int	devfs_mkdir		(void *);
int	devfs_rmdir		(void *);
int	devfs_symlink		(void *);
int	devfs_readdir		(void *);
int	devfs_readlink		(void *);
#define	devfs_abortop		genfs_abortop
int	devfs_inactive		(void *);
int	devfs_reclaim		(void *);
#define	devfs_lock		genfs_lock
#define	devfs_unlock		genfs_unlock
#define	devfs_bmap		genfs_eopnotsupp
#define	devfs_strategy		genfs_eopnotsupp
int	devfs_print		(void *);
int	devfs_pathconf		(void *);
#define	devfs_islocked		genfs_islocked
int	devfs_advlock		(void *);
#define	devfs_lease		genfs_lease_check
#define	devfs_bwrite		genfs_nullop
int	devfs_getpages		(void *);
int	devfs_putpages		(void *);

/* --------------------------------------------------------------------- */

#endif /* _FS_DEVFS_DEVFS_VNOPS_H_ */
