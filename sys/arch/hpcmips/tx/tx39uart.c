/*	$NetBSD: tx39uart.c,v 1.2.2.1 1999/12/27 18:32:13 wrstuden Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "opt_tx39_debug.h"
#include "opt_tx39uartdebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39uartvar.h>

#include "locators.h"

int	tx39uart_match __P((struct device*, struct cfdata*, void*));
void	tx39uart_attach __P((struct device*, struct device*, void*));
int	tx39uart_print __P((void*, const char*));
int	tx39uart_search __P((struct device*, struct cfdata*, void*));

struct tx39uart_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
	int sc_enabled;
};

struct cfattach tx39uart_ca = {
	sizeof(struct tx39uart_softc), tx39uart_match, tx39uart_attach
};

int
tx39uart_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
tx39uart_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39uart_softc *sc = (void*)self;
	tx_chipset_tag_t tc;

	printf("\n");
	sc->sc_tc = tc = ta->ta_tc;

	config_search(tx39uart_search, self, tx39uart_print);
}

int
tx39uart_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct tx39uart_softc *sc = (void*)parent;
	struct tx39uart_attach_args ua;
	
	ua.ua_tc	= sc->sc_tc;
	ua.ua_slot	= cf->cf_loc[TXCOMIFCF_SLOT];

	if (ua.ua_slot == TXCOMIFCF_SLOT_DEFAULT) {
		printf("tx39uart_search: wildcarded slot, skipping\n");
		return 0;
	}
	
	if (!(sc->sc_enabled & (1 << ua.ua_slot)) && /* not attached slot */
	    (*cf->cf_attach->ca_match)(parent, cf, &ua)) {
		config_attach(parent, cf, &ua, tx39uart_print);
		sc->sc_enabled |= (1 << ua.ua_slot);
	}

	return 0;
}

int
tx39uart_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct tx39uart_attach_args *ua = aux;
	
	printf(" slot %d", ua->ua_slot);

	return QUIET;
}
