/*	$NetBSD: pte.h,v 1.5 2002/02/11 18:06:06 uch Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)pte.h	5.5 (Berkeley) 5/9/91
 */

/*
 * SH3
 *
 * Page Table Entry
 *
 *  T.Horiuchi Brains Corp. 05/26/1998
 */
#ifndef _SH3_PTE_H_
#define _SH3_PTE_H_

#define	PDSHIFT		22		/* LOG2(NBPDR) */
#define	NBPD		(1 << PDSHIFT)	/* bytes/page dir */
#define	PDOFSET		(NBPD-1)	/* byte offset into page dir */
#define	NPTEPD		(NBPD / NBPG)

#ifndef _LOCORE
typedef int	pd_entry_t;		/* page directory entry */
typedef int	pt_entry_t;		/* Mach page table entry */
#endif

#define	PD_MASK		0xffc00000	/* page directory address bits */
#define	PT_MASK		0x003ff000	/* page table address bits */
#define	PTES_PER_PTP	(NBPD / NBPG)	/* # of PTEs in a PTP */

/*
 *
 * NetBSD/sh3 PTE format.
 *
 * SH3
 *   PPN   V   PR  SZ C  D  SH
 * [28:10][8][6:5][4][3][2][1]
 *
 * SH4
 *         V  SZ  PR  SZ C  D  SH WT
 * [28:10][8][7][6:5][4][3][2][1][0]
 *
 * + NetBSD/sh3 page size is 4KB. [11:10] and [7] can be used as SW bit.
 * + [31:29] should be available for SW bit...
 * + SH4 WT bit is not stored in PTE. U0 is always write-back. and
 *   P3 is always write-thurogh. (see sh3/trap.c::__setup_pte_sh4())
 *   We use WT bit as SW bit.
 * 
 * Software bit assign
 *   [11:9] - SH4 PCMCIA Assistant bit. (space attribute bit only)
 *   [7]    - Wired page bit.
 *   [0]    - PVlist bit.
 */

/*
 * Hardware bits
 */
#define	PG_FRAME		0xfffff000	/* page frame mask XXX */
#define	PG_V			0x00000100	/* present */
#define	PG_UW			0x00000060	/* kernel/user read/write */
#define	PG_URKR			0x00000040	/* kernel/user read only */
#define	PG_KW			0x00000020	/* kernel read/write */
#define	PG_KR			0x00000000	/* kernel read only */
#define	PG_4K			0x00000010	/* page size 4KB */
#define	PG_N			0x00000008	/* 0=non-cacheable */
#define	PG_M			0x00000004	/* has been modified */
#define	PG_G			0x00000002	/* share status */
#define	PG_WT			0x00000001	/* write through (SH4) */

#define PG_HW_BITS		0x1ffff17e	/* [28:12][8][6:1] */

/*
 * Software bits
 */
#define	PG_W			0x00000080	/* page is wired */
#define	PG_PVLIST		0x00000001	/* mapping has entry on pvlist */
/* SH4 PCMCIA MMU support bits */
/* PTEA SA (Space Attribute bit) */
#define _PG_PCMCIA		0x00000e00
#define _PG_PCMCIA_SHIFT	9
#define _PG_PCMCIA_NONE		0x00000000	/* Non PCMCIA space */
#define _PG_PCMCIA_IO		0x00000200	/* IOIS16 signal */
#define _PG_PCMCIA_IO8		0x00000400	/* 8 bit I/O  */
#define _PG_PCMCIA_IO16		0x00000600	/* 16 bit I/O  */
#define _PG_PCMCIA_MEM8		0x00000800	/* 8 bit common memory */
#define _PG_PCMCIA_MEM16	0x00000a00	/* 16 bit common memory */
#define _PG_PCMCIA_ATTR8	0x00000c00	/* 8 bit attribute */
#define _PG_PCMCIA_ATTR16	0x00000e00	/* 16 bit attribute */

#endif /* !_SH3_PTE_H_ */
