/*	$NetBSD: lfs_extern.h,v 1.14.8.1 1999/12/27 18:36:39 wrstuden Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
/*-
 * Copyright (c) 1991, 1993, 1994
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
 *	@(#)lfs_extern.h	8.6 (Berkeley) 5/8/95
 */

/*
 * Sysctl values for LFS.
 */
#define LFS_WRITEINDIR	 1 /* flush indirect blocks on non-checkpoint writes */
#define LFS_CLEAN_VNHEAD 2 /* put prev unrefed cleaned vnodes on head of free list */
#define LFS_DOSTATS      3
#define LFS_STATS        4
#define LFS_MAXID	 5

#define LFS_NAMES { \
	{ 0, 0 }, \
	{ "flushindir", CTLTYPE_INT }, \
	{ "clean_vnhead", CTLTYPE_INT }, \
	{ "dostats", CTLTYPE_INT }, \
	{ "stats", CTLTYPE_STRUCT }, \
}

struct fid;
struct mount;
struct nameidata;
struct proc;
struct statfs;
struct timeval;
struct inode;
struct uio;
struct mbuf;
struct dinode;
struct buf;
struct vnode;
struct lfs;
struct segment;
struct ucred;

extern struct pool lfs_inode_pool;		/* memory pool for inodes */

__BEGIN_DECLS
/* lfs_alloc.c */
int lfs_vcreate __P((struct mount *, ino_t, struct vnode **));
/* lfs_bio.c */
int lfs_bwrite_ext __P((struct buf *, int));
void lfs_flush_fs __P((struct mount *, int));
void lfs_flush __P((struct lfs *, int));
int lfs_check __P((struct vnode *, ufs_daddr_t, int));
void lfs_freebuf __P((struct buf *));
void lfs_countlocked __P((int *, long *));

/* lfs_cksum.c */
u_long cksum __P((void *, size_t));
u_long lfs_sb_cksum __P((struct dlfs *));

/* lfs_debug.c */
#ifdef DEBUG
void lfs_dump_super __P((struct lfs *));
void lfs_dump_dinode __P((struct dinode *));
void lfs_check_bpp __P((struct lfs *, struct segment *, char *, int));
void lfs_check_segsum __P((struct lfs *, struct segment *, char *, int));
#endif /* DEBUG */

/* lfs_inode.c */
void lfs_init __P((void));
struct dinode *lfs_ifind __P((struct lfs *, ino_t, struct dinode *));

/* lfs_segment.c */
void lfs_imtime __P((struct lfs *));
int lfs_vflush __P((struct vnode *));
int lfs_writevnodes __P((struct lfs *, struct mount *, struct segment *, int));
int lfs_segwrite __P((struct mount *, int));
void lfs_writefile __P((struct lfs *, struct segment *, struct vnode *));
int lfs_writeinode __P((struct lfs *, struct segment *, struct inode *));
int lfs_gatherblock __P((struct segment *, struct buf *, int *));
int lfs_gather __P((struct lfs *, struct segment *, struct vnode *, int (*match )__P ((struct lfs *, struct buf *))));
void lfs_updatemeta __P((struct segment *));
int lfs_initseg __P((struct lfs *));
void lfs_newseg __P((struct lfs *));
int lfs_writeseg __P((struct lfs *, struct segment *));
void lfs_writesuper __P((struct lfs *, daddr_t));
int lfs_match_data __P((struct lfs *, struct buf *));
int lfs_match_indir __P((struct lfs *, struct buf *));
int lfs_match_dindir __P((struct lfs *, struct buf *));
int lfs_match_tindir __P((struct lfs *, struct buf *));
struct buf *lfs_newbuf __P((struct vnode *, ufs_daddr_t, size_t));
void lfs_callback __P((struct buf *));
void lfs_supercallback __P((struct buf *));
void lfs_shellsort __P((struct buf **, ufs_daddr_t *, int));
int lfs_vref __P((struct vnode *));
void lfs_vunref __P((struct vnode *));
void lfs_vunref_head __P((struct vnode *));

/* lfs_subr.c */
void lfs_seglock __P((struct lfs *, unsigned long));
void lfs_segunlock __P((struct lfs *));

/* lfs_syscalls.c */
int lfs_fastvget __P((struct mount *, ino_t, ufs_daddr_t, struct vnode **, struct dinode *, int *));
struct buf *lfs_fakebuf __P((struct vnode *, int, size_t, caddr_t));

/* lfs_vfsops.c */
int lfs_mountroot __P((void));
int lfs_mount __P((struct mount *, const char *, void *, struct nameidata *, struct proc *));
int lfs_mountfs __P((struct vnode *, struct mount *, struct proc *));
int lfs_unmount __P((struct mount *, int, struct proc *));
int lfs_statfs __P((struct mount *, struct statfs *, struct proc *));
int lfs_sync __P((struct mount *, int, struct ucred *, struct proc *));
int lfs_vget __P((struct mount *, ino_t, struct vnode **));
int lfs_fhtovp __P((struct mount *, struct fid *, struct vnode **));
int lfs_vptofh __P((struct vnode *, struct fid *));
int lfs_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		    struct proc *));


int lfs_balloc		__P((void *));
int lfs_valloc		__P((void *));
int lfs_vfree		__P((void *));
int lfs_bwrite		__P((void *));
int lfs_update		__P((void *));
int lfs_truncate	__P((void *));
int lfs_blkatoff	__P((void *));
int lfs_fsync		__P((void *));
int lfs_symlink		__P((void *));
int lfs_mknod		__P((void *));
int lfs_create		__P((void *));
int lfs_mkdir		__P((void *));
int lfs_read		__P((void *));
int lfs_remove		__P((void *));
int lfs_rmdir		__P((void *));
int lfs_link		__P((void *));
int lfs_rename		__P((void *));
int lfs_getattr		__P((void *));
int lfs_close		__P((void *));
int lfs_inactive	__P((void *));
int lfs_reclaim		__P((void *));
int lfs_write		__P((void *));
int lfs_whiteout	__P((void *));

__END_DECLS
extern int lfs_mount_type;
extern int (**lfs_vnodeop_p) __P((void *));
extern int (**lfs_specop_p) __P((void *));
extern int (**lfs_fifoop_p) __P((void *));
