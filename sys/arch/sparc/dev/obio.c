/*	$NetBSD: obio.c,v 1.74 2013/03/24 17:50:26 jdc Exp $	*/

/*-
 * Copyright (c) 1997,1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.74 2013/03/24 17:50:26 jdc Exp $");

#include "locators.h"

#ifdef _KERNEL_OPT
#include "sbus.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#if NSBUS > 0
#include <sparc/dev/sbusvar.h>
#endif
#include <machine/autoconf.h>
#include <machine/oldmon.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>

struct obio4_softc {
	device_t	sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;	/* parent bus tag */
	bus_dma_tag_t	sc_dmatag;	/* parent bus DMA tag */
};

union obio_softc {
	struct	obio4_softc sc_obio;	/* sun4 obio */
#if NSBUS > 0
	struct	sbus_softc sc_sbus;	/* sun4m obio is another sbus slot */
#endif
};


/* autoconfiguration driver */
static	int obiomatch(device_t, cfdata_t, void *);
static	void obioattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(obio, sizeof(union obio_softc),
    obiomatch, obioattach, NULL, NULL);

static int obio_attached;

/*
 * This `obio4_busattachargs' data structure only exists to pass down
 * to obiosearch() the name of a device that must be configured early.
 */
struct obio4_busattachargs {
	struct mainbus_attach_args	*ma;
	const char			*name;
};

#if defined(SUN4)
static	int obioprint(void *, const char *);
static	int obiosearch(device_t, struct cfdata *, const int *, void *);
static	paddr_t obio_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static	int _obio_bus_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
			  vaddr_t, bus_space_handle_t *);

/* There's at most one obio bus, so we can allocate the bus tag statically */
static struct sparc_bus_space_tag obio_space_tag;
#endif

#if NSBUS > 0
/*
 * Translate obio `interrupts' property value to processor IPL (see sbus.c)
 * Apparently, the `interrupts' property on obio devices is just
 * the processor IPL.
 */
