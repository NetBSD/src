/*	$NetBSD: txsim.c,v 1.1.2.1 1999/12/27 18:32:14 wrstuden Exp $ */

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

/*
 * TX System Internal Module.
 */
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <hpcmips/tx/tx39var.h>

int	txsim_match __P((struct device*, struct cfdata*, void*));
void	txsim_attach __P((struct device*, struct device*, void*));
int	txsim_print __P((void*, const char*));
int	txsim_search __P((struct device*, struct cfdata*, void*));

struct txsim_softc {
	struct	device sc_dev;
	int sc_pri; /* attaching device priority */
};

struct cfattach txsim_ca = {
	sizeof(struct txsim_softc), txsim_match, txsim_attach
};

int
txsim_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
    
	if (strcmp(ma->ma_name, match->cf_driver->cd_name))
		return 0;
	return 1;
}

void
txsim_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_softc *sc = (void*)self;

	printf("\n");
	/* 
	 *	interrupt, clock module is used by other system module. 
	 *	so attach first.
	 */
	sc->sc_pri = 2;
	config_search(txsim_search, self, txsim_print);
	/* 
	 *	Other system module. 
	 */
	sc->sc_pri = 1;
	config_search(txsim_search, self, txsim_print);
}

int
txsim_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

int
txsim_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct txsim_softc *sc = (void*)parent;
	struct txsim_attach_args ta;
	
	ta.ta_tc = tx_conf_get_tag();
	
	if ((*cf->cf_attach->ca_match)(parent, cf, &ta) == sc->sc_pri) {
		config_attach(parent, cf, &ta, txsim_print);
	}

	return 0;
}
