/*	$NetBSD: disklabel.h,v 1.1 2000/08/12 22:58:13 wdk Exp $	*/

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

#if 0

#define	LABELSECTOR	0			/* sector containing label */
#define	LABELOFFSET	64			/* offset of label in sector */
#define	MAXPARTITIONS	8			/* number of partitions */
#define	RAW_PART	2			/* raw partition: xx?c */
#define NUMBOOT		2			/* bootxx + xxboot... */

/* Just a dummy */
struct cpu_disklabel {
	int	cd_dummy;			/* must have one element. */
};

#endif /* _MACHINE_DISKLABEL_H_ */


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

#define MAXPARTITIONS	8
#define RAW_PART	2

#define LABELSECTOR	0
#define LABELOFFSET	128

struct cpu_disklabel {
	int	cd_dummy;
};

struct mips_volheader {
	u_int32_t	vh_magic;
#define MIPS_VH_MAGIC	0xbe5a941
	int16_t		root;	/* Root partition number */
	int16_t		swap;	/* Swap partition number */
	char		bootfile[16]; /* Path of default file to boot */
	char		_devparms[48]; /* disk device parameters */
	struct {		/* Disk volume directory */
		char 		name[8];
		int32_t		block;
		int32_t		bytes;
	} voldir[15];
	struct {
		int32_t 	blocks;
		int32_t 	first;
		int32_t 	type;
	} partitions[16];
	int32_t		checksum;
	int32_t		_pad;
} __attribute__((__packed__));

#define MIPS_PTYPE_VOLHDR	0
#define MIPS_PTYPE_TRKREPL      1
#define MIPS_PTYPE_SECREPL      2
#define MIPS_PTYPE_RAW		3
#define MIPS_PTYPE_BSD          4
#define MIPS_PTYPE_BSD42        4
#define MIPS_PTYPE_SYSV         5
#define MIPS_PTYPE_VOLUME	6 /* Entire volume */

#endif /* _MACHINE_DISKLABEL_H_ */
