/*	$NetBSD: pte.h,v 1.3 2003/08/24 17:52:34 chs Exp $	*/

/*-
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_POWERPC_OEA_PTE_H_
#define	_POWERPC_OEA_PTE_H_

#include <sys/queue.h>

/*
 * Page Table Entries
 */
#ifndef	_LOCORE
struct pte {
	register_t pte_hi;
	register_t pte_lo;
};

struct pteg {
	struct pte pt[8];
};
#endif	/* _LOCORE */

/* High word: */
#define	PTE_VALID	0x80000000
#define	PTE_VSID	0x7fffff80
#define	PTE_VSID_SHFT	7
#define	PTE_VSID_LEN	24
#define	PTE_HID		0x00000040
#define	PTE_API		0x0000003f
#define	PTE_API_SHFT	0
/* Low word: */
#define	PTE_RPGN	0xfffff000
#define	PTE_RPGN_SHFT	12
#define	PTE_REF		0x00000100
#define	PTE_CHG		0x00000080
#define	PTE_W		0x00000040	/* 1 = write-through, 0 = write-back */
#define	PTE_I		0x00000020	/* cache inhibit */
#define	PTE_M		0x00000010	/* memory coherency enable */
#define	PTE_G		0x00000008	/* guarded region (not on 601) */
#define	PTE_WIMG	(PTE_W|PTE_I|PTE_M|PTE_G)
#define	PTE_IG		(PTE_I|PTE_G)
#define	PTE_PP		0x00000003
#define	PTE_SO		0x00000000	/* Super. Only       (U: XX, S: RW) */
#define	PTE_SW		0x00000001	/* Super. Write-Only (U: RO, S: RW) */
#define	PTE_BW		0x00000002	/* Supervisor        (U: RW, S: RW) */
#define	PTE_BR		0x00000003	/* Both Read Only    (U: RO, S: RO) */
#define	PTE_RW		PTE_BW
#define	PTE_RO		PTE_BR

#define	PTE_EXEC	0x00000200	/* pseudo bit; page is exec */

/*
 * Extract bits from address
 */
#define	ADDR_SR_SHFT	28
#define	ADDR_PIDX	0x0ffff000
#define	ADDR_PIDX_SHFT	12
#define	ADDR_API_SHFT	22
#define	ADDR_POFF	0x00000fff
#define	ADDR_SEG_WIDTH	4

/*
 * Segment registers
 */
#define SR_KEY_LEN	4		/* key bit width */
#define	SR_TYPE		0x80000000	/* T=0 selects memory format */
#define	SR_SUKEY	0x40000000	/* Supervisor protection key */
#define	SR_PRKEY	0x20000000	/* User protection key */
#define	SR_NOEXEC	0x10000000	/* No-execute protection bit */
#define	SR_VSID		0x00ffffff	/* Virtual segment ID */

#endif	/* _POWERPC_OEA_PTE_H_ */
