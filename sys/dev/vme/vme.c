/* $NetBSD: vme.c,v 1.3 1999/06/30 15:06:05 drochner Exp $ */

/*
 * Copyright (c) 1999
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

static void vme_extractlocators __P((int*, struct vme_attach_args*));
static int vmeprint __P((struct vme_attach_args*, char*));
static int vmesubmatch1 __P((struct device*, struct cfdata*, void*));
static int vmesubmatch __P((struct device*, struct cfdata*, void*));
int vmematch __P((struct device *, struct cfdata *, void *));
void vmeattach __P((struct device*, struct device*,void*));
static struct extent *vme_select_map __P((struct vmebus_softc*, vme_am_t));

#ifdef notyet
int vmedetach __P((struct device*));
#endif

#define VME_SLAVE_DUMMYDRV "vme_slv"

#define VME_NUMCFRANGES 3 /* cf. "files.vme" */

struct cfattach vme_ca = {
	sizeof(struct vmebus_softc), vmematch, vmeattach,
};

struct cfattach vme_slv_ca = {
	0	/* never used */
};

static void
vme_extractlocators(loc, aa)
	int *loc;
	struct vme_attach_args *aa;
{
	int i = 0;

	/* XXX can't use constants in locators.h this way */

	while (i < VME_NUMCFRANGES && i < VME_MAXCFRANGES &&
	       loc[i] != -1) {
		aa->r[i].offset = (vme_addr_t)loc[i];
		aa->r[i].size = (vme_size_t)loc[3 + i];
		aa->r[i].am = (vme_am_t)loc[6 + i];
		i++;
	}
	aa->numcfranges = i;
	aa->ilevel = loc[9];
	aa->ivector = loc[10];
}

static int
vmeprint(v, dummy)
	struct vme_attach_args *v;
	char *dummy;
{
	int i;

	for (i = 0; i < v->numcfranges; i++) {
		printf(" addr %x", v->r[i].offset);
		if (v->r[i].size != -1)
			printf("-%x", v->r[i].offset + v->r[i].size - 1);
		if (v->r[i].am != -1)
			printf(" am %02x", v->r[i].am);
	}
	if (v->ilevel != -1) {
		printf(" irq %d", v->ilevel);
		if (v->ivector != -1)
			printf(" vector %x", v->ivector);
	}
	return (UNCONF);
}

/*
 * This looks for a (dummy) vme device "VME_SLAVE_DUMMYDRV".
 * A callback provided by the bus's parent is called for every such
 * entry in the config database.
 * This is a special hack allowing to communicate the address settings
 * of the VME master's slave side to its driver via the normal
 * configuration mechanism.
 * Needed in following cases:
 *  -DMA windows are hardware settable but not readable by software
 *   (driver gets offsets for DMA address calculations this way)
 *  -DMA windows are software settable, but not persistent
 *   (hardware is set up from config file entry)
 *  -other adapter VME slave ranges which should be kept track of
 *   for address space accounting
 * In any case, the adapter driver must get the data before VME
 * devices are attached.
 */
static int
vmesubmatch1(bus, dev, aux)
	struct device *bus;
	struct cfdata *dev;
	void *aux;
{
	struct vmebus_softc *sc = (struct vmebus_softc*)bus;
	struct vme_attach_args v;

	if (strcmp(dev->cf_driver->cd_name, VME_SLAVE_DUMMYDRV))
		return (0);

	vme_extractlocators(dev->cf_loc, &v);

	v.va_vct = sc->sc_vct; /* for space allocation */

	(*sc->slaveconfig)(bus->dv_parent, &v);
	return (0);
}

static int
vmesubmatch(bus, dev, aux)
	struct device *bus;
	struct cfdata *dev;
	void *aux;
{
	struct vmebus_softc *sc = (struct vmebus_softc*)bus;
	struct vme_attach_args v;

	if (!strcmp(dev->cf_driver->cd_name, VME_SLAVE_DUMMYDRV))
		return (0);

	vme_extractlocators(dev->cf_loc, &v);

	v.va_vct = sc->sc_vct;
	v.va_bdt = sc->sc_bdt;

	if (dev->cf_attach->ca_match(bus, dev, &v)) {
		config_attach(bus, dev, &v, (cfprint_t)vmeprint);
		return (1);
	}
	return (0);
}

