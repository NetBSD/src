/*	$NetBSD: s3c2800.c,v 1.2 2003/01/01 00:46:15 thorpej Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
#include <arm/s3c2xx0/s3c2800reg.h>
#include <arm/s3c2xx0/s3c2800var.h>

#include "locators.h"

/* prototypes */
static int	s3c2800_match(struct device *, struct cfdata *, void *);
static void	s3c2800_attach(struct device *, struct device *, void *);
static int	s3c2800_search(struct device *, struct cfdata *, void *);

/* attach structures */
CFATTACH_DECL(ssio, sizeof(struct s3c2800_softc), s3c2800_match, s3c2800_attach,
    NULL, NULL);

extern struct bus_space s3c2xx0_bs_tag;

struct s3c2xx0_softc *s3c2xx0_softc;

static int
s3c2800_print(void *aux, const char *name)
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
s3c2800_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

void
s3c2800_attach(struct device *parent, struct device *self, void *aux)
{
	struct s3c2800_softc *sc = (struct s3c2800_softc *) self;
	bus_space_tag_t iot;
	const char *which_registers;	/* for panic message */

#define FAIL(which)  do { \
	which_registers=(which); goto abort; }while(/*CONSTCOND*/0)

	s3c2xx0_softc = &(sc->sc_sx);
	sc->sc_sx.sc_iot = iot = &s3c2xx0_bs_tag;

	if (bus_space_map(iot,
		S3C2800_INTCTL_BASE, S3C2800_INTCTL_SIZE,
		BUS_SPACE_MAP_LINEAR, &sc->sc_sx.sc_intctl_ioh))
		FAIL("intc");
	/* tell register addresses to interrupt handler */
	s3c2800_intr_init(sc);

	/* Map the GPIO registers */
	if (bus_space_map(iot, S3C2800_GPIO_BASE, S3C2800_GPIO_SIZE,
		0, &sc->sc_sx.sc_gpio_ioh))
		FAIL("GPIO");

#if 0
	/* Map the DMA controller registers */
	if (bus_space_map(iot, S3C2800_DMAC_BASE, S3C2800_DMAC_SIZE,
		0, &sc->sc_sx.sc_dmach))
		FAIL("DMAC");
#endif

	/* Memory controller */
	if (bus_space_map(iot, S3C2800_MEMCTL_BASE,
		S3C2800_MEMCTL_SIZE, 0, &sc->sc_sx.sc_memctl_ioh))
		FAIL("MEMC");
	/* Clock manager */
	if (bus_space_map(iot, S3C2800_CLKMAN_BASE,
		S3C2800_CLKMAN_SIZE, 0, &sc->sc_sx.sc_clkman_ioh))
		FAIL("CLK");

#if 0
	/* Real time clock */
	if (bus_space_map(iot, S3C2800_RTC_BASE,
		S3C2800_RTC_SIZE, 0, &sc->sc_sx.sc_rtc_ioh))
		FAIL("RTC");
#endif

	if (bus_space_map(iot, S3C2800_TIMER0_BASE,
		S3C2800_TIMER_SIZE, 0, &sc->sc_tmr0_ioh))
		FAIL("TIMER0");

	if (bus_space_map(iot, S3C2800_TIMER1_BASE,
		S3C2800_TIMER_SIZE, 0, &sc->sc_tmr1_ioh))
		FAIL("TIMER0");


	printf("\n");


	/*
	 *  Attach devices.
	 */
	config_search(s3c2800_search, self, NULL);
	return;

abort:
	panic("%s: unable to map %s registers",
	    self->dv_xname, which_registers);

#undef FAIL
}

int
s3c2800_search(struct device * parent, struct cfdata * cf, void *aux)
{
	struct s3c2800_softc *sc = (struct s3c2800_softc *) parent;
	struct s3c2xx0_attach_args aa;

	aa.sa_sc = sc;
	aa.sa_iot = sc->sc_sx.sc_iot;
	aa.sa_addr = cf->cf_loc[SSIOCF_ADDR];
	aa.sa_size = cf->cf_loc[SSIOCF_SIZE];
	aa.sa_index = cf->cf_loc[SSIOCF_INDEX];
	aa.sa_intr = cf->cf_loc[SSIOCF_INTR];

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, s3c2800_print);

	return 0;
}

void
s3c2800_softreset(void)
{
	/* XXX */
}
