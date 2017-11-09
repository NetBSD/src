/*	$NetBSD: imx6_axi.c,v 1.4 2017/11/09 05:57:23 hkenken Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_axi.c,v 1.4 2017/11/09 05:57:23 hkenken Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>

#include "bus_dma_generic.h"
#include "locators.h"

struct axi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_dma_tag_t sc_dmat;
};

static int axi_match(device_t, struct cfdata *, void *);
static void axi_attach(device_t, device_t, void *);
static int axi_search(device_t, struct cfdata *, const int *, void *);
static int axi_critical_search(device_t, struct cfdata *, const int *, void *);
static int axi_search(device_t, struct cfdata *, const int *, void *);
static int axi_print(void *, const char *);

CFATTACH_DECL_NEW(axi, sizeof(struct axi_softc),
    axi_match, axi_attach, NULL, NULL);

/* ARGSUSED */
static int
axi_match(device_t parent __unused, struct cfdata *match __unused,
    void *aux __unused)
{
	return 1;
}

/* ARGSUSED */
static void
axi_attach(device_t parent __unused, device_t self, void *aux __unused)
{
	struct axi_softc *sc;
	struct axi_attach_args aa;

	aprint_normal(": Advanced eXtensible Interface\n");
	aprint_naive("\n");

	sc = device_private(self);
	sc->sc_iot = &armv7_generic_bs_tag;
#if NBUS_DMA_GENERIC > 0
	sc->sc_dmat = &armv7_generic_dma_tag;
#else
	sc->sc_dmat = 0;
#endif

	aa.aa_name = "axi";
	aa.aa_iot = sc->sc_iot;
	aa.aa_dmat = sc->sc_dmat;
	config_search_ia(axi_critical_search, self, "axi", &aa);
	config_search_ia(axi_search, self, "axi", &aa);
}

/* ARGSUSED */
static int
axi_critical_search(device_t parent, struct cfdata *cf,
    const int *ldesc __unused, void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	if ((strcmp(cf->cf_name, "imxccm") != 0) &&
	    (strcmp(cf->cf_name, "imxgpio") != 0) &&
	    (strcmp(cf->cf_name, "imxiomux") != 0) &&
	    (strcmp(cf->cf_name, "imxocotp") != 0) &&
	    (strcmp(cf->cf_name, "imxuart") != 0) &&
	    (strcmp(cf->cf_name, "imxusbphy") != 0))
		return 0;

	aa->aa_name = cf->cf_name;
	aa->aa_addr = cf->cf_loc[AXICF_ADDR];
	aa->aa_size = cf->cf_loc[AXICF_SIZE];
	aa->aa_irq = cf->cf_loc[AXICF_IRQ];
	aa->aa_irqbase = cf->cf_loc[AXICF_IRQBASE];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, axi_print);

	return 0;
}

/* ARGSUSED */
static int
axi_search(device_t parent, struct cfdata *cf, const int *ldesc __unused,
    void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	aa->aa_addr = cf->cf_loc[AXICF_ADDR];
	aa->aa_size = cf->cf_loc[AXICF_SIZE];
	aa->aa_irq = cf->cf_loc[AXICF_IRQ];
	aa->aa_irqbase = cf->cf_loc[AXICF_IRQBASE];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, axi_print);

	return 0;
}

/* ARGSUSED */
static int
axi_print(void *aux, const char *name __unused)
{
	struct axi_attach_args *aa = (struct axi_attach_args *)aux;

	if (aa->aa_addr != AXICF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%lx", aa->aa_addr);
		if (aa->aa_size > AXICF_SIZE_DEFAULT)
			aprint_normal("-0x%lx",
			    aa->aa_addr + aa->aa_size-1);
	}
	if (aa->aa_irq != AXICF_IRQ_DEFAULT)
		aprint_normal(" intr %d", aa->aa_irq);
	if (aa->aa_irqbase != AXICF_IRQBASE_DEFAULT)
		aprint_normal(" irqbase %d", aa->aa_irqbase);

	return (UNCONF);
}
