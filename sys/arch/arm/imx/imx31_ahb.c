/*	$NetBSD: imx31_ahb.c,v 1.4 2010/11/13 05:00:31 bsh Exp $	*/

/*
 * Copyright (c) 2002, 2005  Genetec Corporation.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$Id: imx31_ahb.c,v 1.4 2010/11/13 05:00:31 bsh Exp $");

#include "locators.h"
#include "avic.h"
#include "imxgpio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>

#include <machine/intr.h>

#include <arm/imx/imx31reg.h>
#include <arm/imx/imx31var.h>

struct ahb_softc {
	struct device sc_dev;
	bus_dma_tag_t sc_dmat;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
};

/* prototypes */
static int	ahb_match(device_t, cfdata_t, void *);
static void	ahb_attach(device_t, device_t, void *);
static int 	ahb_search(device_t, cfdata_t, const int *, void *);
static void	ahb_attach_critical(struct ahb_softc *);
static int	ahbbus_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(ahb, sizeof(struct ahb_softc),
    ahb_match, ahb_attach, NULL, NULL);

static struct ahb_softc *ahb_sc;

static int
ahb_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
ahb_attach(device_t parent, device_t self, void *aux)
{
	struct ahb_softc *sc = (struct ahb_softc *)self;
	struct ahb_attach_args ahba;

	ahb_sc = sc;
	sc->sc_memt = &imx_bs_tag;
#if NBUS_DMA_GENERIC > 0
	sc->sc_dmat = &imx_bus_dma_tag;
#else
	sc->sc_dmat = 0;
#endif

	aprint_normal(": AHB-Lite 2.v6 bus interface\n");

	/*
	 * Attach critical devices
	 */
	ahb_attach_critical(sc);

	/*
	 * Attach all other devices
	 */
	ahba.ahba_name = "ahb";
	ahba.ahba_memt = sc->sc_memt;
	ahba.ahba_dmat = sc->sc_dmat;
	config_search_ia(ahb_search, self, "ahb", &ahba);
}

static int
ahb_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct ahb_attach_args * const ahba = aux;

	ahba->ahba_addr = cf->cf_loc[AHBCF_ADDR];
	ahba->ahba_size = cf->cf_loc[AHBCF_SIZE];
	ahba->ahba_intr = cf->cf_loc[AHBCF_INTR];
	ahba->ahba_irqbase = cf->cf_loc[AHBCF_IRQBASE];

	if (config_match(parent, cf, ahba))
		config_attach(parent, cf, ahba, ahbbus_print);

	return 0;
}

static int
ahb_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct ahb_attach_args * const ahba = aux;
	struct ahb_attach_args ahba0;

	if (strcmp(ahba->ahba_name, "ahb"))
		return 0;
	if (ahba->ahba_addr != AHBCF_ADDR_DEFAULT
	    && ahba->ahba_addr != cf->cf_loc[AHBCF_ADDR])
		return 0;
	if (ahba->ahba_size != AHBCF_SIZE_DEFAULT
	    && ahba->ahba_size != cf->cf_loc[AHBCF_SIZE])
		return 0;
	if (ahba->ahba_intr != AHBCF_INTR_DEFAULT
	    && ahba->ahba_intr != cf->cf_loc[AHBCF_INTR])
		return 0;
	if (ahba->ahba_irqbase != AHBCF_IRQBASE_DEFAULT
	    && ahba->ahba_irqbase != cf->cf_loc[AHBCF_IRQBASE])
		return 0;

	ahba0 = *ahba;
	ahba0.ahba_addr = cf->cf_loc[AHBCF_ADDR];
	ahba0.ahba_size = cf->cf_loc[AHBCF_SIZE];
	ahba0.ahba_intr = cf->cf_loc[AHBCF_INTR];
	ahba0.ahba_irqbase = cf->cf_loc[AHBCF_IRQBASE];

	return config_match(parent, cf, &ahba0);
}

#if NAVIC == 0
#error no avic present in config file
#endif

static const struct {
	const char *name;
	bus_addr_t addr;
	bool required;
} critical_devs[] = {
	{ .name = "avic", .addr = INTC_BASE, .required = true },
	{ .name = "gpio1", .addr = GPIO1_BASE, .required = false },
	{ .name = "gpio2", .addr = GPIO2_BASE, .required = false },
	{ .name = "gpio3", .addr = GPIO3_BASE, .required = false },
#if 0
	{ .name = "dmac", .addr = DMAC_BASE, .required = true },
#endif
};

static void
ahb_attach_critical(struct ahb_softc *sc)
{
	struct ahb_attach_args ahba;
	cfdata_t cf;
	size_t i;

	for (i = 0; i < __arraycount(critical_devs); i++) {
		ahba.ahba_name = "ahb";
		ahba.ahba_memt = sc->sc_memt;
		ahba.ahba_dmat = sc->sc_dmat;

		ahba.ahba_addr = critical_devs[i].addr;
		ahba.ahba_size = AHBCF_SIZE_DEFAULT;
		ahba.ahba_intr = AHBCF_INTR_DEFAULT;
		ahba.ahba_irqbase = AHBCF_IRQBASE_DEFAULT;

		cf = config_search_ia(ahb_find, &sc->sc_dev, "ahb", &ahba);
		if (cf == NULL && critical_devs[i].required)
			panic("ahb_attach_critical: failed to find %s!",
			    critical_devs[i].name);

		ahba.ahba_addr = cf->cf_loc[AHBCF_ADDR];
		ahba.ahba_size = cf->cf_loc[AHBCF_SIZE];
		ahba.ahba_intr = cf->cf_loc[AHBCF_INTR];
		ahba.ahba_irqbase = cf->cf_loc[AHBCF_IRQBASE];
		config_attach(&sc->sc_dev, cf, &ahba, ahbbus_print);
	}
}

static int
ahbbus_print(void *aux, const char *name)
{
	struct ahb_attach_args *ahba = (struct ahb_attach_args*)aux;

	if (ahba->ahba_addr != AHBCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%lx", ahba->ahba_addr);
		if (ahba->ahba_size > AHBCF_SIZE_DEFAULT)
			aprint_normal("-0x%lx",
			    ahba->ahba_addr + ahba->ahba_size-1);
	}
	if (ahba->ahba_intr != AHBCF_INTR_DEFAULT)
		aprint_normal(" intr %d", ahba->ahba_intr);
	if (ahba->ahba_irqbase != AHBCF_IRQBASE_DEFAULT)
		aprint_normal(" irqbase %d", ahba->ahba_irqbase);

	return (UNCONF);
}

static inline uint32_t
read_clock_counter_xsc1(void)
{
	uint32_t x;
	__asm volatile("mrc	p14, 0, %0, c1, c0, 0" : "=r" (x) );

	return x;
}

static inline uint32_t
read_clock_counter_xsc2(void)
{
	uint32_t x;
	__asm volatile("mrc	p14, 0, %0, c1, c1, 0" : "=r" (x) );

	return x;
}

