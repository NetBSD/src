/*	$NetBSD: extern.h,v 1.11 2003/09/11 12:19:44 dsl Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/* prototypes */
void mkfs(struct partition *, const char *, int, int, mode_t, uid_t, gid_t);

/* * variables set up by front end. */
extern int	mfs;		/* run as the memory based filesystem */
extern int	Nflag;		/* run mkfs without writing file system */
extern int	Oflag;		/* format as an 4.3BSD file system */
extern int64_t	fssize;		/* file system size */
extern int	sectorsize;	/* bytes/sector */
extern int	rpm;		/* revolutions/minute of drive */
extern int	fsize;		/* fragment size */
extern int	bsize;		/* block size */
extern int	maxbsize;
extern int	cpg;		/* cylinders/cylinder group */
extern int	cpgflg;		/* cylinders/cylinder group flag was given */
extern int	minfree;	/* free space threshold */
extern int	opt;		/* optimization preference (space or time) */
extern int	density;	/* number of bytes per inode */
extern int	num_inodes;	/* number of inodes (overrides density) */
extern int	maxcontig;	/* max contiguous blocks to allocate */
extern int	maxbpg;		/* maximum blocks per file in a cyl group */
extern int	maxblkspercg;
extern int	nrpos;		/* # of distinguished rotational positions */
extern int	avgfilesize;	/* expected average file size */
extern int	avgfpdir;	/* expected number of files per directory */
extern u_long	memleft;	/* virtual memory available */
extern caddr_t	membase;	/* start address of memory based filesystem */
extern int	needswap;	/* Filesystem not in native byte order */
extern int	isappleufs; /* Filesystem is Apple UFS */
extern char	*appleufs_volname;	/* Apple UFS volume name */
