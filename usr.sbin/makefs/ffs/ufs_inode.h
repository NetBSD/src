/*	$NetBSD: ufs_inode.h,v 1.1 2002/01/07 16:56:28 lukem Exp $	*/
/* From:  NetBSD: inode.h,v 1.27 2001/12/18 10:57:23 fvdl Exp $ */

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
 *	@(#)inode.h	8.9 (Berkeley) 5/14/95
 */


struct inode {
	ino_t	  	i_number;	/* The identity of the inode. */
	struct fs	*i_fs;		/* File system */
	union {
		struct	dinode ffs_din;	/* 128 bytes of the on-disk dinode. */
	} i_din;
	int		i_fd;		/* File descriptor */
};

#define	i_ffs_atime		i_din.ffs_din.di_atime
#define	i_ffs_atimensec		i_din.ffs_din.di_atimensec
#define	i_ffs_blocks		i_din.ffs_din.di_blocks
#define	i_ffs_ctime		i_din.ffs_din.di_ctime
#define	i_ffs_ctimensec		i_din.ffs_din.di_ctimensec
#define	i_ffs_db		i_din.ffs_din.di_db
#define	i_ffs_flags		i_din.ffs_din.di_flags
#define	i_ffs_gen		i_din.ffs_din.di_gen
#define	i_ffs_gid		i_din.ffs_din.di_gid
#define	i_ffs_ib		i_din.ffs_din.di_ib
#define	i_ffs_mode		i_din.ffs_din.di_mode
#define	i_ffs_mtime		i_din.ffs_din.di_mtime
#define	i_ffs_mtimensec		i_din.ffs_din.di_mtimensec
#define	i_ffs_nlink		i_din.ffs_din.di_nlink
#define	i_ffs_rdev		i_din.ffs_din.di_rdev
#define	i_ffs_shortlink		i_din.ffs_din.di_shortlink
#define	i_ffs_size		i_din.ffs_din.di_size
#define	i_ffs_uid		i_din.ffs_din.di_uid
