/*	$NetBSD: txioman.c,v 1.2.2.2 2000/11/20 20:47:40 bouyer Exp $ */

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
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txiomanvar.h>

#include "locators.h"

int	txioman_match(struct device *, struct cfdata *, void *);
void	txioman_attach(struct device *, struct device *, void *);
int	txioman_print(void *, const char *);
int	txioman_search(struct device *, struct cfdata *, void *);

struct txioman_softc {
	struct device sc_dev;
};

struct cfattach txioman_ca = {
	sizeof(struct txioman_softc), txioman_match, txioman_attach
};

int
txioman_match(struct device *parent, struct cfdata *cf, void *aux)
{
	platid_mask_t mask;

	/* select platform */
	mask = PLATID_DEREF(cf->cf_loc[TXSIMCF_PLATFORM]);
	if (platid_match(&platid, &mask)) {
		return ATTACH_NORMAL;
	}

	return 0;
}

void
txioman_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct txio_attach_args taa;
	
	taa.taa_tc = ta->ta_tc;
	printf("\n");

	config_search(txioman_search, self, &taa);
}

int
txioman_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct txio_attach_args *taa = aux;
		
	taa->taa_group	= cf->cf_group;
	taa->taa_port	= cf->cf_port;
	taa->taa_type	= cf->cf_type;
	taa->taa_id	= cf->cf_id;
	taa->taa_edge	= cf->cf_edge;
	taa->taa_initial = cf->cf_initial;
	
	config_attach(parent, cf, taa, txioman_print);

	return 0;
}

int
txioman_print(void *aux, const char *pnp)
{
	struct txio_attach_args *taa = aux;
	int edge = taa->taa_edge;
	int type = taa->taa_type;
	
	if (!pnp)  {
		printf(" group %d port %d type %d id %d", taa->taa_group,
		       taa->taa_port, type, taa->taa_id);
		if (type == CONFIG_HOOK_BUTTONEVENT ||
		    type == CONFIG_HOOK_PMEVENT || 
		    type == CONFIG_HOOK_EVENT) {
			printf (" interrupt edge [%s%s]",
				edge & 0x1 ? "p" : "", edge & 0x2 ? "n" : "");
		}
		if (taa->taa_initial != -1)
			printf(" initial %d", taa->taa_initial);
	}
	
	return QUIET;
}
