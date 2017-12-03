/* $NetBSD: plmmc_ifpga.c,v 1.1.18.2 2017/12/03 11:36:04 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: plmmc_ifpga.c,v 1.1.18.2 2017/12/03 11:36:04 jdolecek Exp $");

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

#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgavar.h>

static int  plmmc_ifpga_match(device_t, cfdata_t, void *);
static void plmmc_ifpga_attach(device_t, device_t, void *);
static void plmmc_ifpga_attach_i(device_t);

CFATTACH_DECL_NEW(plmmc_ifpga, sizeof(struct plmmc_softc),
    plmmc_ifpga_match, plmmc_ifpga_attach, NULL, NULL);

static int
plmmc_ifpga_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
plmmc_ifpga_attach(device_t parent, device_t self, void *aux)
{
	struct plmmc_softc *sc = device_private(self);
	struct ifpga_attach_args *ifa = aux;

	sc->sc_dev = self;
	sc->sc_clock_freq = IFPGA_MMC_CLK;
	sc->sc_bst = ifa->ifa_iot;
	if (bus_space_map(ifa->ifa_iot, ifa->ifa_addr, IFPGA_MMC_SIZE, 0,
	    &sc->sc_bsh)) {
		printf("%s: unable to map device\n", device_xname(sc->sc_dev));
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

#if 0
	sc->sc_ih = ifpga_intr_establish(ifa->ifa_irq, IPL_BIO, plmmc_intr, sc);
#endif

	config_interrupts(self, plmmc_ifpga_attach_i);
}

static void
plmmc_ifpga_attach_i(device_t self)
{
	struct plmmc_softc *sc = device_private(self);

	plmmc_init(sc);
}
