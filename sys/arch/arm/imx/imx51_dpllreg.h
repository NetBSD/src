/*	$NetBSD: imx51_dpllreg.h,v 1.1.16.1 2014/08/10 06:53:51 tls Exp $	*/
/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_IMX51_DPLLREG_H
#define	_IMX51_DPLLREG_H

#include <sys/cdefs.h>

/* register offset address */

#define	IMX51_N_DPLLS		3		/* 1..3 */

#define	DPLL_DP_CTL		0x0000
#define	 DP_CTL_HFSM		__BIT(7)
#define	 DP_CTL_REF_CLK_SEL	__BITS(8,9)
#define	 DP_CTL_REF_CLK_SEL_COSC	__SHIFTIN(0x2, DP_CTL_REF_CLK_SEL)
#define	 DP_CTL_REF_CLK_SEL_FPM		__SHIFTIN(0x3, DP_CTL_REF_CLK_SEL)
#define	 DP_CTL_REF_CLK_DIV	__BIT(10)
#define	 DP_CTL_DPDCK0_2_EN	__BIT(12)
#define	DPLL_DP_CONFIG		0x0004
#define	DPLL_DP_OP		0x0008
#define	 DP_OP_PDF		__BITS(3, 0)
#define	 DP_OP_MFI		__BITS(7, 4)
#define	DPLL_DP_MFD		0x000C
#define	DPLL_DP_MFN		0x0010
#define	DPLL_DP_MFNMINUS	0x0014
#define	DPLL_DP_MFNPLUS		0x0018
#define	DPLL_DP_HFS_OP		0x001C
#define	DPLL_DP_HFS_MFD		0x0020
#define	DPLL_DP_HFS_MFN		0x0024
#define	DPLL_DP_TOGC		0x0028
#define	DPLL_DP_DESTAT		0x002C

#endif /* _IMX51_DPLLREG_H */
