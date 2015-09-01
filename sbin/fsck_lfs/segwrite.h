/* $NetBSD: segwrite.h,v 1.7 2015/09/01 06:15:02 dholland Exp $ */
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
/*
 * Determine if it's OK to start a partial in this segment, or if we need
 * to go on to a new segment.
 */
#define	LFS_PARTIAL_FITS(fs) \
	(lfs_sb_getfsbpseg(fs) - (lfs_sb_getoffset(fs) - lfs_sb_getcurseg(fs)) > \
	lfs_sb_getfrag(fs))

/* op values to lfs_writevnodes */
#define	VN_REG		0
#define	VN_DIROP	1

int lfs_segwrite(struct lfs *, int);
void lfs_writefile(struct lfs *, struct segment *, struct uvnode *);
int lfs_writeinode(struct lfs *, struct segment *, struct inode *);
int lfs_gatherblock(struct segment *, struct ubuf *);
int lfs_gather(struct lfs *, struct segment *, struct uvnode *,
	   int (*match) (struct lfs *, struct ubuf *));
void lfs_updatemeta(struct segment *);
int lfs_initseg(struct lfs *);
void lfs_newseg(struct lfs *);
int lfs_writeseg(struct lfs *, struct segment *);
void lfs_writesuper(struct lfs *, daddr_t);

int lfs_match_data(struct lfs *, struct ubuf *);
int lfs_match_indir(struct lfs *, struct ubuf *);
int lfs_match_dindir(struct lfs *, struct ubuf *);
int lfs_match_tindir(struct lfs *, struct ubuf *);

int ulfs_getlbns(struct lfs *, struct uvnode *, daddr_t, struct indir *, int *);
int ulfs_bmaparray(struct lfs *, struct uvnode *, daddr_t, daddr_t *, struct indir *, int *);

int lfs_seglock(struct lfs *, unsigned long);
void lfs_segunlock(struct lfs *);
int lfs_writevnodes(struct lfs *, struct segment *, int);

