/*	$NetBSD: ulfsmount.h,v 1.7.2.2 2013/06/23 06:18:39 tls Exp $	*/
/*  from NetBSD: ufsmount.h,v 1.39 2012/10/19 17:09:08 drochner Exp  */

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)ufsmount.h	8.6 (Berkeley) 3/30/95
 */

#ifndef _UFS_LFS_ULFSMOUNT_H_
#define _UFS_LFS_ULFSMOUNT_H_

#include <sys/mount.h> /* struct export_args30 */

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_lfs.h"
#endif

#include <sys/mutex.h>

#include <ufs/lfs/ulfs_extattr.h>
#include <ufs/lfs/ulfs_quotacommon.h>

struct buf;
struct inode;
struct nameidata;
struct timeval;
struct uio;
struct vnode;

/* This structure describes the ULFS specific mount structure data. */
struct ulfsmount {
	struct	mount *um_mountp;		/* filesystem vfs structure */
	dev_t	um_dev;				/* device mounted */
	struct	vnode *um_devvp;		/* block device mounted vnode */
	u_long	um_fstype;
	u_int32_t um_flags;			/* ULFS-specific flags - see below */
	union {					/* pointer to superblock */
		struct	lfs *lfs;		/* LFS */
	} ulfsmount_u;
#define	um_lfs	ulfsmount_u.lfs

	/* Extended attribute information. */
	struct ulfs_extattr_per_mount um_extattr;

	struct	vnode *um_quotas[ULFS_MAXQUOTAS];	/* pointer to quota files */
	kauth_cred_t   um_cred[ULFS_MAXQUOTAS];	/* quota file access cred */
	u_long	um_nindir;			/* indirect ptrs per block */
	u_long	um_lognindir;			/* log2 of um_nindir */
	u_long	um_bptrtodb;			/* indir ptr to disk block */
	u_long	um_seqinc;			/* inc between seq blocks */
	kmutex_t um_lock;			/* lock on global data */
	union {
	    struct um_q1 {
		time_t	q1_btime[ULFS_MAXQUOTAS];	/* block quota time limit */
		time_t	q1_itime[ULFS_MAXQUOTAS];	/* inode quota time limit */
		char	q1_qflags[ULFS_MAXQUOTAS];	/* quota specific flags */
	    } um_q1;
	    struct um_q2 {
		uint64_t q2_bsize;		/* block size of quota file */
		uint64_t q2_bmask;		/* mask for above */
	    } um_q2;
	} um_q;
#define umq1_btime  um_q.um_q1.q1_btime
#define umq1_itime  um_q.um_q1.q1_itime
#define umq1_qflags um_q.um_q1.q1_qflags
#define umq2_bsize  um_q.um_q2.q2_bsize
#define umq2_bmask  um_q.um_q2.q2_bmask

	void	*um_oldfscompat;		/* save 4.2 rotbl */
	int	um_maxsymlinklen;
	int	um_dirblksiz;
	u_int64_t um_maxfilesize;
	void	*um_snapinfo;			/* snapshot private data */

	const struct ulfs_ops *um_ops;

	void *um_discarddata;
};

struct ulfs_ops {
	void (*uo_itimes)(struct inode *ip, const struct timespec *,
	    const struct timespec *, const struct timespec *);
	int (*uo_update)(struct vnode *, const struct timespec *,
	    const struct timespec *, int);
	int (*uo_truncate)(struct vnode *, off_t, int, kauth_cred_t);
	int (*uo_valloc)(struct vnode *, int, kauth_cred_t, struct vnode **);
	int (*uo_vfree)(struct vnode *, ino_t, int);
	int (*uo_balloc)(struct vnode *, off_t, int, kauth_cred_t, int,
	    struct buf **);
        void (*uo_unmark_vnode)(struct vnode *);
};

#define	ULFS_OPS(vp)	(VFSTOULFS((vp)->v_mount)->um_ops)

#define	ULFS_ITIMES(vp, acc, mod, cre) \
	(*ULFS_OPS(vp)->uo_itimes)(VTOI(vp), (acc), (mod), (cre))
#define	ULFS_UPDATE(vp, acc, mod, flags) \
	(*ULFS_OPS(vp)->uo_update)((vp), (acc), (mod), (flags))
#define	ULFS_TRUNCATE(vp, off, flags, cr) \
	(*ULFS_OPS(vp)->uo_truncate)((vp), (off), (flags), (cr))
#define	ULFS_VALLOC(vp, mode, cr, vpp) \
	(*ULFS_OPS(vp)->uo_valloc)((vp), (mode), (cr), (vpp))
#define	ULFS_VFREE(vp, ino, mode) \
	(*ULFS_OPS(vp)->uo_vfree)((vp), (ino), (mode))
#define	ULFS_BALLOC(vp, off, size, cr, flags, bpp) \
	(*ULFS_OPS(vp)->uo_balloc)((vp), (off), (size), (cr), (flags), (bpp))
#define	ULFS_UNMARK_VNODE(vp) \
	(*ULFS_OPS(vp)->uo_unmark_vnode)((vp))

/* ULFS-specific flags */
#define ULFS_NEEDSWAP	0x01	/* filesystem metadata need byte-swapping */
/*	unused		0x02	   */
#define ULFS_QUOTA	0x04	/* filesystem has QUOTA (v1) */
#define ULFS_QUOTA2	0x08	/* filesystem has QUOTA2 */

/*
 * Filesystem types
 */
#define ULFS1  1
#define ULFS2  2


/*
 * Flags describing the state of quotas.
 */
#define	QTF_OPENING	0x01			/* Q_QUOTAON in progress */
#define	QTF_CLOSING	0x02			/* Q_QUOTAOFF in progress */

/* Convert mount ptr to ulfsmount ptr. */
#define VFSTOULFS(mp)	((struct ulfsmount *)((mp)->mnt_data))

/*
 * Macros to access file system parameters in the ulfsmount structure.
 * Used by ulfs_bmap.
 */
#define MNINDIR(ump)			((ump)->um_nindir)
#define	blkptrtodb(ump, b)		((b) << (ump)->um_bptrtodb)

/*
 * Predicate for byte-swapping support.
 */
#define	FSFMT(vp)	(((vp)->v_mount->mnt_iflag & IMNT_DTYPE) == 0)

#endif /* _KERNEL */

#endif /* !_UFS_LFS_ULFSMOUNT_H_ */
