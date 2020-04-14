/*	$NetBSD: oweeprom.c,v 1.1 2020/04/14 13:35:24 macallan Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Lorenz.
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
 * 1-Wire EEPROM family type device driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: oweeprom.c,v 1.1 2020/04/14 13:35:24 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <dev/onewire/onewiredevs.h>
#include <dev/onewire/onewirereg.h>
#include <dev/onewire/onewirevar.h>

#define EEPROM_CMD_WRITE_SCRATCHPAD	0x0f
#define EEPROM_CMD_READ_SCRATCHPAD	0xaa
#define EEPROM_CMD_COPY_SCRATCHPAD	0x55
#define EEPROM_CMD_READ_MEMORY		0xf0
#define EEPROM_CMD_WRITE_APPREG		0x99
#define EEPROM_CMD_READ_STATUS		0x66
#define EEPROM_CMD_READ_APPREG		0xc3
#define EEPROM_CMD_COPY_LOCK_APPREG	0x5a

struct oweeprom_softc {
	device_t			sc_dev;
	void				*sc_onewire;
	u_int64_t			sc_rom;
};

static int	oweeprom_match(device_t, cfdata_t, void *);
static void	oweeprom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(oweeprom, sizeof(struct oweeprom_softc),
	oweeprom_match, oweeprom_attach, NULL, NULL);

static const struct onewire_matchfam oweeprom_fams[] = {
	{ ONEWIRE_FAMILY_DS2430 },
};

static int
oweeprom_match(device_t parent, cfdata_t match, void *aux)
{
	return (onewire_matchbyfam(aux, oweeprom_fams,
	    __arraycount(oweeprom_fams)));
}

static void
oweeprom_attach(device_t parent, device_t self, void *aux)
{
	struct oweeprom_softc *sc = device_private(self);
	struct onewire_attach_args *oa = aux;
	int i, j;
	uint8_t data[32];

	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_onewire = oa->oa_onewire;
	sc->sc_rom = oa->oa_rom;

	aprint_normal("\n");

	onewire_lock(sc->sc_onewire);
	if (onewire_reset(sc->sc_onewire) != 0) {
		printf("reset failrd\n");
		return;
	}

	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, EEPROM_CMD_READ_MEMORY);
	onewire_write_byte(sc->sc_onewire, 0);
	onewire_read_block(sc->sc_onewire, data, 32);
	printf("EEPROM\n");
	for (i = 0; i < 32; i += 8) {
		printf("%02x:", i);
		for (j = 0; j < 8; j++)
			printf(" %02x", data[i + j]);
		printf("\n");
	}

	if (onewire_reset(sc->sc_onewire) != 0) {
		printf("reset failrd\n");
		return;
	}

	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, EEPROM_CMD_READ_APPREG);
	onewire_write_byte(sc->sc_onewire, 0);
	onewire_read_block(sc->sc_onewire, data, 8);
	printf("Application register\n");
	for (j = 0; j < 8; j++)
		printf(" %02x", data[j]);
	printf("\n");

	if (onewire_reset(sc->sc_onewire) != 0) {
		printf("reset failrd\n");
		return;
	}

	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, EEPROM_CMD_READ_STATUS);
	onewire_write_byte(sc->sc_onewire, 0);
	onewire_read_block(sc->sc_onewire, data, 1);
	printf("Status register %02x\n", data[0]);

	onewire_unlock(sc->sc_onewire);
}
