/*	$NetBSD: intc.c,v 1.9 2003/07/15 03:35:58 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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

/*
 * SH-5 Interrupt Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intc.c,v 1.9 2003/07/15 03:35:58 lukem Exp $");

#include "opt_sh5_intc.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/pbridgevar.h>
#include <sh5/dev/intcvar.h>
#include <sh5/dev/intcreg.h>

/*
 * Check the IRL mode setting, from the config file via opt_sh5_intc.h
 */
#ifdef SH5_INTC_IRL_MODE_INDEP
#ifdef SH5_INTC_IRL_MODE_LEVEL
#error "Only one of SH5_INTC_IRL_MODE_INDEP and SH5_INTC_IRL_MODE_INDEP should be defined"
#endif
#else
#ifndef SH5_INTC_IRL_MODE_LEVEL
/*
 * Default to Independent Mode if no option specified
 */
#define SH5_INTC_IRL_MODE_INDEP
#endif
#endif


static int intcmatch(struct device *, struct cfdata *, void *);
static void intcattach(struct device *, struct device *, void *);

CFATTACH_DECL(intc, sizeof(struct intc_softc),
    intcmatch, intcattach, NULL, NULL);
extern struct cfdriver intc_cd;

static void intc_enable(void *, u_int, int, int);
static void intc_disable(void *, u_int);

