/*	$NetBSD: config.h,v 1.6 2001/07/13 20:30:19 perseant Exp $	*/

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
 *	@(#)config.h	8.3 (Berkeley) 5/24/95
 */

/*
 * Version of the LFS to make.  Default to the newest one.
 */
#define DFL_VERSION LFS_VERSION

/*
 * The following two constants set the default block and fragment sizes.
 * Both constants must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DESBLKSIZE <= MAXBSIZE
 *	sectorsize <= DESFRAGSIZE <= DESBLKSIZE
 *	DESBLKSIZE / DESFRAGSIZE <= 8
 */
#define	DFL_FRAGSIZE	512
#define	DFL_BLKSIZE	8192

/*
 * 1/DFL_MIN_FREE_SEGS gives the fraction of segments to be reserved for
 * the cleaner.
 */
#define DFL_MIN_FREE_SEGS 20

/*
 * MINFREE gives the minimum acceptable percentage of file system
 * blocks which may be free. If the freelist drops below this level
 * only the superuser may continue to allocate blocks.
 */
#define MINFREE		10

/*
 * The following constants set the default block and segment size for a log
 * structured file system.  The block size must be a power of two and less
 * than the segment size.
 */
#define	LFS_MINSEGSIZE		(64 * 1024)
#define	DFL_LFSSEG		(1024 * 1024)
#define	DFL_LFSSEG_SHIFT	20
#define	DFL_LFSSEG_MASK		0xFFFFF

#define	LFS_MINBLOCKSIZE	512
#define	DFL_LFSBLOCK		8192
#define	DFL_LFSBLOCK_SHIFT	13
#define	DFL_LFSBLOCK_MASK	0x1FFF

#define DFL_LFSFRAG		1024
#define DFL_LFS_FFMASK		0x3FF
#define DFL_LFS_FFSHIFT		10
#define DFL_LFS_FBMASK		0x7
#define DFL_LFS_FBSHIFT		3