static int intr_obio2ipl[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
#endif

static int
obiomatch(device_t parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (obio_attached)
		return 0;

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

static void
obioattach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	obio_attached = 1;

	printf("\n");

	if (CPU_ISSUN4) {
#if defined(SUN4)
		union obio_softc *usc = device_private(self);
		struct obio4_softc *sc = &usc->sc_obio;
		struct obio4_busattachargs oa;
		const char *const *cpp;
		static const char *const special4[] = {
			/* find these first */
			"timer",
			"dma",		/* need this before `esp', if any */
			NULL
		};

		sc->sc_dev = self;
		sc->sc_bustag = ma->ma_bustag;
		sc->sc_dmatag = ma->ma_dmatag;

		memcpy(&obio_space_tag, sc->sc_bustag, sizeof(obio_space_tag));
		obio_space_tag.cookie = sc;
		obio_space_tag.parent = sc->sc_bustag;
		obio_space_tag.sparc_bus_map = _obio_bus_map;
		obio_space_tag.sparc_bus_mmap = obio_bus_mmap;

		oa.ma = ma;

		/* Find all `early' obio devices */
		for (cpp = special4; *cpp != NULL; cpp++) {
			oa.name = *cpp;
			config_search_ia(obiosearch, self, "obio", &oa);
		}

		/* Find all other obio devices */
		oa.name = NULL;
		config_search_ia(obiosearch, self, "obio", &oa);
#endif
		return;
	} else if (CPU_ISSUN4M) {
#if NSBUS > 0
		/*
		 * Attach the on-board I/O bus at on a sun4m.
		 * In this case we treat the obio bus as another sbus slot.
		 */
		union obio_softc *usc = device_private(self);
		struct sbus_softc *sc = &usc->sc_sbus;

		static const char *const special4m[] = {
			/* find these first */
			"eeprom",
			"counter",
#if 0 /* Not all sun4m's have an `auxio' */
			"auxio",
#endif
			"",
			/* place device to ignore here */
			"interrupt",
			NULL
		};

		sc->sc_dev = self;
		sc->sc_bustag = ma->ma_bustag;
		sc->sc_dmatag = ma->ma_dmatag;
		sc->sc_intr2ipl = intr_obio2ipl;

		sbus_attach_common(sc, "obio", ma->ma_node, special4m);
#endif
	} else {
		printf("obio on this machine?\n");
	}
}

#if defined(SUN4)
static int
obioprint(void *args, const char *busname)
{
	union obio_attach_args *uoba = args;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;

	aprint_normal(" addr 0x%lx", (u_long)BUS_ADDR_PADDR(oba->oba_paddr));
	if (oba->oba_pri != -1)
		aprint_normal(" level %d", oba->oba_pri);

	return (UNCONF);
}

static int
_obio_bus_map(bus_space_tag_t t, bus_addr_t ba, bus_size_t size, int flags,
	      vaddr_t va, bus_space_handle_t *hp)
{

	if ((flags & OBIO_BUS_MAP_USE_ROM) != 0 &&
	     obio_find_rom_map(ba, size, hp) == 0)
		return (0);

	return (bus_space_map2(t->parent, ba, size, flags, va, hp));
}

static paddr_t
obio_bus_mmap(bus_space_tag_t t, bus_addr_t ba, off_t off, int prot, int flags)
{

	return (bus_space_mmap(t->parent, ba, off, prot, flags));
}

static int
obiosearch(device_t parent, struct cfdata *cf, const int *ldesc,
	   void *aux)
{
	struct obio4_busattachargs *oap = aux;
	union obio_attach_args uoba;
	struct obio4_attach_args *oba = &uoba.uoba_oba4;
	int addr;

	/* Check whether we're looking for a specifically named device */
	if (oap->name != NULL && strcmp(oap->name, cf->cf_name) != 0)
		return (0);

	/*
	 * Avoid sun4m entries which don't have valid PAs.
	 * no point in even probing them.
	 */
	addr = cf->cf_loc[OBIOCF_ADDR];
	if (addr == -1)
		return (0);

	/*
	 * On the 4/100 obio addresses must be mapped at
	 * 0x0YYYYYYY, but alias higher up (we avoid the
	 * alias condition because it causes pmap difficulties)
	 * XXX: We also assume that 4/[23]00 obio addresses
	 * must be 0xZYYYYYYY, where (Z != 0)
	 */
	if (cpuinfo.cpu_type == CPUTYP_4_100 && (addr & 0xf0000000))
		return (0);
	if (cpuinfo.cpu_type != CPUTYP_4_100 && !(addr & 0xf0000000))
		return (0);

	uoba.uoba_isobio4 = 1;
	oba->oba_bustag = &obio_space_tag;
	oba->oba_dmatag = oap->ma->ma_dmatag;
	oba->oba_paddr = BUS_ADDR(PMAP_OBIO, addr);
	oba->oba_pri = cf->cf_loc[OBIOCF_LEVEL];

	if (config_match(parent, cf, &uoba) == 0)
		return (0);

	config_attach(parent, cf, &uoba, obioprint);
	return (1);
}


/*
 * If we can find a mapping that was established by the rom, use it.
 * Else, create a new mapping.
 */
int
obio_find_rom_map(bus_addr_t ba, int len, bus_space_handle_t *hp)
{
#define	getpte(va)		lda(va, ASI_PTE)

	u_long	pa, pf;
	int	pgtype;
	u_long	va, pte;

	if (len > PAGE_SIZE)
		return (EINVAL);

	pa = BUS_ADDR_PADDR(ba);
	pf = pa >> PGSHIFT;
	pgtype = PMAP_T2PTE_4(PMAP_OBIO);

	for (va = OLDMON_STARTVADDR; va < OLDMON_ENDVADDR; va += PAGE_SIZE) {
		pte = getpte(va);
		if ((pte & PG_V) == 0 || (pte & PG_TYPE) != pgtype ||
		    (pte & PG_PFNUM) != pf)
			continue;

		/*
		 * Found entry in PROM's pagetable
		 * note: preserve page offset
		 */
		*hp = (bus_space_handle_t)(va | (pa & PGOFSET));
		return (0);
	}

	return (ENOENT);
}
#endif /* SUN4 */
