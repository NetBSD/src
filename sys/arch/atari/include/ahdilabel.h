/*	$NetBSD: ahdilabel.h,v 1.2 2001/01/07 21:05:16 leo Exp $	*/

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

#ifndef _MACHINE_AHDILABEL_H_
#define _MACHINE_AHDILABEL_H_

#define	AHDI_BSIZE	512		/* AHDI blocksize		*/
#define	AHDI_BBLOCK	0		/* AHDI bootblock (root sector)	*/
#define AHDI_MAXROOTS	(MAXAUXROOTS)	/* max. # of AHDI rootsectors	*/
#define	AHDI_MAXPARTS	(AHDI_MAXROOTS+3) /* max. # of AHDI partitions	*/

/*
 * Various `well known' AHDI partition identifiers.
 */
#define AHDI_MKPID(x,y,z)	(   ((u_int32_t)(x) << 16)	\
				  | ((u_int32_t)(y) << 8)	\
				  | ((u_int32_t)(z))		\
				)
#define	AHDI_PID_XGM	AHDI_MKPID('X','G','M')
#define	AHDI_PID_GEM	AHDI_MKPID('G','E','M')
#define	AHDI_PID_BGM	AHDI_MKPID('B','G','M')
#define	AHDI_PID_RAW	AHDI_MKPID('R','A','W')
#define	AHDI_PID_SWP	AHDI_MKPID('S','W','P')
#define	AHDI_PID_NBD	AHDI_MKPID('N','B','D')
#define	AHDI_PID_NBR	AHDI_MKPID('N','B','R')
#define	AHDI_PID_NBS	AHDI_MKPID('N','B','S')
#define	AHDI_PID_NBU	AHDI_MKPID('N','B','U')

/*
 * Format of AHDI boot block.
 */
#define	AHDI_MAXRPD	4		/* max. # of partition descriptors */
					/* in an AHDI bootblock (aka root) */
#define	AHDI_MAXARPD	2		/* max. # of partition descriptors */
					/* in an AHDI auxilary root sector */
struct ahdi_part {
	u_int8_t	ap_flg;		/* bit 0 is in-use flag            */
	u_int8_t	ap_id[3];	/* id: GEM, BGM, XGM, UNX, MIX     */
	u_int32_t	ap_st;		/* block where partition starts    */
	u_int32_t	ap_size;	/* partition size in blocks        */
} __attribute__((__packed__));
#define	ap_end	ap_size /* in the in-core copy, store end instead of size  */

struct ahdi_root {
	u_int8_t	 ar_fill[0x1c2];/* filler, can be boot code        */
	u_int32_t	 ar_hdsize;	/* size of entire volume in blocks */
	struct ahdi_part ar_parts[AHDI_MAXRPD]; /* root partition table    */
	u_int32_t	 ar_bslst;	/* start of bad-sector list        */
	u_int32_t	 ar_bslsize;	/* # of blocks in bad-sector list  */
	u_int16_t	 ar_checksum;
} __attribute__((__packed__));

/*
 * AHDI partition table.
 */
#ifdef _KERNEL
struct ahdi_ptbl {
	struct cpu_disklabel	*at_cdl;
	u_int			at_hdsize;   /* size of volume in blocks   */
	u_int			at_nroots;   /* # of auxilary root sectors */
	u_int			at_nparts;   /* # of AHDI partitions       */
	struct ahdi_part	at_parts[AHDI_MAXPARTS];
};
#define at_bblock	at_cdl->cd_bblock
#define at_label	at_cdl->cd_label
#define at_bslst	at_cdl->cd_bslst
#define at_bslend	at_cdl->cd_bslend
#define at_roots	at_cdl->cd_roots
#endif /* _KERNEL */

#endif /* _MACHINE_AHDILABEL_H_ */
