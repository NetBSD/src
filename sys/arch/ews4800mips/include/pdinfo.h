/*	$NetBSD: pdinfo.h,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

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

#ifndef _EWS4800MIPS_PDINFO_H_
#define	_EWS4800MIPS_PDINFO_H_

/* Phisical Disk INFOrmation */

#define	PDINFO_SECTOR		8		/* sector */
#define	PDINFO_MAGIC		0xca5e600d
#define	PDINFO_VERSION		2

struct disk_geometory {
	uint32_t cylinders_per_drive;		/* 24 */
	uint32_t tracks_per_cylinder;		/* 28 */
	uint32_t sectors_per_track;		/* 32 */
	uint32_t bytes_per_sector;		/* 36 */
} __attribute__((__packed__));

/* UX internal use */
struct disk_ux {
	uint32_t errorlog_sector;		/* 44 */
	uint32_t errorlog_size_byte;		/* 48 */
	uint32_t mfg_sector;			/* 52 */
	uint32_t mfg_size_byte;			/* 56 */
	uint32_t defect_sector;			/* 60 */
	uint32_t defect_size_byte;		/* 64 */
	uint32_t n_relocation_area;		/* 68 */
	uint32_t relocation_area_sector;	/* 72 */
	uint32_t relocation_area_size_byte;	/* 76 */
	uint32_t next_relocation_area;		/* 80 */
	uint32_t diag_sector;			/* 84 */
	uint32_t diag_size;			/* 88 */
	uint32_t gap_size;			/* 92 */
} __attribute__((__packed__));

/* Sector image */
struct pdinfo_sector {
	uint32_t drive_id;			/* 0 */
	uint32_t magic;				/* 4 */
	uint32_t version;			/* 8 */
	int8_t device_serial_number[12];	/* 12 */
	struct disk_geometory geometory;	/* 24 */
	uint32_t logical_sector;		/* 40 */
	struct disk_ux ux;			/* 44 */
	uint32_t device_depend[104];		/* 96 */
} __attribute__((__packed__));			/* 512 byte */

#if defined(_KERNEL) || defined(_STANDALONE)
boolean_t pdinfo_sector(void *, struct pdinfo_sector *);
boolean_t pdinfo_sanity(const struct pdinfo_sector *);
boolean_t pdinfo_valid(const struct pdinfo_sector *);
#endif

#endif /* _EWS4800MIPS_PDINFO_H_ */
