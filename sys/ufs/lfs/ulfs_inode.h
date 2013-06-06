/*	$NetBSD: ulfs_inode.h,v 1.4 2013/06/06 00:51:25 dholland Exp $	*/
/*  from NetBSD: inode.h,v 1.64 2012/11/19 00:36:21 jakllsch Exp  */

/*
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)inode.h	8.9 (Berkeley) 5/14/95
 */

#ifndef _UFS_LFS_ULFS_INODE_H_
#define	_UFS_LFS_ULFS_INODE_H_

#include <sys/vnode.h>
#include <ufs/lfs/ulfs_dinode.h>
#include <ufs/lfs/ulfs_dir.h>
#include <ufs/lfs/ulfs_quotacommon.h>
#include <miscfs/genfs/genfs_node.h>

/*
 * Lookup result state (other than the result inode). This is
 * currently stashed in the vnode between VOP_LOOKUP and directory
 * operation VOPs, which is gross.
 *
 * XXX ulr_diroff is a lookup hint from the previos call of VOP_LOOKUP.
 * probably it should not be here.
 */
struct ulfs_lookup_results {
	int32_t	  ulr_count;	/* Size of free slot in directory. */
	doff_t	  ulr_endoff;	/* End of useful stuff in directory. */
	doff_t	  ulr_diroff;	/* Offset in dir, where we found last entry. */
	doff_t	  ulr_offset;	/* Offset of free space in directory. */
	u_int32_t ulr_reclen;	/* Size of found directory entry. */
};

/* notyet XXX */
#define ULFS_CHECK_CRAPCOUNTER(dp) ((void)(dp)->i_crapcounter)

/*
 * Per-filesystem inode extensions.
 */
struct lfs_inode_ext;

/*
 * The inode is used to describe each active (or recently active) file in the
 * ULFS filesystem. It is composed of two types of information. The first part
 * is the information that is needed only while the file is active (such as
 * the identity of the file and linkage to speed its lookup). The second part
 * is the permanent meta-data associated with the file which is read in
 * from the permanent dinode from long term storage when the file becomes
 * active, and is put back when the file is no longer being used.
 */
struct inode {
	struct genfs_node i_gnode;
	LIST_ENTRY(inode) i_hash;/* Hash chain. */
	TAILQ_ENTRY(inode) i_nextsnap; /* snapshot file list. */
	struct	vnode *i_vnode;	/* Vnode associated with this inode. */
	struct  ulfsmount *i_ump; /* Mount point associated with this inode. */
	struct	vnode *i_devvp;	/* Vnode for block I/O. */
	u_int32_t i_flag;	/* flags, see below */
	dev_t	  i_dev;	/* Device associated with the inode. */
	ino_t	  i_number;	/* The identity of the inode. */

	union {			/* Associated filesystem. */
		struct	lfs *lfs;	/* LFS */
	} inode_u;
#define	i_lfs	inode_u.lfs

	void	*i_unused1;	/* Unused. */
	struct	 dquot *i_dquot[ULFS_MAXQUOTAS]; /* Dquot structures. */
	u_quad_t i_modrev;	/* Revision level for NFS lease. */
	struct	 lockf *i_lockf;/* Head of byte-level lock list. */

	/*
	 * Side effects; used during (and after) directory lookup.
	 * XXX should not be here.
	 */
	struct ulfs_lookup_results i_crap;
	unsigned i_crapcounter;	/* serial number for i_crap */

	/*
	 * Inode extensions
	 */
	union {
		/* Other extensions could go here... */
		struct  lfs_inode_ext *lfs;
	} inode_ext;
	/*
	 * Copies from the on-disk dinode itself.
	 *
	 * These fields are currently only used by LFS.
	 */
	u_int16_t i_mode;	/* IFMT, permissions; see below. */
	int16_t   i_nlink;	/* File link count. */
	u_int64_t i_size;	/* File byte count. */
	u_int32_t i_flags;	/* Status flags (chflags). */
	int32_t   i_gen;	/* Generation number. */
	u_int32_t i_uid;	/* File owner. */
	u_int32_t i_gid;	/* File group. */
	u_int16_t i_omode;	/* Old mode, for ulfs_reclaim. */

	struct dirhash *i_dirhash;	/* Hashing for large directories */

	/*
	 * The on-disk dinode itself.
	 */
	union {
		struct	ulfs1_dinode *ffs1_din;	/* 128 bytes of the on-disk dinode. */
		struct	ulfs2_dinode *ffs2_din;
	} i_din;
};

#define	i_ffs1_atime		i_din.ffs1_din->di_atime
#define	i_ffs1_atimensec	i_din.ffs1_din->di_atimensec
#define	i_ffs1_blocks		i_din.ffs1_din->di_blocks
#define	i_ffs1_ctime		i_din.ffs1_din->di_ctime
#define	i_ffs1_ctimensec	i_din.ffs1_din->di_ctimensec
#define	i_ffs1_db		i_din.ffs1_din->di_db
#define	i_ffs1_flags		i_din.ffs1_din->di_flags
#define	i_ffs1_gen		i_din.ffs1_din->di_gen
#define	i_ffs1_gid		i_din.ffs1_din->di_gid
#define	i_ffs1_ib		i_din.ffs1_din->di_ib
#define	i_ffs1_mode		i_din.ffs1_din->di_mode
#define	i_ffs1_mtime		i_din.ffs1_din->di_mtime
#define	i_ffs1_mtimensec	i_din.ffs1_din->di_mtimensec
#define	i_ffs1_nlink		i_din.ffs1_din->di_nlink
#define	i_ffs1_rdev		i_din.ffs1_din->di_rdev
#define	i_ffs1_size		i_din.ffs1_din->di_size
#define	i_ffs1_uid		i_din.ffs1_din->di_uid
#define	i_ffs1_ouid		i_din.ffs1_din->di_u.oldids[0]
#define	i_ffs1_ogid		i_din.ffs1_din->di_u.oldids[1]