static int8_t intevt2inum[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1,			/* 0x000 - 0x0e0 */
	-1, -1, -1, -1, -1, -1, -1, -1,			/* 0x100 - 0x1e0 */
	-1, -1,						/* 0x200 - 0x220 */
	INTC_INUM_IRL0,					/* 0x240 */
	-1, -1,						/* 0x260 - 0x280 */
	INTC_INUM_IRL1,					/* 0x2a0 */
	-1, -1,						/* 0x2c0 - 0x2e0 */
	INTC_INUM_IRL2,					/* 0x300 */
	-1, -1,						/* 0x320 - 0x340 */
	INTC_INUM_IRL3,					/* 0x360 */
	-1, -1, -1, -1,					/* 0x380 - 0x3e0 */
	INTC_INUM_TMU_TUNI0, INTC_INUM_TMU_TUNI1,	/* 0x400 - 0x420 */
	INTC_INUM_TMU_TUNI2, INTC_INUM_TMU_TICPI2,	/* 0x440 - 0x460 */
	INTC_INUM_RTC_ATI, INTC_INUM_RTC_PRI,		/* 0x480 - 0x4a0 */
	INTC_INUM_RTC_CUI,				/* 0x4c0 */
	-1, -1, -1, -1,					/* 0x4e0 - 0x540 */
	INTC_INUM_WDT_ITI,				/* 0x560 */
	-1, -1, -1, -1, -1, -1,				/* 0x580 - 0x620 */
	INTC_INUM_DMAC_DMTE0, INTC_INUM_DMAC_DMTE1,	/* 0x640 - 0x660 */
	INTC_INUM_DMAC_DMTE2, INTC_INUM_DMAC_DMTE3,	/* 0x680 - 0x6a0 */
	INTC_INUM_DMAC_DAERR,				/* 0x6c0 */
	-1,						/* 0x6e0 */
	INTC_INUM_SCIF_ERI, INTC_INUM_SCIF_RXI,		/* 0x700 - 0x720 */
	INTC_INUM_SCIF_BRI, INTC_INUM_SCIF_TXI,		/* 0x740 - 0x760 */
	-1, -1, -1, -1,					/* 0x780 - 0x7e0 */
	INTC_INUM_PCI_INTA, INTC_INUM_PCI_INTB,		/* 0x800 - 0x820 */
	INTC_INUM_PCI_INTC, INTC_INUM_PCI_INTD,		/* 0x840 - 0x860 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* 0x880 - 0x9e0 */
	INTC_INUM_PCI_SERR, INTC_INUM_PCI_ERR,		/* 0xa00 - 0xa20 */
	INTC_INUM_PCI_PWR3, INTC_INUM_PCI_PWR2,		/* 0xa40 - 0xa60 */
	INTC_INUM_PCI_PWR1, INTC_INUM_PCI_PWR0,		/* 0xa80 - 0xaa0 */
	-1, -1,						/* 0xac0 - 0xae0 */

	/* 0xb00 - 0x1fe0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};

/*ARGSUSED*/
static int
intcmatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct pbridge_attach_args *pa = args;

	return (strcmp(pa->pa_name, intc_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
intcattach(struct device *parent, struct device *self, void *args)
{
	struct pbridge_attach_args *pa = args;
	struct intc_softc *sc = (struct intc_softc *)self;

	sc->sc_bust = pa->pa_bust;
	bus_space_map(sc->sc_bust, pa->pa_offset, INTC_REG_SIZE,0,&sc->sc_bush);

	intc_reg_write(sc, INTC_REG_INTDISB(0x00), INTC_INTDISB_ALL);
	intc_reg_write(sc, INTC_REG_INTDISB(0x20), INTC_INTDISB_ALL);
	intc_reg_write(sc, INTC_REG_INTPRI(0x00), 0);
	intc_reg_write(sc, INTC_REG_INTPRI(0x20), 0);

#ifdef SH5_INTC_IRL_MODE_INDEP
	intc_reg_write(sc, INTC_REG_ICR_SET, INTC_ICR_SET_IRL_MODE_INDEP);
#else
	intc_reg_write(sc, INTC_REG_ICR_CLEAR, INTC_ICR_CLEAR_IRL_MODE_LEVEL);
#endif

	sh5_intr_init(intc_enable, intc_disable, sc);

	printf(": Interrupt Controller\n");
}

static void
intc_enable(void *arg, u_int intevt, int trigger, int level)
{
	struct intc_softc *sc = arg;
	u_int32_t reg;
	int inum;
	int s;

	KDASSERT(trigger == IST_LEVEL);
	KDASSERT(level > 0 && level < NIPL);

	intevt >>= 5;

#ifdef DEBUG
	if (intevt >= 0x100 || intevt2inum[intevt] < 0)
		panic("intc_enable: Invalid INTEVT: 0x%x", intevt << 5);
#endif

	inum = intevt2inum[intevt];

	s = splhigh();

	/*
	 * Program the priority for this interrupt
	 */
	reg = intc_reg_read(sc, INTC_REG_INTPRI(inum));
	reg &= ~(INTC_INTPRI_MASK << INTC_INTPRI_SHIFT(inum));
	reg |= level << INTC_INTPRI_SHIFT(inum);
	intc_reg_write(sc, INTC_REG_INTPRI(inum), reg);

	/*
	 * Enable the interrupt
	 */
	intc_reg_write(sc, INTC_REG_INTENB(inum), INTC_INTENB_BIT(inum));

	splx(s);
}

static void
intc_disable(void *arg, u_int intevt)
{
	struct intc_softc *sc = arg;
	u_int32_t reg;
	int inum;
	int s;

	intevt >>= 5;

#ifdef DEBUG
	if (intevt >= 0x100 || intevt2inum[intevt] < 0)
		panic("intc_disable: Invalid INTEVT: 0x%x", intevt << 5);
#endif

	s = splhigh();

	inum = intevt2inum[intevt];

	/*
	 * Disable the interrupt
	 */
	intc_reg_write(sc, INTC_REG_INTDISB(inum), INTC_INTDISB_BIT(inum));

	/*
	 * Set the priority to zero
	 */
	reg = intc_reg_read(sc, INTC_REG_INTPRI(inum));
	reg &= ~(INTC_INTPRI_MASK << INTC_INTPRI_SHIFT(inum));
	intc_reg_write(sc, INTC_REG_INTPRI(inum), reg);

	splx(s);
}
