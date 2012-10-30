/*	$NetBSD: memc_68k.c,v 1.7.34.1 2012/10/30 17:20:03 yamt Exp $	*/

/*-
 * Copyright (c) 2000, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 * Support for the MEMECC and MEMC40 memory controllers on MVME68K
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: memc_68k.c,v 1.7.34.1 2012/10/30 17:20:03 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <mvme68k/dev/mainbus.h>

#include <dev/mvme/memcvar.h>
#include <dev/mvme/memcreg.h>
#include <dev/mvme/pcctwovar.h>
#include <dev/mvme/pcctworeg.h>

#include "ioconf.h"

int memc_match(device_t, cfdata_t, void *);
void memc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(memc, sizeof(struct memc_softc),
    memc_match, memc_attach, NULL, NULL);


/* ARGSUSED */
int
memc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;
	uint8_t chipid;
	int rv;

#ifdef MVME68K
	if (machineid != MVME_167 && machineid != MVME_177 &&
	    machineid != MVME_162 && machineid != MVME_172)
		return 0;
#endif

	if (strcmp(ma->ma_name, memc_cd.cd_name))
		return 0;

	if (bus_space_map(ma->ma_bust, ma->ma_offset, MEMC_REGSIZE, 0, &bh))
		return 0;

	rv = bus_space_peek_1(ma->ma_bust, bh, MEMC_REG_CHIP_ID, &chipid);
	bus_space_unmap(ma->ma_bust, bh, MEMC_REGSIZE);

	if (rv)
		return 0;

	/* Verify the Chip Id register is sane */
	if (chipid != MEMC_CHIP_ID_MEMC040 && chipid != MEMC_CHIP_ID_MEMECC)
		return 0;

	return 1;
}

/* ARGSUSED */
void
memc_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct memc_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_bust = ma->ma_bust;

	/* Map the memory controller's registers */
	bus_space_map(sc->sc_bust, ma->ma_offset, MEMC_REGSIZE, 0,
	    &sc->sc_bush);

	/* Finish initialisation in common code */
	memc_init(sc);
}
