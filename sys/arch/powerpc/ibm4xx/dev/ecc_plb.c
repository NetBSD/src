/*	$NetBSD: ecc_plb.c,v 1.10 2005/12/24 22:45:36 perry Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ecc_plb.c,v 1.10 2005/12/24 22:45:36 perry Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/properties.h>

#include <machine/cpu.h>
#include <powerpc/ibm4xx/dcr405gp.h>
#include <powerpc/ibm4xx/dev/plbvar.h>


struct ecc_plb_softc {
	struct device sc_dev;
	u_quad_t sc_ecc_tb;
	u_quad_t sc_ecc_iv;	 /* Interval */
	u_int32_t sc_ecc_cnt;
	u_int sc_memsize;
	int sc_irq;
};

static int	ecc_plbmatch(struct device *, struct cfdata *, void *);
static void	ecc_plbattach(struct device *, struct device *, void *);
static void	ecc_plb_deferred(struct device *);
static int	ecc_plb_intr(void *);

CFATTACH_DECL(ecc_plb, sizeof(struct ecc_plb_softc),
    ecc_plbmatch, ecc_plbattach, NULL, NULL);

static int ecc_plb_found;

static int
ecc_plbmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct plb_attach_args *paa = aux;

	if (strcmp(paa->plb_name, cf->cf_name) != 0)
		return (0);

	if (cf->cf_loc[PLBCF_IRQ] == PLBCF_IRQ_DEFAULT)
		panic("ecc_plbmatch: wildcard IRQ not allowed");

	paa->plb_irq = cf->cf_loc[PLBCF_IRQ];

	return (!ecc_plb_found);
}

static void
ecc_plbattach(struct device *parent, struct device *self, void *aux)
{
	struct ecc_plb_softc *sc = (struct ecc_plb_softc *)self;
	struct plb_attach_args *paa = aux;
	unsigned int processor_freq;
	unsigned int memsiz;

	ecc_plb_found++;

	if (board_info_get("processor-frequency",
		&processor_freq, sizeof(processor_freq)) == -1)
		panic("no processor-frequency");

	if (board_info_get("mem-size", &memsiz, sizeof(memsiz)) == -1)
		panic("no mem-size");

	printf(": ECC controller\n");

	sc->sc_ecc_tb = 0;
	sc->sc_ecc_cnt = 0;
	sc->sc_ecc_iv = processor_freq; /* Set interval */
	sc->sc_memsize = memsiz;
	sc->sc_irq = paa->plb_irq;

	/*
	 * Defer hooking the interrupt until all PLB devices have attached
	 * since the interrupt controller may well be one of those devices...
	 */
	config_defer(self, ecc_plb_deferred);
}

static void
ecc_plb_deferred(struct device *self)
{
	struct ecc_plb_softc *sc = (struct ecc_plb_softc *)self;

	intr_establish(sc->sc_irq, IST_LEVEL, IPL_SERIAL, ecc_plb_intr, NULL);
}

/*
 * ECC fault handler.
 */
