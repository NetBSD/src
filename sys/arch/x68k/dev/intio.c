/*	$NetBSD: intio.c,v 1.1.2.1 1998/12/23 16:47:29 minoura Exp $	*/

/*
 *
 * Copyright (c) 1998 NetBSD Foundation, Inc.
 * All rights reserved.
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
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NetBSD/x68k internal I/O virtual bus.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/mfp.h>


/*
 * bus_space(9) interface
 */
static int intio_bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *));
static void intio_bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));
static int intio_bus_space_subregion __P((bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *));

static struct x68k_bus_space intio_bus = {
#if 0
	X68K_INTIO_BUS,
#endif
	intio_bus_space_map, intio_bus_space_unmap, intio_bus_space_subregion,
	x68k_bus_space_alloc, x68k_bus_space_free,
#if 0
	x68k_bus_space_barrier,
#endif
	x68k_bus_space_read_1, x68k_bus_space_read_2, x68k_bus_space_read_4,
	x68k_bus_space_read_multi_1, x68k_bus_space_read_multi_2,
	x68k_bus_space_read_multi_4,
	x68k_bus_space_read_region_1, x68k_bus_space_read_region_2,
	x68k_bus_space_read_region_4,

	x68k_bus_space_write_1, x68k_bus_space_write_2, x68k_bus_space_write_4,
	x68k_bus_space_write_multi_1, x68k_bus_space_write_multi_2,
	x68k_bus_space_write_multi_4,
	x68k_bus_space_write_region_1, x68k_bus_space_write_region_2,
	x68k_bus_space_write_region_4,

	x68k_bus_space_set_region_1, x68k_bus_space_set_region_2,
	x68k_bus_space_set_region_4,
	x68k_bus_space_copy_region_1, x68k_bus_space_copy_region_2,
	x68k_bus_space_copy_region_4,

	0
};


/*
 * autoconf stuff
 */
static int intio_match __P((struct device *, struct cfdata *, void *));
static void intio_attach __P((struct device *, struct device *, void *));
static int intio_search __P((struct device *, struct cfdata *cf, void *));
static int intio_print __P((void *, const char *));
static void intio_alloc_system_ports __P((struct intio_softc*));

struct cfattach intio_ca = {
	sizeof(struct intio_softc), intio_match, intio_attach
};

static struct intio_interrupt_vector {
	intio_intr_handler_t	iiv_handler;
	void			*iiv_arg;
} iiv[256] = {0,};

extern struct cfdriver intio_cd;

/* used in console initialization */
extern int x68k_realconfig;
int x68k_config_found __P((struct cfdata *, struct device *,
			   void *, cfprint_t));
static struct cfdata *cfdata_intiobus = NULL;

static int
intio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;		/* NULL */
{
	if (strcmp(aux, intio_cd.cd_name) != 0)
		return (0);
	if (cf->cf_unit != 0)
		return (0);
	if (x68k_realconfig == 0)
		cfdata_intiobus = cf; /* XXX */

	return (1);
}


/* used in console initialization: configure only MFP */
static struct intio_attach_args initial_ia = {
	&intio_bus,
	0/*XXX*/,

	MFP_ADDR,		/* ia_addr */
	MFP_INTR,		/* ia_intr */
	-1,			/* ia_errintr */
	-1			/* ia_dma */
};

static void
intio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;		/* NULL */
{
	struct intio_softc *sc = (struct intio_softc *)self;
	struct intio_attach_args ia;

	if (self == NULL) {
		/* console only init */
		x68k_config_found(cfdata_intiobus, NULL, &initial_ia, NULL);
		return;
	}

	printf (" mapped at 0x%08p\n", intiobase);

	sc->sc_map = extent_create("intiomap",
				  PHYS_INTIODEV,
				  PHYS_INTIODEV + 0x400000,
				  M_DEVBUF, NULL, NULL, EX_NOWAIT);
	intio_alloc_system_ports (sc);

	sc->sc_bst = &intio_bus;
	sc->sc_bst->x68k_bus_device = self;
#if 0
	sc->sc_dmat = &intio_dma;
#endif
	bzero(iiv, sizeof (struct intio_interrupt_vector) * 256);

	ia.ia_bst = sc->sc_bst;
	ia.ia_dmat = sc->sc_dmat;

	config_search (intio_search, self, &ia);
}

