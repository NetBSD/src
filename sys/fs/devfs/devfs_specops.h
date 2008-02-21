/* 	$NetBSD: devfs_specops.h,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $ */

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

#ifndef _FS_DEVFS_DEVFS_SPECOPS_H_
#define _FS_DEVFS_DEVFS_SPECOPS_H_

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif

#include <miscfs/specfs/specdev.h>
#include <fs/devfs/devfs_vnops.h>

/* --------------------------------------------------------------------- */

/*
 * Declarations for devfs_specops.c.
 */

extern int (**devfs_specop_p)(void *);

#define	devfs_spec_lookup	spec_lookup
#define	devfs_spec_create	spec_create
#define	devfs_spec_mknod	spec_mknod
#define	devfs_spec_open		spec_open
int	devfs_spec_close	(void *);
#define	devfs_spec_access	devfs_access
#define	devfs_spec_getattr	devfs_getattr
#define	devfs_spec_setattr	devfs_setattr
int	devfs_spec_read		(void *);
int	devfs_spec_write	(void *);
#define	devfs_spec_fcntl	devfs_fcntl
#define	devfs_spec_ioctl	spec_ioctl
#define	devfs_spec_poll		spec_poll
#define	devfs_spec_kqfilter	spec_kqfilter
#define	devfs_spec_revoke	spec_revoke
#define	devfs_spec_mmap		spec_mmap
#define	devfs_spec_fsync	spec_fsync
#define	devfs_spec_seek		spec_seek
#define	devfs_spec_remove	spec_remove
#define	devfs_spec_link		spec_link
#define	devfs_spec_rename	spec_rename
#define	devfs_spec_mkdir	spec_mkdir
#define	devfs_spec_rmdir	spec_rmdir
#define	devfs_spec_symlink	spec_symlink
#define	devfs_spec_readdir	spec_readdir
#define	devfs_spec_readlink	spec_readlink
#define	devfs_spec_abortop	spec_abortop
#define	devfs_spec_inactive	devfs_inactive
#define	devfs_spec_reclaim	devfs_reclaim
#define	devfs_spec_lock		devfs_lock
#define	devfs_spec_unlock	devfs_unlock
#define	devfs_spec_bmap		spec_bmap
#define	devfs_spec_strategy	spec_strategy
#define	devfs_spec_print	devfs_print
#define	devfs_spec_pathconf	spec_pathconf
#define	devfs_spec_islocked	devfs_islocked
#define	devfs_spec_advlock	spec_advlock
#define	devfs_spec_lease	devfs_lease
#define	devfs_spec_bwrite	vn_bwrite
#define	devfs_spec_getpages	spec_getpages
#define	devfs_spec_putpages	spec_putpages

/* --------------------------------------------------------------------- */

#endif /* _FS_DEVFS_DEVFS_SPECOPS_H_ */
