/*	$NetBSD: nbppm.c,v 1.1 2011/08/06 03:53:40 kiyohara Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nbppm.c,v 1.1 2011/08/06 03:53:40 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/config_hook.h>

#include <arm/xscale/pxa2x0_gpio.h>

#include <hpcarm/dev/nbppconvar.h>

#include "locators.h"

struct nbppm_softc {
	device_t sc_dev;
	device_t sc_parent;
	int sc_tag;
};

static int nbppm_match(device_t, cfdata_t, void *);
static void nbppm_attach(device_t, device_t, void *);

static int nbppm_critical_intr(void *);
static int nbppm_suspend_intr(void *);

CFATTACH_DECL_NEW(nbppm, sizeof(struct nbppm_softc), 
    nbppm_match, nbppm_attach, NULL, NULL);


/* ARGSUSED */
static int
nbppm_match(device_t parent, cfdata_t match, void *aux)
{
	struct nbppcon_attach_args *pcon = aux;

	if (strcmp(pcon->aa_name, match->cf_name) ||
	    pcon->aa_tag == NBPPCONCF_TAG_DEFAULT)
		return 0;

	return 1;
}

static void
nbppm_attach(device_t parent, device_t self, void *aux)
{
	struct nbppm_softc *sc = device_private(self);
	struct nbppcon_attach_args *pcon = aux;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_parent = parent;
	sc->sc_tag = pcon->aa_tag;

	/* GPIO 0 is Critical Suspend */
	pxa2x0_gpio_set_function(0, GPIO_IN);
	if (pxa2x0_gpio_intr_establish(0, IST_EDGE_RISING, IPL_HIGH,
					nbppm_critical_intr, sc) == NULL)
		aprint_error_dev(self,
		    "unable to establish critical interrupt\n");

	/* GPIO 1 is Suspend */
	pxa2x0_gpio_set_function(1, GPIO_IN);
	if (pxa2x0_gpio_intr_establish(1, IST_EDGE_BOTH, IPL_HIGH,
						nbppm_suspend_intr, sc) == NULL)
		aprint_error_dev(self,
		    "unable to establish suspend interrupt\n");

	return;
}

static int
nbppm_critical_intr(void *arg)
{
	struct nbppm_softc *sc = arg;

	aprint_normal_dev(sc->sc_dev, "Battery Low\n");
	return 0;
}

static int
nbppm_suspend_intr(void *arg)
{
	struct nbppm_softc *sc = arg;

	aprint_verbose_dev(sc->sc_dev, "lid closed\n");

	return 0;
}
