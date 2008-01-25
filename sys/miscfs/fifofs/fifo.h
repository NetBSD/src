/*	$NetBSD: fifo.h,v 1.25 2008/01/25 14:32:15 ad Exp $	*/

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
 *	@(#)fifo.h	8.6 (Berkeley) 5/21/95
 */

/*
 * Prototypes for fifo operations on vnodes.
 */
int	fifo_lookup(void *);
#define fifo_create	genfs_badop
#define fifo_mknod	genfs_badop
int	fifo_open(void *);
int	fifo_close(void *);
#define fifo_access	genfs_ebadf
#define fifo_getattr	genfs_ebadf
#define fifo_setattr	genfs_ebadf
int	fifo_read(void *);
int	fifo_write(void *);
int	fifo_ioctl(void *);
int	fifo_poll(void *);
int	fifo_kqfilter(void *);
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
#define	fifo_lock	genfs_lock
#define	fifo_unlock	genfs_unlock
int	fifo_inactive(void *);
int	fifo_bmap(void *);
#define fifo_strategy	genfs_badop
int	fifo_print(void *);
#define fifo_islocked	genfs_islocked
int	fifo_pathconf(void *);
#define	fifo_advlock	genfs_einval
#define fifo_bwrite	genfs_nullop
#define	fifo_putpages	genfs_null_putpages

void 	fifo_printinfo(struct vnode *);

extern int (**fifo_vnodeop_p)(void *);