static int
intio_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct intio_attach_args *ia = aux;
	struct intio_softc *sc = (struct intio_softc *)parent;

	ia->ia_bst = sc->sc_bst;
	ia->ia_dmat = sc->sc_dmat;
	ia->ia_addr = cf->cf_addr;
	ia->ia_intr = cf->cf_intr;
	ia->ia_errintr = cf->cf_errintr;
	ia->ia_dma = cf->cf_dma;

	if ((*cf->cf_attach->ca_match)(parent, cf, ia) > 0)
		config_attach(parent, cf, ia, intio_print);

	return (0);
}

static int
intio_print(aux, name)
	void *aux;
	const char *name;
{
	struct intio_attach_args *ia = aux;

/*	if (ia->ia_addr > 0)	*/
		printf (" addr 0x%06x", ia->ia_addr);
	if (ia->ia_intr > 0) {
		printf (" interrupting at 0x%02x", ia->ia_intr);
		if (ia->ia_errintr > 0)
			printf (" and 0x%02x", ia->ia_errintr);
	}
	if (ia->ia_dma >= 0)
		printf (" using DMA ch%d", ia->ia_dma);

	return (QUIET);
}

/*
 * intio memory map manager
 */

int
intio_map_allocate_region(parent, ia, flag)
	struct device *parent;
	struct intio_attach_args *ia;
	enum intio_map_flag flag; /* INTIO_MAP_TESTONLY or INTIO_MAP_ALLOCATE */
{
	struct intio_softc *sc = (struct intio_softc*) parent;
	struct extent *map = sc->sc_map;
	int r;

	r = extent_alloc_region (map, ia->ia_addr, ia->ia_size, 0);
#ifdef DEBUG
	extent_print (map);
#endif
	if (r == 0) {
		if (flag != INTIO_MAP_ALLOCATE)
		extent_free (map, ia->ia_addr, ia->ia_size, 0);
		return 0;
	} 

	return -1;
}

int
intio_map_free_region(parent, ia)
	struct device *parent;
	struct intio_attach_args *ia;
{
	struct intio_softc *sc = (struct intio_softc*) parent;
	struct extent *map = sc->sc_map;

	extent_free (map, ia->ia_addr, ia->ia_size, 0);
#ifdef DEBUG
	extent_print (map);
#endif
	return 0;
}

void
intio_alloc_system_ports(sc)
	struct intio_softc *sc;
{
	extent_alloc_region (sc->sc_map, INTIO_SYSPORT, 16, 0);
}


/*
 * intio bus space stuff.
 */
static int
intio_bus_space_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	/*
	 * Intio bus is mapped permanently.
	 */
	*bshp = (bus_space_handle_t)
	  ((u_int) bpa - PHYS_INTIODEV + intiobase);

	return (0);
}

static void
intio_bus_space_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	return;
}

static int
intio_bus_space_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}


/*
 * interrupt handler
 */
int
intio_intr_establish (vector, name, handler, arg)
	int vector;
	const char *name;	/* XXX */
	intio_intr_handler_t handler;
	void *arg;
{
	if (vector < 16)
		panic ("Invalid interrupt vector");
	if (iiv[vector].iiv_handler)
		return EBUSY;
	iiv[vector].iiv_handler = handler;
	iiv[vector].iiv_arg = arg;

	return 0;
}

int
intio_intr_disestablish (vector, arg)
	int vector;
	void *arg;
{
	if (iiv[vector].iiv_handler == 0 || iiv[vector].iiv_arg != arg)
		return EINVAL;
	iiv[vector].iiv_handler = 0;
	iiv[vector].iiv_arg = 0;

	return 0;
}

int
intio_intr (frame)
	struct frame *frame;
{
	int vector = frame->f_vector / 4;

	/* CAUTION: HERE WE ARE IN SPLHIGH() */
	/* LOWER TO APPROPRIATE IPL AT VERY FIRST IN THE HANDLER!! */

	if (iiv[vector].iiv_handler == 0) {
		printf ("Stray interrupt: %d type %x\n", vector, frame->f_format);
		return 0;
	}

	return (*(iiv[vector].iiv_handler)) (iiv[vector].iiv_arg);
}
