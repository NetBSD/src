/*	$NetBSD: mbrlabel.h,v 1.1 1998/11/16 18:44:26 ws Exp $	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
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

/*
 * Mostly stolen from <sys/arch/i386/disklabel.h>.
 * This file should most likely be somewhere else in the include hierarchy.
 */

/* DOS partition table -- located in boot block */
#define	MBRSECTOR	0		/* MBR relative sector # */
#define	MBRPARTOFF	446
#define	NMBRPART	4

struct mbr_partition {
	unsigned char	mp_flag;	/* bootstrap flags */
	unsigned char	mp_shd;		/* starting head */
	unsigned char	mp_ssect;	/* starting sector */
	unsigned char	mp_scyl;	/* starting cylinder */
	unsigned char	mp_typ;		/* partition type (see below) */
	unsigned char	mp_ehd;		/* end head */
	unsigned char	mp_esect;	/* end sector */
	unsigned char	mp_ecyl;	/* end cylinder */
	unsigned long	mp_start;	/* absolute starting sector number */
	unsigned long	mp_size;	/* partition size in sectors */
};

/* Known MBR partition types: */
#define	MBR_NETBSD	0xa9		/* NetBSD partition type */
#define	MBR_386BSD	0xa5		/* 386BSD partition type */
#define	MBR_FAT12	0x01		/* 12-bit FAT */
#define	MBR_FAT16S	0x04		/* 16-bit FAT, less than 32M */
#define	MBR_EXT		0x05		/* extended partition */
#define	MBR_FAT16B	0x06		/* 16-bit FAT, more than 32M */
#define	MBR_FAT32	0x0b		/* 32-bit FAT */
#define	MBR_FAT32L	0x0c		/* 32-bit FAT, LBA-mapped */
#define	MBR_FAT16L	0x0e		/* 16-bit FAT, LBA-mapped */
#define	MBR_EXT_LBA	0x0f		/* extended partition, LBA-mapped */
#define	MBR_LNXEXT2	0x83		/* Linux native */
