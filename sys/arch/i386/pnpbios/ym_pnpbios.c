/* $NetBSD: ym_pnpbios.c,v 1.17.30.1 2016/10/05 20:55:29 skrll Exp $ */
/*
 * Copyright (c) 1999
 *	Matthias Drochner.  All rights reserved.
 *	Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ym_pnpbios.c,v 1.17.30.1 2016/10/05 20:55:29 skrll Exp $");

#include "mpu_ym.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/ic/ad1848reg.h>
#include <dev/isa/ad1848var.h>

#include <dev/ic/cs4231reg.h>
#include <dev/isa/cs4231var.h>

#include <dev/ic/opl3sa3reg.h>
#include <dev/isa/wssreg.h>
#include <dev/isa/ymvar.h>

int ym_pnpbios_match(device_t, cfdata_t, void *);
void ym_pnpbios_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ym_pnpbios, sizeof(struct ym_softc),
    ym_pnpbios_match, ym_pnpbios_attach, NULL, NULL);

int
ym_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "YMH0021"))
		return 0;

	return 1;
}

void
ym_pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct ym_softc *sc = device_private(self);
	struct ad1848_softc *ac = &sc->sc_ad1848.sc_ad1848;
	struct pnpbiosdev_attach_args *aa = aux;

	ac->sc_dev = self;

	aprint_naive("\n");
	if (pnpbios_io_map(aa->pbt, aa->resc, 0,
				&sc->sc_iot, &sc->sc_sb_ioh) != 0) {
		aprint_error(": can't map sb i/o space\n");
		return;
	}
	if (pnpbios_io_map(aa->pbt, aa->resc, 1,
				&sc->sc_iot, &sc->sc_ioh) != 0) {
		aprint_error(": can't map sb i/o space\n");
		return;
	}
	if (pnpbios_io_map(aa->pbt, aa->resc, 2,
				&sc->sc_iot, &sc->sc_opl_ioh) != 0) {
		aprint_error(": can't map opl i/o space\n");
		return;
	}
#if NMPU_YM > 0
	if (pnpbios_io_map(aa->pbt, aa->resc, 3,
				&sc->sc_iot, &sc->sc_mpu_ioh) != 0) {
		aprint_error(": can't map mpu i/o space\n");
		return;
	}
#endif
	if (pnpbios_io_map(aa->pbt, aa->resc, 4,
				&sc->sc_iot, &sc->sc_controlioh) != 0) {
		aprint_error(": can't map control i/o space\n");
		return;
	}

	sc->sc_ic = aa->ic;

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &sc->ym_irq, NULL)) {
		aprint_error(": can't get IRQ\n");
		return;
	}

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 0, &sc->ym_playdrq)) {
		aprint_error(": can't get DMA channel\n");
		return;
	}
	if (pnpbios_getdmachan(aa->pbt, aa->resc, 1, &sc->ym_recdrq))
		sc->ym_recdrq = sc->ym_playdrq;	/* half-duplex mode */

	aprint_normal("\n");
	pnpbios_print_devres(self, aa);

	aprint_naive("%s", device_xname(self));
	aprint_normal("%s", device_xname(self));

	ac->sc_iot = sc->sc_iot;
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, WSS_CODEC,
	    AD1848_NPORT, &ac->sc_ioh)) {
		aprint_error_dev(self, "bus_space_subregion failed\n");
		return;
	}
	ac->mode = 2;
	ac->MCE_bit = MODE_CHANGE_ENABLE;

	sc->sc_ad1848.sc_ic  = sc->sc_ic;

	ym_attach(sc);
}
