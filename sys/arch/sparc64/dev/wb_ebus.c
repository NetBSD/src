/*	$NetBSD: wb_ebus.c,v 1.1 2020/05/11 15:56:15 jdc Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
__RCSID("$NetBSD: wb_ebus.c,v 1.1 2020/05/11 15:56:15 jdc Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/ic/w83l518dreg.h>
#include <dev/ic/w83l518dvar.h>
#include <dev/ic/w83l518d_sdmmc.h>

static int	wb_ebus_match(device_t, cfdata_t , void *);
static void	wb_ebus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wb_ebus, sizeof(struct wb_softc),
	wb_ebus_match, wb_ebus_attach, NULL, NULL);

static int
wb_ebus_match(device_t parent, cfdata_t match, void *aux) 
{
	struct ebus_attach_args *ea = aux;

	return (strcmp(ea->ea_name, "TAD,wb-sdcard") == 0);
}

static void
wb_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct wb_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;

	sc->wb_dev = self;

	if (bus_space_map(ea->ea_bustag, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
	    ea->ea_reg[0].size, 0, &sc->wb_ioh) == 0)
		sc->wb_iot = ea->ea_bustag;
	else {
		aprint_error(": can't map register space\n");
                return;
	}

	bus_intr_establish(sc->wb_iot, ea->ea_intr[0], IPL_BIO, wb_intr, sc);

	aprint_normal("\n");

	sc->wb_type = WB_DEVNO_SD;
	sc->wb_quirks = WB_QUIRK_1BIT;	/* 4bit bus width always fails */
	wb_attach(sc);
}
