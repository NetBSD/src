/*	$NetBSD: memc.c,v 1.3 2001/05/31 18:46:08 scw Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * Support for the MEMECC and MEMC40 memory controllers on MVME1[67][27]
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <mvme68k/dev/pcctwovar.h>
#include <mvme68k/dev/memcreg.h>

struct memc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_bush;
	struct evcnt		sc_evcnt;
};

int memc_match(struct device *, struct cfdata *, void *);
void memc_attach(struct device *, struct device *, void *);

struct cfattach memc_ca = {
	sizeof(struct memc_softc), memc_match, memc_attach
};

extern struct cfdriver memc_cd;

static void memc040_attach(struct memc_softc *, struct pcctwo_attach_args *);
static void memecc_attach(struct memc_softc *, struct pcctwo_attach_args *);

/* ARGSUSED */
int
memc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcctwo_attach_args *pa = aux;
	bus_space_handle_t bh;
	u_int8_t chipid;
	int rv;

	if (machineid != MVME_167 && machineid != MVME_177 &&
	    machineid != MVME_162 && machineid != MVME_172)
		return (0);

	if (strcmp(pa->pa_name, memc_cd.cd_name))
		return (0);

	if (bus_space_map(pa->pa_bust, pa->pa_offset, MEMC_REGSIZE, 0, &bh))
		return (0);

	rv = bus_space_peek_1(pa->pa_bust, bh, MEMC_REG_CHIP_ID, &chipid);
	bus_space_unmap(pa->pa_bust, bh, MEMC_REGSIZE);

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
	struct pcctwo_attach_args *pa = aux;
	struct memc_softc *sc = (struct memc_softc *) self;
	u_int8_t chipid;
	u_int8_t memcfg;

	sc->sc_bust = pa->pa_bust;

	/* Map the memory controller's registers */
	bus_space_map(sc->sc_bust, pa->pa_offset, MEMC_REGSIZE, 0,
	    &sc->sc_bush);
	chipid = bus_space_read_1(pa->pa_bust, sc->sc_bush, MEMC_REG_CHIP_ID);
	memcfg = bus_space_read_1(pa->pa_bust, sc->sc_bush,
	    MEMC_REG_MEMORY_CONFIG);

	printf(": %dMB %s Memory Controller Chip (Rev %d)\n",
	    MEMC_MEMORY_CONFIG_2_MB(memcfg),
	    (chipid == MEMC_CHIP_ID_MEMC040) ? "Parity" : "ECC",
	    bus_space_read_1(sc->sc_bust, sc->sc_bush, MEMC_REG_CHIP_REVISION));

	printf("%s: Base Address: 0x%x, ", sc->sc_dev.dv_xname,
	    MEMC_BASE_ADDRESS(bus_space_read_1(sc->sc_bust, sc->sc_bush,
		MEMC_REG_BASE_ADDRESS_HI), bus_space_read_1(sc->sc_bust,
		sc->sc_bush, MEMC_REG_BASE_ADDRESS_LO)));

	printf("Fast RAM Read %sabled\n", (bus_space_read_1(sc->sc_bust,
	    sc->sc_bush, MEMC_REG_MEMORY_CONFIG) & MEMC_MEMORY_CONFIG_FSTRD) ?
	    "En" : "Dis");

	switch (chipid) {
	case MEMC_CHIP_ID_MEMC040:
		memc040_attach(sc, pa);
		break;
	case MEMC_CHIP_ID_MEMECC:
		memecc_attach(sc, pa);
		break;
	}
}

static void
memc040_attach(struct memc_softc *sc, struct pcctwo_attach_args *pa)
{

	/* XXX: TBD */
}

static void
memecc_attach(struct memc_softc *sc, struct pcctwo_attach_args *pa)
{

	/* XXX: TBD */
}
