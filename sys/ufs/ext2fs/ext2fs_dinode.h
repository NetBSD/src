/*	$NetBSD: ext2fs_dinode.h,v 1.22.12.2 2013/01/23 00:06:31 yamt Exp $	*/

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
 *	@(#)dinode.h	8.6 (Berkeley) 9/13/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

/*
 * Copyright (c) 1997 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)dinode.h	8.6 (Berkeley) 9/13/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

#ifndef _UFS_EXT2FS_EXT2FS_DINODE_H_
#define _UFS_EXT2FS_EXT2FS_DINODE_H_

#include <sys/stat.h>

/*
 * The root inode is the root of the file system.  Inode 0 can't be used for
 * normal purposes and bad blocks are normally linked to inode 1, thus
 * the root inode is 2.
 * Inode 3 to 10 are reserved in ext2fs.
 */
#define	EXT2_BADBLKINO		((ino_t)1)
#define	EXT2_ROOTINO		((ino_t)2)
#define	EXT2_ACLIDXINO		((ino_t)3)
#define	EXT2_ACLDATAINO		((ino_t)4)
#define	EXT2_BOOTLOADERINO	((ino_t)5)
#define	EXT2_UNDELDIRINO	((ino_t)6)
#define	EXT2_RESIZEINO		((ino_t)7)
#define	EXT2_JOURNALINO		((ino_t)8)
#define	EXT2_FIRSTINO		((ino_t)11)

/*
 * A dinode contains all the meta-data associated with a UFS file.
 * This structure defines the on-disk format of a dinode. Since
 * this structure describes an on-disk structure, all its fields
 * are defined by types with precise widths.
 */

/*
 * XXX these are the same values as UFS_NDADDR/UFS_NIADDR and it is
 * far from clear that there isn't code that relies on them being the
 * same.
 */
#define	EXT2FS_NDADDR	12		/* Direct addresses in inode. */
#define	EXT2FS_NIADDR	3		/* Indirect addresses in inode. */

#define EXT2_MAXSYMLINKLEN ((EXT2FS_NDADDR+EXT2FS_NIADDR) * sizeof (uint32_t))

struct ext2fs_dinode {
	uint16_t	e2di_mode;	/*   0: IFMT, permissions; see below. */
	uint16_t	e2di_uid;	/*   2: Owner UID */
	uint32_t	e2di_size;	/*   4: Size (in bytes) */
	uint32_t	e2di_atime;	/*   8: Access time */
	uint32_t	e2di_ctime;	/*  12: Create time */
	uint32_t	e2di_mtime;	/*  16: Modification time */
	uint32_t	e2di_dtime;	/*  20: Deletion time */
	uint16_t	e2di_gid;	/*  24: Owner GID */
	uint16_t	e2di_nlink;	/*  26: File link count */
	uint32_t	e2di_nblock;	/*  28: Blocks count */
	uint32_t	e2di_flags;	/*  32: Status flags (chflags) */
	uint32_t	e2di_version;	/*  36: was reserved1 */
	uint32_t	e2di_blocks[EXT2FS_NDADDR+EXT2FS_NIADDR];
					/* 40: disk blocks */
	uint32_t	e2di_gen;	/* 100: generation number */
	uint32_t	e2di_facl;	/* 104: file ACL (not implemented) */
	uint32_t	e2di_dacl;	/* 108: dir ACL (not implemented) */
	uint32_t	e2di_faddr;	/* 112: fragment address */
	uint16_t	e2di_nblock_high; /* 116: Blocks count bits 47:32 */
	uint16_t	e2di_facl_high; /* 118: file ACL bits 47:32 */
	uint16_t	e2di_uid_high;	/* 120: Owner UID top 16 bits */
	uint16_t	e2di_gid_high;	/* 122: Owner GID top 16 bits */
	uint32_t	e2di_linux_reserved3; /* 124 */
};


/* XXX how does this differ from EXT2_MAXSYMLINKLEN above? */
#define	E2MAXSYMLINKLEN	((EXT2FS_NDADDR + EXT2FS_NIADDR) * sizeof(uint32_t))

/* File permissions. */
#define	EXT2_IEXEC		0000100		/* Executable. */
#define	EXT2_IWRITE		0000200		/* Writable. */
#define	EXT2_IREAD		0000400		/* Readable. */
#define	EXT2_ISVTX		0001000		/* Sticky bit. */
#define	EXT2_ISGID		0002000		/* Set-gid. */
#define	EXT2_ISUID		0004000		/* Set-uid. */

/* File types. */
#define	EXT2_IFMT		0170000		/* Mask of file type. */
#define	EXT2_IFIFO		0010000		/* Named pipe (fifo). */
#define	EXT2_IFCHR		0020000		/* Character device. */
#define	EXT2_IFDIR		0040000		/* Directory file. */
#define	EXT2_IFBLK		0060000		/* Block device. */
#define	EXT2_IFREG		0100000		/* Regular file. */
#define	EXT2_IFLNK		0120000		/* Symbolic link. */
#define	EXT2_IFSOCK		0140000		/* UNIX domain socket. */

/* file flags */
#define EXT2_SECRM		0x00000001 /* Secure deletion */
#define EXT2_UNRM		0x00000002 /* Undelete */
#define EXT2_COMPR		0x00000004 /* Compress file */
#define EXT2_SYNC		0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE		0x00000010 /* Immutable file */
#define EXT2_APPEND		0x00000020 /* writes to file may only append */
#define EXT2_NODUMP		0x00000040 /* do not dump file */
#define EXT2_NOATIME		0x00000080 /* do not update atime */
#define EXT2_INDEX		0x00001000 /* hash-indexed directory */
#define EXT2_IMAGIC		0x00002000 /* AFS directory */
#define EXT2_JOURNAL_DATA	0x00004000 /* file data should be journaled */
#define EXT2_NOTAIL		0x00008000 /* file tail should not be merged */
#define EXT2_DIRSYNC		0x00010000 /* dirsync behaviour */
#define EXT2_TOPDIR		0x00020000 /* Top of directory hierarchies*/
#define EXT2_HUGE_FILE		0x00040000 /* Set to each huge file */
#define EXT2_EXTENTS		0x00080000 /* Inode uses extents */
#define EXT2_EOFBLOCKS		0x00400000 /* Blocks allocated beyond EOF */

/* Size of on-disk inode. */
#define EXT2_REV0_DINODE_SIZE	sizeof(struct ext2fs_dinode)
#define EXT2_DINODE_SIZE(fs)	((fs)->e2fs.e2fs_rev > E2FS_REV0 ?	\
				    (fs)->e2fs.e2fs_inode_size :	\
				    EXT2_REV0_DINODE_SIZE)

/*
 * The e2di_blocks fields may be overlaid with other information for
 * file types that do not have associated disk storage. Block
 * and character devices overlay the first data block with their
 * dev_t value. Short symbolic links place their path in the
 * di_db area.
 */

#define e2di_rdev		e2di_blocks[0]
#define e2di_shortlink		e2di_blocks

/* e2fs needs byte swapping on big-endian systems */
#if BYTE_ORDER == LITTLE_ENDIAN
#	define e2fs_iload(old, new)	\
		memcpy((new),(old),sizeof(struct ext2fs_dinode))
#	define e2fs_isave(old, new)	\
		memcpy((new),(old),sizeof(struct ext2fs_dinode))
#else
void e2fs_i_bswap(struct ext2fs_dinode *, struct ext2fs_dinode *);
#	define e2fs_iload(old, new) e2fs_i_bswap((old), (new))
#	define e2fs_isave(old, new) e2fs_i_bswap((old), (new))
#endif

#endif /* !_UFS_EXT2FS_EXT2FS_DINODE_H_ */
