/*	$NetBSD: pte.h,v 1.7.8.1 2006/05/24 10:57:00 yamt Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * pte.h rewritten by chuck based on the jolitz version, plus random
 * info on the pentium and other processors found on the net. The
 * goal of this rewrite is to provide enough documentation on the MMU
 * hardware that the reader will be able to understand it without having
 * to refer to a hardware manual.
 *
 * Modified for the ns32532 by Matthias Pfaller.
 */

#ifndef _NS532_PTE_H_
#define	_NS532_PTE_H_

/*
 * ns32532 MMU hardware structure:
 *
 * The ns32532 MMU is a two-level MMU which maps 4GB of virtual memory.
 * The pagesize is 4K (4096 [0x1000] bytes).
 *
 * The first level table is called a "page directory" and it contains
 * 1024 page directory entries (PDEs). Each PDE is 4 bytes (an int),
 * so a PD fits in a single 4K page. This page is the page directory
 * page (PDP). Each PDE in a PDP maps 4MB of space (1024 * 4MB = 4GB).
 * A PDE contains the physical address of the second level
 * table: The page table.
 *
 * A page table consists of 1024 page table entries (PTEs). Each PTE is
 * 4 bytes (an int), so a page table also fits in a single 4K page. A 4K
 * page being used as a page table is called a page table page (PTP).
 * Each PTE in a PTP maps one 4K page (1024 * 4K = 4MB). A PTE contains
 * the physical address of the page it maps and some flag bits (described
 * below).
 *
 * The processor has a special register, "ptb0", which points to the
 * the PDP which is currently controlling the mappings of the virtual
 * address space.
 *
 * The following picture shows the translation process for a 4K page:
 *
 * ptb0 register [PA of PDP]
 *      |
 *      |
 *      |   bits <31-22> of VA         bits <21-12> of VA   bits <11-0>
 *      |   index the PDP (0 - 1023)   index the PTP        are the page offset
 *      |         |                           |                  |
 *      |         v                           |                  |
 *      +--->+----------+                     |                  |
 *           | PD Page  |   PA of             v                  |
 *           |          |---PTP-------->+------------+           |
 *           | 1024 PDE |               | page table |--PTE--+   |
 *           | entries  |               | (aka PTP)  |       |   |
 *           +----------+               | 1024 PTE   |       |   |
 *                                      | entries    |       |   |
 *                                      +------------+       |   |
 *                                                           |   |
 *                                                bits <31-12>   bits <11-0>
 *                                                p h y s i c a l  a d d r
 *
 * The ns32532 caches PTEs in a TLB. It is important to flush out old
 * TLB mappings when making a change to a mappings. Writing to the
 * ptb0 register will flush the entire TLB. The ns32532 also has an
 * instruction that will invalidate the mapping of a single page (which
 * is useful if you are changing a single page's mappings because it
 * preserves all the cached TLB entries).
 *
 * As shows, bits 31-12 of the PTE contain PA of the page being mapped.
 * the rest of the PTE is defined as follows:
 *   bit#	name	use
 *   11		n/a	available for OS use, hardware ignores it
 *   10		n/a	available for OS use, hardware ignores it
 *   9		n/a	available for OS use, hardware ignores it
 *   8		M	modified page
 *   7		R	referenced page
 *   6		CI	cache inhibited page
 *   5		n/a	reserved
 *   4		n/a	reserved
 *   3		n/a	reserved
 *   2		U/S	user/supervisor bit (0=supervisor only, 1=both u&s)
 *   1		R/W	read/write bit (0=read only, 1=read-write)
 *   0		V	valid page
 *
 * notes:
 *  - Useraccessible pages (U/S == 1) are *always* writable in kernel mode.
 *    copyout and friends use the ns32532's dual address space instructions
 *    for this reason.
 */

#if !defined(_LOCORE)

/*
 * here we define the data types for PDEs and PTEs
 */

typedef uint32_t pd_entry_t;		/* PDE */
typedef uint32_t pt_entry_t;		/* PTE */

#endif

/*
 * now we define various for playing with virtual addresses
 */

#define	PDSHIFT		22		/* offset of PD index in VA */
#define	NBPD		(1 << PDSHIFT)	/* # bytes mapped by PD (4MB) */
#define	PDOFSET		(NBPD-1)	/* mask for non-PD part of VA */
#if 0 /* not used? */
#define	NPTEPD		(NBPD / PAGE_SIZE)	/* # of PTEs in a PD */
#else
#define	PTES_PER_PTP	(NBPD / PAGE_SIZE)	/* # of PTEs in a PTP */
#endif
#define	PD_MASK		0xffc00000	/* page directory address bits */
#define	PT_MASK		0x003ff000	/* page table address bits */

/*
 * here we define the bits of the PDE/PTE, as described above:
 *
 * XXXCDC: need to rename these (PG_u == ugly).
 */

#define	PG_V		0x00000001	/* present */
#define	PG_RO		0x00000000	/* read-only by user and kernel */
#define	PG_RW		0x00000002	/* read-write by user */
#define	PG_u		0x00000004	/* accessible by user */
#define	PG_PROT		0x00000006	/* all protection bits */
#define	PG_N		0x00000040	/* non-cacheable */
#define	PG_U		0x00000080	/* has been used */
#define	PG_M		0x00000100	/* has been modified */
#define	PG_AVAIL1	0x00000200	/* ignored by hardware */
#define	PG_AVAIL2	0x00000400	/* ignored by hardware */
#define	PG_AVAIL3	0x00000800	/* ignored by hardware */
#define	PG_FRAME	0xfffff000	/* page frame mask */

/*
 * various short-hand protection codes
 */

#define	PG_KR		0x00000000	/* kernel read-only */
#define	PG_KW		0x00000002	/* kernel read-write */

#endif /* _NS532_PTE_H_ */
