/*	$NetBSD: ufs_extern.h,v 1.50.2.1 2007/02/27 16:55:23 yamt Exp $	*/

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
 *	@(#)ufs_extern.h	8.10 (Berkeley) 5/14/95
 */

#ifndef _UFS_UFS_EXTERN_H_
#define _UFS_UFS_EXTERN_H_

struct buf;
struct componentname;
struct direct;
struct disklabel;
struct dquot;
struct fid;
struct flock;
struct indir;
struct inode;
struct mbuf;
struct mount;
struct nameidata;
struct lwp;
struct ufs_args;
struct ufsmount;
struct uio;
struct vattr;
struct vnode;

extern struct pool ufs_direct_pool;	/* memory pool for directs */

__BEGIN_DECLS
#define	ufs_abortop	genfs_abortop
int	ufs_access(void *);
int	ufs_advlock(void *);
int	ufs_bmap(void *);
int	ufs_close(void *);
int	ufs_create(void *);
int	ufs_getattr(void *);
int	ufs_inactive(void *);
#define	ufs_fcntl	genfs_fcntl
#define	ufs_ioctl	genfs_enoioctl
int	ufs_islocked(void *);
#define	ufs_lease_check genfs_lease_check
int	ufs_link(void *);
int	ufs_lock(void *);
int	ufs_lookup(void *);
int	ufs_mkdir(void *);
int	ufs_mknod(void *);
#define	ufs_mmap	genfs_mmap
#define	ufs_revoke	genfs_revoke
int	ufs_open(void *);
int	ufs_pathconf(void *);
int	ufs_print(void *);
int	ufs_readdir(void *);
int	ufs_readlink(void *);
int	ufs_remove(void *);
int	ufs_rename(void *);
int	ufs_rmdir(void *);
#define	ufs_seek	genfs_seek
#define	ufs_poll	genfs_poll
int	ufs_setattr(void *);
int	ufs_strategy(void *);
int	ufs_symlink(void *);
int	ufs_unlock(void *);
int	ufs_whiteout(void *);
int	ufsspec_close(void *);
int	ufsspec_read(void *);
int	ufsspec_write(void *);

int	ufsfifo_read(void *);
int	ufsfifo_write(void *);
int	ufsfifo_close(void *);

/* ufs_bmap.c */
typedef	bool (*ufs_issequential_callback_t)(const struct ufsmount *,
						 daddr_t, daddr_t);
int	ufs_bmaparray(struct vnode *, daddr_t, daddr_t *, struct indir *,
		      int *, int *, ufs_issequential_callback_t);
int	ufs_getlbns(struct vnode *, daddr_t, struct indir *, int *);

/* ufs_ihash.c */
void	ufs_ihashinit(void);
void	ufs_ihashreinit(void);
void	ufs_ihashdone(void);
struct vnode *ufs_ihashlookup(dev_t, ino_t);
struct vnode *ufs_ihashget(dev_t, ino_t, int);
void	ufs_ihashins(struct inode *);
void	ufs_ihashrem(struct inode *);

/* ufs_inode.c */
int	ufs_reclaim(struct vnode *, struct lwp *);
int	ufs_balloc_range(struct vnode *, off_t, off_t, kauth_cred_t, int);

/* ufs_lookup.c */
void	ufs_dirbad(struct inode *, doff_t, const char *);
int	ufs_dirbadentry(struct vnode *, struct direct *, int);
void	ufs_makedirentry(struct inode *, struct componentname *,
			 struct direct *);
int	ufs_direnter(struct vnode *, struct vnode *, struct direct *,
		     struct componentname *, struct buf *);
int	ufs_dirremove(struct vnode *, struct inode *, int, int);
int	ufs_dirrewrite(struct inode *, struct inode *, ino_t, int, int, int);
int	ufs_dirempty(struct inode *, ino_t, kauth_cred_t);
int	ufs_checkpath(struct inode *, struct inode *, kauth_cred_t);
int	ufs_blkatoff(struct vnode *, off_t, char **, struct buf **);

/* ufs_quota.c */
int	getinoquota(struct inode *);
int	chkdq(struct inode *, int64_t, kauth_cred_t, int);
int	chkdqchg(struct inode *, int64_t, kauth_cred_t, int);
int	chkiq(struct inode *, int32_t, kauth_cred_t, int);
int	chkiqchg(struct inode *, int32_t, kauth_cred_t, int);
void	chkdquot(struct inode *);
int	quotaon(struct lwp *, struct mount *, int, caddr_t);
int	quotaoff(struct lwp *, struct mount *, int);
int	getquota(struct mount *, u_long, int, caddr_t);
int	setquota(struct mount *, u_long, int, caddr_t);
int	setuse(struct mount *, u_long, int, caddr_t);
int	qsync(struct mount *);
int	dqget(struct vnode *, u_long, struct ufsmount *, int, struct dquot **);
void	dqref(struct dquot *);
void	dqrele(struct vnode *, struct dquot *);
int	dqsync(struct vnode *, struct dquot *);
void	dqflush(struct vnode *);

/* ufs_vfsops.c */
void	ufs_init(void);
void	ufs_reinit(void);
void	ufs_done(void);
int	ufs_start(struct mount *, int, struct lwp *);
int	ufs_root(struct mount *, struct vnode **);
int	ufs_quotactl(struct mount *, int, uid_t, void *, struct lwp *);
int	ufs_fhtovp(struct mount *, struct ufid *, struct vnode **);

/* ufs_vnops.c */
void	ufs_vinit(struct mount *, int (**)(void *),
		  int (**)(void *), struct vnode **);
int	ufs_makeinode(int, struct vnode *, struct vnode **,
		      struct componentname *);
int	ufs_gop_alloc(struct vnode *, off_t, off_t, int, kauth_cred_t);
void	ufs_gop_markupdate(struct vnode *, int);

/*
 * Snapshot function prototypes.
 */

void	ffs_snapgone(struct inode *);

/*
 * Soft dependency function prototypes.
 */
int   softdep_setup_directory_add(struct buf *, struct inode *, off_t,
				  ino_t, struct buf *, int);
void  softdep_change_directoryentry_offset(struct inode *, caddr_t,
					   caddr_t, caddr_t, int);
void  softdep_setup_remove(struct buf *, struct inode *, struct inode *, int);
void  softdep_setup_directory_change(struct buf *, struct inode *,
				     struct inode *, ino_t, int);
void  softdep_change_linkcnt(struct inode *);
void  softdep_releasefile(struct inode *);

__END_DECLS

#endif /* !_UFS_UFS_EXTERN_H_ */
