/*	$NetBSD: mq200subr.c,v 1.1.6.1 2002/06/23 17:36:51 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMURA Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifdef _KERNEL
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#else
#include <stdio.h>
#endif
#include <sys/types.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include "opt_mq200.h"
#include "mq200var.h"
#include "mq200reg.h"
#include "mq200priv.h"

#define ABS(a)	((a) < 0 ? -(a) : (a))

int mq200_depth_table[] = {
	[MQ200_GCC_1BPP] =		1,
	[MQ200_GCC_2BPP] =		2,
	[MQ200_GCC_4BPP] =		4,
	[MQ200_GCC_8BPP] =		8,
	[MQ200_GCC_16BPP] =		16,
	[MQ200_GCC_24BPP] =		32,
	[MQ200_GCC_ARGB888] =		32,
	[MQ200_GCC_ABGR888] =		32,
	[MQ200_GCC_16BPP_DIRECT] =	16,
	[MQ200_GCC_24BPP_DIRECT] =	32,
	[MQ200_GCC_ARGB888_DIRECT] =	32,
	[MQ200_GCC_ABGR888_DIRECT] =	32,
};

struct mq200_crt_param mq200_crt_params[] = {
	[MQ200_CRT_640x480_60Hz] =
	{	640, 480, 25175,	/* width, height, dot clock */
		800,			/* HD Total */
		525,			/* VD Total */
		656, 752,		/* HS Start, HS End */
		490, 492,		/* VS Start, VS End */
		(MQ200_GC1CRTC_HSYNC_ACTVLOW |
		    MQ200_GC1CRTC_VSYNC_ACTVLOW |
		    MQ200_GC1CRTC_BLANK_PEDESTAL_EN),
	},
	[MQ200_CRT_800x600_60Hz] =
	{	800, 600, 40000,	/* width, height, dot clock */
		1054,			/* HD Total */
		628,			/* VD Total */
		839, 967,		/* HS Start, HS End */
		601, 605,		/* VS Start, VS End */
		MQ200_GC1CRTC_BLANK_PEDESTAL_EN,
	},
	[MQ200_CRT_1024x768_60Hz] =
	{	1024, 768, 65000,	/* width, height, dot clock */
		1344,			/* HD Total */
		806,			/* VD Total */
		1048,	1184,		/* HS Start, HS End */
		771,	777,		/* VS Start, VS End */
		(MQ200_GC1CRTC_HSYNC_ACTVLOW |
		    MQ200_GC1CRTC_VSYNC_ACTVLOW |
		    MQ200_GC1CRTC_BLANK_PEDESTAL_EN),
	},			
};

int mq200_crt_nparams = sizeof(mq200_crt_params)/sizeof(*mq200_crt_params);

/*
 * get PLL setting register value for given frequency
 */
void
mq200_pllparam(int reqout, u_int32_t *res)
{
	int n, m, p, out;
	int ref = 12288;
	int bn, bm, bp, e;

	e = ref;
	for (p = 0; p <= 4; p++) {
		for (n = 0; n < (1<<5); n++) {
			m = (reqout * ((n + 1) << p)) / ref - 1;
			out = ref * (m + 1) / ((n + 1) << p);
			if (0xff < m)
				break;
			if (40 <= m &&
			    1000 <= ref/(n + 1) &&
			    170000 <= ref*(m+1)/(n+1) &&
			    ref*(m+1)/(n+1) <= 340000 &&
			    ABS(reqout - out) <= e) {
				e = ABS(reqout - out);
				bn = n;
				bm = m;
				bp = p;
			}
		}
	}

#if 0
	out = ref * (bm + 1) / ((bn + 1) << bp);
	printf("PLL: %d.%03d x (%d+1) / (%d+1) / %d = %d.%03d\n",
	    ref / 1000, ref % 1000, bm, bn, (1<<bp),
	    out / 1000, out % 1000);
#endif
	*res = ((bm << MQ200_PLL_M_SHIFT) |
		(bn << MQ200_PLL_N_SHIFT) |
		(bp << MQ200_PLL_P_SHIFT));
}

void
mq200_set_pll(struct mq200_softc *sc, int pll, int clock)
{
	struct mq200_regctx *paramreg, *enreg;
	u_int32_t param, enbit;

	switch (pll) {
	case MQ200_CLOCK_PLL1:
		paramreg = &sc->sc_regctxs[MQ200_I_PLL(1)];
		enreg = &sc->sc_regctxs[MQ200_I_DCMISC];
		enbit = MQ200_DCMISC_PLL1_ENABLE;
		break;
	case MQ200_CLOCK_PLL2:
		paramreg = &sc->sc_regctxs[MQ200_I_PLL(2)];
		enreg = &sc->sc_regctxs[MQ200_I_PMC];
		enbit = MQ200_PMC_PLL2_ENABLE;
		break;
	case MQ200_CLOCK_PLL3:
		paramreg = &sc->sc_regctxs[MQ200_I_PLL(3)];
		enreg = &sc->sc_regctxs[MQ200_I_PMC];
		enbit = MQ200_PMC_PLL3_ENABLE;
		break;
	default:
		printf("mq200: invalid PLL: %d\n", pll);
		return;
	}
	if (clock != 0 && clock != -1) {
		/* PLL Programming	*/
		mq200_pllparam(clock, &param);
		mq200_mod(sc, paramreg, MQ200_PLL_PARAM_MASK, param);
		/* enable PLL	*/
		mq200_on(sc, enreg, enbit);
	}

	DPRINTF("%s %d.%03dMHz\n",
	    mq200_clknames[pll], clock/1000, clock%1000);
}

