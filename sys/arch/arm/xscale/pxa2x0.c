/*	$NetBSD: pxa2x0.c,v 1.1.2.2 2002/11/11 21:56:54 nathanw Exp $ */

/*
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
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
 *
 *
 * Autoconfiguration support for the Intel PXA2[15]0 application
 * processor. This code is derived from arm/sa11x0/sa11x0.c
 */

/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 */
/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>

#include "locators.h"

/* prototypes */
static int	pxa2x0_match(struct device *, struct cfdata *, void *);
static void	pxa2x0_attach(struct device *, struct device *, void *);
static int 	pxa2x0_search(struct device *, struct cfdata *, void *);

/* attach structures */
CFATTACH_DECL(pxaip, sizeof(struct pxa2x0_softc), pxa2x0_match, pxa2x0_attach,
    NULL, NULL);

extern struct bus_space pxa2x0_bs_tag;

struct pxa2x0_softc *pxa2x0_softc;

static int
pxa2x0_print(void *aux, const char *name)
{
	struct pxa2x0_attach_args *sa = (struct pxa2x0_attach_args*)aux;

	if (sa->pxa_size)
                printf(" addr 0x%lx", sa->pxa_addr);
        if (sa->pxa_size > 1)
                printf("-0x%lx", sa->pxa_addr + sa->pxa_size - 1);
        if (sa->pxa_intr > 1)
                printf(" intr %d", sa->pxa_intr);
	if (sa->pxa_gpio != -1)
		printf(" gpio %d", sa->pxa_gpio);

        return (UNCONF);
}

int
pxa2x0_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

void
pxa2x0_attach(struct device *parent, struct device *self, void *aux)
{
	struct pxa2x0_softc *sc = (struct pxa2x0_softc*)self;
	bus_space_tag_t iot;
	const char *which_registers; /* for panic message */

#define FAIL(which)  do { \
	which_registers=(which); goto abort; }while(/*CONSTCOND*/0)

	pxa2x0_softc = sc;
	sc->saip.sc_iot = iot = &pxa2x0_bs_tag;

	if (bus_space_map(iot, 
			  PXA2X0_INTCTL_BASE, PXA2X0_INTCTL_SIZE,
			0, &sc->saip.sc_ioh))
		FAIL("intc");

	/* Map the GPIO registers */
	if (bus_space_map(iot, PXA2X0_GPIO_BASE, PXA2X0_GPIO_SIZE,
			  0, &sc->saip.sc_gpioh))
		FAIL("GPIO");

	/* Map the DMA controller registers */
	if (bus_space_map(iot, PXA2X0_DMAC_BASE, PXA2X0_DMAC_SIZE,
			  0, &sc->saip.sc_dmach))
		FAIL("DMAC");

	/* Memory controller */
	if( bus_space_map(iot, PXA2X0_MEMCTL_BASE,
			  PXA2X0_MEMCTL_SIZE, 0, &sc->sc_memctl_ioh) )
		FAIL("MEMC");
	/* Clock manager */
	if( bus_space_map(iot, PXA2X0_CLKMAN_BASE,
			  PXA2X0_CLKMAN_SIZE, 0, &sc->sc_clkman_ioh) )
		FAIL("CLK");

	/* Real time clock */
	if( bus_space_map(iot, PXA2X0_RTC_BASE,
			  PXA2X0_RTC_SIZE, 0, &sc->sc_rtc_ioh) )
		FAIL("RTC");

#if 1
	/* This takes 2 secs at most. */
	{
		int cpuclock;

		cpuclock = pxa2x0_measure_cpuclock( sc ) / 1000;
		printf( " CPU clock = %d.%03d MHz", cpuclock/1000, cpuclock%1000 );
	}
#endif
	printf("\n");

	pxa2x0_set_intcbase(sc->saip.sc_ioh);
	pxa2x0_intr_init();

	/*
	 *  Attach devices.
	 */
	config_search(pxa2x0_search, self, NULL);
	return;

 abort:
	panic("%s: unable to map %s registers\n", 
	      self->dv_xname, which_registers);

#undef FAIL
}

int
pxa2x0_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pxa2x0_softc *sc = (struct pxa2x0_softc *)parent;
	struct pxa2x0_attach_args aa;

	aa.pxa_sc = sc;
        aa.pxa_iot = sc->saip.sc_iot;
        aa.pxa_addr = cf->cf_loc[PXAIPCF_ADDR];
        aa.pxa_size = cf->cf_loc[PXAIPCF_SIZE];
	aa.pxa_index = cf->cf_loc[PXAIPCF_INDEX];
	aa.pxa_sa.sa_membase = 0;
        aa.pxa_sa.sa_memsize = 0;
        aa.pxa_intr = cf->cf_loc[PXAIPCF_INTR];
	aa.pxa_gpio = cf->cf_loc[PXAIPCF_GPIO];

        if (config_match(parent, cf, &aa))
                config_attach(parent, cf, &aa, pxa2x0_print);

        return 0;
}

static inline uint32_t
read_clock_counter(void)
{
  uint32_t x;
  __asm __volatile("mrc	p14, 0, %0, c1, c0, 0" : "=r" (x) );

  return x;
}

int
pxa2x0_measure_cpuclock( struct pxa2x0_softc *sc )
{
	uint32_t rtc0, rtc1, start, end;
	uint32_t pmcr_save;
	bus_space_tag_t iot = sc->saip.sc_iot;
	bus_space_handle_t rtc_ioh = sc->sc_rtc_ioh;
	int irq = disable_interrupts(I32_bit|F32_bit);

	__asm __volatile( "mrc p14, 0, %0, c0, c0, 0" : "=r" (pmcr_save) );
	/* Enable clock counter */
	__asm __volatile( "mcr p14, 0, %0, c0, c0, 0" : : "r" (0x0001) );

	rtc0 = bus_space_read_4( iot, rtc_ioh, RTC_RCNR );
	/* Wait for next second starts */
	while( (rtc1 = bus_space_read_4( iot, rtc_ioh, RTC_RCNR )) == rtc0 )
		;
	start = read_clock_counter();
	while( rtc1 == bus_space_read_4( iot, rtc_ioh, RTC_RCNR ) )
		;		/* Wait for 1sec */
	end = read_clock_counter();

	__asm __volatile( "mcr p14, 0, %0, c0, c0, 0" : : "r" (pmcr_save) );
	restore_interrupts(irq);

	return end - start;
}

void
pxa2x0_turbo_mode( int f )
{
  __asm __volatile("mcr p14, 0, %0, c6, c0, 0" : : "r" (f));
}
