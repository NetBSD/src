/*	$NetBSD: pte.h,v 1.2 2002/03/02 15:30:49 chris Exp $	*/

/*
 * Copyright (c) 1994 Mark Brinicombe.
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name "RiscBSD" nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RISCBSD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL RISCBSD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM32_PTE_H_
#define _ARM32_PTE_H_

#define PDSHIFT		20		/* LOG2(NBPDR) */
#define NBPD		(1 << PDSHIFT)	/* bytes/page dir */
#define NPTEPD		(NBPD / NBPG)

#ifndef _LOCORE
typedef	u_int32_t	pd_entry_t;		/* page directory entry */
typedef	u_int32_t	pt_entry_t;		/* page table entry */
#endif

#define PD_MASK		0xfff00000	/* page directory address bits */
#define PT_MASK		0x000ff000	/* page table address bits */

#define PG_FRAME	0xfffff000

/* The PT_SIZE definition is misleading... A page table is only 0x400
 * bytes long. But since VM mapping can only be done to 0x1000 a single
 * 1KB blocks cannot be steered to a va by itself. Therefore the
 * pages tables are allocated in blocks of 4. i.e. if a 1 KB block
 * was allocated for a PT then the other 3KB would also get mapped
 * whenever the 1KB was mapped.
 */
 
#define PT_SIZE		0x1000
#define PD_SIZE		0x4000

/* Access permissions for L1 sections and L2 pages */
#define AP_KR		0x00
#define AP_KRW		0x01
#define AP_KRWUR	0x02
#define AP_KRWURW	0x03

#define AP_W		0x01
#define AP_U		0x02

/* Physical bits in a pte */
#define PT_B		0x04	/* Phys - Buffered (write) */
#define PT_C		0x08	/* Phys - Cacheable */
#define PT_U		0x10	/* Phys - Updateable */

#ifndef _LOCORE
extern pt_entry_t	pte_cache_mode;

#define PT_CACHEABLE	(pte_cache_mode)
#endif

/* Page R/M attributes (in pmseg.attrs). */
#define PT_M		0x01	/* Virt - Modified */
#define PT_H		0x02	/* Virt - Handled (Used) */
/* Mapping wired/writeable/cacheable attributes (in pv_flags). */
#define PT_W		0x04	/* Virt - Wired */
#define PT_Wr		0x08	/* Virt / Phys Write */
#define PT_NC		0x10	/* Cacheing disabled (multi-mapped page) */

/* access permissions for L2 pages (all sub pages have the same perms) */
#define PT_AP(x)	((x << 10) | (x << 8) | (x << 6)  | (x << 4))

/* shift for access permissions in a L1 section mapping */
#define AP_SECTION_SHIFT	10

/* Page table types and masks */
#define L1_PAGE		0x01	/* L1 page table mapping */
#define L1_SECTION	0x02	/* L1 section mapping */
#define L1_FPAGE	0x03	/* L1 fine page mapping */
#define L1_MASK		0x03	/* Mask for L1 entry type */
#define L2_LPAGE	0x01	/* L2 large page (64KB) */
#define L2_SPAGE	0x02	/* L2 small page (4KB) */
#define L2_MASK		0x03	/* Mask for L2 entry type */
#define L2_INVAL	0x00	/* L2 invalid type */

/* PTE construction macros */
#define	L2_LPTE(p, a, f)	((p) | PT_AP(a) | L2_LPAGE | (f))
#define L2_SPTE(p, a, f)	((p) | PT_AP(a) | L2_SPAGE | (f))
#define L2_PTE(p, a)		L2_SPTE((p), (a), PT_CACHEABLE)
#define L2_PTE_NC(p, a)		L2_SPTE((p), (a), PT_B)
#define L2_PTE_NC_NB(p, a)	L2_SPTE((p), (a), 0)
#define L1_SECPTE(p, a, f)	((p) | ((a) << AP_SECTION_SHIFT) | (f) \
				| L1_SECTION | PT_U)

#define L1_PTE(p)	((p) | 0x00 | L1_PAGE | PT_U)
#define L1_SEC(p, c)	L1_SECPTE((p), AP_KRW, (c))

#define L1_SEC_SIZE	(1 << PDSHIFT)
#define L2_LPAGE_SIZE	(NBPG * 16)

/* Domain types */
#define DOMAIN_FAULT		0x00
#define DOMAIN_CLIENT		0x01
#define DOMAIN_RESERVED		0x02
#define DOMAIN_MANAGER		0x03

#endif

/* End of pte.h */
