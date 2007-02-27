/*	$NetBSD: vtoc.h,v 1.1.28.1 2007/02/27 16:50:21 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _EWS4800MIPS_VTOC_H_
#define	_EWS4800MIPS_VTOC_H_

/* Volume Table Of Contents */

#define	VTOC_MAXPARTITIONS	16
#define	VTOC_SECTOR		1	/* sector */
#define	VTOC_MINSIZE		16	/* sector */

#define	VTOC_MAGIC		0x600ddeee
#define	VTOC_VERSION		1

#define	VTOC_TAG_NONAME		0x00
#define	VTOC_TAG_BOOT		0x01
#define	VTOC_TAG_ROOT		0x02
#define	VTOC_TAG_SWAP		0x03
#define	VTOC_TAG_USR		0x04
#define	VTOC_TAG_RAWDISK	0x05
#define	VTOC_TAG_STAND		0x06	/* bfs */
#define	VTOC_TAG_VAR		0x07
#define	VTOC_TAG_HOME		0x08
#define	__VTOC_TAG_BSDFFS	0xff	/* ews4800mips port original define */

#define	VTOC_FLAG_UNMOUNT	0x01
#define	VTOC_FLAG_RDONLY	0x10

struct ux_partition {
	uint16_t tag;			/* 0 */
	uint16_t flags;			/* 2 */
	uint32_t start_sector;		/* 4 */
	int32_t nsectors;		/* 8 */
} __attribute__((__packed__));

/* Sector image */
struct vtoc_sector {
	uint32_t bootinfo[3];		/*  0 */
	uint32_t magic;			/* 12 */
	uint32_t version;		/* 16 */
	int8_t volume[8];		/* 20 */
	uint16_t sector_size_byte;	/* 28 */
	uint16_t npartitions;		/* 30 */
	uint32_t reserved[10];		/* 32 */
	struct ux_partition partition[VTOC_MAXPARTITIONS];	/* 72 */
	uint32_t timestamp[VTOC_MAXPARTITIONS];	/* 264 */
	int32_t padding[46];		/* 328 */
} __attribute__((__packed__));		/* 512 byte */

struct pdinfo_sector;

#if defined(_KERNEL) || defined(_STANDALONE)
bool vtoc_sector(void *, struct vtoc_sector *, int);
bool vtoc_valid(const struct vtoc_sector *);
bool vtoc_sanity(const struct vtoc_sector *);
const struct ux_partition *vtoc_find_bfs(const struct vtoc_sector *);
bool vtoc_write(struct vtoc_sector *, struct pdinfo_sector *);
#endif

#endif /* _EWS4800MIPS_VTOC_H_ */
