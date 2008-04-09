/*	$NetBSD: dmac_0448.h,v 1.6 2008/04/09 15:40:30 tsutsui Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: $Hdr: dmac_0448.h,v 4.300 91/06/09 06:21:36 root Rel41 $ SONY
 *
 *	@(#)dmac_0448.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Copyright (c) 1989- by SONY Corporation.
 */
/*
 *	dmac_0448.h
 *		DMAC L7A0448
 */

/*	dmac register base address	*/
#define DMAC_BASE		0xbfe00000

/*	register definition	*/
#define DMAC_GSTAT		(DMAC_BASE + 0xf)
#define DMAC_GSEL		(DMAC_BASE + 0xe)

#define DMAC_CSTAT		(DMAC_BASE + 0x2)
#define DMAC_CCTL		(DMAC_BASE + 0x3)
#define DMAC_CTRCL		(DMAC_BASE + 0x4)
#define DMAC_CTRCM		(DMAC_BASE + 0x5)
#define DMAC_CTRCH		(DMAC_BASE + 0x6)
#define DMAC_CTAG		(DMAC_BASE + 0x7)
#define DMAC_CWID		(DMAC_BASE + 0x8)
#define DMAC_COFSL		(DMAC_BASE + 0x9)
#define DMAC_COFSH		(DMAC_BASE + 0xa)
#define DMAC_CMAP		(DMAC_BASE + 0xc)
#define DMAC_CMAPH		(DMAC_BASE + 0xc)
#define DMAC_CMAPL		(DMAC_BASE + 0xd)

#define dmac_gstat		(*(volatile uint8_t *)DMAC_GSTAT)
#define dmac_gsel		(*(volatile uint8_t *)DMAC_GSEL)

#define dmac_cstat		(*(volatile uint8_t *)DMAC_CSTAT)
#define dmac_cctl		(*(volatile uint8_t *)DMAC_CCTL)
#define dmac_ctrcl		(*(volatile uint8_t *)DMAC_CTRCL)
#define dmac_ctrcm		(*(volatile uint8_t *)DMAC_CTRCM)
#define dmac_ctrch		(*(volatile uint8_t *)DMAC_CTRCH)
#define dmac_ctag		(*(volatile uint8_t *)DMAC_CTAG)
#define dmac_cwid		(*(volatile uint8_t *)DMAC_CWID)
#define dmac_cofsl		(*(volatile uint8_t *)DMAC_COFSL)
#define dmac_cofsh		(*(volatile uint8_t *)DMAC_COFSH)
#define dmac_cmap		(*(volatile uint16_t *)DMAC_CMAP)
#define dmac_cmaph		(*(volatile uint8_t *)DMAC_CMAPH)
#define dmac_cmapl		(*(volatile uint8_t *)DMAC_CMAPL)

/*	status/control bit definition	*/
#define	DM_TCZ			0x80
#define	DM_A28			0x40
#define	DM_AFIX			0x20
#define	DM_APAD			0x10
#define	DM_ZINTEN		0x08
#define	DM_RST			0x04
#define	DM_MODE			0x02
#define DM_ENABLE		0x01

/*	general status bit definition	*/
#define CH_INT(x)		(uint8_t)(1 << (2 * x))
#define CH0_INT			0x01
#define CH1_INT			0x04
#define CH2_INT			0x10
#define CH3_INT			0x40

#define CH_MRQ(x)		(uint8_t)(1 << (2 * x + 1))
#define CH0_MRQ			0x02
#define CH1_MRQ			0x08
#define CH2_MRQ			0x20
#define CH3_MRQ			0x80

/*	channel definition	*/
#define	CH_SCSI			0
#define	CH_FDC			1
#define	CH_AUDIO		2
#define	CH_VIDEO		3

/*	dma status		*/

struct	dm_stat {
	unsigned int dm_gstat;
	unsigned int dm_cstat;
	unsigned int dm_cctl;
	unsigned int dm_tcnt;
	unsigned int dm_offset;
	unsigned int dm_tag;
	unsigned int dm_width;
};

#define	DMAC_WAIT	nops(10)

#define PINTEN		0xbfc80001
# define	DMA_INTEN	0x10
#define PINTSTAT	0xbfc80003
