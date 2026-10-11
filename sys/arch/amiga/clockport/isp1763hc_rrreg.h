/*	$NetBSD: isp1763hc_rrreg.h,v 1.1 2026/10/11 17:31:04 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * Individual Computers RapidRoad USB on a clockport.
 *
 * 8-bit window to the ISP1763A via CPLD.
 */

#ifndef _AMIGA_CLOCKPORT_ISP1763HC_RRREG_H_
#define	_AMIGA_CLOCKPORT_ISP1763HC_RRREG_H_

#define	RR_NREGS		16

#define	RR_BANKA		0	/* chip address lines 7:3, ctx A */
#define	 RR_BANKA_ID_MASK	0x1b
#define	 RR_BANKA_ID		0x10
#define	RR_BANKB		1	/* same, ctx B */
#define	RR_STATUS		2
#define	 RR_STATUS_GATE		__BIT(0)
#define	 RR_STATUS_INT		__BIT(1)
#define	RR_CTRL			3
#define	 RR_CTRL_GATE		__BIT(0)
#define	 RR_CTRL_CTXB		__BIT(1)
#define	RR_MEMLO		4	/* chip 0xc4 */
#define	RR_MEMHI		5	/* chip 0xc5 */
#define	RR_DATA			6	/* chip 0xc6 */
#define	RR_WINDOW		8	/* chip registers bank */

#define	RR_BANK(reg)		((reg) >> 3)
#define	RR_SLOT(reg)		(RR_WINDOW + ((reg) & 7))

/* config flags */
#define	RR_FLAGS_FORCE		__BIT(0)

#endif /* !_AMIGA_CLOCKPORT_ISP1763HC_RRREG_H_ */
