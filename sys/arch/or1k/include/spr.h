/* $NetBSD: spr.h,v 1.1 2014/09/03 19:34:26 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OR1K_SPR_H_
#define _OR1K_SPR_H_

#define	SPR_GROUP	__BITS(15,11)
#define SPR_REG		__BITS(10,0)

#define	SPR_MAKE(g,r)	(__SHIFTIN((g), SPR_GROUP)|__SHIFTIN((r), SPR_REG))

#define	SPR_VR		SPR_MAKE(0, 0)
#define	SPR_UPR		SPR_MAKE(0, 1)
#define	SPR_CPUCFGR	SPR_MAKE(0, 2)
#define	SPR_DMMUCFGR	SPR_MAKE(0, 3)
#define	SPR_IMMUCFGR	SPR_MAKE(0, 4)
#define	SPR_DCCFGR	SPR_MAKE(0, 5)
#define	SPR_ICCFGR	SPR_MAKE(0, 6)
#define	SPR_DCFGR	SPR_MAKE(0, 7)
#define	SPR_PCCFGR	SPR_MAKE(0, 8)
#define	SPR_VR2		SPR_MAKE(0, 9)
#define	SPR_AVR		SPR_MAKE(0, 10)
#define	SPR_EVBAR	SPR_MAKE(0, 11)
#define	SPR_AECR	SPR_MAKE(0, 12)
#define	SPR_AESR	SPR_MAKE(0, 13)
#define	SPR_NPC		SPR_MAKE(0, 16)
#define	SPR_SR		SPR_MAKE(0, 17)
#define  SR_CID		__BITS(31,28)
#define  SR_SUMRA	__BIT(16)
#define  SR_FO		__BIT(15)
#define  SR_EPH		__BIT(14)
#define  SR_DSX		__BIT(13)
#define  SR_OVE		__BIT(12)
#define  SR_OV		__BIT(11)
#define  SR_CY		__BIT(10)
#define  SR_F		__BIT(9)
#define  SR_CE		__BIT(8)
#define  SR_LEE		__BIT(7)
#define  SR_IME		__BIT(6)
#define  SR_DME		__BIT(5)
#define  SR_ICE		__BIT(4)
#define  SR_DCE		__BIT(3)
#define  SR_IEE		__BIT(2)
#define  SR_TEE		__BIT(1)
#define  SR_SM		__BIT(0)
#define	SPR_PPC		SPR_MAKE(0, 18)
#define	SPR_FPCSR	SPR_MAKE(0, 20)
#define  FPCSR_DZF	__BIT(11)
#define  FPCSR_INF	__BIT(10)
#define  FPCSR_IVF	__BIT(9)
#define  FPCSR_IXF	__BIT(8)
#define  FPCSR_ZF	__BIT(7)
#define  FPCSR_QNF	__BIT(6)
#define  FPCSR_SNF	__BIT(5)
#define  FPCSR_UNF	__BIT(4)
#define  FPCSR_OVF	__BIT(3)
#define  FPCSR_RM	__BITS(2,1)
#define  FPCSR_RM_RN	0
#define  FPCSR_RM_RZ	1
#define  FPCSR_RM_RP	2
#define  FPCSR_RM_RM	3
#define  FPCSR_FPEE	__BIT(0)
#define	SPR_ISRn(n)	SPR_MAKE(0, 21+(n))
#define	SPR_EPCRn(n)	SPR_MAKE(0, 32+(n))
#define	SPR_EEARn(n)	SPR_MAKE(0, 48+(n))
#define	SPR_ESRn(n)	SPR_MAKE(0, 64+(n))
#define SPR_GRPN(cid,n)	SPR_MASK(0, 1024+(cid)*32+(n))

#endif /* _OR1K_SPR_H_ */
