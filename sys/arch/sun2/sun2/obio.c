/*	$NetBSD: obio.c,v 1.15 2005/06/30 17:03:54 drochner Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Matthew Fredette.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.15 2005/06/30 17:03:54 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun2/sun2/control.h>
#include <sun2/sun2/machdep.h>

/* Does this machine have a Multibus? */
extern int cpu_has_multibus;

static int  obio_match(struct device *, struct cfdata *, void *);
static void obio_attach(struct device *, struct device *, void *);

struct obio_softc {
	struct device	sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;	/* parent bus tag */
	bus_dma_tag_t	sc_dmatag;	/* parent bus dma tag */
};

CFATTACH_DECL(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

static int obio_attached;

static	paddr_t obio_bus_mmap(bus_space_tag_t, bus_type_t, bus_addr_t, off_t,
	    int, int);
static	int _obio_bus_map(bus_space_tag_t, bus_type_t, bus_addr_t, bus_size_t,
	    int, vaddr_t, bus_space_handle_t *);
static	int _obio_addr_bad(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    size_t);
static	int _obio_bus_peek(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    size_t, void *);
static	int _obio_bus_poke(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    size_t, uint32_t);

static struct sun68k_bus_space_tag obio_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	_obio_bus_map,			/* bus_space_map */ 
	NULL,				/* bus_space_unmap */
	NULL,				/* bus_space_subregion */
	NULL,				/* bus_space_barrier */ 
	obio_bus_mmap,			/* bus_space_mmap */ 
	NULL,				/* bus_intr_establish */
	_obio_bus_peek,			/* bus_space_peek_N */
	_obio_bus_poke			/* bus_space_poke_N */
}; 

static int 
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (obio_attached)
		return 0;

	return (ma->ma_name == NULL || strcmp(cf->cf_name, ma->ma_name) == 0);
}

static void 
obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct obio_softc *sc = (struct obio_softc *)self;
	struct obio_attach_args oba;
	const char *const *cpp;
	static const char *const special[] = {
		/* find these first */
		NULL
	};

	obio_attached = 1;

	printf("\n");

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	obio_space_tag.cookie = sc;
	obio_space_tag.parent = sc->sc_bustag;

	/*
	 * Prepare the skeleton attach arguments for our devices.
	 * The values we give in the locators are indications to
	 * sun68k_bus_search about which locators must and must not
	 * be defined.
	 */
	oba = *ma;
	oba.oba_bustag = &obio_space_tag;
	oba.oba_paddr = LOCATOR_REQUIRED;
	oba.oba_pri = LOCATOR_OPTIONAL;

	/* Find all `early' obio devices */
	for (cpp = special; *cpp != NULL; cpp++) {
		oba.oba_name = *cpp;
		config_search_ia(sun68k_bus_search, self, "obio", &oba);
	}

	/* Find all other obio devices */
	oba.oba_name = NULL;
	config_search_ia(sun68k_bus_search, self, "obio", &oba);
}

int
_obio_bus_map(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr,
    bus_size_t size, int flags, vaddr_t vaddr, bus_space_handle_t *hp)
{
	struct obio_softc *sc = t->cookie;

	return (bus_space_map2(sc->sc_bustag, PMAP_OBIO, paddr, size,
			       flags | _SUN68K_BUS_MAP_USE_PROM, vaddr, hp));
}

paddr_t
obio_bus_mmap(bus_space_tag_t t, bus_type_t btype, bus_addr_t paddr, off_t off,
    int prot, int flags)
{
	struct obio_softc *sc = t->cookie;

	return (bus_space_mmap2(sc->sc_bustag, PMAP_OBIO, paddr, off,
				prot, flags));
}

/*
 * The sun2 obio bus doesn't give bus errors, so we check on 
 * probed obio physical addresses to make sure they're ok.
 */
int
_obio_addr_bad(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, size_t s)
{
	u_int pte;
	paddr_t pa;

	/* Get the physical address for this page. */
	pte = get_pte((vaddr_t) (h + o));
	if (!(pte & PG_VALID))
		return (-1);
	pa = PG_PA(pte);

	/*
	 * Return nonzero if it's bad.  All sun2 Multibus
	 * machines have all obio devices between 0x2000
	 * and 0x4000, and all sun2 VME machines have all 
	 * obio devices outside of this range.
	 */
	return ((!!cpu_has_multibus) != (pa >= 0x2000 && pa < 0x4000));
}
	
int
_obio_bus_peek(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, size_t s,
    void *vp)
{
	struct obio_softc *sc = t->cookie;

	return (_obio_addr_bad(t, h, o, s) ||
		_bus_space_peek(sc->sc_bustag, h, o, s, vp));
}

int
_obio_bus_poke(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, size_t s,
    uint32_t v)
{
	struct obio_softc *sc = t->cookie;

	return (_obio_addr_bad(t, h, o, s) ||
		_bus_space_poke(sc->sc_bustag, h, o, s, v));
}
