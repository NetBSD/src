/*	$NetBSD: octeonreg.h,v 1.1 2020/06/15 07:48:12 simonb Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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


#ifndef _OCTEONREG_H_
#define	_OCTEONREG_H_

#define	OCTEON_PLL_REF_CLK		50000000	/* defined as 50MHz */


/* OCTEON II */
#define	MIO_RST_BOOT			UINT64_C(0x1180000001600)
#define	  MIO_RST_BOOT_C_MUL		__BITS(36,30)
#define	  MIO_RST_BOOT_PNR_MUL		__BITS(29,24)


/* OCTEON III */
#define	MIO_FUS_PDF			UINT64_C(0x1180000001428)
#define	  MIO_FUS_PDF_IS_71XX		__BIT(32)

#define	RST_BOOT			UINT64_C(0x1180006001600)
#define	  RST_BOOT_C_MUL		__BITS(36,30)
#define	  RST_BOOT_PNR_MUL		__BITS(29,24)
#define	RST_DELAY			UINT64_C(0x1180006001608)
#define	RST_CFG				UINT64_C(0x1180006001610)
#define	RST_OCX				UINT64_C(0x1180006001618)
#define	RST_INT				UINT64_C(0x1180006001628)
#define	RST_CKILL			UINT64_C(0x1180006001638)
#define	RST_CTL(n)			(UINT64_C(0x1180006001640) + (n) * 0x8)
#define	RST_SOFT_RST			UINT64_C(0x1180006001680)
#define	RST_SOFT_PRST(n)		(UINT64_C(0x11800060016c0) + (n) * 0x8)
#define	RST_PP_POWER			UINT64_C(0x1180006001700)
#define	RST_POWER_DBG			UINT64_C(0x1180006001708)
#define	RST_REF_CNTR			UINT64_C(0x1180006001758)
#define	RST_COLD_DATA(n)		(UINT64_C(0x11800060017c0) + (n) * 0x8)


#endif /* !_OCTEONREG_H_ */
