/*	$NetBSD: mainbus.c,v 1.2 2000/03/18 22:33:03 scw Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford
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
 *	      This product includes software developed by the NetBSD
 *	      Foundation, Inc. and its contributors.
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
 * Derived from the mainbus code in mvme68k/autoconf.c by Chuck Cranor.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _MVME68K_BUS_DMA_PRIVATE
#include <machine/bus.h>
#undef _MVME68K_BUS_DMA_PRIVATE
#include <machine/cpu.h>

#include <mvme68k/dev/mainbus.h>


void mainbus_attach __P((struct device *, struct device *, void *));
int mainbus_match __P((struct device *, struct cfdata *, void *));
int mainbus_print __P((void *, const char *));

struct mainbus_softc {
	struct device sc_dev;
	struct mvme68k_bus_dma_tag sc_dmat;
};

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mainbus_match, mainbus_attach
};


struct mainbus_devices {
	const char *md_name;
	bus_addr_t md_offset;
};

#ifdef MVME147
static struct mainbus_devices mainbusdevs_147[] = {
	{"pcc", MAINBUS_PCC_OFFSET},
	{NULL, 0}
};
#endif

#if defined(MVME167) || defined(MVME177)
static struct mainbus_devices mainbusdevs_1x7[] = {
	{"pcctwo", MAINBUS_PCCTWO_OFFSET},
	{"vmetwo", MAINBUS_VMETWO_OFFSET},
	{NULL, 0}
};
#endif

/* ARGSUSED */
int
mainbus_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	static int mainbus_matched;

	if (mainbus_matched)
		return (0);

	return ((mainbus_matched = 1));
}

/* ARGSUSED */
void
mainbus_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct mainbus_softc *sc;
	struct mainbus_attach_args ma;
	struct mainbus_devices *devices;
	int i;

	sc = (struct mainbus_softc *) self;

	/*
	 * Initialise the mainbus Bus DMA tag.
	 */
	sc->sc_dmat._cookie = sc;
	sc->sc_dmat._dmamap_create = _bus_dmamap_create;
	sc->sc_dmat._dmamap_destroy = _bus_dmamap_destroy;
	sc->sc_dmat._dmamap_load = _bus_dmamap_load_direct;
	sc->sc_dmat._dmamap_load_mbuf = _bus_dmamap_load_mbuf_direct;
	sc->sc_dmat._dmamap_load_uio = _bus_dmamap_load_uio_direct;
	sc->sc_dmat._dmamap_load_raw = _bus_dmamap_load_raw_direct;
	sc->sc_dmat._dmamap_unload = _bus_dmamap_unload;
	sc->sc_dmat._dmamap_sync = _bus_dmamap_sync;
	sc->sc_dmat._dmamem_alloc = _bus_dmamem_alloc;
	sc->sc_dmat._dmamem_free = _bus_dmamem_free;
	sc->sc_dmat._dmamem_map = _bus_dmamem_map;
	sc->sc_dmat._dmamem_unmap = _bus_dmamem_unmap;
	sc->sc_dmat._dmamem_mmap = _bus_dmamem_mmap;

	printf("\n");

	/*
	 * Attach children appropriate for this CPU.
	 */
	switch (machineid) {
#ifdef MVME147
	case MVME_147:
		devices = mainbusdevs_147;
		break;
#endif

#ifdef MVME162
	case MVME_162:
		devices = mainbusdevs_162;
		break;
#endif

#if defined(MVME167) || defined(MVME177)
	case MVME_166:
	case MVME_167:
	case MVME_177:
		devices = mainbusdevs_1x7;
		break;
#endif

	default:
		panic("mainbus_attach: impossible CPU type");
	}

	for (i = 0; devices[i].md_name != NULL; ++i) {
		ma.ma_name = devices[i].md_name;
		ma.ma_dmat = &sc->sc_dmat;
		ma.ma_bust = MVME68K_INTIO_BUS_SPACE;
		ma.ma_offset = devices[i].md_offset;

		(void) config_found(self, &ma, mainbus_print);
	}
}

int
mainbus_print(aux, cp)
	void *aux;
	const char *cp;
{
	struct mainbus_attach_args *ma;

	ma = aux;

	if (cp)
		printf("%s at %s", ma->ma_name, cp);

	printf(" offset 0x%lx", ma->ma_offset);

	return (UNCONF);
}
