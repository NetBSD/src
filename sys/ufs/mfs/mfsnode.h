/*	$NetBSD: mfsnode.h,v 1.10 2000/05/19 20:42:21 thorpej Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)mfsnode.h	8.3 (Berkeley) 5/19/95
 */

/*
 * This structure defines the control data for the memory based file system.
 */

struct mfsnode {
	struct	vnode *mfs_vnode;	/* vnode associated with this mfsnode */
	caddr_t	mfs_baseoff;		/* base of file system in memory */
	long	mfs_size;		/* size of memory file system */
	struct	proc *mfs_proc;		/* supporting process */
	struct	buf_queue mfs_buflist;	/* list of I/O requests */
};

/*
 * Convert between mfsnode pointers and vnode pointers
 */
#define VTOMFS(vp)	((struct mfsnode *)(vp)->v_data)
#define MFSTOV(mfsp)	((mfsp)->mfs_vnode)

/* Prototypes for MFS operations on vnodes. */
#define	mfs_lookup	genfs_badop
#define	mfs_create	genfs_badop
#define	mfs_mknod	genfs_badop
#define	mfs_access	genfs_badop
#define	mfs_getattr	genfs_badop
#define	mfs_setattr	genfs_badop
#define	mfs_read	genfs_badop
#define	mfs_write	genfs_badop
#define	mfs_poll	genfs_badop
#define	mfs_mmap	genfs_badop
#define	mfs_seek	genfs_badop
#define	mfs_remove	genfs_badop
#define	mfs_link	genfs_badop
#define	mfs_rename	genfs_badop
#define	mfs_mkdir	genfs_badop
#define	mfs_rmdir	genfs_badop
#define	mfs_symlink	genfs_badop
#define	mfs_readdir	genfs_badop
#define	mfs_readlink	genfs_badop
#define	mfs_abortop	genfs_badop
#define	mfs_lock	genfs_nolock
#define	mfs_unlock	genfs_nounlock
#define	mfs_islocked	genfs_noislocked
#define	mfs_pathconf	genfs_badop
#define	mfs_advlock	genfs_badop
#define	mfs_blkatoff	genfs_badop
#define	mfs_valloc	genfs_badop
#define	mfs_vfree	genfs_badop
#define	mfs_truncate	genfs_badop
#define	mfs_update	genfs_badop
#define	mfs_bwrite	vn_bwrite
#define	mfs_revoke	genfs_revoke
