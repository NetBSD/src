/*	$NetBSD: mfs_extern.h,v 1.19 2004/04/21 01:05:46 christos Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)mfs_extern.h	8.4 (Berkeley) 3/30/95
 */

#ifndef _UFS_MFS_MFS_EXTERN_H_
#define _UFS_MFS_MFS_EXTERN_H_

#include <sys/mallocvar.h>
MALLOC_DECLARE(M_MFSNODE);

struct buf;
struct mount;
struct nameidata;
struct proc;
struct statvfs;
struct ucred;
struct vnode;

__BEGIN_DECLS
#define	mfs_ioctl	genfs_enoioctl

/* mfs_vfsops.c */
int	mfs_mountroot	__P((void));
int	mfs_initminiroot	__P((caddr_t));
int	mfs_mount	__P((struct mount *, const char *, void *,
			     struct nameidata *, struct proc *));
int	mfs_start	__P((struct mount *, int, struct proc *));
int	mfs_statvfs	__P((struct mount *, struct statvfs *, struct proc *));

void	mfs_init	__P((void));
void	mfs_reinit	__P((void));
void	mfs_done	__P((void));

/* mfs_vnops.c */
int	mfs_open	__P((void *));
int	mfs_strategy	__P((void *));
void	mfs_doio	__P((struct buf *, caddr_t));
int	mfs_bmap	__P((void *));
int	mfs_close	__P((void *));
int	mfs_inactive	__P((void *));
int	mfs_reclaim	__P((void *));
int	mfs_print	__P((void *));

__END_DECLS

#endif /* !_UFS_MFS_MFS_EXTERN_H_ */
