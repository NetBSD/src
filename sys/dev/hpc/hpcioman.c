/*	$NetBSD: hpcioman.c,v 1.1 2001/04/30 11:42:17 takemura Exp $ */

/*-
 * Copyright (c) 1999-2001 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/hpc/hpciovar.h>
#include <dev/hpc/hpciomanvar.h>

#include "locators.h"

int	hpcioman_match(struct device *, struct cfdata *, void *);
void	hpcioman_attach(struct device *, struct device *, void *);
int	hpcioman_print(void *, const char *);
int	hpcioman_search(struct device *, struct cfdata *, void *);

struct hpcioman_softc {
	struct device sc_dev;
};

struct cfattach hpcioman_ca = {
	sizeof(struct hpcioman_softc), hpcioman_match, hpcioman_attach
};

int
hpcioman_match(struct device *parent, struct cfdata *cf, void *aux)
{
	platid_mask_t mask;

	/* select platform */
	mask = PLATID_DEREF(cf->cf_loc[HPCIOIFCF_PLATFORM]);

	return platid_match(&platid, &mask);
}

void
hpcioman_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");

	config_search(hpcioman_search, self, aux);
}

int
hpcioman_search(struct device *parent, struct cfdata *cf, void *aux)
{
	//struct hpcioman_softc *sc = (struct hpcioman_softc *)parent;
	struct hpcio_attach_args *haa = aux;
	struct hpcioman_attach_args hma;

	/* get io chip */
	hma.hma_hc = (*haa->haa_getchip)(haa->haa_sc, cf->cf_iochip);

	/* interrupt mode */
	hma.hma_intr_mode = HPCIO_INTR_HOLD;
	if (cf->cf_level != HPCIOMANCF_LEVEL_DEFAULT) {
		hma.hma_intr_mode |= HPCIO_INTR_LEVEL;
		if (cf->cf_level == 0)
			hma.hma_intr_mode |= HPCIO_INTR_LOW;
		else
			hma.hma_intr_mode |= HPCIO_INTR_HIGH;
	} else {
		hma.hma_intr_mode |= HPCIO_INTR_EDGE;
		switch (cf->cf_edge) {
		case 1:
			hma.hma_intr_mode |= HPCIO_INTR_POSEDGE;
			break;
		case 2:
			hma.hma_intr_mode |= HPCIO_INTR_NEGEDGE;
			break;
		case 0:
		case 3:
		case HPCIOMANCF_EDGE_DEFAULT:
			hma.hma_intr_mode |= HPCIO_INTR_POSEDGE;
			hma.hma_intr_mode |= HPCIO_INTR_NEGEDGE;
			break;
		default:
			printf("%s(%d): invalid configuration, edge=%d",
			    __FILE__, __LINE__, cf->cf_edge);
			break;
		}
	}

	hma.hma_type = cf->cf_type;
	hma.hma_id = cf->cf_id;
	hma.hma_port = cf->cf_port;
	if (cf->cf_active == 0) {
		hma.hma_on = 0;
		hma.hma_off = 1;
	} else {
		hma.hma_on = 1;
		hma.hma_off = 0;
	}
	hma.hma_initvalue = -1; /* none */
	if (cf->cf_initvalue != HPCIOMANCF_INITVALUE_DEFAULT) {
		if (cf->cf_initvalue == 0)
			hma.hma_initvalue = hma.hma_off;
		else
			hma.hma_initvalue = hma.hma_on;
	}

	config_attach(parent, cf, &hma, hpcioman_print);

	return (0);
}

int
hpcioman_print(void *aux, const char *pnp)
{
	struct hpcioman_attach_args *hma = aux;
	int type = hma->hma_type;

	if (!pnp)  {
		printf(" iochip %s, port %d, type %d, id %d",
		       hma->hma_hc ? hma->hma_hc->hc_name : "not found",
		       hma->hma_port, type, hma->hma_id);
		if (type == CONFIG_HOOK_BUTTONEVENT ||
		    type == CONFIG_HOOK_PMEVENT || 
		    type == CONFIG_HOOK_EVENT) {
			if (hma->hma_intr_mode & HPCIO_INTR_EDGE)
				printf (", interrupt edge [%s%s]",
				    (hma->hma_intr_mode&HPCIO_INTR_POSEDGE) ? "p" : "",
				    (hma->hma_intr_mode&HPCIO_INTR_NEGEDGE) ? "n" : "");
			else
				printf (", interrupt level %s",
				    (hma->hma_intr_mode&HPCIO_INTR_HIGH) ? "high" : "low");
		}
		if (hma->hma_initvalue != -1)
			printf(", initial value %d", hma->hma_initvalue);
		if (hma->hma_on == 0)
			printf(", active low");
	}
	
	return QUIET;
}