void
mq200_setup_regctx(struct mq200_softc *sc)
{
	int i;
	static int offsets[MQ200_I_MAX] = {
		[MQ200_I_DCMISC] =		MQ200_DCMISCR,
		[MQ200_I_PLL(2)] =		MQ200_PLL2R,
		[MQ200_I_PLL(3)] =		MQ200_PLL3R,
		[MQ200_I_PMC] =			MQ200_PMCR,
		[MQ200_I_MM01] =		MQ200_MMR(1),
		[MQ200_I_GCC(MQ200_GC1)] =	MQ200_GCCR(MQ200_GC1),
		[MQ200_I_GCC(MQ200_GC2)] =	MQ200_GCCR(MQ200_GC2),
	};

	for (i = 0; i < sizeof(offsets)/sizeof(*offsets); i++) {
		if (offsets[i] == 0)
#ifdef MQ200_DEBUG
			if (i != MQ200_I_PMC)
				panic("%s(%d): register context %d is empty\n",
				    __FILE__, __LINE__, i);
#endif
		sc->sc_regctxs[i].offset = offsets[i];
	}
}

void
mq200_setup(struct mq200_softc *sc)
{
	const struct mq200_clock_setting *clock;
	const struct mq200_crt_param *crt;

	clock = &sc->sc_md->md_clock_settings[sc->sc_flags & MQ200_SC_GC_MASK];
	crt = sc->sc_crt;

	/* disable GC1 and GC2	*/
	//mq200_write(sc, MQ200_GCCR(MQ200_GC1), 0);
	mq200_write2(sc, &sc->sc_regctxs[MQ200_I_GCC(MQ200_GC1)], 0);
	mq200_write(sc, MQ200_GC1CRTCR, 0);
	//mq200_write(sc, MQ200_GCCR(MQ200_GC2), 0);
	mq200_write2(sc, &sc->sc_regctxs[MQ200_I_GCC(MQ200_GC2)], 0);

	while (mq200_read(sc, MQ200_PMCR) & MQ200_PMC_SEQPROGRESS)
	    /* busy wait */;

	/*
	 * setup around clock
	 */
	/* setup eatch PLLs	*/
	mq200_set_pll(sc, MQ200_CLOCK_PLL1, clock->pll1);
	mq200_set_pll(sc, MQ200_CLOCK_PLL2, clock->pll2);
	mq200_set_pll(sc, MQ200_CLOCK_PLL3, clock->pll3);
	if (sc->sc_flags & MQ200_SC_GC1_ENABLE)
		mq200_set_pll(sc, clock->gc[MQ200_GC1], crt->clock);

	/* setup MEMORY clock */
	if (clock->mem == MQ200_CLOCK_PLL2)
		mq200_on(sc, &sc->sc_regctxs[MQ200_I_MM01],
		    MQ200_MM01_CLK_PLL2);
	else
		mq200_off(sc, &sc->sc_regctxs[MQ200_I_MM01],
		    MQ200_MM01_CLK_PLL2);
	DPRINTF("MEM: PLL%d\n", (clock->mem == MQ200_CLOCK_PLL2)?2:1);

	/* setup GE clock */
	mq200_mod(sc, &sc->sc_regctxs[MQ200_I_PMC],
	    MQ200_PMC_GE_CLK_MASK | MQ200_PMC_GE_ENABLE,
	    (clock->ge << MQ200_PMC_GE_CLK_SHIFT) | MQ200_PMC_GE_ENABLE);
	DPRINTF(" GE: PLL%d\n", clock->ge);

	/*
	 * setup GC1	(CRT contoller)
	 */
	if (sc->sc_flags & MQ200_SC_GC1_ENABLE) {
		/* GC03R	Horizontal Display Control	*/
		mq200_write(sc, MQ200_GCHDCR(MQ200_GC1),
		    (((u_int32_t)crt->hdtotal-2)<<MQ200_GC1HDC_TOTAL_SHIFT) |
		    ((u_int32_t)crt->width << MQ200_GCHDC_END_SHIFT));

		/* GC03R	Vertical Display Control	*/
		mq200_write(sc, MQ200_GCVDCR(MQ200_GC1),
		    (((u_int32_t)crt->vdtotal-1)<<MQ200_GC1VDC_TOTAL_SHIFT) |
		    (((u_int32_t)crt->height - 1) << MQ200_GCVDC_END_SHIFT));

		/* GC04R	Horizontal Sync Control		*/
		mq200_write(sc, MQ200_GCHSCR(MQ200_GC1),
		    ((u_int32_t)crt->hsstart << MQ200_GCHSC_START_SHIFT) |
		    ((u_int32_t)crt->hsend << MQ200_GCHSC_END_SHIFT));

		/* GC05R	Vertical Sync Control		*/
		mq200_write(sc, MQ200_GCVSCR(MQ200_GC1),
		    ((u_int32_t)crt->vsstart << MQ200_GCVSC_START_SHIFT) |
		    ((u_int32_t)crt->vsend << MQ200_GCVSC_END_SHIFT));

		/* GC00R	GC1 Control			*/
		//mq200_write(sc, MQ200_GCCR(MQ200_GC1),
		mq200_write2(sc, &sc->sc_regctxs[MQ200_I_GCC(MQ200_GC1)],
		    (MQ200_GCC_ENABLE |
			(clock->gc[MQ200_GC1] << MQ200_GCC_RCLK_SHIFT) |
			MQ200_GCC_MCLK_FD_1 |
			(1 << MQ200_GCC_MCLK_SD_SHIFT)));

		/* GC01R	CRT Control			*/
		mq200_write(sc, MQ200_GC1CRTCR,
		    MQ200_GC1CRTC_DACEN | crt->opt);

		sc->sc_width[MQ200_GC1] = crt->width;
		sc->sc_height[MQ200_GC1] = crt->height;

		DPRINTF("GC1: %s\n",
		    mq200_clknames[clock->gc[MQ200_GC1]]);
	}

	while (mq200_read(sc, MQ200_PMCR) & MQ200_PMC_SEQPROGRESS)
	    /* busy wait */;

	/*
	 * setup GC2	(FP contoller)
	 */
	if (sc->sc_flags & MQ200_SC_GC2_ENABLE) {
		//mq200_write(sc, MQ200_GCCR(MQ200_GC2),
		mq200_write2(sc, &sc->sc_regctxs[MQ200_I_GCC(MQ200_GC2)],
		    MQ200_GCC_ENABLE |
		    (clock->gc[MQ200_GC2] << MQ200_GCC_RCLK_SHIFT) |
		    MQ200_GCC_MCLK_FD_1 | (1 << MQ200_GCC_MCLK_SD_SHIFT));
		DPRINTF("GC2: %s\n",
		    mq200_clknames[clock->gc[MQ200_GC2]]);
	}

	while (mq200_read(sc, MQ200_PMCR) & MQ200_PMC_SEQPROGRESS)
	    /* busy wait */;

	/*
	 * disable unused PLLs
	 */
	if (clock->pll1 == 0) {
		DPRINTF("PLL1 disable\n");
		mq200_off(sc, &sc->sc_regctxs[MQ200_I_DCMISC],
		    MQ200_DCMISC_PLL1_ENABLE);
	}
	if (clock->pll2 == 0) {
		DPRINTF("PLL2 disable\n");
		mq200_off(sc, &sc->sc_regctxs[MQ200_I_PMC],
		    MQ200_PMC_PLL2_ENABLE);
	}
	if (clock->pll3 == 0) {
		DPRINTF("PLL3 disable\n");
		mq200_off(sc,  &sc->sc_regctxs[MQ200_I_PMC],
		    MQ200_PMC_PLL3_ENABLE);
	}
}

