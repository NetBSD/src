/*	$NetBSD: lfs.h,v 1.172 2015/08/02 18:18:46 dholland Exp $	*/

/*  from NetBSD: dinode.h,v 1.22 2013/01/22 09:39:18 dholland Exp  */
/*  from NetBSD: dir.h,v 1.21 2009/07/22 04:49:19 dholland Exp  */

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 The NetBSD Foundation, Inc.
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
 *	@(#)lfs.h	8.9 (Berkeley) 5/8/95
 */
/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
 *
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
 *	@(#)dinode.h	8.9 (Berkeley) 3/29/95
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)dir.h	8.5 (Berkeley) 4/27/95
 */

/*
 * NOTE: COORDINATE ON-DISK FORMAT CHANGES WITH THE FREEBSD PROJECT.
 */

#ifndef _UFS_LFS_LFS_H_
#define _UFS_LFS_LFS_H_

#include <sys/rwlock.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/condvar.h>
#include <sys/mount.h>
#include <sys/pool.h>

/*
 * Compile-time options for LFS.
 */
#define LFS_IFIND_RETRIES  16
#define LFS_LOGLENGTH      1024 /* size of debugging log */
#define LFS_MAX_ACTIVE	   10	/* Dirty segments before ckp forced */

/*
 * Fixed filesystem layout parameters
 */
#define	LFS_LABELPAD	8192		/* LFS label size */
#define	LFS_SBPAD	8192		/* LFS superblock size */

#define	LFS_UNUSED_INUM	0		/* 0: out of band inode number */
#define	LFS_IFILE_INUM	1		/* 1: IFILE inode number */
					/* 2: Root inode number */
#define	LFS_LOSTFOUNDINO 3		/* 3: lost+found inode number */
#define	LFS_FIRST_INUM	4		/* 4: first free inode number */

/*
 * The root inode is the root of the file system.  Inode 0 can't be used for
 * normal purposes and historically bad blocks were linked to inode 1, thus
 * the root inode is 2.  (Inode 1 is no longer used for this purpose, however
 * numerous dump tapes make this assumption, so we are stuck with it).
 */
#define	ULFS_ROOTINO	((ino_t)2)

/*
 * The Whiteout inode# is a dummy non-zero inode number which will
 * never be allocated to a real file.  It is used as a place holder
 * in the directory entry which has been tagged as a LFS_DT_WHT entry.
 * See the comments about ULFS_ROOTINO above.
 */
#define	ULFS_WINO	((ino_t)1)


#define	LFS_V1_SUMMARY_SIZE	512     /* V1 fixed summary size */
#define	LFS_DFL_SUMMARY_SIZE	512	/* Default summary size */

#define LFS_MAXNAMLEN	255		/* maximum name length in a dir */

#define ULFS_NXADDR	2
#define	ULFS_NDADDR	12		/* Direct addresses in inode. */
#define	ULFS_NIADDR	3		/* Indirect addresses in inode. */

/*
 * Adjustable filesystem parameters
 */
#ifndef LFS_ATIME_IFILE
# define LFS_ATIME_IFILE 0 /* Store atime info in ifile (optional in LFSv1) */
#endif
#define LFS_MARKV_MAXBLKCNT	65536	/* Max block count for lfs_markv() */

/*
 * Directories
 */

/*
 * A directory consists of some number of blocks of LFS_DIRBLKSIZ
 * bytes, where LFS_DIRBLKSIZ is chosen such that it can be transferred
 * to disk in a single atomic operation (e.g. 512 bytes on most machines).
 *
 * Each LFS_DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct lfs_direct at the front of it, containing its inode number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary.
 * All names are guaranteed null terminated.
 * The maximum length of a name in a directory is LFS_MAXNAMLEN.
 *
 * The macro DIRSIZ(fmt, dp) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > DIRSIZ(fmt, dp).  All LFS_DIRBLKSIZ bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->d_reclen.  If the first entry of
 * a directory block is free, then its dp->d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->d_ino set to 0.
 */

/*
 * Directory block size.
 */
#undef	LFS_DIRBLKSIZ
#define	LFS_DIRBLKSIZ	DEV_BSIZE

/*
 * Convert between stat structure types and directory types.
 */
#define	LFS_IFTODT(mode)	(((mode) & 0170000) >> 12)
#define	LFS_DTTOIF(dirtype)	((dirtype) << 12)

/*
 * The LFS_DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct lfs_direct
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#define	LFS_DIRECTSIZ(namlen) \
	((sizeof(struct lfs_direct) - (LFS_MAXNAMLEN+1)) + (((namlen)+1 + 3) &~ 3))

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define LFS_DIRSIZ(oldfmt, dp, needswap)	\
    (((oldfmt) && !(needswap)) ?		\
    LFS_DIRECTSIZ((dp)->d_type) : LFS_DIRECTSIZ((dp)->d_namlen))
#else
#define LFS_DIRSIZ(oldfmt, dp, needswap)	\
    (((oldfmt) && (needswap)) ?			\
    LFS_DIRECTSIZ((dp)->d_type) : LFS_DIRECTSIZ((dp)->d_namlen))
#endif

/* Constants for the first argument of LFS_DIRSIZ */
#define LFS_OLDDIRFMT	1
#define LFS_NEWDIRFMT	0

/*
 * Theoretically, directories can be more than 2Gb in length; however, in
 * practice this seems unlikely. So, we define the type doff_t as a 32-bit
 * quantity to keep down the cost of doing lookup on a 32-bit machine.
 */
#define	doff_t		int32_t
#define	lfs_doff_t	int32_t
#define	LFS_MAXDIRSIZE	(0x7fffffff)

/*
 * File types for d_type
 */
#define	LFS_DT_UNKNOWN	 0
#define	LFS_DT_FIFO	 1
#define	LFS_DT_CHR	 2
#define	LFS_DT_DIR	 4
#define	LFS_DT_BLK	 6
#define	LFS_DT_REG	 8
#define	LFS_DT_LNK	10
#define	LFS_DT_SOCK	12
#define	LFS_DT_WHT	14

/*
 * (See notes above)
 */
#define d_ino d_fileno
struct lfs_direct {
	u_int32_t d_fileno;		/* inode number of entry */
	u_int16_t d_reclen;		/* length of this record */
	u_int8_t  d_type; 		/* file type, see below */
	u_int8_t  d_namlen;		/* length of string in d_name */
	char	  d_name[LFS_MAXNAMLEN + 1];/* name with length <= LFS_MAXNAMLEN */
};

/*
 * Template for manipulating directories.  Should use struct lfs_direct's,
 * but the name field is LFS_MAXNAMLEN - 1, and this just won't do.
 */
struct lfs_dirtemplate {
	u_int32_t	dot_ino;
	int16_t		dot_reclen;
	u_int8_t	dot_type;
	u_int8_t	dot_namlen;
	char		dot_name[4];	/* must be multiple of 4 */
	u_int32_t	dotdot_ino;
	int16_t		dotdot_reclen;
	u_int8_t	dotdot_type;
	u_int8_t	dotdot_namlen;
	char		dotdot_name[4];	/* ditto */
};

/*
 * This is the old format of directories, sans type element.
 */
struct lfs_odirtemplate {
	u_int32_t	dot_ino;
	int16_t		dot_reclen;
	u_int16_t	dot_namlen;
	char		dot_name[4];	/* must be multiple of 4 */
	u_int32_t	dotdot_ino;
	int16_t		dotdot_reclen;
	u_int16_t	dotdot_namlen;
	char		dotdot_name[4];	/* ditto */
};

/*
 * Inodes
 */

/*
 * A dinode contains all the meta-data associated with a LFS file.
 * This structure defines the on-disk format of a dinode. Since
 * this structure describes an on-disk structure, all its fields
 * are defined by types with precise widths.
 */

struct ulfs1_dinode {
	u_int16_t	di_mode;	/*   0: IFMT, permissions; see below. */
	int16_t		di_nlink;	/*   2: File link count. */
	u_int32_t	di_inumber;	/*   4: Inode number. */
	u_int64_t	di_size;	/*   8: File byte count. */
	int32_t		di_atime;	/*  16: Last access time. */
	int32_t		di_atimensec;	/*  20: Last access time. */
	int32_t		di_mtime;	/*  24: Last modified time. */
	int32_t		di_mtimensec;	/*  28: Last modified time. */
	int32_t		di_ctime;	/*  32: Last inode change time. */
	int32_t		di_ctimensec;	/*  36: Last inode change time. */
	int32_t		di_db[ULFS_NDADDR]; /*  40: Direct disk blocks. */
	int32_t		di_ib[ULFS_NIADDR]; /*  88: Indirect disk blocks. */
	u_int32_t	di_flags;	/* 100: Status flags (chflags). */
	u_int32_t	di_blocks;	/* 104: Blocks actually held. */
	int32_t		di_gen;		/* 108: Generation number. */
	u_int32_t	di_uid;		/* 112: File owner. */
	u_int32_t	di_gid;		/* 116: File group. */
	u_int64_t	di_modrev;	/* 120: i_modrev for NFSv4 */
};

struct ulfs2_dinode {
	u_int16_t	di_mode;	/*   0: IFMT, permissions; see below. */
	int16_t		di_nlink;	/*   2: File link count. */
	u_int32_t	di_uid;		/*   4: File owner. */
	u_int32_t	di_gid;		/*   8: File group. */
	u_int32_t	di_blksize;	/*  12: Inode blocksize. */
	u_int64_t	di_size;	/*  16: File byte count. */
	u_int64_t	di_blocks;	/*  24: Bytes actually held. */
	int64_t		di_atime;	/*  32: Last access time. */
	int64_t		di_mtime;	/*  40: Last modified time. */
	int64_t		di_ctime;	/*  48: Last inode change time. */
	int64_t		di_birthtime;	/*  56: Inode creation time. */
	int32_t		di_mtimensec;	/*  64: Last modified time. */
	int32_t		di_atimensec;	/*  68: Last access time. */
	int32_t		di_ctimensec;	/*  72: Last inode change time. */
	int32_t		di_birthnsec;	/*  76: Inode creation time. */
	int32_t		di_gen;		/*  80: Generation number. */
	u_int32_t	di_kernflags;	/*  84: Kernel flags. */
	u_int32_t	di_flags;	/*  88: Status flags (chflags). */
	int32_t		di_extsize;	/*  92: External attributes block. */
	int64_t		di_extb[ULFS_NXADDR];/* 96: External attributes block. */
	int64_t		di_db[ULFS_NDADDR]; /* 112: Direct disk blocks. */
	int64_t		di_ib[ULFS_NIADDR]; /* 208: Indirect disk blocks. */
	u_int64_t	di_modrev;	/* 232: i_modrev for NFSv4 */
	u_int32_t	di_inumber;	/* 240: Inode number */
	u_int32_t	di_spare[3];	/* 244: Reserved; currently unused */
};

/*
 * The di_db fields may be overlaid with other information for
 * file types that do not have associated disk storage. Block
 * and character devices overlay the first data block with their
 * dev_t value. Short symbolic links place their path in the
 * di_db area.
 */
#define	di_rdev		di_db[0]

/* Size of the on-disk inode. */
#define	LFS_DINODE1_SIZE	(sizeof(struct ulfs1_dinode))	/* 128 */
#define	LFS_DINODE2_SIZE	(sizeof(struct ulfs2_dinode))

/* File types, found in the upper bits of di_mode. */
#define	LFS_IFMT	0170000		/* Mask of file type. */
#define	LFS_IFIFO	0010000		/* Named pipe (fifo). */
#define	LFS_IFCHR	0020000		/* Character device. */
#define	LFS_IFDIR	0040000		/* Directory file. */
#define	LFS_IFBLK	0060000		/* Block device. */
#define	LFS_IFREG	0100000		/* Regular file. */
#define	LFS_IFLNK	0120000		/* Symbolic link. */
#define	LFS_IFSOCK	0140000		/* UNIX domain socket. */
#define	LFS_IFWHT	0160000		/* Whiteout. */

/*
 * "struct buf" associated definitions
 */

/* Unassigned disk addresses. */
#define	UNASSIGNED	-1
#define UNWRITTEN	-2

/* Unused logical block number */
#define LFS_UNUSED_LBN	-1

/*
 * "struct inode" associated definitions
 */

/* For convenience */
#define IN_ALLMOD (IN_MODIFIED|IN_ACCESS|IN_CHANGE|IN_UPDATE|IN_MODIFY|IN_ACCESSED|IN_CLEANING)

/*
 * On-disk and in-memory checkpoint segment usage structure.
 */
typedef struct segusage SEGUSE;
struct segusage {
	u_int32_t su_nbytes;		/* 0: number of live bytes */
	u_int32_t su_olastmod;		/* 4: SEGUSE last modified timestamp */
	u_int16_t su_nsums;		/* 8: number of summaries in segment */
	u_int16_t su_ninos;		/* 10: number of inode blocks in seg */

#define	SEGUSE_ACTIVE		0x01	/*  segment currently being written */
#define	SEGUSE_DIRTY		0x02	/*  segment has data in it */
#define	SEGUSE_SUPERBLOCK	0x04	/*  segment contains a superblock */
#define SEGUSE_ERROR		0x08	/*  cleaner: do not clean segment */
#define SEGUSE_EMPTY		0x10	/*  segment is empty */
#define SEGUSE_INVAL		0x20	/*  segment is invalid */
	u_int32_t su_flags;		/* 12: segment flags */
	u_int64_t su_lastmod;		/* 16: last modified timestamp */
};

typedef struct segusage_v1 SEGUSE_V1;
struct segusage_v1 {
	u_int32_t su_nbytes;		/* 0: number of live bytes */
	u_int32_t su_lastmod;		/* 4: SEGUSE last modified timestamp */
	u_int16_t su_nsums;		/* 8: number of summaries in segment */
	u_int16_t su_ninos;		/* 10: number of inode blocks in seg */
	u_int32_t su_flags;		/* 12: segment flags  */
};

/*
 * On-disk file information.  One per file with data blocks in the segment.
 */
typedef struct finfo FINFO;
struct finfo {
	u_int32_t fi_nblocks;		/* number of blocks */
	u_int32_t fi_version;		/* version number */
	u_int32_t fi_ino;		/* inode number */
	u_int32_t fi_lastlength;	/* length of last block in array */
	int32_t	  fi_blocks[1];		/* array of logical block numbers */
};
/* sizeof FINFO except fi_blocks */
#define	FINFOSIZE	(sizeof(FINFO) - sizeof(int32_t))

/*
 * Index file inode entries.
 */
typedef struct ifile IFILE;
struct ifile {
	u_int32_t if_version;		/* inode version number */
#define	LFS_UNUSED_DADDR	0	/* out-of-band daddr */
	int32_t	  if_daddr;		/* inode disk address */
#define LFS_ORPHAN_NEXTFREE	(~(u_int32_t)0) /* indicate orphaned file */
	u_int32_t if_nextfree;		/* next-unallocated inode */
	u_int32_t if_atime_sec;		/* Last access time, seconds */
	u_int32_t if_atime_nsec;	/* and nanoseconds */
};

typedef struct ifile_v1 IFILE_V1;
struct ifile_v1 {
	u_int32_t if_version;		/* inode version number */
	int32_t	  if_daddr;		/* inode disk address */
	u_int32_t if_nextfree;		/* next-unallocated inode */
#if LFS_ATIME_IFILE
	struct timespec if_atime;	/* Last access time */
#endif
};

/*
 * Cleaner information structure.  This resides in the ifile and is used
 * to pass information from the kernel to the cleaner.
 */
typedef struct _cleanerinfo {
	u_int32_t clean;		/* number of clean segments */
	u_int32_t dirty;		/* number of dirty segments */
	/* XXX64 bfree and avail must -> 64 */
	int32_t   bfree;		/* disk blocks free */
	int32_t	  avail;		/* disk blocks available */
	u_int32_t free_head;		/* head of the inode free list */
	u_int32_t free_tail;		/* tail of the inode free list */
#define LFS_CLEANER_MUST_CLEAN	0x01
	u_int32_t flags;		/* status word from the kernel */
} CLEANERINFO;

/*
 * On-disk segment summary information
 */
typedef struct segsum_v1 SEGSUM_V1;
struct segsum_v1 {
	u_int32_t ss_sumsum;		/* 0: check sum of summary block */
	u_int32_t ss_datasum;		/* 4: check sum of data */
	u_int32_t ss_magic;		/* 8: segment summary magic number */
#define SS_MAGIC	0x061561
	int32_t	  ss_next;		/* 12: next segment */
	u_int32_t ss_create;		/* 16: creation time stamp */
	u_int16_t ss_nfinfo;		/* 20: number of file info structures */
	u_int16_t ss_ninos;		/* 22: number of inodes in summary */

#define	SS_DIROP	0x01		/* segment begins a dirop */
#define	SS_CONT		0x02		/* more partials to finish this write*/
#define	SS_CLEAN	0x04		/* written by the cleaner */
#define	SS_RFW		0x08		/* written by the roll-forward agent */
#define	SS_RECLAIM	0x10		/* written by the roll-forward agent */
	u_int16_t ss_flags;		/* 24: used for directory operations */
	u_int16_t ss_pad;		/* 26: extra space */
	/* FINFO's and inode daddr's... */
};

typedef struct segsum SEGSUM;
struct segsum {
	u_int32_t ss_sumsum;		/* 0: check sum of summary block */
	u_int32_t ss_datasum;		/* 4: check sum of data */
	u_int32_t ss_magic;		/* 8: segment summary magic number */
	int32_t	  ss_next;		/* 12: next segment (disk address) */
	u_int32_t ss_ident;		/* 16: roll-forward fsid */
#define ss_ocreate ss_ident /* ident is where create was in v1 */
	u_int16_t ss_nfinfo;		/* 20: number of file info structures */
	u_int16_t ss_ninos;		/* 22: number of inodes in summary */
	u_int16_t ss_flags;		/* 24: used for directory operations */
	u_int8_t  ss_pad[2];		/* 26: extra space */
	u_int32_t ss_reclino;           /* 28: inode being reclaimed */
	u_int64_t ss_serial;		/* 32: serial number */
	u_int64_t ss_create;		/* 40: time stamp */
	/* FINFO's and inode daddr's... */
};


/*
 * On-disk super block.
 *
 * We have separate superblock structures for the 32-bit and 64-bit
 * LFS, and accessor functions to hide the differences.
 *
 * For lfs64, the format version is always 2; version 1 lfs is old.
 * For both, the inode format version is 0; for lfs32 this selects the
 * same 32-bit inode as always, and for lfs64 this selects the larger
 * 64-bit inode structure we got from ffsv2.
 *
 * In lfs64:
 *   - inode numbers are still 32 bit
 *   - segments may not be larger than 4G (counted in bytes)
 *   - there may not be more than 2^32 (or perhaps 2^31) segments
 *   - the total volume size is limited to 2^63 frags and/or 2^63
 *     disk blocks, and probably in practice 2^63 bytes.
 */

#define	       LFS_MAGIC       		0x070162
#define        LFS_MAGIC_SWAPPED	0x62010700

#define        LFS64_MAGIC     		0x19620701
#define        LFS64_MAGIC_SWAPPED      0x01076219

#define	       LFS_VERSION     		2

#define LFS_MIN_SBINTERVAL     5	/* min superblock segment spacing */
#define LFS_MAXNUMSB	       10	/* max number of superblocks */

/* flags for dlfs_pflags */
#define LFS_PF_CLEAN 0x1

/* Inode format versions */
#define LFS_44INODEFMT 0
#define LFS_MAXINODEFMT 0

struct dlfs {
	u_int32_t dlfs_magic;	  /* 0: magic number */
	u_int32_t dlfs_version;	  /* 4: version number */

	u_int32_t dlfs_size;	  /* 8: number of blocks in fs (v1) */
				  /*	number of frags in fs (v2) */
	u_int32_t dlfs_ssize;	  /* 12: number of blocks per segment (v1) */
				  /*	 number of bytes per segment (v2) */
	u_int32_t dlfs_dsize;	  /* 16: number of disk blocks in fs */
	u_int32_t dlfs_bsize;	  /* 20: file system block size */
	u_int32_t dlfs_fsize;	  /* 24: size of frag blocks in fs */
	u_int32_t dlfs_frag;	  /* 28: number of frags in a block in fs */

/* Checkpoint region. */
	u_int32_t dlfs_freehd;	  /* 32: start of the free inode list */
	int32_t   dlfs_bfree;	  /* 36: number of free frags */
	u_int32_t dlfs_nfiles;	  /* 40: number of allocated inodes */
	int32_t	  dlfs_avail;	  /* 44: blocks available for writing */
	int32_t	  dlfs_uinodes;	  /* 48: inodes in cache not yet on disk */
	int32_t	  dlfs_idaddr;	  /* 52: inode file disk address */
	u_int32_t dlfs_ifile;	  /* 56: inode file inode number */
	int32_t	  dlfs_lastseg;	  /* 60: address of last segment written */
	int32_t	  dlfs_nextseg;	  /* 64: address of next segment to write */
	int32_t	  dlfs_curseg;	  /* 68: current segment being written */
	int32_t	  dlfs_offset;	  /* 72: offset in curseg for next partial */
	int32_t	  dlfs_lastpseg;  /* 76: address of last partial written */
	u_int32_t dlfs_inopf;	  /* 80: v1: time stamp; v2: inodes per frag */

/* These are configuration parameters. */
	u_int32_t dlfs_minfree;	  /* 84: minimum percentage of free blocks */

/* These fields can be computed from the others. */
	u_int64_t dlfs_maxfilesize; /* 88: maximum representable file size */
	u_int32_t dlfs_fsbpseg;	    /* 96: frags (fsb) per segment */
	u_int32_t dlfs_inopb;	  /* 100: inodes per block */
	u_int32_t dlfs_ifpb;	  /* 104: IFILE entries per block */
	u_int32_t dlfs_sepb;	  /* 108: SEGUSE entries per block */
	u_int32_t dlfs_nindir;	  /* 112: indirect pointers per block */
	u_int32_t dlfs_nseg;	  /* 116: number of segments */
	u_int32_t dlfs_nspf;	  /* 120: number of sectors per fragment */
	u_int32_t dlfs_cleansz;	  /* 124: cleaner info size in blocks */
	u_int32_t dlfs_segtabsz;  /* 128: segment table size in blocks */
	u_int32_t dlfs_segmask;	  /* 132: calculate offset within a segment */
	u_int32_t dlfs_segshift;  /* 136: fast mult/div for segments */
	u_int32_t dlfs_bshift;	  /* 140: calc block number from file offset */
	u_int32_t dlfs_ffshift;	  /* 144: fast mult/div for frag from file */
	u_int32_t dlfs_fbshift;	  /* 148: fast mult/div for frag from block */
	u_int64_t dlfs_bmask;	  /* 152: calc block offset from file offset */
	u_int64_t dlfs_ffmask;	  /* 160: calc frag offset from file offset */
	u_int64_t dlfs_fbmask;	  /* 168: calc frag offset from block offset */
	u_int32_t dlfs_blktodb;	  /* 176: blktodb and dbtoblk shift constant */
	u_int32_t dlfs_sushift;	  /* 180: fast mult/div for segusage table */

	int32_t	  dlfs_maxsymlinklen; /* 184: max length of an internal symlink */
				  /* 188: superblock disk offsets */
	int32_t	  dlfs_sboffs[LFS_MAXNUMSB];

	u_int32_t dlfs_nclean;	  /* 228: Number of clean segments */
	u_char	  dlfs_fsmnt[MNAMELEN];	 /* 232: name mounted on */
	u_int16_t dlfs_pflags;	  /* 322: file system persistent flags */
	int32_t	  dlfs_dmeta;	  /* 324: total number of dirty summaries */
	u_int32_t dlfs_minfreeseg; /* 328: segments not counted in bfree */
	u_int32_t dlfs_sumsize;	  /* 332: size of summary blocks */
	u_int64_t dlfs_serial;	  /* 336: serial number */
	u_int32_t dlfs_ibsize;	  /* 344: size of inode blocks */
	int32_t	  dlfs_s0addr;	  /* 348: start of segment 0 */
	u_int64_t dlfs_tstamp;	  /* 352: time stamp */
	u_int32_t dlfs_inodefmt;  /* 360: inode format version */
	u_int32_t dlfs_interleave; /* 364: segment interleave */
	u_int32_t dlfs_ident;	  /* 368: per-fs identifier */
	u_int32_t dlfs_fsbtodb;	  /* 372: fsbtodb and dbtodsb shift constant */
	u_int32_t dlfs_resvseg;   /* 376: segments reserved for the cleaner */
	int8_t	  dlfs_pad[128];  /* 380: round to 512 bytes */
/* Checksum -- last valid disk field. */
	u_int32_t dlfs_cksum;	  /* 508: checksum for superblock checking */
};

struct dlfs64 {
	u_int32_t dlfs_magic;	  /* 0: magic number */
	u_int32_t dlfs_version;	  /* 4: version number (2) */

	u_int64_t dlfs_size;	  /* 8: number of frags in fs (v2) */
	u_int64_t dlfs_dsize;	  /* 16: number of disk blocks in fs */
	u_int32_t dlfs_ssize;	  /* 24: number of bytes per segment (v2) */
	u_int32_t dlfs_bsize;	  /* 28: file system block size */
	u_int32_t dlfs_fsize;	  /* 32: size of frag blocks in fs */
	u_int32_t dlfs_frag;	  /* 36: number of frags in a block in fs */

/* Checkpoint region. */
	u_int32_t dlfs_freehd;	  /* 40: start of the free inode list */
	u_int32_t dlfs_nfiles;	  /* 44: number of allocated inodes */
	int64_t   dlfs_bfree;	  /* 48: number of free frags */
	int64_t	  dlfs_avail;	  /* 56: blocks available for writing */
	int64_t	  dlfs_idaddr;	  /* 64: inode file disk address */
	int32_t	  dlfs_uinodes;	  /* 72: inodes in cache not yet on disk */
	u_int32_t dlfs_ifile;	  /* 76: inode file inode number */
	int64_t	  dlfs_lastseg;	  /* 80: address of last segment written */
	int64_t	  dlfs_nextseg;	  /* 88: address of next segment to write */
	int64_t	  dlfs_curseg;	  /* 96: current segment being written */
	int64_t	  dlfs_offset;	  /* 104: offset in curseg for next partial */
	int64_t	  dlfs_lastpseg;  /* 112: address of last partial written */
	u_int32_t dlfs_inopf;	  /* 120: inodes per frag */

/* These are configuration parameters. */
	u_int32_t dlfs_minfree;	  /* 124: minimum percentage of free blocks */

/* These fields can be computed from the others. */
	u_int64_t dlfs_maxfilesize; /* 128: maximum representable file size */
	u_int32_t dlfs_fsbpseg;	  /* 136: frags (fsb) per segment */
	u_int32_t dlfs_inopb;	  /* 140: inodes per block */
	u_int32_t dlfs_ifpb;	  /* 144: IFILE entries per block */
	u_int32_t dlfs_sepb;	  /* 148: SEGUSE entries per block */
	u_int32_t dlfs_nindir;	  /* 152: indirect pointers per block */
	u_int32_t dlfs_nseg;	  /* 156: number of segments */
	u_int32_t dlfs_nspf;	  /* 160: number of sectors per fragment */
	u_int32_t dlfs_cleansz;	  /* 164: cleaner info size in blocks */
	u_int32_t dlfs_segtabsz;  /* 168: segment table size in blocks */
	u_int32_t dlfs_bshift;	  /* 172: calc block number from file offset */
	u_int32_t dlfs_ffshift;	  /* 176: fast mult/div for frag from file */
	u_int32_t dlfs_fbshift;	  /* 180: fast mult/div for frag from block */
	u_int64_t dlfs_bmask;	  /* 184: calc block offset from file offset */
	u_int64_t dlfs_ffmask;	  /* 192: calc frag offset from file offset */
	u_int64_t dlfs_fbmask;	  /* 200: calc frag offset from block offset */
	u_int32_t dlfs_blktodb;	  /* 208: blktodb and dbtoblk shift constant */
	u_int32_t dlfs_sushift;	  /* 212: fast mult/div for segusage table */

				  /* 216: superblock disk offsets */
	int64_t	   dlfs_sboffs[LFS_MAXNUMSB];

	int32_t	  dlfs_maxsymlinklen; /* 296: max len of an internal symlink */
	u_int32_t dlfs_nclean;	  /* 300: Number of clean segments */
	u_char	  dlfs_fsmnt[MNAMELEN];	 /* 304: name mounted on */
	u_int16_t dlfs_pflags;	  /* 394: file system persistent flags */
	int32_t	  dlfs_dmeta;	  /* 396: total number of dirty summaries */
	u_int32_t dlfs_minfreeseg; /* 400: segments not counted in bfree */
	u_int32_t dlfs_sumsize;	  /* 404: size of summary blocks */
	u_int32_t dlfs_ibsize;	  /* 408: size of inode blocks */
	u_int32_t dlfs_inodefmt;  /* 412: inode format version */
	u_int64_t dlfs_serial;	  /* 416: serial number */
	int64_t	  dlfs_s0addr;	  /* 424: start of segment 0 */
	u_int64_t dlfs_tstamp;	  /* 432: time stamp */
	u_int32_t dlfs_interleave; /* 440: segment interleave */
	u_int32_t dlfs_ident;	  /* 444: per-fs identifier */
	u_int32_t dlfs_fsbtodb;	  /* 448: fsbtodb and dbtodsb shift constant */
	u_int32_t dlfs_resvseg;   /* 452: segments reserved for the cleaner */
	int8_t	  dlfs_pad[52];   /* 456: round to 512 bytes */
/* Checksum -- last valid disk field. */
	u_int32_t dlfs_cksum;	  /* 508: checksum for superblock checking */
};

/* Type used for the inode bitmap */
typedef u_int32_t lfs_bm_t;

/*
 * Linked list of segments whose byte count needs updating following a
 * file truncation.
 */
struct segdelta {
	long segnum;
	size_t num;
	LIST_ENTRY(segdelta) list;
};

/*
 * In-memory super block.
 */
struct lfs {
	union {				/* on-disk parameters */
		struct dlfs u_32;
		struct dlfs64 u_64;
	} lfs_dlfs_u;

/* These fields are set at mount time and are meaningless on disk. */
	unsigned lfs_is64 : 1;		/* are we lfs64 or lfs32? */

	struct segment *lfs_sp;		/* current segment being written */
	struct vnode *lfs_ivnode;	/* vnode for the ifile */
	u_int32_t  lfs_seglock;		/* single-thread the segment writer */
	pid_t	  lfs_lockpid;		/* pid of lock holder */
	lwpid_t	  lfs_locklwp;		/* lwp of lock holder */
	u_int32_t lfs_iocount;		/* number of ios pending */
	u_int32_t lfs_writer;		/* don't allow any dirops to start */
	u_int32_t lfs_dirops;		/* count of active directory ops */
	u_int32_t lfs_dirvcount;	/* count of VDIROP nodes in this fs */
	u_int32_t lfs_doifile;		/* Write ifile blocks on next write */
	u_int32_t lfs_nactive;		/* Number of segments since last ckp */
	int8_t	  lfs_fmod;		/* super block modified flag */
	int8_t	  lfs_ronly;		/* mounted read-only flag */
#define LFS_NOTYET  0x01
#define LFS_IFDIRTY 0x02
#define LFS_WARNED  0x04
#define LFS_UNDIROP 0x08
	int8_t	  lfs_flags;		/* currently unused flag */
	u_int16_t lfs_activesb;		/* toggle between superblocks */
	daddr_t	  lfs_sbactive;		/* disk address of current sb write */
	struct vnode *lfs_flushvp;	/* vnode being flushed */
	int lfs_flushvp_fakevref;	/* fake vref count for flushvp */
	struct vnode *lfs_unlockvp;	/* being inactivated in lfs_segunlock */
	u_int32_t lfs_diropwait;	/* # procs waiting on dirop flush */
	size_t lfs_devbsize;		/* Device block size */
	size_t lfs_devbshift;		/* Device block shift */
	krwlock_t lfs_fraglock;
	krwlock_t lfs_iflock;		/* Ifile lock */
	kcondvar_t lfs_stopcv;		/* Wrap lock */
	struct lwp *lfs_stoplwp;
	pid_t lfs_rfpid;		/* Process ID of roll-forward agent */
	int	  lfs_nadirop;		/* number of active dirop nodes */
	long	  lfs_ravail;		/* blocks pre-reserved for writing */
	long	  lfs_favail;		/* blocks pre-reserved for writing */
	struct lfs_res_blk *lfs_resblk;	/* Reserved memory for pageout */
	TAILQ_HEAD(, inode) lfs_dchainhd; /* dirop vnodes */
	TAILQ_HEAD(, inode) lfs_pchainhd; /* paging vnodes */
#define LFS_RESHASH_WIDTH 17
	LIST_HEAD(, lfs_res_blk) lfs_reshash[LFS_RESHASH_WIDTH];
	int	  lfs_pdflush;		 /* pagedaemon wants us to flush */
	u_int32_t **lfs_suflags;	/* Segment use flags */
#ifdef _KERNEL
	struct pool lfs_clpool;		/* Pool for struct lfs_cluster */
	struct pool lfs_bpppool;	/* Pool for bpp */
	struct pool lfs_segpool;	/* Pool for struct segment */
#endif /* _KERNEL */
#define LFS_MAX_CLEANIND 64
	daddr_t  lfs_cleanint[LFS_MAX_CLEANIND]; /* Active cleaning intervals */
	int 	 lfs_cleanind;		/* Index into intervals */
	int lfs_sleepers;		/* # procs sleeping this fs */
	int lfs_pages;			/* dirty pages blaming this fs */
	lfs_bm_t *lfs_ino_bitmap;	/* Inuse inodes bitmap */
	int lfs_nowrap;			/* Suspend log wrap */
	int lfs_wrappass;		/* Allow first log wrap requester to pass */
	int lfs_wrapstatus;		/* Wrap status */
	int lfs_reclino;		/* Inode being reclaimed */
	daddr_t lfs_startseg;           /* Segment we started writing at */
	LIST_HEAD(, segdelta) lfs_segdhd;	/* List of pending trunc accounting events */

#ifdef _KERNEL
	/* ULFS-level information */
	u_int32_t um_flags;			/* ULFS flags (below) */
	u_long	um_nindir;			/* indirect ptrs per block */
	u_long	um_lognindir;			/* log2 of um_nindir */
	u_long	um_bptrtodb;			/* indir ptr to disk block */
	u_long	um_seqinc;			/* inc between seq blocks */
	int um_maxsymlinklen;
	int um_dirblksiz;
	u_int64_t um_maxfilesize;

	/* Stuff used by quota2 code, not currently operable */
	unsigned lfs_use_quota2 : 1;
	uint32_t lfs_quota_magic;
	uint8_t lfs_quota_flags;
	uint64_t lfs_quotaino[2];

	/* Sleep address replacing &lfs_avail inside the on-disk superblock */
	/* XXX: should be replaced with a condvar */
	int lfs_availsleep;
	/* This one replaces &lfs_nextseg... all ditto */
	int lfs_nextsegsleep;
#endif
};

/*
 * Structures used by lfs_bmapv and lfs_markv to communicate information
 * about inodes and data blocks.
 */
typedef struct block_info {
	u_int32_t bi_inode;		/* inode # */
	int32_t	bi_lbn;			/* logical block w/in file */
	int32_t	bi_daddr;		/* disk address of block */
	u_int64_t bi_segcreate;		/* origin segment create time */
	int	bi_version;		/* file version number */
	void	*bi_bp;			/* data buffer */
	int	bi_size;		/* size of the block (if fragment) */
} BLOCK_INFO;

/* Compatibility for 1.5 binaries */
typedef struct block_info_15 {
	u_int32_t bi_inode;		/* inode # */
	int32_t	bi_lbn;			/* logical block w/in file */
	int32_t	bi_daddr;		/* disk address of block */
	u_int32_t bi_segcreate;		/* origin segment create time */
	int	bi_version;		/* file version number */
	void	*bi_bp;			/* data buffer */
	int	bi_size;		/* size of the block (if fragment) */
} BLOCK_INFO_15;

/* In-memory description of a segment about to be written. */
struct segment {
	struct lfs	 *fs;		/* file system pointer */
	struct buf	**bpp;		/* pointer to buffer array */
	struct buf	**cbpp;		/* pointer to next available bp */
	struct buf	**start_bpp;	/* pointer to first bp in this set */
	struct buf	 *ibp;		/* buffer pointer to inode page */
	struct ulfs1_dinode    *idp;          /* pointer to ifile dinode */
	struct finfo	 *fip;		/* current fileinfo pointer */
	struct vnode	 *vp;		/* vnode being gathered */
	void	 *segsum;		/* segment summary info */
	u_int32_t ninodes;		/* number of inodes in this segment */
	int32_t seg_bytes_left;		/* bytes left in segment */
	int32_t sum_bytes_left;		/* bytes left in summary block */
	u_int32_t seg_number;		/* number of this segment */
	int32_t *start_lbp;		/* beginning lbn for this set */

#define SEGM_CKP	0x0001		/* doing a checkpoint */
#define SEGM_CLEAN	0x0002		/* cleaner call; don't sort */
#define SEGM_SYNC	0x0004		/* wait for segment */
#define SEGM_PROT	0x0008		/* don't inactivate at segunlock */
#define SEGM_PAGEDAEMON	0x0010		/* pagedaemon called us */
#define SEGM_WRITERD	0x0020		/* LFS writed called us */
#define SEGM_FORCE_CKP	0x0040		/* Force checkpoint right away */
#define SEGM_RECLAIM	0x0080		/* Writing to reclaim vnode */
#define SEGM_SINGLE	0x0100		/* Opportunistic writevnodes */
	u_int16_t seg_flags;		/* run-time flags for this segment */
	u_int32_t seg_iocount;		/* number of ios pending */
	int	  ndupino;		/* number of duplicate inodes */
};

/* Statistics Counters */
struct lfs_stats {	/* Must match sysctl list in lfs_vfsops.h ! */
	u_int	segsused;
	u_int	psegwrites;
	u_int	psyncwrites;
	u_int	pcleanwrites;
	u_int	blocktot;
	u_int	cleanblocks;
	u_int	ncheckpoints;
	u_int	nwrites;
	u_int	nsync_writes;
	u_int	wait_exceeded;
	u_int	write_exceeded;
	u_int	flush_invoked;
	u_int	vflush_invoked;
	u_int	clean_inlocked;
	u_int	clean_vnlocked;
	u_int   segs_reclaimed;
};

/* Fcntls to take the place of the lfs syscalls */
struct lfs_fcntl_markv {
	BLOCK_INFO *blkiov;	/* blocks to relocate */
	int blkcnt;		/* number of blocks */
};

#define LFCNSEGWAITALL	_FCNR_FSPRIV('L', 14, struct timeval)
#define LFCNSEGWAIT	_FCNR_FSPRIV('L', 15, struct timeval)
#define LFCNBMAPV	_FCNRW_FSPRIV('L', 2, struct lfs_fcntl_markv)
#define LFCNMARKV	_FCNRW_FSPRIV('L', 3, struct lfs_fcntl_markv)
#define LFCNRECLAIM	 _FCNO_FSPRIV('L', 4)

struct lfs_fhandle {
	char space[28];	/* FHANDLE_SIZE_COMPAT (but used from userland too) */
};
#define LFCNREWIND       _FCNR_FSPRIV('L', 6, int)
#define LFCNINVAL        _FCNR_FSPRIV('L', 7, int)
#define LFCNRESIZE       _FCNR_FSPRIV('L', 8, int)
#define LFCNWRAPSTOP	 _FCNR_FSPRIV('L', 9, int)
#define LFCNWRAPGO	 _FCNR_FSPRIV('L', 10, int)
#define LFCNIFILEFH	 _FCNW_FSPRIV('L', 11, struct lfs_fhandle)
#define LFCNWRAPPASS	 _FCNR_FSPRIV('L', 12, int)
# define LFS_WRAP_GOING   0x0
# define LFS_WRAP_WAITING 0x1
#define LFCNWRAPSTATUS	 _FCNW_FSPRIV('L', 13, int)

/* Debug segment lock */
#ifdef notyet
# define ASSERT_SEGLOCK(fs) KASSERT(LFS_SEGLOCK_HELD(fs))
# define ASSERT_NO_SEGLOCK(fs) KASSERT(!LFS_SEGLOCK_HELD(fs))
# define ASSERT_DUNNO_SEGLOCK(fs)
# define ASSERT_MAYBE_SEGLOCK(fs)
#else /* !notyet */
# define ASSERT_DUNNO_SEGLOCK(fs) \
	DLOG((DLOG_SEG, "lfs func %s seglock wrong (%d)\n", __func__, \
		LFS_SEGLOCK_HELD(fs)))
# define ASSERT_SEGLOCK(fs) do {					\
	if (!LFS_SEGLOCK_HELD(fs)) {					\
		DLOG((DLOG_SEG, "lfs func %s seglock wrong (0)\n", __func__)); \
	}								\
} while(0)
# define ASSERT_NO_SEGLOCK(fs) do {					\
	if (LFS_SEGLOCK_HELD(fs)) {					\
		DLOG((DLOG_SEG, "lfs func %s seglock wrong (1)\n", __func__)); \
	}								\
} while(0)
# define ASSERT_MAYBE_SEGLOCK(x)
#endif /* !notyet */

/*
 * Arguments to mount LFS filesystems
 */
struct ulfs_args {
	char	*fspec;			/* block special device to mount */
};

__BEGIN_DECLS
void lfs_itimes(struct inode *, const struct timespec *,
    const struct timespec *, const struct timespec *);
__END_DECLS

#endif /* !_UFS_LFS_LFS_H_ */
