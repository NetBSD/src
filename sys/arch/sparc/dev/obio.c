/*	$NetBSD: obio.c,v 1.50.4.3 2002/04/17 00:04:24 nathanw Exp $	*/

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


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <sparc/dev/sbusvar.h>
#include <machine/autoconf.h>
#include <machine/oldmon.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>

struct obio4_softc {
	struct device	sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;	/* parent bus tag */
	bus_dma_tag_t	sc_dmatag;	/* parent bus dma tag */
};

union obio_softc {
	struct	device sc_dev;		/* base device */
	struct	obio4_softc sc_obio;	/* sun4 obio */
	struct	sbus_softc sc_sbus;	/* sun4m obio is another sbus slot */
};


/* autoconfiguration driver */
static	int obiomatch  __P((struct device *, struct cfdata *, void *));
static	void obioattach __P((struct device *, struct device *, void *));

struct cfattach obio_ca = {
	sizeof(union obio_softc), obiomatch, obioattach
};


/*
 * This `obio4_busattachargs' data structure only exists to pass down
 * to obiosearch() the name of a device that must be configured early.
 */
struct obio4_busattachargs {
	struct mainbus_attach_args	*ma;
	const char			*name;
};

#if defined(SUN4)
static	int obioprint  __P((void *, const char *));
static	int obiosearch   __P((struct device *, struct cfdata *, void *));
static	paddr_t obio_bus_mmap __P((bus_space_tag_t, bus_addr_t, off_t,
			       int, int));
static	int _obio_bus_map __P((bus_space_tag_t, bus_addr_t,
			       bus_size_t, int,
			       vaddr_t, bus_space_handle_t *));

static struct sparc_bus_space_tag obio_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	_obio_bus_map,			/* bus_space_map */ 
	NULL,				/* bus_space_unmap */
	NULL,				/* bus_space_subregion */
	NULL,				/* bus_space_barrier */ 
	obio_bus_mmap,			/* bus_space_mmap */ 
	NULL				/* bus_intr_establish */
}; 
#endif

/*
 * Translate obio `interrupts' property value to processor IPL (see sbus.c)
 * Apparently, the `interrupts' property on obio devices is just
 * the processor IPL.
 */
static int intr_obio2ipl[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

int
obiomatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0);
}

void
obioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/*
	 * There is only one obio bus
	 */
	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}
	printf("\n");

	if (CPU_ISSUN4) {
#if defined(SUN4)
		struct obio4_softc *sc = &((union obio_softc *)self)->sc_obio;
		struct obio4_busattachargs oa;
		const char *const *cpp;
		static const char *const special4[] = {
			/* find these first */
			"timer",
			"dma",		/* need this before `esp', if any */
			NULL
		};

		sc->sc_bustag = ma->ma_bustag;
		sc->sc_dmatag = ma->ma_dmatag;

		obio_space_tag.cookie = sc;
		obio_space_tag.parent = sc->sc_bustag;

		oa.ma = ma;

		/* Find all `early' obio devices */
		for (cpp = special4; *cpp != NULL; cpp++) {
			oa.name = *cpp;
			(void)config_search(obiosearch, self, &oa);
		}

		/* Find all other obio devices */
		oa.name = NULL;
		(void)config_search(obiosearch, self, &oa);
#endif
		return;
	} else if (CPU_ISSUN4M) {
		/*
		 * Attach the on-board I/O bus at on a sun4m.
		 * In this case we treat the obio bus as another sbus slot.
		 */
		struct sbus_softc *sc = &((union obio_softc *)self)->sc_sbus;

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

		sc->sc_bustag = ma->ma_bustag;
		sc->sc_dmatag = ma->ma_dmatag;
		sc->sc_intr2ipl = intr_obio2ipl;

		sbus_attach_common(sc, "obio", ma->ma_node, special4m);
	} else {
		printf("obio on this machine?\n");
	}
}

#if defined(SUN4)
int
obioprint(args, busname)
	void *args;
	const char *busname;
{
	union obio_attach_args *uoba = args;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;

	printf(" addr 0x%lx", (u_long)BUS_ADDR_PADDR(oba->oba_paddr));
	if (oba->oba_pri != -1)
		printf(" level %d", oba->oba_pri);

	return (UNCONF);
}

int
_obio_bus_map(t, ba, size, flags, va, hp)
	bus_space_tag_t t;
	bus_addr_t ba;
	bus_size_t size;
	int	flags;
	vaddr_t va;
	bus_space_handle_t *hp;
{
	struct obio4_softc *sc = t->cookie;

	if ((flags & OBIO_BUS_MAP_USE_ROM) != 0 &&
	     obio_find_rom_map(ba, size, hp) == 0)
		return (0);

	return (bus_space_map2(sc->sc_bustag, ba, size, flags, va, hp));
}

paddr_t
obio_bus_mmap(t, ba, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t ba;
	off_t off;
	int prot;
	int flags;
{
	struct obio4_softc *sc = t->cookie;

	return (bus_space_mmap(sc->sc_bustag, ba, off, prot, flags));
}

int
obiosearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct obio4_busattachargs *oap = aux;
	union obio_attach_args uoba;
	struct obio4_attach_args *oba = &uoba.uoba_oba4;

	/* Check whether we're looking for a specifically named device */
	if (oap->name != NULL && strcmp(oap->name, cf->cf_driver->cd_name) != 0)
		return (0);

	/*
	 * Avoid sun4m entries which don't have valid PAs.
	 * no point in even probing them. 
	 */
	if (cf->cf_loc[0] == -1)
		return (0);

	/*
	 * On the 4/100 obio addresses must be mapped at
	 * 0x0YYYYYYY, but alias higher up (we avoid the
	 * alias condition because it causes pmap difficulties)
	 * XXX: We also assume that 4/[23]00 obio addresses
	 * must be 0xZYYYYYYY, where (Z != 0)
	 */
	if (cpuinfo.cpu_type == CPUTYP_4_100 && (cf->cf_loc[0] & 0xf0000000))
		return (0);
	if (cpuinfo.cpu_type != CPUTYP_4_100 && !(cf->cf_loc[0] & 0xf0000000))
		return (0);

	uoba.uoba_isobio4 = 1;
	oba->oba_bustag = &obio_space_tag;
	oba->oba_dmatag = oap->ma->ma_dmatag;
	oba->oba_paddr = BUS_ADDR(PMAP_OBIO, cf->cf_loc[0]);
	oba->oba_pri = cf->cf_loc[1];

	if ((*cf->cf_attach->ca_match)(parent, cf, &uoba) == 0)
		return (0);

	config_attach(parent, cf, &uoba, obioprint);
	return (1);
}


/*
 * If we can find a mapping that was established by the rom, use it.
 * Else, create a new mapping.
 */
int
obio_find_rom_map(ba, len, hp)
	bus_addr_t	ba;
	int		len;
	bus_space_handle_t *hp;
{
#define	getpte(va)		lda(va, ASI_PTE)

	u_long	pa, pf;
	int	pgtype;
	u_long	va, pte;

	if (len > NBPG)
		return (EINVAL);

	pa = BUS_ADDR_PADDR(ba);
	pf = pa >> PGSHIFT;
	pgtype = PMAP_T2PTE_4(PMAP_OBIO);

	for (va = OLDMON_STARTVADDR; va < OLDMON_ENDVADDR; va += NBPG) {
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
