/*	$NetBSD: bootblock.h,v 1.10 2002/06/02 10:44:30 isaki Exp $	*/

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
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
#define	_SYS_BOOTBLOCK_H

#ifdef _KERNEL
#include <sys/stdint.h>
#else
#include <stdint.h>
#endif

/* ------------------------------------------
 * shared --
 *	definintions shared by many platforms
 */

	/* Maximum # of blocks in bbi_block_table, each bbi_block_size long */
#define	SHARED_BBINFO_MAXBLOCKS	118	/* so sizeof(shared_bbinfo) == 512 */

struct shared_bbinfo {
	uint8_t bbi_magic[32];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[SHARED_BBINFO_MAXBLOCKS];
};


/* ------------------------------------------
 * alpha --
 *	Alpha (disk, but also tape) Boot Block.
 * 
 *	See Section (III) 3.6.1 of the Alpha Architecture Reference Manual.
 */

struct alpha_boot_block {
	uint64_t bb_data[63];		/* data (disklabel, also as below) */
	uint64_t bb_cksum;		/* checksum of the boot block,
					 * taken as uint64_t's
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
		uint64_t _cksum;					\
		int _i;							\
									\
		_cksum = 0;						\
		for (_i = 0;						\
		    _i < (sizeof _bb->bb_data / sizeof _bb->bb_data[0]); \
		    _i++)						\
			_cksum += _bb->bb_data[_i];			\
		*(cksum) = _cksum;					\
	} while (0)

/* ------------------------------------------
 * apple --
 *	Apple computers boot block related information
 */

/*
 *	Driver Descriptor Map, from Inside Macintosh: Devices, SCSI Manager
 *	pp 12-13.  The driver descriptor map always resides on physical block 0.
 */
struct apple_drvr_descriptor {
	uint32_t	descBlock;	/* first block of driver */
	uint16_t	descSize;	/* driver size in blocks */
	uint16_t	descType;	/* system type */
};

/*
 *	system types; Apple reserves 0-15
 */
#define	APPLE_DRVR_TYPE_MACINTOSH	1

#define	APPLE_DRVR_MAP_MAGIC		0x4552
#define	APPLE_DRVR_MAP_MAX_DESCRIPTORS	61

struct apple_drvr_map {
	uint16_t	sbSig;		/* map signature */
	uint16_t	sbBlockSize;	/* block size of device */
	uint32_t	sbBlkCount;	/* number of blocks on device */
	uint16_t	sbDevType;	/* (used internally by ROM) */
	uint16_t	sbDevID;	/* (used internally by ROM) */
	uint32_t	sbData;		/* (used internally by ROM) */
	uint16_t	sbDrvrCount;	/* number of driver descriptors */
	struct apple_drvr_descriptor sb_dd[APPLE_DRVR_MAP_MAX_DESCRIPTORS];
	uint16_t	pad[3];
} __attribute__ ((packed));

/*
 *	Partition map structure from Inside Macintosh: Devices, SCSI Manager
 *	pp. 13-14.  The partition map always begins on physical block 1.
 *
 *	With the exception of block 0, all blocks on the disk must belong to
 *	exactly one partition.  The partition map itself belongs to a partition
 *	of type `APPLE_PARTITION_MAP', and is not limited in size by anything
 *	other than available disk space.  The partition map is not necessarily
 *	the first partition listed.
 */
#define	APPLE_PART_MAP_ENTRY_MAGIC	0x504d

struct apple_part_map_entry {
	uint16_t	pmSig;		/* partition signature */
	uint16_t	pmSigPad;	/* (reserved) */
	uint32_t	pmMapBlkCnt;	/* number of blocks in partition map */
	uint32_t	pmPyPartStart;	/* first physical block of partition */
	uint32_t	pmPartBlkCnt;	/* number of blocks in partition */
	uint8_t		pmPartName[32];	/* partition name */
	uint8_t		pmPartType[32];	/* partition type */
	uint32_t	pmLgDataStart;	/* first logical block of data area */
	uint32_t	pmDataCnt;	/* number of blocks in data area */
	uint32_t	pmPartStatus;	/* partition status information */
	uint32_t	pmLgBootStart;	/* first logical block of boot code */
	uint32_t	pmBootSize;	/* size of boot code, in bytes */
	uint32_t	pmBootLoad;	/* boot code load address */
	uint32_t	pmBootLoad2;	/* (reserved) */
	uint32_t	pmBootEntry;	/* boot code entry point */
	uint32_t	pmBootEntry2;	/* (reserved) */
	uint32_t	pmBootCksum;	/* boot code checksum */
	int8_t		pmProcessor[16]; /* processor type (e.g. "68020") */
	uint8_t		pmBootArgs[128]; /* A/UX boot arguments */
	uint8_t		pad[248];	/* pad to end of block */
};

#define	APPLE_PART_TYPE_DRIVER		"APPLE_DRIVER"
#define	APPLE_PART_TYPE_DRIVER43	"APPLE_DRIVER43"
#define	APPLE_PART_TYPE_DRIVERATA	"APPLE_DRIVER_ATA"
#define	APPLE_PART_TYPE_DRIVERIOKIT	"APPLE_DRIVER_IOKIT"
#define	APPLE_PART_TYPE_FWDRIVER	"APPLE_FWDRIVER"
#define	APPLE_PART_TYPE_FWB_COMPONENT	"FWB DRIVER COMPONENTS"
#define	APPLE_PART_TYPE_FREE		"APPLE_FREE"
#define	APPLE_PART_TYPE_MAC		"APPLE_HFS"
#define	APPLE_PART_TYPE_NETBSD		"NETBSD"
#define	APPLE_PART_TYPE_NBSD_PPCBOOT	"NETBSD/MACPPC"
#define	APPLE_PART_TYPE_NBSD_68KBOOT	"NETBSD/MAC68K"
#define	APPLE_PART_TYPE_PATCHES		"APPLE_PATCHES"
#define	APPLE_PART_TYPE_PARTMAP		"APPLE_PARTITION_MAP"
#define	APPLE_PART_TYPE_PATCHES		"APPLE_PATCHES"
#define	APPLE_PART_TYPE_SCRATCH		"APPLE_SCRATCH"
#define	APPLE_PART_TYPE_UNIX		"APPLE_UNIX_SVR2"


/* ------------------------------------------
 * macppc
 */

#define	MACPPC_BOOT_BLOCK_OFFSET	2048
#define	MACPPC_BOOT_BLOCK_BLOCKSIZE	512
#define	MACPPC_BOOT_BLOCK_MAX_SIZE	2048	/* XXX: could be up to 6144 */
	/* Magic string -- 32 bytes long (including the NUL) */
#define	MACPPC_BBINFO_MAGIC		"NetBSD/macppc bootxx   20020515"

/* ------------------------------------------
 * news68k, newsmips
 */

#define	NEWS_BOOT_BLOCK_LABELOFFSET	64 /* XXX from <machine/disklabel.h> */
#define	NEWS_BOOT_BLOCK_OFFSET		0
#define	NEWS_BOOT_BLOCK_BLOCKSIZE	512
#define	NEWS_BOOT_BLOCK_MAX_SIZE	(512 * 16)

