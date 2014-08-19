/*	$NetBSD: md.h,v 1.1.6.2 2014/08/20 00:05:16 tls Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/disklabel.h>
#include <sys/bootblock.h>

/* md.h -- Machine specific definitions for the mac68k */

/*
 * Apple Partition Map Types
 *    Reserved - Entry hidden by sysinst from user
 *    NetBSD   - Entry used for NetBSD
 *    MacOS    - Entry used for MacOS and mapped to NetBSD
 *    Other    - Entry use unknown, mapped for scratch. This may
 *               include partitions used by other systems (Linux).
 */
#define MAP_EOL      0
#define MAP_RESERVED 1
#define MAP_NETBSD   2
#define MAP_MACOS    3
#define MAP_OTHER    4

typedef struct {
	int type;               /* Entry type from above */
	const char *name;             /* Partition Type string */
} MAP_TYPE;

/*
 * Define Apple Partition Map types typically seen on 68k Macs
 *    This should match the definitions in include/machine/disklabel.h
 *    and must conform to the matching rules in arch/mac68k/mac68k/disksubr.c
 */
extern MAP_TYPE map_types[];

/*
 * Define NetBSD partition types
 */
#define ROOT_PART 1
#define UFS_PART 2
#define SWAP_PART 3
#define HFS_PART 4
#define SCRATCH_PART 5

int usefull;			/* on install, clobber entire disk */

typedef struct {
        int size;               /* number of blocks in map for I/O */
        int in_use_cnt;         /* number of block in current use in map */
        int usable_cnt;         /* number of partitions usable by NetBSD */
        int hfs_cnt;            /* number of MacOS partitions found */
        int root_cnt;           /* number of root partitions in map */
        int swap_cnt;           /* number of swap partitions in map */
        int usr_cnt;            /* number of usr partitions in map */
        int selected;           /* current partition selection in mblk */
	int mblk[MAXPARTITIONS];/* map block number of usable partition */
        struct apple_part_map_entry *blk;
} MAP;

/*
 * Define the default size for the Disk Partition Map if we have to
 *  write one to the disk.  It should be a power of two minus one for
 *  the Block0 block, i.e. 7, 15, 31, 63.
 */
#define NEW_MAP_SIZE 15

MAP map;

/* constants and defines */

/*
 * The Block Zero Block is stored in the Partition Map Entry in the
 *  pmPad (or pmBootArgs) area.  The current NetBSD definition of this
 *  area is incomplete, so we have our own local definition here.
 */
typedef struct {
    u_int32_t magic;	/* magic number that identifes Block Zero Block */
    u_int8_t cluster;	/* Autorecovery cluster grouping */
    u_int8_t type;	/* 1=>Std FS, 2=>Autorecovery FS, 3=>SWAP FS */
    u_int16_t inode;	/* bad block inode number */
    struct {
	unsigned int root : 1;	/* FS contains a Root FS */
	unsigned int usr  : 1;	/* FS contains a Usr FS */
	unsigned int crit : 1;	/* FS contains a "Critical"? FS */
	unsigned int used : 1;  /* FS in use */
	unsigned int      : 7;
	unsigned int slice : 5;	/* Slice number to assocate with plus one */
	unsigned int part  : 16; /* reserved, but we'll hide disk part here */
    } flags;
    u_int32_t tmade;	/* time FS was created */
    u_int32_t tmount;	/* time of last mount */
    u_int32_t tumount;	/* time of last umount */
    struct {
	u_int32_t size;		/* size of map in bytes */
	u_int32_t ents;		/* number of used entries */
	u_int32_t start;	/* start of cltblk map */
    } abm;		/* altblk map */
    u_int32_t filler[7];	/* reserved for abm expansion */
    u_int8_t mount_point[64];	/* mount point for the FS */
} EBZB;

/*
 * Also define Block 0 on the device.  We only use this when we have to
 *  initialize the entire disk and then we really only care about the
 *  Signature word, but someday someone might want to fill in other
 *  parts.
 */
typedef struct {
    unsigned short	sbSig;		/* unique value for SCSI block 0 */
    unsigned short 	sbBlkSize;	/* block size of device */
    unsigned long 	sbBlkCount;	/* number of blocks on device */
    unsigned short 	sbDevType;	/* device type */
    unsigned short 	sbDevId;	/* device id */
    unsigned long 	sbData;		/* not used */
    unsigned short 	sbDrvrCount;	/* driver descriptor count */
    unsigned long 	ddBlock;	/* 1st driver's starting block */
    unsigned short 	ddSize;		/* size of 1st driver (512-byte blks) */
    unsigned short 	ddType;		/* system type (1 for Mac+) */
    unsigned short 	ddPad[243];	/* ARRAY[0..242] OF INTEGER; not used */
} Block0;

/*
 * Default Disk Partition Map used for an uninitilized disk.
 *  Has minimal entry for an old Apple SCSI driver, a newer 43 SCSI
 *  driver and an IDE driver (for those Macs with IDE).
 */
extern struct apple_part_map_entry new_map[];

/* Megs required for a full X installation. */
#define XNEEDMB 50

/*
 * Machine-specific command to write a new label to a disk.
 * For example, i386 uses "/sbin/disklabel -w -r", just like i386
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 *
 * The mac68k port doesn't support real disklabels so we don't define the
 * command string. The Apple Disk Partition Map gets written in the
 * md_pre_disklabel() routine, which also forces the incore copy to be
 * updated. If native disklabels are supported or if disklabel() is
 * fixed to work for writing labels, this command should be defined
 * to a value that will force the writing of the label. In that case,
 * the code in md_pre_disklabel() which forces the incore update can be
 * removed, though its presence won't hurt.
 *
 * #define DISKLABEL_CMD
 */

/* Definition of files to retrieve from ftp. */
#define SET_KERNEL_1_NAME	"kern-GENERIC"
#define SET_KERNEL_2_NAME	"kern-GENERICSBC"

extern void	sortmerge(void);
extern void	disp_selected_part(int);
extern void	reset_part_flags(struct apple_part_map_entry *);
extern int	whichType(struct apple_part_map_entry *);
extern void	report_errors(void);
