/*	$NetBSD: txsim.c,v 1.3.2.3 2001/04/21 17:53:45 bouyer Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"
/*
 * TX System Internal Module.
 */
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txsnd.h>

int	txsim_match(struct device *, struct cfdata *, void *);
void	txsim_attach(struct device *, struct device *, void *);
int	txsim_print(void *, const char*);
int	txsim_search(struct device *, struct cfdata *, void *);

struct txsim_softc {
	struct	device sc_dev;
	int sc_pri; /* attaching device priority */
};

struct cfattach txsim_ca = {
	sizeof(struct txsim_softc), txsim_match, txsim_attach
};

int
txsim_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
    
#ifdef VR41XX
	if (!platid_match(&platid, &platid_mask_CPU_MIPS_TX_3900)
	    && !platid_match(&platid, &platid_mask_CPU_MIPS_TX_3920))
		return 1;
#endif /* !TX39XX */
	if (strcmp(ma->ma_name, match->cf_driver->cd_name))
		return 0;
	return 1;
}

void
txsim_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_softc *sc = (void*)self;

	printf("\n");

	tx_sound_init(tx_conf_get_tag());
	/* 
	 * interrupt, clock module is used by other system module. 
	 * so attach first.
	 */
	sc->sc_pri = ATTACH_FIRST;
	config_search(txsim_search, self, txsim_print);
	/*
	 * unified I/O manager requires all I/O capable module already
	 * attached.
	 */
	sc->sc_pri = ATTACH_NORMAL;
	config_search(txsim_search, self, txsim_print);
	/* 
	 * UART module uses platform dependent config_hooks.
	 */
	sc->sc_pri = ATTACH_LAST;
	config_search(txsim_search, self, txsim_print);
}

int
txsim_print(void *aux, const char *pnp)
{
	return pnp ? QUIET : UNCONF;
}

int
txsim_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct txsim_softc *sc = (void*)parent;
	struct txsim_attach_args ta;
	
	ta.ta_tc = tx_conf_get_tag();
	
	if ((*cf->cf_attach->ca_match)(parent, cf, &ta) == sc->sc_pri) {
		config_attach(parent, cf, &ta, txsim_print);
	}

	return 0;
}
