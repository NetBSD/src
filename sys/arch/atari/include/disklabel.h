/*	$NetBSD: disklabel.h,v 1.6.8.1 2001/08/03 04:11:17 lukem Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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
 * On a volume, exclusively used by NetBSD, the boot block starts at
 * sector 0. To allow shared use of a volume between two or more OS's,
 * the vendor specific AHDI format is supported. In this case the boot
 * block is located at the start of an AHDI partition. In any case the
 * size of the boot block must be at least 8KB.
 */
#define	BBMINSIZE	8192		/* minimum size of boot block      */
#define LABELSECTOR	0		/* `natural' start of boot block   */
#define LABELOFFSET	516		/* offset of disk label in bytes,
					   relative to start of boot block */
#define LABELMAXSIZE	1020		/* maximum size of disk label, at
					   least (MAXPARTITIONS * 16 + 152)*/
#define MAXPARTITIONS	16		/* max. # of NetBSD partitions     */
#define SWAP_PART	1		/* xx?b is default swap partition  */
#define RAW_PART	2		/* xx?c is raw partition	   */

#define MAXAUXROOTS	29		/* max. # of auxiliary root sectors */

struct cpu_disklabel {
	u_int		cd_bblock;	/* start of NetBSD boot block      */
#define NO_BOOT_BLOCK	((u_int)-1)
	u_int		cd_label;	/* offset of NetBSD disk label     */
#ifdef DISKLABEL_AHDI
	u_int		cd_bslst;	/* start of AHDI bad sector list   */
	u_int		cd_bslend;	/* end of AHDI bad sector list     */
	u_int		cd_roots[MAXAUXROOTS+1]; /* auxiliary root sectors  */
#endif /* DISKLABEL_AHDI */
};

struct bootblock {
	u_int8_t	bb_xxboot[LABELOFFSET - sizeof(u_int32_t)];
						/* first-stage boot loader */
	u_int32_t	bb_magic;		/* boot block magic number */
#define	NBDAMAGIC	0x4e424441 /* 'NBDA' */
#define	AHDIMAGIC	0x41484449 /* 'AHDI' */
	u_int8_t	bb_label[LABELMAXSIZE];	/* disk pack label         */
	u_int8_t	bb_bootxx[BBMINSIZE - LABELOFFSET - LABELMAXSIZE];
						/* second-stage boot loader*/
};

struct disklabel;
#define	BBGETLABEL(bb, dl)	*(dl) = *((struct disklabel *)(bb)->bb_label)
#define	BBSETLABEL(bb, dl)	*((struct disklabel *)(bb)->bb_label) = *(dl)

#endif /* _MACHINE_DISKLABEL_H_ */
