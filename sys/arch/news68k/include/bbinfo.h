/*	$NetBSD: bbinfo.h,v 1.1 2002/04/27 10:19:57 tsutsui Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#define MAXBLOCKNUM	64	/* enough for a 512k boot program (bs 8K) */
#define BOOTSECTOR_OFFSET 512

/* Magic string -- 32 bytes long (including the NUL) */
#define BBINFO_MAGIC	"NetBSD/news68k bootxx          "
#define BBINFO_MAGICSIZE sizeof(BBINFO_MAGIC)

struct bbinfo {
	uint8_t bbi_magic[BBINFO_MAGICSIZE];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[MAXBLOCKNUM];
	uint32_t bbi_entry_point;
};
