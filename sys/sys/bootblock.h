/*	$NetBSD: bootblock.h,v 1.4 2002/05/14 06:34:21 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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

/*
 * Copyright (c) 1994, 1999 Christopher G. Demetriou
 * All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _SYS_BOOTBLOCK_H
#define _SYS_BOOTBLOCK_H

#ifdef _KERNEL
#include <sys/stdint.h>
#else
#include <stdint.h>
#endif

/*
 * alpha --
 *	Alpha (disk, but also tape) Boot Block.
 * 
 *	See Section (III) 3.6.1 of the Alpha Architecture Reference Manual.
 */

struct alpha_boot_block {
	u_int64_t bb_data[63];		/* data (disklabel, also as below) */
	u_int64_t bb_cksum;		/* checksum of the boot block,
					 * taken as u_int64_t's
					 */
};
#define	bb_secsize	bb_data[60]	/* secondary size (blocks) */
#define	bb_secstart	bb_data[61]	/* secondary start (blocks) */
#define	bb_flags	bb_data[62]	/* unknown flags (set to zero) */

#define	ALPHA_BOOT_BLOCK_OFFSET		0	/* offset of boot block. */
#define	ALPHA_BOOT_BLOCK_BLOCKSIZE	512	/* block size for sector
						 * size/start, and for boot
						 * block itself.
						 */

#define	ALPHA_BOOT_BLOCK_CKSUM(bb,cksum)				\
	do {								\
		const struct alpha_boot_block *_bb = (bb);		\
		u_int64_t _cksum;					\
		int _i;							\
									\
		_cksum = 0;						\
		for (_i = 0;						\
		    _i < (sizeof _bb->bb_data / sizeof _bb->bb_data[0]); \
		    _i++)						\
			_cksum += _bb->bb_data[_i];			\
		*(cksum) = _cksum;					\
	} while (0)


/*
 * pmax --
 *	PMAX (DECstation / MIPS) boot block information
 */

/*
 * If mode is 0, there is just one sequence of blocks and one Dec_BootMap
 * is used.  If mode is 1, there are multiple sequences of blocks
 * and multiple Dec_BootMaps are used, the last with numBlocks = 0.
 */
struct pmax_boot_map {
	int32_t	num_blocks;		/* Number of blocks to read. */
	int32_t	start_block;		/* Starting block on disk. */
};

/*
 * This is the structure of a disk or tape boot block.  The boot_map
 * can either be a single boot count and start block (contiguous mode)
 * or a list of up to 61 (to fill a 512 byte sector) block count and
 * start block pairs.  Under NetBSD, contiguous mode is always used.
 */
struct pmax_boot_block {
	u_int8_t	pad[8];
	int32_t		magic;			/* PMAX_BOOT_MAGIC */
	int32_t		mode;			/* Mode for boot info. */
	u_int32_t	load_addr;		/* Address to start loading. */
	u_int32_t	exec_addr;		/* Address to start execing. */
	struct		pmax_boot_map map[61];	/* boot program section(s). */
} __attribute__((__packed__));

#define PMAX_BOOT_MAGIC			0x0002757a
#define PMAX_BOOTMODE_CONTIGUOUS	0
#define PMAX_BOOTMODE_SCATTERED		1

#define PMAX_BOOT_BLOCK_OFFSET		0
#define PMAX_BOOT_BLOCK_BLOCKSIZE	512


/*
 * sparc
 */

#define SPARC_BOOT_BLOCK_OFFSET		512
#define SPARC_BOOT_BLOCK_BLOCKSIZE	512
#define SPARC_BOOT_BLOCK_MAX_SIZE	(512 * 15)

	/* Maximum # of blocks in bbi_block_table, each bbi_block_size long */
#define	SPARC_BBINFO_MAXBLOCKS		256

	/* Magic string -- 32 bytes long (including the NUL) */
#define	SPARC_BBINFO_MAGIC		"NetBSD/sparc bootxx            "
#define	SPARC_BBINFO_MAGICSIZE		sizeof(SPARC_BBINFO_MAGIC)

struct sparc_bbinfo {
	uint8_t bbi_magic[SPARC_BBINFO_MAGICSIZE];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[SPARC_BBINFO_MAXBLOCKS];
};


/*
 * sparc64
 */

#define SPARC64_BOOT_BLOCK_OFFSET	512
#define SPARC64_BOOT_BLOCK_BLOCKSIZE	512
#define SPARC64_BOOT_BLOCK_MAX_SIZE	(512 * 15)


/*
 * sun68k (sun2, sun3)
 */

#define SUN68K_BOOT_BLOCK_OFFSET	512
#define SUN68K_BOOT_BLOCK_BLOCKSIZE	512
#define SUN68K_BOOT_BLOCK_MAX_SIZE	(512 * 15)

	/* Maximum # of blocks in bbi_block_table, each bbi_block_size long */
#define	SUN68K_BBINFO_MAXBLOCKS		64

	/* Magic string -- 32 bytes long (including the NUL) */
#define	SUN68K_BBINFO_MAGIC		"NetBSD/sun68k bootxx           "
#define	SUN68K_BBINFO_MAGICSIZE		sizeof(SUN68K_BBINFO_MAGIC)

struct sun68k_bbinfo {
	uint8_t bbi_magic[SUN68K_BBINFO_MAGICSIZE];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[SUN68K_BBINFO_MAXBLOCKS];
};


/*
 * vax --
 *	VAX boot block information
 */

struct vax_boot_block {
/* Note that these don't overlap any of the pmax boot block */
	u_int8_t	pad0[2];
	u_int8_t	bb_id_offset;	/* offset in words to id (magic1)*/
	u_int8_t	bb_mbone;	/* must be one */
	u_int16_t	bb_lbn_hi;	/* lbn (hi word) of bootstrap */
	u_int16_t	bb_lbn_low;	/* lbn (low word) of bootstrap */
	u_int8_t	pad1[332];

	/* The rest of these fields are identification area and describe
	 * the secondary block for uVAX VMB.
	 */
	u_int8_t	bb_magic1;	/* magic number */
	u_int8_t	bb_mbz1;	/* must be zero */
	u_int8_t	bb_pad1;	/* any value */
	u_int8_t	bb_sum1;	/* ~(magic1 + mbz1 + pad1) */

	u_int8_t	bb_mbz2;	/* must be zero */
	u_int8_t	bb_volinfo;	/* volinfo */
	u_int8_t	bb_pad2a;	/* any value */
	u_int8_t	bb_pad2b;	/* any value */

	u_int32_t	bb_size;	/* size in blocks of bootstrap */
	u_int32_t	bb_load;	/* load offset to bootstrap */
	u_int32_t	bb_entry;	/* byte offset in bootstrap */
	u_int32_t	bb_sum3;	/* sum of previous 3 fields */

	/* The rest is unused.
	 */
	u_int8_t	pad2[148];
} __attribute__((__packed__));

#define	VAX_BOOT_MAGIC1			0x18	/* size of BB info? */
#define	VAX_BOOT_VOLINFO_NONE		0x00	/* no special info */
#define	VAX_BOOT_VOLINFO_SS		0x01	/* single sided */
#define	VAX_BOOT_VOLINFO_DS		0x81	/* double sided */

#define	VAX_BOOT_SIZE			15	/* 15 blocks */
#define	VAX_BOOT_LOAD			0	/* no load offset */
#define	VAX_BOOT_ENTRY			0x200	/* one block in */

#define VAX_BOOT_BLOCK_OFFSET		0
#define VAX_BOOT_BLOCK_BLOCKSIZE	512

#endif	/* !_SYS_BOOTBLOCK_H */
