/* 	$NetBSD: devfs_fifoops.h,v 1.1.2.1 2008/02/18 22:07:02 mjf Exp $ */

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
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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

#ifndef _FS_DEVFS_DEVFS_FIFOOPS_H_
#define _FS_DEVFS_DEVFS_FIFOOPS_H_

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

#include <miscfs/fifofs/fifo.h>
#include <fs/devfs/devfs_vnops.h>

/* --------------------------------------------------------------------- */

/*
 * Declarations for devfs_fifoops.c.
 */

extern int (**devfs_fifoop_p)(void *);

#define	devfs_fifo_lookup	fifo_lookup
#define	devfs_fifo_create	fifo_create
#define	devfs_fifo_mknod	fifo_mknod
#define	devfs_fifo_open		fifo_open
int	devfs_fifo_close	(void *);
#define	devfs_fifo_access	devfs_access
#define	devfs_fifo_getattr	devfs_getattr
#define	devfs_fifo_setattr	devfs_setattr
int	devfs_fifo_read		(void *);
int	devfs_fifo_write	(void *);
#define	devfs_fifo_fcntl	devfs_fcntl
#define	devfs_fifo_ioctl	fifo_ioctl
#define	devfs_fifo_poll		fifo_poll
#define	devfs_fifo_kqfilter	fifo_kqfilter
#define	devfs_fifo_revoke	fifo_revoke
#define	devfs_fifo_mmap		fifo_mmap
#define	devfs_fifo_fsync	fifo_fsync
#define	devfs_fifo_seek		fifo_seek
#define	devfs_fifo_remove	fifo_remove
#define	devfs_fifo_link		fifo_link
#define	devfs_fifo_rename	fifo_rename
#define	devfs_fifo_mkdir	fifo_mkdir
#define	devfs_fifo_rmdir	fifo_rmdir
#define	devfs_fifo_symlink	fifo_symlink
#define	devfs_fifo_readdir	fifo_readdir
#define	devfs_fifo_readlink	fifo_readlink
#define	devfs_fifo_abortop	fifo_abortop
#define	devfs_fifo_inactive	devfs_inactive
#define	devfs_fifo_reclaim	devfs_reclaim
#define	devfs_fifo_lock		devfs_lock
#define	devfs_fifo_unlock	devfs_unlock
#define	devfs_fifo_bmap		fifo_bmap
#define	devfs_fifo_strategy	fifo_strategy
#define	devfs_fifo_print	devfs_print
#define	devfs_fifo_pathconf	fifo_pathconf
#define	devfs_fifo_islocked	devfs_islocked
#define	devfs_fifo_advlock	fifo_advlock
#define	devfs_fifo_lease	devfs_lease
#define	devfs_fifo_bwrite	devfs_bwrite
#define	devfs_fifo_getpages	genfs_badop
#define	devfs_fifo_putpages	fifo_putpages

/* --------------------------------------------------------------------- */
#endif /* _FS_DEVFS_DEVFS_FIFOOPS_H_ */
