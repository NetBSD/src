/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

/* Derived from s3c2410.c */
/*
 * Copyright (c) 2003, 2005  Genetec corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2440.c,v 1.1.4.2 2012/02/18 07:31:29 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/s3c2xx0/s3c2440reg.h>
#include <arm/s3c2xx0/s3c2440var.h>
#include <arm/s3c2xx0/s3c2440_dma.h>

#include "locators.h"
#include "opt_cpuoptions.h"

/* prototypes */
static int	s3c2440_match(struct device *, struct cfdata *, void *);
static void	s3c2440_attach(struct device *, struct device *, void *);
static int	s3c2440_search(struct device *, struct cfdata *,
			       const int *, void *);

/* attach structures */
CFATTACH_DECL_NEW(ssio, sizeof(struct s3c24x0_softc), s3c2440_match, s3c2440_attach,
    NULL, NULL);

extern struct bus_space s3c2xx0_bs_tag;

struct s3c2xx0_softc *s3c2xx0_softc;

#ifdef DEBUG_PORTF
volatile uint8_t *portf;	/* for debug */
#endif

static int
s3c2440_print(void *aux, const char *name)
{
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *) aux;

	if (sa->sa_size)
		aprint_normal(" addr 0x%lx", sa->sa_addr);
	if (sa->sa_size > 1)
		aprint_normal("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_intr != SSIOCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->sa_intr);
	if (sa->sa_index != SSIOCF_INDEX_DEFAULT)
		aprint_normal(" unit %d", sa->sa_index);

	return (UNCONF);
}

int
s3c2440_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

void
s3c2440_attach(struct device *parent, struct device *self, void *aux)
{
	struct s3c24x0_softc *sc = device_private(self);
	bus_space_tag_t iot;
	const char *which_registers;	/* for panic message */

#define FAIL(which)  do { \
	which_registers=(which); goto abort; }while(/*CONSTCOND*/0)

	s3c2xx0_softc = &(sc->sc_sx);
	sc->sc_sx.sc_iot = iot = &s3c2xx0_bs_tag;

	if (bus_space_map(iot,
		S3C2440_INTCTL_BASE, S3C2440_INTCTL_SIZE,
		BUS_SPACE_MAP_LINEAR, &sc->sc_sx.sc_intctl_ioh))
		FAIL("intc");
	/* tell register addresses to interrupt handler */
	s3c2440_intr_init(sc);

	/* Map the GPIO registers */
	if (bus_space_map(iot, S3C2440_GPIO_BASE, S3C2440_GPIO_SIZE,
		0, &sc->sc_sx.sc_gpio_ioh))
		FAIL("GPIO");
#ifdef DEBUG_PORTF
	{
		extern volatile uint8_t *portf;
		/* make all ports output */
		bus_space_write_2(iot, sc->sc_sx.sc_gpio_ioh, GPIO_PCONF, 0x5555);
		portf = (volatile uint8_t *)
			((char *)bus_space_vaddr(iot, sc->sc_sx.sc_gpio_ioh) + GPIO_PDATF);
	}
#endif

#if 1
	/* Map the DMA controller registers */
	if (bus_space_map(iot, S3C2440_DMAC_BASE, S3C2440_DMAC_SIZE,
		0, &sc->sc_sx.sc_dmach))
		FAIL("DMAC");
#endif

	/* Memory controller */
	if (bus_space_map(iot, S3C2440_MEMCTL_BASE,
		S3C24X0_MEMCTL_SIZE, 0, &sc->sc_sx.sc_memctl_ioh))
		FAIL("MEMC");
	/* Clock manager */
	if (bus_space_map(iot, S3C2440_CLKMAN_BASE,
		S3C24X0_CLKMAN_SIZE, 0, &sc->sc_sx.sc_clkman_ioh))
		FAIL("CLK");

#if 0
	/* Real time clock */
	if (bus_space_map(iot, S3C2410_RTC_BASE,
		S3C24X0_RTC_SIZE, 0, &sc->sc_sx.sc_rtc_ioh))
		FAIL("RTC");
#endif

	if (bus_space_map(iot, S3C2440_TIMER_BASE,
		S3C24X0_TIMER_SIZE, 0, &sc->sc_timer_ioh)) {
		FAIL("TIMER");
        }

	/* calculate current clock frequency */
	s3c24x0_clock_freq(&sc->sc_sx);
	aprint_normal(": fclk %d MHz hclk %d MHz pclk %d MHz\n",
	       sc->sc_sx.sc_fclk / 1000000, sc->sc_sx.sc_hclk / 1000000,
	       sc->sc_sx.sc_pclk / 1000000);

	aprint_naive("\n");

	/* get busdma tag for the platform */
	sc->sc_sx.sc_dmat = s3c2xx0_bus_dma_init(&s3c2xx0_bus_dma);

	s3c2440_dma_init();
	
	/*
	 *  Attach devices.
	 */
	config_search_ia(s3c2440_search, self, "ssio", NULL);
	return;

