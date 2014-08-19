/* $Id: imx23_ahb.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $ */

/*
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

static int	ahb_match(device_t, cfdata_t, void *);
static void	ahb_attach(device_t, device_t, void *);
static int	ahb_detach(device_t, int);
static int	ahb_activate(device_t, enum devact);

static int	ahb_search_cb(device_t, cfdata_t, const int *, void *);
static int	ahb_print(void *,const char *);

CFATTACH_DECL3_NEW(ahb,
	sizeof(struct ahb_softc),
	ahb_match,
	ahb_attach,
	ahb_detach,
	ahb_activate,
	NULL,
	NULL,
	0);

static int
ahb_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *mb = aux;

	if ((mb->mb_iobase == AHB_BASE) && (mb->mb_iosize == AHB_SIZE))
		return 1;

	return 0;
}

static void
ahb_attach(device_t parent, device_t self, void *aux)
{
	struct ahb_attach_args aa;
	static int ahb_attached = 0;

	if (ahb_attached)
		return;

	aa.aa_iot = &imx23_bus_space;
	aa.aa_dmat = &imx23_bus_dma_tag;

	aprint_normal("\n");

	config_search_ia(ahb_search_cb, self, "ahb", &aa);

	ahb_attached = 1;

	return;
}

static int
ahb_detach(device_t self, int flags)
{
	return 0;
}

static int
ahb_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * config_search_ia() callback function.
 */
static int
ahb_search_cb(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct apb_attach_args *aa = aux;

	aa->aa_name = cf->cf_name;
	aa->aa_addr = cf->cf_loc[AHBCF_ADDR];
	aa->aa_size = cf->cf_loc[AHBCF_SIZE];
	aa->aa_irq = cf->cf_loc[AHBCF_IRQ];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, ahb_print);

	return 0;
}

/*
 * Called from config_attach()
 */
static int
ahb_print(void *aux, const char *name __unused)
{
	struct apb_attach_args *aa = aux;

	if (aa->aa_addr != AHBCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%lx", aa->aa_addr);
		if (aa->aa_size > AHBCF_SIZE_DEFAULT)
			aprint_normal("-0x%lx", aa->aa_addr + aa->aa_size-1);
	}

	if (aa->aa_irq != AHBCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", aa->aa_irq);
	
	return UNCONF;
}
