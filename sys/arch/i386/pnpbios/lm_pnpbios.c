/*	$NetBSD: lm_pnpbios.c,v 1.2.6.1 2000/07/30 17:54:10 bouyer Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Squier.
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
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/nslm7xvar.h>


int lm_pnpbios_match __P((struct device *, struct cfdata *, void *));
void lm_pnpbios_attach __P((struct device *, struct device *, void *));
int lm_pnpbios_hints_index __P((const char *));


struct cfattach lm_pnpbios_ca = {
	sizeof(struct lm_softc), lm_pnpbios_match, lm_pnpbios_attach
};

/*
 * XXX - no known pnpbios ids for lm series chips.
 */
struct lm_pnpbios_hint {
	char idstr[8];
	int io_region_idx_lm7x;
};

/*
 * Currently no known valid pnpbios id's - PNP0C02 is
 * for reserved motherboard resources, probing it is bad.
 */
struct lm_pnpbios_hint lm_pnpbios_hints[] = {
	{ { 0 }, 0 }
};


int
lm_pnpbios_hints_index(idstr)
	const char *idstr;
{
	int idx = 0;

	while (lm_pnpbios_hints[idx].idstr[0] != 0) {
		if (!strcmp(lm_pnpbios_hints[idx].idstr, idstr))
			return idx;
		++idx;
	}

	return -1;
}

int
lm_pnpbios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pnpbiosdev_attach_args *aa = aux;
	struct lm_pnpbios_hint *wph;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int rv;

	int wphi;

	if ((wphi = lm_pnpbios_hints_index(aa->idstr)) == -1)
		return (0);

	wph = &lm_pnpbios_hints[wphi];

	if (pnpbios_io_map(aa->pbt, aa->resc, wph->io_region_idx_lm7x,
			   &iot, &ioh)) {
		return (0);
	}

	rv = lm_probe(iot, ioh);

	pnpbios_io_unmap(aa->pbt, aa->resc, wph->io_region_idx_lm7x,
			 iot, ioh);

	return (rv);
}

void
lm_pnpbios_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct lm_softc *sc = (void *)self;
	struct pnpbiosdev_attach_args *aa = aux;
	struct lm_pnpbios_hint *wph;

	wph = &lm_pnpbios_hints[lm_pnpbios_hints_index(aa->idstr)];

	if (pnpbios_io_map(aa->pbt, aa->resc, wph->io_region_idx_lm7x,
			   &sc->lm_iot, &sc->lm_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	printf("\n");
	pnpbios_print_devres(self, aa);

	printf("%s", self->dv_xname);

	/* Bus-independant attach */
	lm_attach(sc);
}