int
vmematch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
vmeattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)self;

	struct vmebus_attach_args *aa =
	    (struct vmebus_attach_args*)aux;

	sc->sc_vct = aa->va_vct;
	sc->sc_bdt = aa->va_bdt;

	/* the "bus" are we ourselves */
	sc->sc_vct->bus = sc;

	sc->slaveconfig = aa->va_slaveconfig;

	printf("\n");

	/*
	 * set up address space accounting - assume incomplete decoding
	 */
	sc->vme32ext = extent_create("vme32", 0, 0xffffffff,
				     M_DEVBUF, 0, 0, 0);
	if (!sc->vme32ext) {
		printf("error creating A32 map\n");
		return;
	}

	sc->vme24ext = extent_create("vme24", 0, 0x00ffffff,
				     M_DEVBUF, 0, 0, 0);
	if (!sc->vme24ext) {
		printf("error creating A24 map\n");
		return;
	}

	sc->vme16ext = extent_create("vme16", 0, 0x0000ffff,
				     M_DEVBUF, 0, 0, 0);
	if (!sc->vme16ext) {
		printf("error creating A16 map\n");
		return;
	}

	if (sc->slaveconfig) {
		/* first get info about the bus master's slave side,
		 if present */
		config_search((cfmatch_t)vmesubmatch1, self, 0);
	}
	config_search((cfmatch_t)vmesubmatch, self, 0);

#ifdef VMEDEBUG
	if (sc->vme32ext)
		extent_print(sc->vme32ext);
	if (sc->vme24ext)
		extent_print(sc->vme24ext);
	if (sc->vme16ext)
		extent_print(sc->vme16ext);
#endif
}

#ifdef notyet
int
vmedetach(dev)
	struct device *dev;
{
	struct vmebus_softc *sc = (struct vmebus_softc*)dev;

	if (sc->slaveconfig) {
		/* allow bus master to free its bus ressources */
		(*sc->slaveconfig)(dev->dv_parent, 0);
	}

	/* extent maps should be empty now */

	if (sc->vme32ext) {
#ifdef VMEDEBUG
		extent_print(sc->vme32ext);
#endif
		extent_destroy(sc->vme32ext);
	}
	if (sc->vme24ext) {
#ifdef VMEDEBUG
		extent_print(sc->vme24ext);
#endif
		extent_destroy(sc->vme24ext);
	}
	if (sc->vme16ext) {
#ifdef VMEDEBUG
		extent_print(sc->vme16ext);
#endif
		extent_destroy(sc->vme16ext);
	}

	return (0);
}
#endif

static struct extent *
vme_select_map(sc, ams)
	struct vmebus_softc *sc;
	vme_am_t ams;
{
	if ((ams & VME_AM_ADRSIZEMASK) == VME_AM_A32)
		return (sc->vme32ext);
	else if ((ams & VME_AM_ADRSIZEMASK) == VME_AM_A24)
		return (sc->vme24ext);
	else if ((ams & VME_AM_ADRSIZEMASK) == VME_AM_A16)
		return (sc->vme16ext);
	else
		return (0);
}

int
_vme_space_alloc(sc, addr, len, ams)
	struct vmebus_softc *sc;
	vme_addr_t addr;
	vme_size_t len;
	vme_am_t ams;
{
	struct extent *ex;

	ex = vme_select_map(sc, ams);
	if (!ex)
		return (EINVAL);

	return (extent_alloc_region(ex, addr, len, EX_NOWAIT));
}

void
_vme_space_free(sc, addr, len, ams)
	struct vmebus_softc *sc;
	vme_addr_t addr;
	vme_size_t len;
	vme_am_t ams;
{
	struct extent *ex;

	ex = vme_select_map(sc, ams);
	if (!ex) {
		panic("vme_space_free: invalid am %x", ams);
		return;
	}

	extent_free(ex, addr, len, EX_NOWAIT);
}

int
_vme_space_get(sc, len, ams, align, addr)
	struct vmebus_softc *sc;
	vme_size_t len;
	vme_am_t ams;
	u_long align;
	vme_addr_t *addr;
{
	struct extent *ex;

	ex = vme_select_map(sc, ams);
	if (!ex)
		return (EINVAL);

	return (extent_alloc(ex, len, align, EX_NOBOUNDARY, EX_NOWAIT,
			     (u_long *)addr));
}
