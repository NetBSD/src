/*	$NetBSD: fifo.h,v 1.16.24.1 2001/09/26 19:55:08 nathanw Exp $	*/

/*
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)fifo.h	8.6 (Berkeley) 5/21/95
 */

/*
 * Prototypes for fifo operations on vnodes.
 */
int	fifo_lookup	__P((void *));
#define fifo_create	genfs_badop
#define fifo_mknod	genfs_badop
int	fifo_open	__P((void *));
int	fifo_close	__P((void *));
#define fifo_access	genfs_ebadf
#define fifo_getattr	genfs_ebadf
#define fifo_setattr	genfs_ebadf
int	fifo_read	__P((void *));
int	fifo_write	__P((void *));
#define fifo_lease_check genfs_nullop
int	fifo_ioctl	__P((void *));
int	fifo_poll	__P((void *));
#define fifo_revoke	genfs_revoke
#define fifo_mmap	genfs_badop
#define fifo_fsync	genfs_nullop
#define fifo_seek	genfs_badop
#define fifo_remove	genfs_badop
#define fifo_link	genfs_badop
#define fifo_rename	genfs_badop
#define fifo_mkdir	genfs_badop
#define fifo_rmdir	genfs_badop
#define fifo_symlink	genfs_badop
#define fifo_readdir	genfs_badop
#define fifo_readlink	genfs_badop
#define fifo_abortop	genfs_badop
#define fifo_reclaim	genfs_nullop
#define	fifo_lock	genfs_nolock
#define	fifo_unlock	genfs_nounlock
int	fifo_inactive	__P((void *));
int	fifo_bmap	__P((void *));
#define fifo_strategy	genfs_badop
int	fifo_print	__P((void *));
#define fifo_islocked	genfs_noislocked
int	fifo_pathconf	__P((void *));
#define	fifo_advlock	genfs_einval
#define fifo_blkatoff	genfs_badop
#define fifo_valloc	genfs_badop
#define fifo_reallocblks genfs_badop
#define fifo_vfree	genfs_badop
#define fifo_truncate	genfs_nullop
#define fifo_update	genfs_nullop
#define fifo_bwrite	genfs_nullop
int	fifo_putpages	__P((void *));

void 	fifo_printinfo __P((struct vnode *));

extern int (**fifo_vnodeop_p) __P((void *));