static int
ecc_plb_intr(void *arg)
{
	struct ecc_plb_softc *sc = arg;
	u_int32_t		esr, ear;
	int			ce, ue;
	u_quad_t		tb;
	u_long			tmp, msr, dat;

	/* This code needs to be improved to handle double-bit errors */
	/* in some intelligent fashion. */

	mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
	esr = mfdcr(DCR_SDRAM0_CFGDATA);

	mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_BEAR);
	ear = mfdcr(DCR_SDRAM0_CFGDATA);

	/* Always clear the error to stop the intr ASAP. */

	mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
	mtdcr(DCR_SDRAM0_CFGDATA, 0xffffffff);

	if (esr == 0x00) {
		/* No current error.  Could happen due to intr. nesting */
		return(1);
	}

	/*
	 * Only report errors every once per second max. Do this using the TB,
	 * because the system time (via microtime) may be adjusted when the
	 * date is set and can't reliably be used to measure intervals.
	 */

	__asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b"
		: "=r"(tb), "=r"(tmp));
	sc->sc_ecc_cnt++;

	if ((tb - sc->sc_ecc_tb) < sc->sc_ecc_iv)
		return(1);

	ce = (esr & SDRAM0_ECCESR_CE) != 0x00;
	ue = (esr & SDRAM0_ECCESR_UE) != 0x00;

	printf("ECC: Error CNT=%d ESR=%x EAR=%x %s BKNE=%d%d%d%d "
		"BLCE=%d%d%d%d CBE=%d%d.\n",
		sc->sc_ecc_cnt, esr, ear,
		(ue) ? "Uncorrectable" : "Correctable",
		((esr & SDRAM0_ECCESR_BKEN(0)) != 0x00),
		((esr & SDRAM0_ECCESR_BKEN(1)) != 0x00),
		((esr & SDRAM0_ECCESR_BKEN(2)) != 0x00),
		((esr & SDRAM0_ECCESR_BKEN(3)) != 0x00),
		((esr & SDRAM0_ECCESR_BLCEN(0)) != 0x00),
		((esr & SDRAM0_ECCESR_BLCEN(1)) != 0x00),
		((esr & SDRAM0_ECCESR_BLCEN(2)) != 0x00),
		((esr & SDRAM0_ECCESR_BLCEN(3)) != 0x00),
		((esr & SDRAM0_ECCESR_CBEN(0)) != 0x00),
		((esr & SDRAM0_ECCESR_CBEN(1)) != 0x00));

	/* Should check for uncorrectable errors and panic... */

	if (sc->sc_ecc_cnt > 1000) {
		printf("ECC: Too many errors, recycling entire "
			"SDRAM (size = %d).\n", sc->sc_memsize);

		/*
		 * Can this code be changed to run without disabling data MMU
		 * and disabling intrs?
		 * Does kernel always map all of physical RAM VA=PA? If so,
		 * just loop over lowmem.
		 */
		__asm volatile(
			"mfmsr 	%0;"
			"li	%1, 0x00;"
			"ori	%1, %1, 0x8010;"
			"andc	%1, %0, %1;"
			"mtmsr	%1;"
			"sync;isync;"
			"li	%1, 0x00;"
			"1:"
			"dcbt	0, %1;"
			"sync;isync;"
			"lwz	%2, 0(%1);"
			"stw	%2, 0(%1);"
			"sync;isync;"
			"dcbf	0, %1;"
			"sync;isync;"
			"addi	%1, %1, 0x20;"
			"addic.	%3, %3, -0x20;"
			"bge 	1b;"
			"mtmsr %0;"
			"sync;isync;"
		: "=&r" (msr), "=&r" (tmp), "=&r" (dat)
		: "r" (sc->sc_memsize) : "0" );

		mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
		esr = mfdcr(DCR_SDRAM0_CFGDATA);

		mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
		mtdcr(DCR_SDRAM0_CFGDATA, 0xffffffff);

		/*
		 * Correctable errors here are OK, mem should be clean now.
		 *
		 * Should check for uncorrectable errors and panic...
		 */
		printf("ECC: Recycling complete, ESR=%x. "
			"Checking for persistent errors.\n", esr);

		__asm volatile(
			"mfmsr 	%0;"
			"li	%1, 0x00;"
			"ori	%1, %1, 0x8010;"
			"andc	%1, %0, %1;"
			"mtmsr	%1;"
			"sync;isync;"
			"li	%1, 0x00;"
			"1:"
			"dcbt	0, %1;"
			"sync;isync;"
			"lwz	%2, 0(%1);"
			"stw	%2, 0(%1);"
			"sync;isync;"
			"dcbf	0, %1;"
			"sync;isync;"
			"addi	%1, %1, 0x20;"
			"addic.	%3, %3, -0x20;"
			"bge 	1b;"
			"mtmsr %0;"
			"sync;isync;"
		: "=&r" (msr), "=&r" (tmp), "=&r" (dat)
		: "r" (sc->sc_memsize) : "0" );

		mtdcr(DCR_SDRAM0_CFGADDR, DCR_SDRAM0_ECCESR);
		esr = mfdcr(DCR_SDRAM0_CFGDATA);

		/*
		 * If esr is non zero here, we're screwed.
		 * Should check this and panic.
		 */
		printf("ECC: Persistent error check complete, "
			"final ESR=%x.\n", esr);
	}

	sc->sc_ecc_tb = tb;
	sc->sc_ecc_cnt = 0;

	return(1);
}
