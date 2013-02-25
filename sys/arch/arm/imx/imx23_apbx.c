/* $Id: imx23_apbx.c,v 1.1.6.2 2013/02/25 00:28:27 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <arm/mainbus/mainbus.h>

#include <arm/imx/imx23var.h>

#include "locators.h"

static int	apbx_match(device_t, cfdata_t, void *);
static void	apbx_attach(device_t, device_t, void *);
static int	apbx_detach(device_t, int);
static int	apbx_activate(device_t, enum devact );

static int	apbx_search_cb(device_t, cfdata_t, const int *, void *);
static int	apbx_search_crit_cb(device_t, cfdata_t, const int *, void *);
static int	apbx_print(void *,const char *);

struct apbx_softc {
	device_t	sc_dev;
	device_t	dmac; /* DMA controller device for this bus. */
};

CFATTACH_DECL3_NEW(apbx,
	sizeof(struct apbx_softc),
	apbx_match,
	apbx_attach,
	apbx_detach,
	apbx_activate,
	NULL,
	NULL,
	0);

static int
apbx_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *mb = aux;

	if ((mb->mb_iobase == APBX_BASE) && (mb->mb_iosize == APBX_SIZE))
		return 1;

	return 0;
}

static void
apbx_attach(device_t parent, device_t self, void *aux)
{
	struct apb_attach_args aa;
	static int apbx_attached = 0;

	if (apbx_attached)
		return;

	aa.aa_iot = &imx23_bus_space;
	aa.aa_dmat = &imx23_bus_dma_tag;

	aprint_normal("\n");

	config_search_ia(apbx_search_crit_cb, self, "apbx", &aa);
	config_search_ia(apbx_search_cb, self, "apbx", &aa);

	apbx_attached = 1;

	return;
}

static int
apbx_detach(device_t self, int flags)
{
	return 0;
}

static int
apbx_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * config_search_ia() callback function.
 */
static int
apbx_search_cb(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct apb_attach_args *aa = aux;

	aa->aa_name = cf->cf_name;
	aa->aa_addr = cf->cf_loc[APBXCF_ADDR];
	aa->aa_size = cf->cf_loc[APBXCF_SIZE];
	aa->aa_irq = cf->cf_loc[APBXCF_IRQ];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, apbx_print);

	return 0;
}

/*
 * config_search_ia() callback function which only applies to critical devices.
 */
static int
apbx_search_crit_cb(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct apb_attach_args *aa = aux;

	/* Return if not critical device. */
	if ((strcmp(cf->cf_name, "timrot") != 0)
	    && (strcmp(cf->cf_name, "apbdma") != 0))
		return 0;

	aa->aa_name = cf->cf_name;
	aa->aa_addr = cf->cf_loc[APBXCF_ADDR];
	aa->aa_size = cf->cf_loc[APBXCF_SIZE];
	aa->aa_irq = cf->cf_loc[APBXCF_IRQ];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, apbx_print);

	return 0;
}

/*
 * Called from config_attach()
 */
static int
apbx_print(void *aux, const char *name __unused)
{
	struct apb_attach_args *aa = aux;

	if (aa->aa_addr != APBXCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%lx", aa->aa_addr);
		if (aa->aa_size > APBXCF_SIZE_DEFAULT)
			aprint_normal("-0x%lx", aa->aa_addr + aa->aa_size-1);
	}

	if (aa->aa_irq != APBXCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", aa->aa_irq);
	
	return UNCONF;
}
