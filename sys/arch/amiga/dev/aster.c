/*	$NetBSD: aster.c,v 1.22.2.1 2012/10/30 17:18:47 yamt Exp $ */

/*-
 * Copyright (c) 1998,2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
__KERNEL_RCSID(0, "$NetBSD: aster.c,v 1.22.2.1 2012/10/30 17:18:47 yamt Exp $");

/*
 * zbus ISDN Blaster, ISDN Master driver.
 */

#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/bus.h>

#include <amiga/include/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/drcustom.h>

#include <amiga/dev/supio.h>
#include <amiga/dev/zbusvar.h>


struct aster_softc {
	struct bus_space_tag sc_bst;
};

int astermatch(device_t, cfdata_t, void *);
void asterattach(device_t, device_t, void *);
int asterprint(void *, const char *);

CFATTACH_DECL_NEW(aster, sizeof(struct aster_softc),
    astermatch, asterattach, NULL, NULL);

int
astermatch(device_t parent, cfdata_t cf, void *aux)
{

	struct zbus_args *zap;

	zap = aux;

	if (zap->manid == 5001 && zap->prodid == 1)	/* VMC ISDN Blaster */
		return (1);

	if (zap->manid == 2092 && (zap->prodid == 64 ||	/* BSC ISDN Master */
	    zap->prodid == 65))				/* BSC ISDN Master II */
		return (1);

	if (zap->manid == 5000 && zap->prodid == 1)	/* ITH ISDN Master II */
		return (1);

	if (zap->manid == 4626 && zap->prodid == 5 && zap->serno == 0)
		return (1);			/* Schoenfeld ISDN Surfer */

	if (zap->manid == 2189 && zap->prodid == 3)
		return (1);			/* Zeus Dev. ? ISDN board */

	return (0);
}

void
asterattach(device_t parent, device_t self, void *aux)
{
	struct aster_softc *astrsc;
	struct zbus_args *zap;
	struct supio_attach_args supa;

	astrsc = device_private(self);
	zap = aux;

	astrsc->sc_bst.base = (u_long)zap->va + 0;
	astrsc->sc_bst.absm = &amiga_bus_stride_2;
	supa.supio_ipl = 2;	/* could be 6. isic_supio will decide. */

	switch (zap->manid) {
	case 5001:
		supa.supio_name = "isic31VMC ISDN Blaster";
		break;
	case 2092:
		supa.supio_name = "isic31BSC ISDN Master/Master II";
		break;
	case 5000:
		supa.supio_name = "isic13ITH ISDN Master II";
		break;
	case 2189:
		supa.supio_name = "isic@BZeus ISDN Link";
		break;
	case 4626:
		if (zap->serno == 0) {
			supa.supio_name =
				"isic1CIndividual Comp. ISDN Surfer";
			((volatile u_int8_t *)zap->va)[0x00fe] = 0xff;
			if (((volatile u_int8_t *)zap->va)[0x00fe] & 0x80)
				supa.supio_ipl = 6;
			break;
		}
		/* FALLTHROUGH */
	}

	if (parent)
		printf(" IPL %d: %s\n", supa.supio_ipl, supa.supio_name+6);

	supa.supio_iot = &astrsc->sc_bst;

	supa.supio_iobase = 0;
	supa.supio_arg = 0;
	config_found(self, &supa, asterprint); /* XXX */
#ifdef __notyet__
	hyper3i_attach_subr(self, &supa, asterprint);
#endif
}

int
asterprint(void *aux, const char *pnp)
{
	struct supio_attach_args *supa;

	supa = aux;

	if (pnp == NULL)
		return(QUIET);

	aprint_normal("%s at %s port 0x%04x",
	    supa->supio_name, pnp, supa->supio_iobase);

	return(UNCONF);
}
