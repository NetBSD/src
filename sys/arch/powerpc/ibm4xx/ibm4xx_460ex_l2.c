/*	$NetBSD: ibm4xx_460ex_l2.c,v 1.2 2026/09/28 20:41:00 rkujawa Exp $	*/

/*
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
 * AMCC PPC460EX on-chip 256KB L2 cache support
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibm4xx_460ex_l2.c,v 1.2 2026/09/28 20:41:00 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/ibm4xx_460ex_l2.h>

bool ibm4xx_460ex_l2_enabled = false;
uint32_t ibm4xx_460ex_l2_cfg;

static void
ibm4xx_460ex_l2_cmd(uint32_t cmd)
{

	mtdcr(DCR_L2C0_CMD, cmd);
	while ((mfdcr(DCR_L2C0_SR) & L2C_SR_CC) == 0)
		;
}

/*
 * Enable L2 cache, as write-through, with hardware snooping on the 
 * low latency PLB segment.
 */
void
ibm4xx_460ex_l2cache_enable(void)
{
	uint32_t cfg;

	/* Hand the SRAM0 data arrays back from the SRAM controller. */
	mtdcr(DCR_SRAM0_SB0CR, 0);
	mtdcr(DCR_SRAM0_SB1CR, 0);
	mtdcr(DCR_SRAM0_SB2CR, 0);
	mtdcr(DCR_SRAM0_SB3CR, 0);

	/* RDBW is required; switch the array into L2 mode. */
	mtdcr(DCR_L2C0_CFG, L2C_CFG_RDBW | L2C_CFG_L2M | L2C_CFG_SS_256KB);

	/* Clear the array and any parity errors. */
	mtdcr(DCR_L2C0_ADDR, 0);
	ibm4xx_460ex_l2_cmd(L2C_CMD_HCC);
	ibm4xx_460ex_l2_cmd(L2C_CMD_CCP);
	ibm4xx_460ex_l2_cmd(L2C_CMD_CTE);
	__asm volatile ("msync" ::: "memory");

	/* All of the LL, and the HB alias. */
	mtdcr(DCR_L2C0_SNP0, L2C_SNP_SSR_32GB | L2C_SNP_ESR);
	mtdcr(DCR_L2C0_SNP1, 0x80000000 | L2C_SNP_SSR_32GB | L2C_SNP_ESR);
	__asm volatile ("sync; isync" ::: "memory");

	cfg = mfdcr(DCR_L2C0_CFG);
	cfg &= ~(L2C_CFG_DCW | L2C_CFG_PMUX | L2C_CFG_PMIM | L2C_CFG_TPEI |
	    L2C_CFG_CPEI | L2C_CFG_NAM | L2C_CFG_NBRM);
	cfg |= L2C_CFG_ICU | L2C_CFG_DCU | L2C_CFG_TPC | L2C_CFG_CPC |
	    L2C_CFG_FRAN | L2C_CFG_CPIM | L2C_CFG_TPIM | L2C_CFG_LIM |
	    L2C_CFG_SMCM | L2C_CFG_SNP440 | L2C_CFG_RDBW;
	mtdcr(DCR_L2C0_CFG, cfg);
	__asm volatile ("sync; isync" ::: "memory");

	ibm4xx_460ex_l2_cfg = mfdcr(DCR_L2C0_CFG);
	if (ibm4xx_460ex_l2_cfg & L2C_CFG_L2M)
		ibm4xx_460ex_l2_enabled = true;
}
