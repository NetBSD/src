/*	$NetBSD: mca.c,v 1.2.2.1 2001/04/09 01:56:48 nathanw Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * Copyright (c) 1996-1999 Scott D. Telford.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Telford <s.telford@ed.ac.uk>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * MCA Bus device
 */

#include "opt_mcaverbose.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/mca/mcareg.h>
#include <dev/mca/mcavar.h>
#include <dev/mca/mcadevs.h>

int	mca_match __P((struct device *, struct cfdata *, void *));
void	mca_attach __P((struct device *, struct device *, void *));

struct cfattach mca_ca = {
	sizeof(struct device), mca_match, mca_attach
};

int	mca_submatch __P((struct device *, struct cfdata *, void *));
int	mca_print __P((void *, const char *));

int
mca_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mcabus_attach_args *mba = aux;

	if (strcmp(mba->mba_busname, cf->cf_driver->cd_name))
		return (0);

	/* sanity (only mca0 supported currently) */
	if (mba->mba_bus < 0 || mba->mba_bus > 0)
		return (0);

	/* XXX check other indicators? */

	return (1);
}

int
mca_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct mca_attach_args *ma = aux;
	char devinfo[256];

	if (pnp) {
		mca_devinfo(ma->ma_id, devinfo);
		printf("%s slot %d: %s", pnp, ma->ma_slot + 1, devinfo);
	}

	/*
	 * Print "configured" for Memory Extension boards - there is no
	 * meaningfull driver for them, they "just work".
	 */
	switch(ma->ma_id) {
	case MCA_PRODUCT_HRAM: case MCA_PRODUCT_IQRAM: case MCA_PRODUCT_MICRAM:
	case MCA_PRODUCT_ASTRAM: case MCA_PRODUCT_KINGRAM:
	case MCA_PRODUCT_KINGRAM8: case MCA_PRODUCT_KINGRAM16:
	case MCA_PRODUCT_KINGRAM609: case MCA_PRODUCT_HYPRAM:
	case MCA_PRODUCT_QRAM1: case MCA_PRODUCT_QRAM2: case MCA_PRODUCT_EVERAM:
	case MCA_PRODUCT_BOCARAM: case MCA_PRODUCT_IBMRAM1:
	case MCA_PRODUCT_IBMRAM2: case MCA_PRODUCT_IBMRAM3:
	case MCA_PRODUCT_IBMRAM4: case MCA_PRODUCT_IBMRAM5:
	case MCA_PRODUCT_IBMRAM6: case MCA_PRODUCT_IBMRAM7:
		printf(": configured\n");
		return (QUIET);
	default:
		return (UNCONF);
	}
}

int
mca_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mca_attach_args *ma = aux;

	if (cf->mcacf_slot != MCA_UNKNOWN_SLOT &&
	    cf->mcacf_slot != ma->ma_slot)
		return 0;
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

void
mca_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mcabus_attach_args *mba = aux;
	bus_space_tag_t iot, memt;
	bus_dma_tag_t dmat;
	mca_chipset_tag_t mc;
	int slot;

	mca_attach_hook(parent, self, mba);
	printf("\n");

	iot = mba->mba_iot;
	memt = mba->mba_memt;
	mc = mba->mba_mc;
	dmat = mba->mba_dmat;

	/*
	 * Search for and attach subdevices.
	 *
	 * NB: In the adapter setup register, slots are numbered from 0,
	 * but officially they are numbered from 1.
	 * We use the former convention internally and the latter for text
	 * messages and in config files.
	 */

	for (slot = 0; slot < MCA_MAX_SLOTS; slot++) {
		struct mca_attach_args ma;
		int reg;

		ma.ma_iot = iot;
		ma.ma_memt = memt;
		ma.ma_dmat = dmat;
		ma.ma_mc = mc;
		ma.ma_slot = slot;

		for(reg = 0; reg < 8; reg++)
			ma.ma_pos[reg]=mca_conf_read(mc, slot, reg);

		ma.ma_id = ma.ma_pos[0] + (ma.ma_pos[1] << 8);
		if (ma.ma_id == 0xffff)	/* no adapter here */
			continue;

		if (ma.ma_pos[2] & MCA_POS2_ENABLE)
			config_found_sm(self, &ma, mca_print, mca_submatch);
		else {
			mca_print(&ma, self->dv_xname);
			printf(" disabled\n");
		}
	}
}
