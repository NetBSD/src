/*	$NetBSD: memc_68k.c,v 1.1.8.2 2002/03/16 15:58:52 jdolecek Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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


int memc_match(struct device *, struct cfdata *, void *);
void memc_attach(struct device *, struct device *, void *);

struct cfattach memc_ca = {
	sizeof(struct memc_softc), memc_match, memc_attach
};

extern struct cfdriver memc_cd;


/* ARGSUSED */
int
memc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;
	u_int8_t chipid;
	int rv;

#ifdef MVME68K
	if (machineid != MVME_167 && machineid != MVME_177 &&
	    machineid != MVME_162 && machineid != MVME_172)
		return (0);
#endif

	if (strcmp(ma->ma_name, memc_cd.cd_name))
		return (0);

	if (bus_space_map(ma->ma_bust, ma->ma_offset, MEMC_REGSIZE, 0, &bh))
		return (0);

	rv = bus_space_peek_1(ma->ma_bust, bh, MEMC_REG_CHIP_ID, &chipid);
	bus_space_unmap(ma->ma_bust, bh, MEMC_REGSIZE);

	if (rv)
		return (0);

	/* Verify the Chip Id register is sane */
	if (chipid != MEMC_CHIP_ID_MEMC040 && chipid != MEMC_CHIP_ID_MEMECC)
		return (0);

	return (1);
}

/* ARGSUSED */
void
memc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	struct memc_softc *sc = (struct memc_softc *) self;

	sc->sc_bust = ma->ma_bust;

	/* Map the memory controller's registers */
	bus_space_map(sc->sc_bust, ma->ma_offset, MEMC_REGSIZE, 0,
	    &sc->sc_bush);

	/* Finish initialisation in common code */
	memc_init(sc);
}
