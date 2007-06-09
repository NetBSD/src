/*	$NetBSD: ffs_extern.h,v 1.55.6.1 2007/06/09 23:58:19 ad Exp $	*/

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
 *	@(#)ffs_extern.h	8.6 (Berkeley) 3/30/95
 */

#ifndef _UFS_FFS_FFS_EXTERN_H_
#define _UFS_FFS_FFS_EXTERN_H_

/*
 * Sysctl values for the fast filesystem.
 */
#define FFS_CLUSTERREAD		1	/* cluster reading enabled */
#define FFS_CLUSTERWRITE	2	/* cluster writing enabled */
#define FFS_REALLOCBLKS		3	/* block reallocation enabled */
#define FFS_ASYNCFREE		4	/* asynchronous block freeing enabled */
#define FFS_LOG_CHANGEOPT	5	/* log optimalization strategy change */
#define FFS_MAXID		6	/* number of valid ffs ids */

struct buf;
struct fid;
struct fs;
struct inode;
struct ufs1_dinode;
struct ufs2_dinode;
struct mount;
struct nameidata;
struct lwp;
struct statvfs;
struct timeval;
struct timespec;
struct ufsmount;
struct uio;
struct vnode;
struct mbuf;
struct cg;

#if defined(_KERNEL)

#define	FFS_ITIMES(ip, acc, mod, cre) \
	while ((ip)->i_flag & (IN_ACCESS | IN_CHANGE | IN_UPDATE | IN_MODIFY)) \
		ffs_itimes(ip, acc, mod, cre)

extern struct pool ffs_inode_pool;	/* memory pool for inodes */
extern struct pool ffs_dinode1_pool;	/* memory pool for UFS1 dinodes */
extern struct pool ffs_dinode2_pool;	/* memory pool for UFS2 dinodes */

#endif /* defined(_KERNEL) */

__BEGIN_DECLS

#if defined(_KERNEL)

/* ffs_alloc.c */
int	ffs_alloc(struct inode *, daddr_t, daddr_t , int, kauth_cred_t,
		  daddr_t *);
int	ffs_realloccg(struct inode *, daddr_t, daddr_t, int, int ,
		      kauth_cred_t, struct buf **, daddr_t *);
#if 0
int	ffs_reallocblks(void *);
#endif
int	ffs_valloc(struct vnode *, int, kauth_cred_t, struct vnode **);
daddr_t	ffs_blkpref_ufs1(struct inode *, daddr_t, int, int32_t *);
daddr_t	ffs_blkpref_ufs2(struct inode *, daddr_t, int, int64_t *);
void	ffs_blkfree(struct fs *, struct vnode *, daddr_t, long, ino_t);
int	ffs_vfree(struct vnode *, ino_t, int);
void	ffs_clusteracct(struct fs *, struct cg *, int32_t, int);
int	ffs_checkfreefile(struct fs *, struct vnode *, ino_t);

/* ffs_balloc.c */
int	ffs_balloc(struct vnode *, off_t, int, kauth_cred_t, int,
    struct buf **);

/* ffs_inode.c */
int	ffs_update(struct vnode *, const struct timespec *,
    const struct timespec *, int);
int	ffs_truncate(struct vnode *, off_t, int, kauth_cred_t, struct lwp *);

/* ffs_vfsops.c */
void	ffs_init(void);
void	ffs_reinit(void);
void	ffs_done(void);
int	ffs_mountroot(void);
int	ffs_mount(struct mount *, const char *, void *, struct nameidata *,
		  struct lwp *);
int	ffs_reload(struct mount *, kauth_cred_t, struct lwp *);
int	ffs_mountfs(struct vnode *, struct mount *, struct lwp *);
int	ffs_unmount(struct mount *, int, struct lwp *);
int	ffs_flushfiles(struct mount *, int, struct lwp *);
int	ffs_statvfs(struct mount *, struct statvfs *, struct lwp *);
int	ffs_sync(struct mount *, int, kauth_cred_t, struct lwp *);
int	ffs_vget(struct mount *, ino_t, struct vnode **);
int	ffs_fhtovp(struct mount *, struct fid *, struct vnode **);
int	ffs_vptofh(struct vnode *, struct fid *, size_t *);
int	ffs_extattrctl(struct mount *, int, struct vnode *, int,
		       const char *, struct lwp *);
int	ffs_suspendctl(struct mount *, int);
int	ffs_sbupdate(struct ufsmount *, int);
int	ffs_cgupdate(struct ufsmount *, int);

/* ffs_vnops.c */
int	ffs_read(void *);
int	ffs_write(void *);
int	ffs_fsync(void *);
int	ffs_reclaim(void *);
int	ffs_getpages(void *);
void	ffs_gop_size(struct vnode *, off_t, off_t *, int);
int	ffs_openextattr(void *);
int	ffs_closeextattr(void *);
int	ffs_getextattr(void *);
int	ffs_setextattr(void *);
int	ffs_listextattr(void *);
int	ffs_deleteextattr(void *);

#ifdef SYSCTL_SETUP_PROTO
SYSCTL_SETUP_PROTO(sysctl_vfs_ffs_setup);
#endif /* SYSCTL_SETUP_PROTO */

/*
 * Snapshot function prototypes.
 */
int	ffs_snapblkfree(struct fs *, struct vnode *, daddr_t, long, ino_t);
void	ffs_snapremove(struct vnode *);
int	ffs_snapshot(struct mount *, struct vnode *, struct timespec *);
void	ffs_snapshot_mount(struct mount *);
void	ffs_snapshot_unmount(struct mount *);
void	ffs_snapgone(struct inode *);

/*
 * Soft dependency function prototypes.
 */
void	softdep_initialize(void);
void	softdep_reinitialize(void);
int	softdep_mount(struct vnode *, struct mount *, struct fs *,
		      kauth_cred_t);
int	softdep_flushworklist(struct mount *, int *, struct lwp *);
int	softdep_flushfiles(struct mount *, int, struct lwp *);
void	softdep_update_inodeblock(struct inode *, struct buf *, int);
void	softdep_load_inodeblock(struct inode *);
void	softdep_freefile(struct vnode *, ino_t, int);
void	softdep_setup_freeblocks(struct inode *, off_t, int);
void	softdep_setup_inomapdep(struct buf *, struct inode *, ino_t);
void	softdep_setup_blkmapdep(struct buf *, struct fs *, daddr_t);
void	softdep_setup_allocdirect(struct inode *, daddr_t, daddr_t,
				  daddr_t, long, long, struct buf *);
void	softdep_setup_allocindir_meta(struct buf *, struct inode *,
				      struct buf *, int, daddr_t);
void	softdep_setup_allocindir_page(struct inode *, daddr_t,
				      struct buf *, int, daddr_t, daddr_t,
				      struct buf *);
void	softdep_fsync_mountdev(struct vnode *);
int	softdep_sync_metadata(void *);

extern int (**ffs_vnodeop_p)(void *);
extern int (**ffs_specop_p)(void *);
extern int (**ffs_fifoop_p)(void *);

#endif /* defined(_KERNEL) */

/* ffs_appleufs.c */
struct appleufslabel;
u_int16_t ffs_appleufs_cksum(const struct appleufslabel *);
int	ffs_appleufs_validate(const char*, const struct appleufslabel *,
			      struct appleufslabel *);
void	ffs_appleufs_set(struct appleufslabel *, const char *, time_t,
			 uint64_t);

/* ffs_bswap.c */
void	ffs_sb_swap(struct fs*, struct fs *);
void	ffs_dinode1_swap(struct ufs1_dinode *, struct ufs1_dinode *);
void	ffs_dinode2_swap(struct ufs2_dinode *, struct ufs2_dinode *);
struct csum;
void	ffs_csum_swap(struct csum *, struct csum *, int);
struct csum_total;
void	ffs_csumtotal_swap(struct csum_total *, struct csum_total *);
void	ffs_cg_swap(struct cg *, struct cg *, struct fs *);

/* ffs_subr.c */
void	ffs_load_inode(struct buf *, struct inode *, struct fs *, ino_t);
int	ffs_freefile(struct fs *, struct vnode *, ino_t, int);
void	ffs_fragacct(struct fs *, int, int32_t[], int, int);
int	ffs_isblock(struct fs *, u_char *, int32_t);
int	ffs_isfreeblock(struct fs *, u_char *, int32_t);
void	ffs_clrblock(struct fs *, u_char *, int32_t);
void	ffs_setblock(struct fs *, u_char *, int32_t);
void	ffs_itimes(struct inode *, const struct timespec *,
    const struct timespec *, const struct timespec *);

__END_DECLS

#endif /* !_UFS_FFS_FFS_EXTERN_H_ */
