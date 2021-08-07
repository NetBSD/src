/*	$NetBSD: tx39uart.c,v 1.17 2021/08/07 16:18:54 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tx39uart.c,v 1.17 2021/08/07 16:18:54 thorpej Exp $");

#include "opt_tx39uart_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39uartvar.h>

#include "locators.h"

int	tx39uart_match(device_t, cfdata_t, void *);
void	tx39uart_attach(device_t, device_t, void *);
int	tx39uart_print(void *, const char *);
int	tx39uart_search(device_t, cfdata_t, const int *, void *);

struct tx39uart_softc {
	tx_chipset_tag_t sc_tc;
	int sc_enabled;
};

CFATTACH_DECL_NEW(tx39uart, sizeof(struct tx39uart_softc),
    tx39uart_match, tx39uart_attach, NULL, NULL);

int
tx39uart_match(device_t parent, cfdata_t cf, void *aux)
{
	return ATTACH_LAST;
}

void
tx39uart_attach(device_t parent, device_t self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct tx39uart_softc *sc = device_private(self);
	tx_chipset_tag_t tc;

	printf("\n");
	sc->sc_tc = tc = ta->ta_tc;

	config_search(self, NULL,
	    CFARGS(.search = tx39uart_search));
}

int
tx39uart_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct tx39uart_softc *sc = device_private(parent);
	struct tx39uart_attach_args ua;
	
	ua.ua_tc	= sc->sc_tc;
	ua.ua_slot	= cf->cf_loc[TXCOMIFCF_SLOT];

	if (ua.ua_slot == TXCOMIFCF_SLOT_DEFAULT) {
		printf("tx39uart_search: wildcarded slot, skipping\n");
		return 0;
	}
	
	if (!(sc->sc_enabled & (1 << ua.ua_slot)) && /* not attached slot */
	    config_probe(parent, cf, &ua)) {
		config_attach(parent, cf, &ua, tx39uart_print, CFARGS_NONE);
		sc->sc_enabled |= (1 << ua.ua_slot);
	}

	return 0;
}

int
tx39uart_print(void *aux, const char *pnp)
{
	struct tx39uart_attach_args *ua = aux;
	
	aprint_normal(" slot %d", ua->ua_slot);

	return QUIET;
}
