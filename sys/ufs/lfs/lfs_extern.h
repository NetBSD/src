/*	$NetBSD: lfs_extern.h,v 1.24.6.3 2002/03/16 16:02:25 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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

/* Copied from ext2fs for ITIMES.  XXX This is a bogus use of v_tag. */
#define IS_LFS_VNODE(vp)   (vp->v_tag == VT_LFS)

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
struct dlfs;
struct lfs;
struct segment;
struct ucred;

extern struct pool lfs_inode_pool;		/* memory pool for inodes */

__BEGIN_DECLS
/* lfs_alloc.c */
int lfs_rf_valloc(struct lfs *, ino_t, int, struct proc *, struct vnode **);
void lfs_vcreate(struct mount *, ino_t, struct vnode *);
/* lfs_bio.c */
int lfs_availwait(struct lfs *, int);
int lfs_bwrite_ext(struct buf *, int);
int lfs_fits(struct lfs *, int);
void lfs_flush_fs(struct lfs *, int);
void lfs_flush(struct lfs *, int);
int lfs_check(struct vnode *, ufs_daddr_t, int);
#ifdef MALLOCLOG
void lfs_freebuf_malloclog(struct buf *, char *, int);
struct buf *lfs_newbuf_malloclog(struct lfs *, struct vnode *,
				 ufs_daddr_t, size_t, char *, int);
#define lfs_freebuf(BP) lfs_freebuf_malloclog((BP), __FILE__, __LINE__)
#define lfs_newbuf(F, V, A, S) lfs_newbuf_malloclog((F),(V),(A),(S),__FILE__,__LINE__)
#else
void lfs_freebuf(struct buf *);
struct buf *lfs_newbuf(struct lfs *, struct vnode *, ufs_daddr_t, size_t);
#endif
void lfs_countlocked(int *, long *);
int lfs_reserve(struct lfs *, struct vnode *, int);

/* lfs_cksum.c */
u_int32_t cksum(void *, size_t);
u_int32_t lfs_sb_cksum(struct dlfs *);

/* lfs_debug.c */
#ifdef DEBUG
void lfs_dump_super(struct lfs *);
void lfs_dump_dinode(struct dinode *);
void lfs_check_bpp(struct lfs *, struct segment *, char *, int);
void lfs_check_segsum(struct lfs *, struct segment *, char *, int);
#endif /* DEBUG */

/* lfs_inode.c */
struct dinode *lfs_ifind(struct lfs *, ino_t, struct buf *);

/* lfs_segment.c */
void lfs_imtime(struct lfs *);
int lfs_vflush(struct vnode *);
int lfs_writevnodes(struct lfs *, struct mount *, struct segment *, int);
int lfs_segwrite(struct mount *, int);
void lfs_writefile(struct lfs *, struct segment *, struct vnode *);
int lfs_writeinode(struct lfs *, struct segment *, struct inode *);
int lfs_gatherblock(struct segment *, struct buf *, int *);
int lfs_gather(struct lfs *, struct segment *, struct vnode *, int (*match )(struct lfs *, struct buf *));
void lfs_updatemeta(struct segment *);
int lfs_initseg(struct lfs *);
void lfs_newseg(struct lfs *);
int lfs_writeseg(struct lfs *, struct segment *);
void lfs_writesuper(struct lfs *, daddr_t);
int lfs_match_data(struct lfs *, struct buf *);
int lfs_match_indir(struct lfs *, struct buf *);
int lfs_match_dindir(struct lfs *, struct buf *);
int lfs_match_tindir(struct lfs *, struct buf *);
void lfs_callback(struct buf *);
void lfs_supercallback(struct buf *);
void lfs_shellsort(struct buf **, ufs_daddr_t *, int);
int lfs_vref(struct vnode *);
void lfs_vunref(struct vnode *);
void lfs_vunref_head(struct vnode *);

/* lfs_subr.c */
void lfs_seglock(struct lfs *, unsigned long);
void lfs_segunlock(struct lfs *);

/* lfs_syscalls.c */
int lfs_fastvget(struct mount *, ino_t, ufs_daddr_t, struct vnode **, struct dinode *, int *);
struct buf *lfs_fakebuf(struct vnode *, int, size_t, caddr_t);

/* lfs_vfsops.c */
void lfs_init(void);
void lfs_reinit(void);
void lfs_done(void);
int lfs_mountroot(void);
int lfs_mount(struct mount *, const char *, void *, struct nameidata *, struct proc *);
int lfs_mountfs(struct vnode *, struct mount *, struct proc *);
int lfs_unmount(struct mount *, int, struct proc *);
int lfs_statfs(struct mount *, struct statfs *, struct proc *);
int lfs_sync(struct mount *, int, struct ucred *, struct proc *);
int lfs_vget(struct mount *, ino_t, struct vnode **);
int lfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int lfs_vptofh(struct vnode *, struct fid *);
int lfs_sysctl(int *, u_int, void *, size_t *, void *, size_t, struct proc *);

/* lfs_vnops.c */
void lfs_unmark_vnode(struct vnode *);
void lfs_itimes(struct inode *, struct timespec *, struct timespec *,
		struct timespec *);

int lfs_balloc	(void *);
int lfs_valloc	(void *);
int lfs_vfree	(void *);
int lfs_bwrite	(void *);
int lfs_update	(void *);
int lfs_truncate(void *);
int lfs_blkatoff(void *);
int lfs_fsync	(void *);
int lfs_symlink	(void *);
int lfs_mknod	(void *);
int lfs_create	(void *);
int lfs_mkdir	(void *);
int lfs_read	(void *);
int lfs_remove	(void *);
int lfs_rmdir	(void *);
int lfs_link	(void *);
int lfs_rename	(void *);
int lfs_getattr	(void *);
int lfs_setattr	(void *);
int lfs_close	(void *);
int lfs_inactive(void *);
int lfs_reclaim	(void *);
int lfs_write	(void *);
int lfs_whiteout(void *);
int lfs_getpages(void *);
int lfs_putpages(void *);

__END_DECLS
extern int lfs_mount_type;
extern int (**lfs_vnodeop_p)(void *);
extern int (**lfs_specop_p)(void *);
extern int (**lfs_fifoop_p)(void *);
extern struct genfs_ops lfs_genfsops;
