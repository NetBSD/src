/*	$NetBSD: ulfs_dinode.h,v 1.6 2013/06/08 02:11:11 dholland Exp $	*/
/*  from NetBSD: dinode.h,v 1.22 2013/01/22 09:39:18 dholland Exp  */

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
 * NOTE: COORDINATE ON-DISK FORMAT CHANGES WITH THE FREEBSD PROJECT.
 */

#ifndef	_UFS_LFS_ULFS_DINODE_H_
#define	_UFS_LFS_ULFS_DINODE_H_

#include <ufs/lfs/lfs.h>

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
 * in the directory entry which has been tagged as a DT_W entry.
 * See the comments about ULFS_ROOTINO above.
 */
#define	ULFS_WINO	((ino_t)1)

/*
 * Maximum length of a symlink that can be stored within the inode.
 */
#define ULFS1_MAXSYMLINKLEN	((ULFS_NDADDR + ULFS_NIADDR) * sizeof(int32_t))
#define ULFS2_MAXSYMLINKLEN	((ULFS_NDADDR + ULFS_NIADDR) * sizeof(int64_t))

#define ULFS_MAXSYMLINKLEN(ip) \
	((ip)->i_ump->um_fstype == ULFS1) ? \
	ULFS1_MAXSYMLINKLEN : ULFS2_MAXSYMLINKLEN

/* File permissions. */
#define	IEXEC		0000100		/* Executable. */
#define	IWRITE		0000200		/* Writable. */
#define	IREAD		0000400		/* Readable. */
#define	ISVTX		0001000		/* Sticky bit. */
#define	ISGID		0002000		/* Set-gid. */
#define	ISUID		0004000		/* Set-uid. */

/* File types. */
#define	LFS_IFMT	0170000		/* Mask of file type. */
#define	LFS_IFIFO	0010000		/* Named pipe (fifo). */
#define	LFS_IFCHR	0020000		/* Character device. */
#define	LFS_IFDIR	0040000		/* Directory file. */
#define	LFS_IFBLK	0060000		/* Block device. */
#define	LFS_IFREG	0100000		/* Regular file. */
#define	LFS_IFLNK	0120000		/* Symbolic link. */
#define	LFS_IFSOCK	0140000		/* UNIX domain socket. */
#define	LFS_IFWHT	0160000		/* Whiteout. */

#endif /* !_UFS_LFS_ULFS_DINODE_H_ */
