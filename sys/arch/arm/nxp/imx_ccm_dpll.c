/* $NetBSD: imx_ccm_dpll.c,v 1.1 2026/11/26 03:39:30 hkenken Exp $ */

/*-
 * Copyright (c) 2026 Hashimoto Kenichi <hkenken@NetBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_dpll.c,v 1.1 2026/11/26 03:39:30 hkenken Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

/* register offset address */

#define	IMX_CCM_N_DPLLS			3		/* 1..3 */

#define	DPLL_DP_CTL			0x0000
#define	 DP_CTL_HFSM			__BIT(7)
#define	 DP_CTL_REF_CLK_SEL		__BITS(8,9)
#define	 DP_CTL_REF_CLK_SEL_COSC	__SHIFTIN(0x2, DP_CTL_REF_CLK_SEL)
#define	 DP_CTL_REF_CLK_SEL_FPM		__SHIFTIN(0x3, DP_CTL_REF_CLK_SEL)
#define	 DP_CTL_REF_CLK_DIV		__BIT(10)
#define	 DP_CTL_DPDCK0_2_EN		__BIT(12)
#define	DPLL_DP_CONFIG			0x0004
#define	DPLL_DP_OP			0x0008
#define	 DP_OP_PDF			__BITS(3, 0)
#define	 DP_OP_MFI			__BITS(7, 4)
#define	DPLL_DP_MFD			0x000C
#define	 DP_MFD				__BITS(26, 0)
#define	DPLL_DP_MFN			0x0010
#define	DPLL_DP_MFNMINUS		0x0014
#define	DPLL_DP_MFNPLUS			0x0018
#define	DPLL_DP_HFS_OP			0x001C
#define	DPLL_DP_HFS_MFD			0x0020
#define	DPLL_DP_HFS_MFN			0x0024
#define	DPLL_DP_TOGC			0x0028
#define	DPLL_DP_DESTAT			0x002C

const char *
imx_ccm_dpll_get_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_dpll *dpll = &clk->u.dpll;

	KASSERT(clk->type == IMX_CCM_DPLL);

	const uint32_t val = CCM_READ(sc, clk->regidx, DPLL_DP_CTL);
	const int sel = __SHIFTOUT(val, DP_CTL_REF_CLK_SEL);

	if (sel < dpll->nparents)
		return dpll->parents[sel];

	return NULL;
}

u_int
imx_ccm_dpll_get_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct clk *clkp, *clkp_parent;
	uint32_t dp_ctrl;
	uint32_t dp_op;
	uint32_t dp_mfd;
	uint32_t dp_mfn;
	uint32_t mfi;
	int32_t mfn;
	uint32_t mfd;
	uint32_t pdf;
	int64_t rate;

	KASSERT(clk->type == IMX_CCM_DPLL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return 0;

	dp_ctrl = CCM_READ(sc, clk->regidx, DPLL_DP_CTL);
	if (dp_ctrl & DP_CTL_HFSM) {
		dp_op = CCM_READ(sc, clk->regidx, DPLL_DP_HFS_OP);
		dp_mfd = CCM_READ(sc, clk->regidx, DPLL_DP_HFS_MFD);
		dp_mfn = CCM_READ(sc, clk->regidx, DPLL_DP_HFS_MFN);
	} else {
		dp_op = CCM_READ(sc, clk->regidx, DPLL_DP_OP);
		dp_mfd = CCM_READ(sc, clk->regidx, DPLL_DP_MFD);
		dp_mfn = CCM_READ(sc, clk->regidx, DPLL_DP_MFN);
	}

	pdf = __SHIFTOUT(dp_op, DP_OP_PDF);
	mfi = uimax(5, __SHIFTOUT(dp_op, DP_OP_MFI));
	mfd = __SHIFTOUT(dp_mfd, DP_MFD);
	if (dp_mfn & __BIT(26))
		/* 27bit signed value */
		mfn = (int32_t)(__BITS(31,27) | dp_mfn);
	else
		mfn = dp_mfn;

	rate = prate;
	if (dp_ctrl & DP_CTL_REF_CLK_DIV)
		rate /= 2;
	rate *= 4;
	rate = (int64_t)rate * mfi + (int64_t)rate * mfn / (mfd + 1);
	rate /= pdf + 1;

	if (!(dp_ctrl & DP_CTL_DPDCK0_2_EN))
		rate /= 2;

	return rate;
}

