/*	$NetBSD: mq200machdep.c,v 1.2 2003/07/15 02:29:29 lukem Exp $	*/

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mq200machdep.c,v 1.2 2003/07/15 02:29:29 lukem Exp $");

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

#if MQ200_SETUPREGS
#define OP_(n)          (((n) << 2) | 1)
#define OP_END		OP_(1)
#define OP_MASK		OP_(2)
#define OP_LOADPLLPARAM	OP_(3)
#define OP_LOADFROMREG	OP_(4)
#define OP_STORETOREG	OP_(5)
#define OP_LOADIMM	OP_(6)
#define OP_OR		OP_(7)

static void mq200_setupregs(struct mq200_softc *sc, u_int32_t *ops);

static u_int32_t mcr530_init_ops[] = {
	MQ200_PMCR,	0,	/* power management control */
	MQ200_DCMISCR,	MQ200_DCMISC_OSC_ENABLE |
			MQ200_DCMISC_FASTPOWSEQ_DISABLE |
			MQ200_DCMISC_OSCFREQ_12_25,
	OP_END
};
#endif /* MQ200_SETUPREGS */

static struct mq200_clock_setting mcr530_clocks[] = {
	/* CRT: off	FP: off	*/
	{
		MQ200_CLOCK_PLL1,	/* memory clock			*/
		MQ200_CLOCK_PLL1,	/* graphics engine clock	*/
		{
		0,			/* GC1(CRT)	clock		*/
		0,			/* GC2(FP)	clock		*/
		},
		30000,			/* PLL1	30MHz			*/
		0,			/* PLL2	disable			*/
		0,			/* PLL3	disable			*/
	},
	/* CRT: on	FP: off	*/
	{
		MQ200_CLOCK_PLL1,	/* memory clock			*/
		MQ200_CLOCK_PLL2,	/* graphics engine clock	*/
		{
		MQ200_CLOCK_PLL3,	/* GC1(CRT)	clock		*/
		0,			/* GC2(FP)	clock		*/
		},
		83000,			/* PLL1	83MHz			*/
		30000,			/* PLL2	30MHz			*/
		-1,			/* PLL3	will be set by GC1	*/
	},
	/* CRT: off	FP: on	*/
	{
		MQ200_CLOCK_PLL1,	/* memory clock			*/
		MQ200_CLOCK_PLL2,	/* graphics engine clock	*/
		{
		0,			/* GC1(CRT)	clock		*/
		MQ200_CLOCK_PLL2,	/* GC2(FP)	clock		*/
		},
		30000,			/* PLL1	30MHz			*/
		18800,			/* PLL2	18.8MHz			*/
		0,			/* PLL3	disable			*/
	},
	/* CRT: on	FP: on	*/
	{
		MQ200_CLOCK_PLL1,	/* memory clock			*/
		MQ200_CLOCK_PLL2,	/* graphics engine clock	*/
		{
		MQ200_CLOCK_PLL3,	/* GC1(CRT)	clock		*/
		MQ200_CLOCK_PLL2,	/* GC2(FP)	clock		*/
		},
		83000,			/* PLL1	83MHz			*/
		18800,			/* PLL2	18.8MHz			*/
		-1,			/* PLL3	will be set by GC1	*/
	},
};

static struct mq200_md_param machdep_params[] = {
	{ 
		&platid_mask_MACH_NEC_MCR_530,
		640, 240,	/* flat panel size		*/
		12288,		/* base clock is 12.288 MHz	*/
		MQ200_MD_HAVECRT | MQ200_MD_HAVEFP,
#if MQ200_SETUPREGS
		mcr530_init_ops,
#else
		NULL,
#endif /* MQ200_SETUPREGS */
		mcr530_clocks,
		/* DCMISC	*/
		MQ200_DCMISC_OSC_ENABLE |
		MQ200_DCMISC_FASTPOWSEQ_DISABLE |
		MQ200_DCMISC_OSCFREQ_12_25,
		/* PMC		*/
		0,
		/* MM01		*/
		MQ200_MM01_DRAM_AUTO_REFRESH_EN |
		MQ200_MM01_GE_PB_EN |
		MQ200_MM01_CPU_PB_EN |
		MQ200_MM01_SLOW_REFRESH_EN |
		(0x143e << MQ200_MM01_REFRESH_SHIFT),
	},
};

void
mq200_mdsetup(struct mq200_softc *sc)
{
	const struct mq200_md_param *mdp;

	sc->sc_md = NULL;
	for (mdp = machdep_params; mdp->md_platform != NULL; mdp++) {
		platid_mask_t mask;
		mask = PLATID_DEREF(mdp->md_platform);
		if (platid_match(&platid, &mask)) {
			sc->sc_md = mdp;
			break;
		}
	}

	if (sc->sc_md) {
		sc->sc_width[MQ200_GC2] = mdp->md_fp_width;
		sc->sc_height[MQ200_GC2] = mdp->md_fp_height;
		sc->sc_baseclock = mdp->md_baseclock;

		sc->sc_regctxs[MQ200_I_DCMISC	].val = mdp->md_init_dcmisc;
		sc->sc_regctxs[MQ200_I_PMC	].val = mdp->md_init_pmc;
		sc->sc_regctxs[MQ200_I_MM01	].val = mdp->md_init_mm01;

#if MQ200_SETUPREGS
		mq200_setupregs(sc, mdp->md_init_ops);
#endif
	}
}

#if MQ200_SETUPREGS
static void
mq200_setupregs(struct mq200_softc *sc, u_int32_t *ops)
{
	u_int32_t reg, mask, accum;

	while (1) {
		switch (ops[0] & 0x3) {
		case 0:
			if (mask == ~0) {
				mq200_write(sc, ops[0], ops[1]);
			} else {
				reg = mq200_read(sc, ops[0]);
				reg = (reg & ~mask) | (ops[1] & mask);
				mq200_write(sc, ops[0], reg);
			}
			break;
		case 1:
			switch (ops[0]) {
			case OP_END:
				return;
			case OP_MASK:
				mask = ops[1];
				break;
			case OP_LOADPLLPARAM:
				mq200_pllparam(ops[1], &accum);
				break;
			case OP_LOADFROMREG:
				reg = mq200_read(sc, ops[1]);
				accum = (accum & ~mask) | (reg & mask);
				break;
			case OP_STORETOREG:
				if (mask == ~0) {
					mq200_write(sc, ops[1], accum);
				} else {
					reg = mq200_read(sc, ops[1]);
					reg = (reg & ~mask) | (accum & mask);
					mq200_write(sc, ops[1], reg);
				}
				break;
			case OP_LOADIMM:
				accum = (accum & ~mask) | (ops[1] & mask);
				break;
			case OP_OR:
				accum = (accum | ops[1]);
				break;
			}
			break;
		}
		if (ops[0] != OP_MASK)
			mask = ~0;
		ops += 2;
	}
}
#endif /* MQ200_SETUPREGS */
