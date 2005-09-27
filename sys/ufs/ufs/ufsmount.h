/*	$NetBSD: ufsmount.h,v 1.23 2005/09/27 06:48:56 yamt Exp $	*/

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

#ifndef _UFS_UFS_UFSMOUNT_H_
#define _UFS_UFS_UFSMOUNT_H_

/*
 * Arguments to mount UFS-based filesystems
 */
struct ufs_args {
	char	*fspec;			/* block special device to mount */
};

/*
 * Arguments to mount MFS
 */
struct mfs_args {
	char	*fspec;			/* name to export for statfs */
	struct	export_args30 _pad1; /* compat with old userland tools */
	caddr_t	base;			/* base of file system in memory */
	u_long	size;			/* size of file system */
};

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#endif

#include <ufs/ufs/extattr.h>

struct buf;
struct inode;
struct nameidata;
struct timeval;
struct ucred;
struct uio;
struct vnode;

/* This structure describes the UFS specific mount structure data. */
struct ufsmount {
	struct	mount *um_mountp;		/* filesystem vfs structure */
	dev_t	um_dev;				/* device mounted */
	struct	vnode *um_devvp;		/* block device mounted vnode */
	u_long	um_fstype;
	u_int32_t um_flags;			/* UFS-specific flags - see below */
	union {					/* pointer to superblock */
		struct	fs *fs;			/* FFS */
		struct	lfs *lfs;		/* LFS */
		struct  m_ext2fs *e2fs; /* EXT2FS */
	} ufsmount_u;
#define	um_fs	ufsmount_u.fs
#define	um_lfs	ufsmount_u.lfs
#define um_e2fs	ufsmount_u.e2fs
#define um_e2fsb ufsmount_u.e2fs->s_es

	/* Extended attribute information. */
	struct ufs_extattr_per_mount um_extattr;

	struct	vnode *um_quotas[MAXQUOTAS];	/* pointer to quota files */
	struct	ucred *um_cred[MAXQUOTAS];	/* quota file access cred */
	u_long	um_nindir;			/* indirect ptrs per block */
	u_long	um_lognindir;			/* log2 of um_nindir */
	u_long	um_bptrtodb;			/* indir ptr to disk block */
	u_long	um_seqinc;			/* inc between seq blocks */
	time_t	um_btime[MAXQUOTAS];		/* block quota time limit */
	time_t	um_itime[MAXQUOTAS];		/* inode quota time limit */
	char	um_qflags[MAXQUOTAS];		/* quota specific flags */
	void	*um_oldfscompat;		/* save 4.2 rotbl */
	TAILQ_HEAD(inodelst, inode) um_snapshots; /* list of active snapshots */
	daddr_t	*um_snapblklist;		/* snapshot block hints list */
	int	um_maxsymlinklen;
	int	um_dirblksiz;
	u_int64_t um_maxfilesize;

	const struct ufs_ops *um_ops;
};

struct ufs_ops {
	void (*uo_itimes)(struct inode *ip, const struct timespec *,
	    const struct timespec *, const struct timespec *);
};

#define	UFS_ITIMES(vp, acc, mod, cre) \
	(*VFSTOUFS((vp)->v_mount)->um_ops->uo_itimes)((ip), (acc), (mod), (cre))

/* UFS-specific flags */
#define UFS_NEEDSWAP	0x01	/* filesystem metadata need byte-swapping */
#define UFS_ISAPPLEUFS	0x02	/* filesystem is Apple UFS */

/*
 * Filesystem types
 */
#define UFS1  1
#define UFS2  2


/*
 * Flags describing the state of quotas.
 */
#define	QTF_OPENING	0x01			/* Q_QUOTAON in progress */
#define	QTF_CLOSING	0x02			/* Q_QUOTAOFF in progress */

/* Convert mount ptr to ufsmount ptr. */
#define VFSTOUFS(mp)	((struct ufsmount *)((mp)->mnt_data))

#ifdef APPLE_UFS
#define UFS_MPISAPPLEUFS(ump)	((ump)->um_flags & UFS_ISAPPLEUFS)
#else
#define UFS_MPISAPPLEUFS(ump)	(0)
#endif

/*
 * Macros to access file system parameters in the ufsmount structure.
 * Used by ufs_bmap.
 */
#define MNINDIR(ump)			((ump)->um_nindir)
#define	blkptrtodb(ump, b)		((b) << (ump)->um_bptrtodb)
#endif /* _KERNEL */

#endif /* !_UFS_UFS_UFSMOUNT_H_ */