abort:
	panic("%s: unable to map %s registers",
	    self->dv_xname, which_registers);

#undef FAIL
}

int
s3c2440_search(struct device * parent, struct cfdata * cf,
	       const int *ldesc, void *aux)
{
	struct s3c24x0_softc *sc = device_private(parent);
	struct s3c2xx0_attach_args aa;

	aa.sa_sc = sc;
	aa.sa_iot = sc->sc_sx.sc_iot;
	aa.sa_addr = cf->cf_loc[SSIOCF_ADDR];
	aa.sa_size = cf->cf_loc[SSIOCF_SIZE];
	aa.sa_index = cf->cf_loc[SSIOCF_INDEX];
	aa.sa_intr = cf->cf_loc[SSIOCF_INTR];

	aa.sa_dmat = sc->sc_sx.sc_dmat;

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, s3c2440_print);

	return 0;
}

/*
 * fill sc_pclk, sc_hclk, sc_fclk from values of clock controller register.
 *
 * s3c24x0_clock_freq2() is meant to be called from kernel startup routines.
 * s3c24x0_clock_freq() is for after kernel initialization is done.
 */
void
s3c24x0_clock_freq2(vaddr_t clkman_base, int *fclk, int *hclk, int *pclk)
{
	uint32_t pllcon, divn, camdivn;
	int mdiv, pdiv, sdiv;
	uint32_t f, h, p;

	pllcon = *(volatile uint32_t *)(clkman_base + CLKMAN_MPLLCON);
	divn = *(volatile uint32_t *)(clkman_base + CLKMAN_CLKDIVN);
        camdivn = *(volatile uint32_t *)(clkman_base + CLKMAN_CAMDIVN);

	mdiv = (pllcon & PLLCON_MDIV_MASK) >> PLLCON_MDIV_SHIFT;
	pdiv = (pllcon & PLLCON_PDIV_MASK) >> PLLCON_PDIV_SHIFT;
	sdiv = (pllcon & PLLCON_SDIV_MASK) >> PLLCON_SDIV_SHIFT;

	f = ((mdiv + 8) * S3C2XX0_XTAL_CLK) / ((pdiv + 2) * (1 << sdiv)) * 2;
	h = f;

        /* HDIVN of CLKDIVN can have 4 distinct values */
        switch( (divn & CLKDIVN_HDIVN_MASK) >> CLKDIVN_HDIVN_SHIFT )
        {
          case 0:
            /* 00b: HCLK = FCLK/1*/
            break;
          case 1:
            /* 01b: HCLK = FCLK/2*/
            h /= 2;
            break;
          case 2:
            /* 10b: HCLK = FCLK/4 when CAMDIVN[9] (HCLK4_HALF) = 0
             *      HCLK = FCLK/8 when CAMDIVN[9] (HCLK4_HALF) = 1 */
            if( camdivn & CLKCAMDIVN_HCLK4_HALF )
              h /= 8;
            else
              h /= 4;
            break;
          case 3:
            /* 11b: HCLK = FCLK/3 when CAMDIVN[8] (HCLK3_HALF) = 0
             *      HCLK = FCLK/6 when CAMDIVN[8] (HCLK3_HALF) = 1 */
            if( camdivn & CLKCAMDIVN_HCLK3_HALF )
              h /= 6;
            else
              h /= 3;
            break;
        }

	p = h;

	if (divn & CLKDIVN_PDIVN)
		p /= 2;

	if (fclk) *fclk = f;
	if (hclk) *hclk = h;
	if (pclk) *pclk = p;

}

void
s3c24x0_clock_freq(struct s3c2xx0_softc *sc)
{
	s3c24x0_clock_freq2(
		(vaddr_t)bus_space_vaddr(sc->sc_iot, sc->sc_clkman_ioh),
		&sc->sc_fclk, &sc->sc_hclk, &sc->sc_pclk);
}

/*
 * Issue software reset command.
 * called with MMU off.
 *
 * S3C2410 doesn't have sowtware reset bit like S3C2800.
 * use watch dog timer and make it fire immediately.
 */
void
s3c2440_softreset(void)
{
	disable_interrupts(I32_bit|F32_bit);

	*(volatile unsigned int *)(S3C2440_WDT_BASE + WDT_WTCON)
		= (0 << WTCON_PRESCALE_SHIFT) | WTCON_ENABLE |
		WTCON_CLKSEL_16 | WTCON_ENRST;
}


