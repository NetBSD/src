/*	$NetBSD: disklabel.h,v 1.5 2001/06/11 01:50:54 wiz Exp $	*/

/*
 * Copyright (c) 2000 Wayne Knowles.     All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

/*
 * MIPS partition conventions:
 *
 * Partition 0 - root
 * Partition 1 - swap
 * Partition 6 - usr
 * Partition 7 - volume body
 * Partition 8 - volume header
 * Partition 10 - whole disk
 */

#define MAXPARTITIONS	8	/* XXX - NetBSD Compatibility */
#define RAW_PART	2
#define LABELSECTOR	1
#define LABELOFFSET	0

#define MIPS_PARTITIONS	16	/* Number or partitions for Mips */
#define MIPS_NVOLDIR	15	/* Number of volume directory files */
#define MIPS_VDIRSZ	8	/* File name size in volume directory */
#define MIPS_BFSIZE	16	/* Boot filename size */
#define	MIPS_VHSECTOR	0	/* Sector number for MIPS volume header */

/*
 * Devices parameters that RISC/os uses for mapping logical block numbers
 * to physical device addresses.
 */
struct mips_devparams {
	u_int8_t	dp_skew;	/* spiral addressing skew */
	u_int8_t	dp_gap1;	/* words of 0 before header */
	u_int8_t	dp_gap2;	/* words of 0 between hdr and data */
	u_int8_t	dp_spare0;	/* spare space */
	u_int16_t	dp_cyls;	/* number of cylinders */
	u_int16_t	dp_shd0;	/* starting head vol 0 */
	u_int16_t	dp_trks0;	/* number of tracks vol 0 */
	u_int16_t	dp_shd1;	/* starting head vol 1 */
	u_int16_t	dp_trks1;	/* number of tracks vol 1 */
	u_int16_t	dp_secs;	/* number of sectors/track */
	u_int16_t	dp_secbytes;	/* length of sector in bytes */
	u_int16_t	dp_interleave;	/* sector interleave */
	int32_t		dp_flags;	/* controller characteristics */
	int32_t		dp_datarate;	/* bytes/sec for kernel stats */
	int32_t		dp_nretries;	/* max num retries on data error */
	int32_t		dp_spare1;	/* spare entries */
	int32_t		dp_spare2;
	int32_t		dp_spare3;
	int32_t		dp_spare4;
} __attribute__((__packed__));

/*
 * Volume directory is used as a shortcut to find bootstraps etc
 */
struct mips_voldir {			/* Disk volume directory */
	char 		vd_name[MIPS_VDIRSZ];
	int32_t		vd_lba;
	int32_t		vd_len;
};

struct mips_partitions {
	int32_t 	pt_size;	/* # of logical blocks in partition */
	int32_t 	pt_offset;	/* first logical block of partiton */
	int32_t 	pt_fstype;
};

/*
 * Mips RISC/os compatible volume header and partition table
 *
 * 1 sector (512 bytes) in size, and normally copied to the first sector
 * and cylinder of every track on a disk. 
 */
struct mips_volheader {
	u_int32_t	vh_magic;
#define MIPS_VHMAGIC	0xbe5a941
	int16_t		vh_root;		/* Root partition number */
	int16_t		vh_swap;		/* Swap partition number */
	char		bootfile[MIPS_BFSIZE];	/* default file to boot */
	struct mips_devparams	vh_dp;		/* disk device parameters */
	struct mips_voldir 	vh_voldir[MIPS_NVOLDIR];
	struct mips_partitions	vh_part[MIPS_PARTITIONS];
	int32_t		vh_cksum;
	int32_t		vh_pad;
} __attribute__((__packed__));

#define MIPS_FS_VOLHDR	0
#define MIPS_FS_TRKREPL 1
#define MIPS_FS_SECREPL	2
#define MIPS_FS_RAW	3
#define MIPS_FS_BSD     4
#define MIPS_FS_BSD42   4
#define MIPS_FS_SYSV    5
#define MIPS_FS_VOLUME	6 /* Entire volume */

/* Just a dummy */
struct cpu_disklabel {
	int	cd_dummy;			/* must have one element. */
};

#endif /* _MACHINE_DISKLABEL_H_ */
