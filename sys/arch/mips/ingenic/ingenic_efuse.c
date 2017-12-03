/*	$NetBSD: ingenic_efuse.c,v 1.3.18.2 2017/12/03 11:36:28 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Michael Lorenz
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

/*
 * a driver for the 'EFUSE Slave Interface' found on JZ4780
 * more or less 8kBit of non-volatile storage containing things like MAC
 * address, various encryption keys, boot code, serial numbers and parameters.
 * Using it only to get the MAC address for now.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ingenic_efuse.c,v 1.3.18.2 2017/12/03 11:36:28 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <mips/ingenic/ingenic_var.h>
#include <mips/ingenic/ingenic_regs.h>

#include "opt_ingenic.h"

static int ingenic_efuse_match(device_t, struct cfdata *, void *);
static void ingenic_efuse_attach(device_t, device_t, void *);

struct efuse_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	uint8_t			sc_data[0x20];
};

static void ingenic_efuse_read(struct efuse_softc *, int, int, uint8_t *);

void ingenic_set_enaddr(uint8_t *);

CFATTACH_DECL_NEW(ingenic_efuse, sizeof(struct efuse_softc),
    ingenic_efuse_match, ingenic_efuse_attach, NULL, NULL);

/* ARGSUSED */
static int
ingenic_efuse_match(device_t parent, struct cfdata *match, void *aux)
{
	struct apbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, "efuse") != 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
ingenic_efuse_attach(device_t parent, device_t self, void *aux)
{
	struct efuse_softc *sc = device_private(self);
	struct apbus_attach_args *aa = aux;
	int error;

	sc->sc_dev = self;

	sc->sc_iot = aa->aa_bst;

	if (aa->aa_addr == 0)
		aa->aa_addr = JZ_EFUSE;

	error = bus_space_map(aa->aa_bst, aa->aa_addr, 0x30, 0, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aa->aa_name, error);
		return;
	}

	aprint_naive(": Ingenic EFUSE Slave Interface\n");
	aprint_normal(": Ingenic EFUSE Slave Interface\n");
	ingenic_efuse_read(sc, 8, 0x20, sc->sc_data);
	ingenic_set_enaddr(&sc->sc_data[0x1a]);
#ifdef INGENIC_DEBUG
	{
		int i, j;
		for (i = 0; i < 0x20; i += 8) {
			printf("%02x:", i);
			for (j = 0; j < 8; j++)
				printf(" %02x", sc->sc_data[i + j]);
			printf("\n");
		}
	}
#endif
}

static void
ingenic_efuse_read(struct efuse_softc *sc, int addr, int len, uint8_t *buf)
{
	uint32_t abuf;
	int i;

	/* default, just in case */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, JZ_EFUCFG, 0x00040000);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, JZ_EFUCTRL,
		JZ_EFUSE_READ |
		(addr << JZ_EFUSE_ADDR_SHIFT) |
		((len - 1) << JZ_EFUSE_SIZE_SHIFT));
	do {} while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, JZ_EFUSTATE) &
		JZ_EFUSE_RD_DONE) == 0);
	for (i = 0; i < len; i += 4) {
		abuf = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    JZ_EFUDATA0 + i);
		memcpy(buf, &abuf, 4);
		buf += 4;
	}
}
