/*	$NetBSD: uturn.c,v 1.6 2009/11/15 15:53:05 skrll Exp $	*/

/*	$OpenBSD: uturn.c,v 1.6 2007/12/29 01:26:14 kettenis Exp $	*/

/*
 * Copyright (c) 2004 Michael Shalayeff
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* TODO IOA programming */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>

struct uturn_regs {
	uint64_t	resv0[2];
	uint64_t	status;		/* 0x10: */
	uint64_t	resv1[5];
	uint64_t	debug;		/* 0x40: */
};

struct uturn_softc {
	device_t sc_dv;

	struct uturn_regs volatile *sc_regs;
};

int	uturnmatch(device_t, cfdata_t, void *);
void	uturnattach(device_t, device_t, void *);
static void uturn_callback(device_t self, struct confargs *ca);

CFATTACH_DECL_NEW(uturn, sizeof(struct uturn_softc),
    uturnmatch, uturnattach, NULL, NULL);

extern struct cfdriver uturn_cd;

int
uturnmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* there will be only one */
	if (ca->ca_type.iodc_type != HPPA_TYPE_IOA ||
	    ca->ca_type.iodc_sv_model != HPPA_IOA_UTURN)
		return 0;

	if (ca->ca_type.iodc_model == 0x58 &&
	    ca->ca_type.iodc_revision >= 0x20)
		return 0;

	return 1;
}

void
uturnattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux, nca;
	struct uturn_softc *sc = device_private(self);
	bus_space_handle_t ioh;

	if (bus_space_map(ca->ca_iot, ca->ca_hpa, IOMOD_HPASIZE, 0, &ioh)) {
		aprint_error(": can't map IO space\n");
		return;
	}

	sc->sc_dv = self;
	sc->sc_regs = (struct uturn_regs *)ca->ca_hpa;

	aprint_normal(": %s rev %d\n",
	    ca->ca_type.iodc_revision < 0x10? "U2" : "UTurn",
	    ca->ca_type.iodc_revision & 0xf);

	/* keep it real */
	((struct iomod *)ioh)->io_control = 0x80;

	/*
	 * U2/UTurn is actually a combination of an Upper Bus Converter (UBC)
	 * and a Lower Bus Converter (LBC).  This driver attaches to the UBC;
	 * the LBC isn't very interesting, so we skip it.  This is easy, since
	 * it always is module 63, hence the MAXMODBUS - 1 below.
	 */
	nca = *ca;
	nca.ca_hpabase = 0;
	nca.ca_nmodules = MAXMODBUS - 1;
	pdc_scanbus(self, &nca, uturn_callback);

	/* XXX On some machines, PDC doesn't tell us about all devices. */
	switch (cpu_hvers) {
	case HPPA_BOARD_HP809:
	case HPPA_BOARD_HP819:
	case HPPA_BOARD_HP829:
	case HPPA_BOARD_HP839:
	case HPPA_BOARD_HP849:
	case HPPA_BOARD_HP859:
	case HPPA_BOARD_HP869:

	case HPPA_BOARD_HP800D:
	case HPPA_BOARD_HP821:
		nca.ca_hpabase = ((struct iomod *)ioh)->io_io_low << 16;
		pdc_scanbus(self, &nca, uturn_callback);
		break;
	default:
		break;
	}
}

static void
uturn_callback(device_t self, struct confargs *ca)
{

	config_found_sm_loc(self, "gedoens", NULL, ca, mbprint, mbsubmatch);
}