	/* Magic string -- 32 bytes long (including the NUL) */
#define	NEWS68K_BBINFO_MAGIC		"NetBSD/news68k bootxx  20020518"
#define	NEWSMIPS_BBINFO_MAGIC		"NetBSD/newsmips bootxx 20020518"

/* ------------------------------------------
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
	uint8_t		pad[8];
	int32_t		magic;			/* PMAX_BOOT_MAGIC */
	int32_t		mode;			/* Mode for boot info. */
	uint32_t	load_addr;		/* Address to start loading. */
	uint32_t	exec_addr;		/* Address to start execing. */
	struct		pmax_boot_map map[61];	/* boot program section(s). */
} __attribute__((__packed__));

#define	PMAX_BOOT_MAGIC			0x0002757a
#define	PMAX_BOOTMODE_CONTIGUOUS	0
#define	PMAX_BOOTMODE_SCATTERED		1

#define	PMAX_BOOT_BLOCK_OFFSET		0
#define	PMAX_BOOT_BLOCK_BLOCKSIZE	512


/* ------------------------------------------
 * sparc
 */

#define	SPARC_BOOT_BLOCK_OFFSET		512
#define	SPARC_BOOT_BLOCK_BLOCKSIZE	512
#define	SPARC_BOOT_BLOCK_MAX_SIZE	(512 * 15)
	/* Magic string -- 32 bytes long (including the NUL) */
