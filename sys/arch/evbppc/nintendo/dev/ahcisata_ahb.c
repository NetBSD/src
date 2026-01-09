/* $NetBSD: ahcisata_ahb.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: ahcisata_ahb.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

#include <machine/wii.h>
#include <machine/wiiu.h>

#include "ahb.h"

#define SATA_HCCFG_INT_REG	0x400
#define  SATA_HCCFG_INT_PORT1	__BIT(5)
#define  SATA_HCCFG_INT_PORT0	__BIT(3)
#define SATA_HCCFG_INTMSK_REG	0x404

#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_ahcit, (sc)->sc_ahcih, (reg))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_ahcit, (sc)->sc_ahcih, (reg), (val))

static int
ahcisata_ahb_intr(void *arg)
{
	struct ahci_softc * const sc = arg;
	uint32_t val, mask;
	int ret = 0;

	val = RD4(sc, SATA_HCCFG_INT_REG);
	mask = RD4(sc, SATA_HCCFG_INTMSK_REG);

	if ((val & mask) != 0) {
		ret = ahci_intr(sc);
	}

	WR4(sc, SATA_HCCFG_INT_REG, val);

	return ret;
}

static int
ahcisata_ahb_match(device_t parent, cfdata_t cf, void *aux)
{
	return wiiu_native;
}

static void
ahcisata_ahb_attach(device_t parent, device_t self, void *aux)
{
	struct ahb_attach_args * const aaa = aux;
	struct ahci_softc * const sc = device_private(self);

	sc->sc_atac.atac_dev = self;
	sc->sc_dmat = aaa->aaa_dmat;
	sc->sc_ahcit = aaa->aaa_bst;
	sc->sc_ahcis = 0x408;
	if (bus_space_map(sc->sc_ahcit, aaa->aaa_addr, sc->sc_ahcis, 0,
	    &sc->sc_ahcih) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_ahci_ports = 1;
	sc->sc_save_init_data = true;
	sc->sc_ahci_quirks |= AHCI_QUIRK_BADPMP;

	aprint_naive("\n");
	aprint_normal(": AHCI SATA controller\n");

	WR4(sc, SATA_HCCFG_INTMSK_REG, SATA_HCCFG_INT_PORT0);
	WR4(sc, SATA_HCCFG_INT_REG, ~0U);

	ahb_intr_establish(aaa->aaa_irq, IPL_BIO, ahcisata_ahb_intr, sc,
	    device_xname(self));

	ahci_attach(sc);
}

CFATTACH_DECL_NEW(ahcisata_ahb, sizeof(struct ahci_softc),
	ahcisata_ahb_match, ahcisata_ahb_attach, NULL, NULL);
