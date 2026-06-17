/*	$NetBSD: tlb.h,v 1.9 2026/06/17 15:08:54 rkujawa Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IBM4XX_TLB_H_
#define _IBM4XX_TLB_H_

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#define NTLB	64

/* TLBHI entries */
#define TLB_EPN_MASK	0xfffff000 /* It's 0xfffffc00, but as we use 4K pages we don't need two lower bits */
#define TLB_EPN_SHFT	12
#define TLB_SIZE_MASK	0x00000380
#define TLB_SIZE_SHFT	7
#define TLB_VALID	0x00000040
#define TLB_ENDIAN	0x00000020
#define TLB_U0		0x00000010

#define TLB_SIZE_1K	0
#define TLB_SIZE_4K	1
#define TLB_SIZE_16K	2
#define TLB_SIZE_64K	3
#define TLB_SIZE_256K	4
#define TLB_SIZE_1M	5
#define TLB_SIZE_4M	6
#define TLB_SIZE_16M	7

#define	TLB_PG_1K	(TLB_SIZE_1K << TLB_SIZE_SHFT)
#define	TLB_PG_4K	(TLB_SIZE_4K << TLB_SIZE_SHFT)
#define	TLB_PG_16K	(TLB_SIZE_16K << TLB_SIZE_SHFT)
#define	TLB_PG_64K	(TLB_SIZE_64K << TLB_SIZE_SHFT)
#define	TLB_PG_256K	(TLB_SIZE_256K << TLB_SIZE_SHFT)
#define	TLB_PG_1M	(TLB_SIZE_1M << TLB_SIZE_SHFT)
#define	TLB_PG_4M	(TLB_SIZE_4M << TLB_SIZE_SHFT)
#define	TLB_PG_16M	(TLB_SIZE_16M << TLB_SIZE_SHFT)

/* TLBLO entries */
#define TLB_RPN_MASK	0xfffffc00	/* Real Page Number mask */
#define TLB_EX		0x00000200	/* EXecute enable */
#define TLB_WR		0x00000100	/* WRite enable */
#define TLB_ZSEL_MASK	0x000000f0	/* Zone SELect mask */
#define TLB_ZSEL_SHFT	4
#define TLB_W		0x00000008	/* Write-through */
#define TLB_I		0x00000004	/* Inhibit caching */
#define TLB_M		0x00000002	/* Memory coherent */
#define TLB_G		0x00000001	/* Guarded */

#define TLB_ZONE(z)	(((z) << TLB_ZSEL_SHFT) & TLB_ZSEL_MASK)

/* We only need two zones for kernel and user-level processes */
#define TLB_SU_ZONE	0	/* Kernel-only access controlled permission bits in TLB */
#define TLB_U_ZONE	1	/* Access always controlled by permission bits in TLB entry */

#define TLB_HI(epn,size,flags)	(((epn)&TLB_EPN_MASK)|(((size)<<TLB_SIZE_SHFT)&TLB_SIZE_MASK)|(flags))
#define TLB_LO(rpn,zone,flags)	(((rpn)&TLB_RPN_MASK)|(((zone)<<TLB_ZSEL_SHFT)&TLB_ZSEL_MASK)|(flags))

/*
 * 440/460 (Book E) TLB entries
 */
/* Word 0 */
#define TLB44_EPN_MASK	0xfffffc00
#define TLB44_V		0x00000200	/* Valid */
#define TLB44_TS	0x00000100	/* Translation Space */
#define TLB44_SIZE_MASK	0x000000f0
#define TLB44_SIZE_SHFT	4
/* page size encodings 1K to 16M are identical to the 40x TLB_SIZE_* values */
#define TLB44_SIZE_256M	9
#define TLB44_SIZE_1G	10
/* Word 1 */
#define TLB44_RPN_MASK	0xfffffc00
#define TLB44_ERPN_MASK	0x0000000f	/* phys addr bits 32:35 */
/* Word 2 */
#define TLB44_U0	0x00008000
#define TLB44_U1	0x00004000
#define TLB44_U2	0x00002000
#define TLB44_U3	0x00001000
#define TLB44_W		0x00000800	/* Write-through */
#define TLB44_I		0x00000400	/* Inhibit caching */
#define TLB44_M		0x00000200	/* Memory coherent */
#define TLB44_G		0x00000100	/* Guarded */
#define TLB44_E		0x00000080	/* Little endian */
#define TLB44_UX	0x00000020	/* User execute */
#define TLB44_UW	0x00000010	/* User write */
#define TLB44_UR	0x00000008	/* User read */
#define TLB44_SX	0x00000004	/* Supervisor execute */
#define TLB44_SW	0x00000002	/* Supervisor write */
#define TLB44_SR	0x00000001	/* Supervisor read */

/* 40x TLBLO WIMG flags (TLB_W..TLB_G, also used as TTE_*) -> word 2 */
#define TLB44_WIMG(flags)	(((flags) & 0xf) << 8)

#ifndef _LOCORE

typedef struct tlb_s {
	u_int tlb_hi;
	u_int tlb_lo;
} tlb_t;

struct	pmap;

void	ppc4xx_tlb_enter(int, vaddr_t, u_int);
void	ppc4xx_tlb_flush(vaddr_t, int);
void	ppc4xx_tlb_flush_all(void);
void	ppc4xx_tlb_init(void);
int	ppc4xx_tlb_new_pid(struct pmap *);
void	ppc4xx_tlb_reserve(paddr_t, vaddr_t, size_t, int);
void 	*ppc4xx_tlb_mapiodev(paddr_t, psize_t);
#ifdef PPC_IBM440
/* 36-bit physical address variant for 440/460 I/O above 4GB */
void	ppc44x_tlb_reserve(uint64_t, vaddr_t, size_t, int);
/* pin a TS=0 identity entry for a 256MB RAM chunk above 256MB */
void	ppc44x_tlb_reserve_ts0(paddr_t);
/* claim locore-pinned boot TLB slots before the first reserve */
void	ppc44x_tlb_boot_reserved(int);
/* recover the low-32 PA of a VA mapped via a reserved TLB entry */
bool	ppc44x_tlb_reverse(vaddr_t, paddr_t *);
#endif

#ifndef ppc4xx_tlbflags
#define	ppc4xx_tlbflags(va, pa)	(0)
#endif

#endif /* !_LOCORE */

#define TLB_PID_INVALID 0xFFFF

#endif	/* _IBM4XX_TLB_H_ */