#define	SPARC_BBINFO_MAGIC		"NetBSD/sparc bootxx    20020515"


/* ------------------------------------------
 * sparc64
 */

#define	SPARC64_BOOT_BLOCK_OFFSET	512
#define	SPARC64_BOOT_BLOCK_BLOCKSIZE	512
#define	SPARC64_BOOT_BLOCK_MAX_SIZE	(512 * 15)


/* ------------------------------------------
 * sun68k (sun2, sun3)
 */

#define	SUN68K_BOOT_BLOCK_OFFSET	512
#define	SUN68K_BOOT_BLOCK_BLOCKSIZE	512
#define	SUN68K_BOOT_BLOCK_MAX_SIZE	(512 * 15)
	/* Magic string -- 32 bytes long (including the NUL) */
#define	SUN68K_BBINFO_MAGIC		"NetBSD/sun68k bootxx   20020515"


/* ------------------------------------------
 * vax --
 *	VAX boot block information
 */

struct vax_boot_block {
/* Note that these don't overlap any of the pmax boot block */
	uint8_t		pad0[2];
	uint8_t		bb_id_offset;	/* offset in words to id (magic1)*/
	uint8_t		bb_mbone;	/* must be one */
	uint16_t	bb_lbn_hi;	/* lbn (hi word) of bootstrap */
	uint16_t	bb_lbn_low;	/* lbn (low word) of bootstrap */
	uint8_t		pad1[332];

	/* The rest of these fields are identification area and describe
	 * the secondary block for uVAX VMB.
	 */
	uint8_t		bb_magic1;	/* magic number */
	uint8_t		bb_mbz1;	/* must be zero */
	uint8_t		bb_pad1;	/* any value */
	uint8_t		bb_sum1;	/* ~(magic1 + mbz1 + pad1) */

	uint8_t		bb_mbz2;	/* must be zero */
	uint8_t		bb_volinfo;	/* volinfo */
	uint8_t		bb_pad2a;	/* any value */
	uint8_t		bb_pad2b;	/* any value */

	uint32_t	bb_size;	/* size in blocks of bootstrap */
	uint32_t	bb_load;	/* load offset to bootstrap */
	uint32_t	bb_entry;	/* byte offset in bootstrap */
	uint32_t	bb_sum3;	/* sum of previous 3 fields */

	/* The rest is unused.
	 */
	uint8_t		pad2[148];
} __attribute__((__packed__));

#define	VAX_BOOT_MAGIC1			0x18	/* size of BB info? */
#define	VAX_BOOT_VOLINFO_NONE		0x00	/* no special info */
#define	VAX_BOOT_VOLINFO_SS		0x01	/* single sided */
#define	VAX_BOOT_VOLINFO_DS		0x81	/* double sided */

#define	VAX_BOOT_SIZE			15	/* 15 blocks */
#define	VAX_BOOT_LOAD			0	/* no load offset */
#define	VAX_BOOT_ENTRY			0x200	/* one block in */

#define	VAX_BOOT_BLOCK_OFFSET		0
#define	VAX_BOOT_BLOCK_BLOCKSIZE	512


/* ------------------------------------------
 * x68k
 */

#define X68K_BOOT_BLOCK_OFFSET		0
#define X68K_BOOT_BLOCK_BLOCKSIZE	512
#define X68K_BOOT_BLOCK_MAX_SIZE	(512 * 16)
	/* Magic string -- 32 bytes long (including the NUL) */
#define X68K_BBINFO_MAGIC		"NetBSD/x68k bootxx     20020601"

#endif	/* !_SYS_BOOTBLOCK_H */
