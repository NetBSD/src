/* $NetBSD: vars.c,v 1.10 2005/06/27 02:48:28 christos Exp $	 */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>		/* XXX */
#include <ufs/lfs/lfs.h>
#include "fsck.h"

/* variables previously of file scope (from fsck.h) */
struct dups *duplist;		/* head of dup list */
struct dups *muldup;		/* end of unique duplicate dup block numbers */

struct zlncnt *zlnhead;		/* head of zero link count list */

struct lfs *fs;

daddr_t idaddr;			/* inode block containing ifile inode */
long numdirs, listmax, inplast;

long dev_bsize;			/* computed value of DEV_BSIZE */
long secsize;			/* actual disk sector size */
char nflag;			/* assume a no response */
char yflag;			/* assume a yes response */
int bflag;			/* location of alternate super block */
int debug;			/* output debugging info */
int exitonfail;
int preen;			/* just fix normal inconsistencies */
int quiet;			/* don't report clean filesystems */
char havesb;			/* superblock has been read */
char skipclean;			/* skip clean file systems if preening */
int fsmodified;			/* 1 => write done to file system */
int fsreadfd;			/* file descriptor for reading file system */
int rerun;			/* rerun fsck.  Only used in non-preen mode */

daddr_t maxfsblock;		/* number of blocks in the file system */
#ifndef VERBOSE_BLOCKMAP
char *blockmap;			/* ptr to primary blk allocation map */
#else
ino_t *blockmap;
#endif
ino_t maxino;			/* number of inodes in file system */
ino_t lastino;			/* last inode in use */
char *statemap;			/* ptr to inode state table */
char *typemap;			/* ptr to inode type table */
int16_t *lncntp;		/* ptr to link count table */

ino_t lfdir;			/* lost & found directory inode number */
int lfmode;			/* lost & found directory creation mode */

daddr_t n_blks;			/* number of blocks in use */
ino_t n_files;			/* number of files in use */

struct ufs1_dinode zino;
