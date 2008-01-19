/*	$NetBSD: mainbus.c,v 1.18.64.1 2008/01/19 12:14:24 bouyer Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.18.64.1 2008/01/19 12:14:24 bouyer Exp $");

#include "vmetwo.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _MVME68K_BUS_DMA_PRIVATE
#define _MVME68K_BUS_SPACE_PRIVATE
#include <machine/bus.h>
#undef _MVME68K_BUS_DMA_PRIVATE
#undef _MVME68K_BUS_SPACE_PRIVATE
#include <machine/cpu.h>

#include <mvme68k/dev/mainbus.h>

#if defined(MVME162) || defined(MVME172) || defined(MVME167) || defined(MVME177)
#if NVMETWO == 0
#include <dev/vme/vmevar.h>
#include <dev/mvme/mvmebus.h>
#include <dev/mvme/vme_twovar.h>
#endif
#endif

void mainbus_attach(struct device *, struct device *, void *);
int mainbus_match(struct device *, struct cfdata *, void *);
int mainbus_print(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);


struct mainbus_devices {
	const char *md_name;
	bus_addr_t md_offset;
};

#ifdef MVME147
static struct mainbus_devices mainbusdevs_147[] = {
	{"pcc", MAINBUS_PCC_OFFSET},
	{"timekeeper", MAINBUS_TK147_OFFSET},
	{NULL, 0}
};
#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
static struct mainbus_devices mainbusdevs_1x7[] = {
	{"pcctwo", MAINBUS_PCCTWO_OFFSET},
	{"vmetwo", MAINBUS_VMETWO_OFFSET},
	{"timekeeper", MAINBUS_TIMEKEEPER_OFFSET},
	{NULL, 0}
};
#endif

struct mvme68k_bus_dma_tag _mainbus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load_direct,
	_bus_dmamap_load_mbuf_direct,
	_bus_dmamap_load_uio_direct,
	_bus_dmamap_load_raw_direct,
	_bus_dmamap_unload,
	NULL,			/* Set up at run-time */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};

struct mvme68k_bus_space_tag _mainbus_space_tag = {
	NULL,
	_bus_space_map,
	_bus_space_unmap,
	_bus_space_peek_1,
	_bus_space_peek_2,
	_bus_space_peek_4,
	_bus_space_poke_1,
	_bus_space_poke_2,
	_bus_space_poke_4
};


/* ARGSUSED */
int
mainbus_match(struct device *parent, struct cfdata *cf, void *args)
{
	static int mainbus_matched;

	if (mainbus_matched)
		return 0;

	return (mainbus_matched = 1);
}

/* ARGSUSED */
void
mainbus_attach(struct device *parent, struct device *self, void *args)
{
	struct mainbus_attach_args ma;
	struct mainbus_devices *devices;
	int i;

	printf("\n");

	/*
	 * Attach children appropriate for this CPU.
	 */
	switch (machineid) {
#ifdef MVME147
	case MVME_147:
		devices = mainbusdevs_147;
		_mainbus_dma_tag._dmamap_sync = _bus_dmamap_sync_030;
		break;
#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
	case MVME_162:
	case MVME_167:
	case MVME_172:
	case MVME_177:
		devices = mainbusdevs_1x7;
		_mainbus_dma_tag._dmamap_sync = _bus_dmamap_sync_0460;
		break;
#endif

	default:
		panic("mainbus_attach: impossible CPU type");
	}

	for (i = 0; devices[i].md_name != NULL; ++i) {
		/*
		 * On mvme162 and up, if the kernel config file had no vmetwo0
		 * device, we have to do some manual initialisation on the
		 * VMEChip2 to get local interrupts working (ABORT switch,
		 * hardware assisted soft interrupts).
		 */
#if defined(MVME162) || defined(MVME172) || defined(MVME167) || defined(MVME177)
#if NVMETWO == 0
		if (devices[i].md_offset == MAINBUS_VMETWO_OFFSET
#if defined(MVME147)
		    && machineid != MVME_147
#endif
		    ) {
			(void)vmetwo_probe(&_mainbus_space_tag,
			    intiobase_phys + MAINBUS_VMETWO_OFFSET);
			continue;
		}
#endif
#endif
		ma.ma_name = devices[i].md_name;
		ma.ma_dmat = &_mainbus_dma_tag;
		ma.ma_bust = &_mainbus_space_tag;
		ma.ma_offset = devices[i].md_offset + intiobase_phys;

		(void)config_found(self, &ma, mainbus_print);
	}


	/*
	 * Attach the memory controllers on mvme162->mvme177.
	 * Note: These *must* be attached after the PCCChip2/MCChip.
	 * They must also be attached *after* the VMEchip2 has been
	 * initialised (either by the driver, or the vmetwo_probe()
	 * call above).
	 */
#if defined(MVME162) || defined(MVME172) || defined(MVME167) || defined(MVME177)
#if defined(MVME147)
	if (machineid != MVME_147)
#endif
	{
		ma.ma_name = "memc";
		ma.ma_dmat = &_mainbus_dma_tag;
		ma.ma_bust = &_mainbus_space_tag;
		ma.ma_offset = MAINBUS_MEMC1_OFFSET + intiobase_phys;
		(void)config_found(self, &ma, mainbus_print);
		ma.ma_offset = MAINBUS_MEMC2_OFFSET + intiobase_phys;
		(void)config_found(self, &ma, mainbus_print);
	}
#endif

	/*
	 * Attach Industry Pack modules on mvme162 and mvme172
	 */
#if defined(MVME162) || defined(MVME172)
#if defined(MVME147) || defined(MVME167) || defined(MVME177)
	if (machineid == MVME_162 || machineid == MVME_172)
#endif
	{
		ma.ma_name = "ipack";
		ma.ma_dmat = &_mainbus_dma_tag;
		ma.ma_bust = &_mainbus_space_tag;
		ma.ma_offset = MAINBUS_IPACK_OFFSET + intiobase_phys;
		(void)config_found(self, &ma, mainbus_print);
	}
#endif
}

int
mainbus_print(void *aux, const char *cp)
{
	struct mainbus_attach_args *ma;

	ma = aux;

	if (cp)
		aprint_normal("%s at %s", ma->ma_name, cp);

	aprint_normal(" address 0x%lx", ma->ma_offset);

	return UNCONF;
}
