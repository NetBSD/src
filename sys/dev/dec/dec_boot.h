/*	$NetBSD: dec_boot.h,v 1.2 2000/06/16 23:33:47 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)dec_boot.h	8.1 (Berkeley) 6/10/93
 *
 * devDiskLabel.h --
 *
 *      This defines the disk label that Sun writes on the 0'th sector of
 *      the 0'th cylinder of its SMD disks.  The disk label contains some
 *      geometry information and also the division of the disk into a
 *      number of partitions.  Each partition is identified to the drive
 *      by a different unit number.
 *
 * from: Header: /sprite/src/kernel/dev/RCS/devDiskLabel.h,
 *	v 9.4 90/03/01 12:22:36 jhh Exp  SPRITE (Berkeley)
 */

#ifndef _DEV_DEC_DEC_BOOT_H_
#define _DEV_DEC_DEC_BOOT_H_

/*
 * Boot block information on the 0th sector.
 * The boot program is stored in sequences of contiguous blocks.
 *
 * NOTE: The standard disk label offset is 64 which is
 * after the boot information expected by the PROM boot loader.
 */

/* PMAX (DECstation / MIPS) boot block information
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

/* VAX boot block information
 *
 */
struct vax_boot_block {
/* Note that these don't overlap any of the pmax boot block
 */
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

#define	VAX_BOOT_SIZE			16	/* 16 blocks */
#define	VAX_BOOT_LOAD			0	/* no load offset */
#define	VAX_BOOT_ENTRY			0x200	/* one block in */

#define VAX_BOOT_BLOCK_OFFSET		0
#define VAX_BOOT_BLOCK_BLOCKSIZE	512

/* The following describes the ULTRIX partition tables.
 */
/*
 * DEC_NUM_DISK_PARTS is the number of partitions that are recorded in
 * the label information.  The size of the padding in the Dec_DiskLabel
 * type is dependent on this number...
 */
#define DEC_NUM_DISK_PARTS	8

/*
 * A disk is divided into partitions and this type specifies where a
 * partition starts and how many bytes it contains.
 */
typedef struct dec_disk_map {
	int32_t	num_blocks;	/* Number of 512 byte blocks in partition. */
	int32_t	start_block;	/* Start of partition in blocks. */
} dec_disk_map;

/*
 * Label information on the 31st (DEC_LABEL_SECTOR) sector.
 */
typedef struct dec_disklabel {
    u_int8_t	pad0[440];		/* DIFFERENT from sprite!!! */
    int32_t	magic;			/* DEC_LABEL_MAGIC */
    int32_t	is_partitioned;		/* 1 if disk is partitioned. */
    dec_disk_map map[DEC_NUM_DISK_PARTS]; /* Indicates disk partitions. */
} dec_disklabel;

#define DEC_LABEL_MAGIC		0x00032957
#define DEC_LABEL_SECTOR	31

#endif	/* !_DEV_DEC_DEC_BOOT_H_ */
