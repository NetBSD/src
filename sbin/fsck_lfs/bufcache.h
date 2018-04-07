/*	$NetBSD: bufcache.h,v 1.13.12.1 2018/04/07 04:12:10 pgoyette Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	@(#)buf.h	8.9 (Berkeley) 3/30/95
 */

#define BUF_CACHE_SIZE 1000

/*
 * The buffer header describes an I/O operation.
 */
struct ubuf {
	LIST_ENTRY(ubuf) b_hash;	/* Hash chain. */
	LIST_ENTRY(ubuf) b_vnbufs;	/* Buffer's associated vnode. */
	TAILQ_ENTRY(ubuf) b_freelist;	/* Free list position if not active. */
	volatile long	b_flags;	/* B_* flags. */
	long	b_bcount;		/* Valid bytes in buffer. */
#undef b_data
	char	*b_data;		/* Data in buffer */
	daddr_t	b_lblkno;		/* Logical block number. */
	daddr_t	b_blkno;		/* Underlying physical block number */
	struct	uvnode *b_vp;		/* File vnode. */
	int	b_hashval;		/* Hash value */
	void	(*b_iodone)(void *);	/* unused */
};

#define b_bufsize b_bcount

/*
 * These flags are kept in b_flags.
 */
#define	B_AGE		0x00000001	/* Move to age queue when I/O done. */
#define	B_BUSY		0x00000010	/* I/O in progress. */
#define	B_DELWRI	0x00000080	/* Delay I/O until buffer reused. */
#define	B_DONE		0x00000200	/* I/O completed. */
#define	B_ERROR		0x00000800	/* I/O error occurred. */
#define	B_GATHERED	0x00001000	/* LFS: already in a segment. */
#define	B_INVAL		0x00002000	/* Does not contain valid info. */
#define	B_LOCKED	0x00004000	/* Locked in core (not reusable). */
#define	B_READ		0x00100000	/* Read buffer. */
#define	B_DONTFREE	0x00010000	/* b_data not managed by bufcache */

#define	b_cflags	b_flags

LIST_HEAD(bufhash_struct, ubuf);

#if !defined(NOCRED)
#define	NOCRED	((void *)-1)	/* dummy; not actually used */
#endif /* !defined(NOCRED) */

void bufinit(int);
void bufrehash(int);
void bufstats(void);
void buf_destroy(struct ubuf *);
void bremfree(struct ubuf *);
struct ubuf *incore(struct uvnode *, daddr_t);
struct ubuf *getblk(struct uvnode *, daddr_t, int);
void bwrite(struct ubuf *);
void brelse(struct ubuf *, int);
int bread(struct uvnode *, daddr_t, int, int, struct ubuf **);
void reassignbuf(struct ubuf *, struct uvnode *);
void dump_free_lists(void);