#define	i_ffs2_atime		i_din.ffs2_din->di_atime
#define	i_ffs2_atimensec	i_din.ffs2_din->di_atimensec
#define	i_ffs2_birthtime	i_din.ffs2_din->di_birthtime
#define	i_ffs2_birthnsec	i_din.ffs2_din->di_birthnsec
#define	i_ffs2_blocks		i_din.ffs2_din->di_blocks
#define	i_ffs2_blksize		i_din.ffs2_din->di_blksize
#define	i_ffs2_ctime		i_din.ffs2_din->di_ctime
#define	i_ffs2_ctimensec	i_din.ffs2_din->di_ctimensec
#define	i_ffs2_db		i_din.ffs2_din->di_db
#define	i_ffs2_flags		i_din.ffs2_din->di_flags
#define	i_ffs2_gen		i_din.ffs2_din->di_gen
#define	i_ffs2_gid		i_din.ffs2_din->di_gid
#define	i_ffs2_ib		i_din.ffs2_din->di_ib
#define	i_ffs2_mode		i_din.ffs2_din->di_mode
#define	i_ffs2_mtime		i_din.ffs2_din->di_mtime
#define	i_ffs2_mtimensec	i_din.ffs2_din->di_mtimensec
#define	i_ffs2_nlink		i_din.ffs2_din->di_nlink
#define	i_ffs2_rdev		i_din.ffs2_din->di_rdev
#define	i_ffs2_size		i_din.ffs2_din->di_size
#define	i_ffs2_uid		i_din.ffs2_din->di_uid
#define	i_ffs2_kernflags	i_din.ffs2_din->di_kernflags
#define	i_ffs2_extsize		i_din.ffs2_din->di_extsize
#define	i_ffs2_extb		i_din.ffs2_din->di_extb

/* These flags are kept in i_flag. */
#define	IN_ACCESS	0x0001		/* Access time update request. */
#define	IN_CHANGE	0x0002		/* Inode change time update request. */
#define	IN_UPDATE	0x0004		/* Inode was written to; update mtime. */
#define	IN_MODIFY	0x2000		/* Modification time update request. */
#define	IN_MODIFIED	0x0008		/* Inode has been modified. */
#define	IN_ACCESSED	0x0010		/* Inode has been accessed. */
/* #define	IN_UNUSED	0x0020 */	/* unused, was IN_RENAME */
#define	IN_SHLOCK	0x0040		/* File has shared lock. */
#define	IN_EXLOCK	0x0080		/* File has exclusive lock. */
#define	IN_CLEANING	0x0100		/* LFS: file is being cleaned */
#define	IN_ADIROP	0x0200		/* LFS: dirop in progress */
#define	IN_SPACECOUNTED	0x0400		/* Blocks to be freed in free count. */
#define	IN_PAGING       0x1000		/* LFS: file is on paging queue */
#define IN_CDIROP       0x4000          /* LFS: dirop completed pending i/o */
#if defined(_KERNEL)

/*
 * The DIP macro is used to access fields in the dinode that are
 * not cached in the inode itself.
 */
#define	DIP(ip, field) \
	(((ip)->i_ump->um_fstype == ULFS1) ? \
	(ip)->i_ffs1_##field : (ip)->i_ffs2_##field)

#define	DIP_ASSIGN(ip, field, value)					\
	do {								\
		if ((ip)->i_ump->um_fstype == ULFS1)			\
			(ip)->i_ffs1_##field = (value);			\
		else							\
			(ip)->i_ffs2_##field = (value);			\
	} while(0)

#define	DIP_ADD(ip, field, value)					\
	do {								\
		if ((ip)->i_ump->um_fstype == ULFS1)			\
			(ip)->i_ffs1_##field += (value);		\
		else							\
			(ip)->i_ffs2_##field += (value);		\
	} while(0)

#define	 SHORTLINK(ip) \
	(((ip)->i_ump->um_fstype == ULFS1) ? \
	(void *)(ip)->i_ffs1_db : (void *)(ip)->i_ffs2_db)


/*
 * Structure used to pass around logical block paths generated by
 * ulfs_getlbns and used by truncate and bmap code.
 */
struct indir {
	daddr_t in_lbn;		/* Logical block number. */
	int	in_off;			/* Offset in buffer. */
	int	in_exists;		/* Flag if the block exists. */
};

/* Convert between inode pointers and vnode pointers. */
#define	VTOI(vp)	((struct inode *)(vp)->v_data)
#define	ITOV(ip)	((ip)->i_vnode)

/* This overlays the fid structure (see fstypes.h). */
struct ufid {
	u_int16_t ufid_len;	/* Length of structure. */
	u_int16_t ufid_pad;	/* Force 32-bit alignment. */
	u_int32_t ufid_ino;	/* File number (ino). */
	int32_t	  ufid_gen;	/* Generation number. */
};
#endif /* _KERNEL */

#endif /* !_UFS_LFS_ULFS_INODE_H_ */
