/*	$NetBSD: vexpress_plmmc.c,v 1.2.2.2 2015/04/06 15:17:56 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Sergio L. Pascual.
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

/* Interface to plmmc (PL181) MMC driver. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vexpress_plmmc.c,v 1.2.2.2 2015/04/06 15:17:56 skrll Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/malloc.h>

#include <sys/termios.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/ic/pl181reg.h>
#include <dev/ic/pl181var.h>

#include <evbarm/vexpress/platform.h>
#include <evbarm/vexpress/vexpress_var.h>

static int  plmmc_vexpress_match(device_t, cfdata_t, void *);
static void plmmc_vexpress_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(vexpressplmmc, sizeof(struct plmmc_softc),
    plmmc_vexpress_match, plmmc_vexpress_attach, NULL, NULL);

static int
plmmc_vexpress_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
plmmc_vexpress_attach(device_t parent, device_t self, void *aux)
{
	struct plmmc_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;
	void *ih;

	sc->sc_dev = self;
	sc->sc_clock_freq = VEXPRESS_REF_FREQ;
	sc->sc_bst = aa->aa_iot;
	if (bus_space_map(aa->aa_iot, aa->aa_addr, 0x1000, 0,
	    &sc->sc_bsh)) {
		printf("%s: unable to map device\n", device_xname(sc->sc_dev));
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

        ih = intr_establish(aa->aa_irq, IPL_BIO, IST_LEVEL_LOW, plmmc_intr, sc);
        if (ih == NULL)
                panic("%s: cannot install interrupt handler",
                    device_xname(sc->sc_dev));
	plmmc_init(sc);
}