void
mq200_win_enable(struct mq200_softc *sc, int gc, 
    u_int32_t depth, u_int32_t start,
    int width, int height, int stride)
{

	DPRINTF("enable window on GC%d: %dx%d(%dx%d)\n",
	    gc + 1, width, height,  sc->sc_width[gc], sc->sc_height[gc]);

	if (sc->sc_width[gc] < width) {
		if (mq200_depth_table[depth])
			start += (height - sc->sc_height[gc]) * 
			    mq200_depth_table[depth] / 8;
		width = sc->sc_width[gc];
	}

	if (sc->sc_height[gc] < height) {
		start += (height - sc->sc_height[gc]) * stride;
		height = sc->sc_height[gc];
	}

	/* GC08R	Window Horizontal Control	*/
	mq200_write(sc, MQ200_GCWHCR(gc),
	    (((u_int32_t)width - 1) << MQ200_GCWHC_WIDTH_SHIFT) |
	    ((sc->sc_width[gc] - width)/2));

	/* GC09R	Window Vertical Control		*/
	mq200_write(sc, MQ200_GCWVCR(gc),
	    (((u_int32_t)height - 1) << MQ200_GCWVC_HEIGHT_SHIFT) |
	    ((sc->sc_height[gc] - height)/2));

	/* GC00R	GC Control	*/
	mq200_mod(sc, &sc->sc_regctxs[MQ200_I_GCC(gc)],
	    (MQ200_GCC_WINEN | MQ200_GCC_DEPTH_MASK),
	    (MQ200_GCC_WINEN | (depth << MQ200_GCC_DEPTH_SHIFT)));
}

void
mq200_win_disable(struct mq200_softc *sc, int gc)
{
	/* GC00R	GC Control	*/
	mq200_off(sc, &sc->sc_regctxs[MQ200_I_GCC(gc)], MQ200_GCC_WINEN);
}
