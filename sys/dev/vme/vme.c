/* $NetBSD: vme.c,v 1.31 2024/04/24 02:27:33 thorpej Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vme.c,v 1.31 2024/04/24 02:27:33 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/vmem.h>
#include <sys/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

static void vme_extractlocators(int*, struct vme_attach_args *);
static int vmeprint(struct vme_attach_args *, char *);
static int vmesubmatch1(device_t, cfdata_t, const int *, void *);
static int vmesubmatch(device_t, cfdata_t, const int *, void *);
int vmematch(device_t, cfdata_t, void *);
void vmeattach(device_t, device_t, void *);
static vmem_t *vme_select_map(struct vmebus_softc*, vme_am_t);

#define VME_SLAVE_DUMMYDRV "vme_slv"

#define VME_NUMCFRANGES 3 /* cf. "files.vme" */

CFATTACH_DECL_NEW(vme, sizeof(struct vmebus_softc),
    vmematch, vmeattach, NULL, NULL);

const struct cfattach vme_slv_ca = {
	0	/* never used */
};

static void
vme_extractlocators(int *loc, struct vme_attach_args *aa)
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
vmeprint(struct vme_attach_args *v, char *dummy)
{
	int i;

	for (i = 0; i < v->numcfranges; i++) {
		aprint_normal(" addr %x", v->r[i].offset);
		if (v->r[i].size != -1)
			aprint_normal("-%x", v->r[i].offset + v->r[i].size - 1);
		if (v->r[i].am != -1)
			aprint_normal(" am %02x", v->r[i].am);
	}
	if (v->ilevel != -1) {
		aprint_normal(" irq %d", v->ilevel);
		if (v->ivector != -1)
			aprint_normal(" vector %x", v->ivector);
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
vmesubmatch1(device_t bus, cfdata_t dev, const int *ldesc, void *aux)
{
	struct vmebus_softc *sc = device_private(bus);
	struct vme_attach_args v;

	if (strcmp(dev->cf_name, VME_SLAVE_DUMMYDRV))
		return (0);

	vme_extractlocators(dev->cf_loc, &v);

	v.va_vct = sc->sc_vct; /* for space allocation */

	(*sc->slaveconfig)(device_parent(bus), &v);
	return (0);
}

static int
vmesubmatch(device_t bus, cfdata_t dev, const int *ldesc, void *aux)
{
	struct vmebus_softc *sc = device_private(bus);
	struct vme_attach_args v;

	if (!strcmp(dev->cf_name, VME_SLAVE_DUMMYDRV))
		return (0);

	vme_extractlocators(dev->cf_loc, &v);

	v.va_vct = sc->sc_vct;
	v.va_bdt = sc->sc_bdt;

	if (config_probe(bus, dev, &v)) {
		config_attach(bus, dev, &v, (cfprint_t)vmeprint, CFARGS_NONE);
		return (1);
	}
	return (0);
}

int
vmematch(device_t parent, cfdata_t match, void *aux)
{
	return (1);
}

void
vmeattach(device_t parent, device_t self, void *aux)
{
	struct vmebus_softc *sc = device_private(self);

	struct vmebus_attach_args *aa = aux;

	sc->sc_vct = aa->va_vct;
	sc->sc_bdt = aa->va_bdt;

	/* the "bus" are we ourselves */
	sc->sc_vct->bus = sc;

	sc->slaveconfig = aa->va_slaveconfig;

	printf("\n");

	/*
	 * set up address space accounting - assume incomplete decoding
	 */
	sc->vme32_arena = vmem_create("vme32",
				      0,		/* base */
	/* XXX loses last byte */     0xffffffff,	/* size */
				      1,		/* quantum */
				      NULL,		/* allocfn */
				      NULL,		/* releasefn */
				      NULL,		/* source */
				      0,		/* qcache_max */
				      VM_SLEEP,
				      IPL_NONE);
	if (!sc->vme32_arena) {
		device_printf(self, "error creating A32 map\n");
		return;
	}

	sc->vme24_arena = vmem_create("vme24",
				      0,		/* base */
				      0x01000000,	/* size */
				      1,		/* quantum */
				      NULL,		/* allocfn */
				      NULL,		/* releasefn */
				      NULL,		/* source */
				      0,		/* qcache_max */
				      VM_SLEEP,
				      IPL_NONE);
	if (!sc->vme24_arena) {
		device_printf(self, "error creating A24 map\n");
		return;
	}

	sc->vme16_arena = vmem_create("vme16",
				      0,		/* base */
				      0x00010000,	/* size */
				      1,		/* quantum */
				      NULL,		/* allocfn */
				      NULL,		/* releasefn */
				      NULL,		/* source */
				      0,		/* qcache_max */
				      VM_SLEEP,
				      IPL_NONE);
	if (!sc->vme16_arena) {
		device_printf(self, "error creating A16 map\n");
		return;
	}

	if (sc->slaveconfig) {
		/* first get info about the bus master's slave side,
		 if present */
		config_search(self, NULL,
		    CFARGS(.search = vmesubmatch1));
	}
	config_search(self, NULL,
	    CFARGS(.search = vmesubmatch));

#if 0	/* XXX VMEDEBUG */
	if (sc->vme32_arena)
		vmem_print(sc->vme32_arena);
	if (sc->vme24_arena)
		vmem_print(sc->vme24_arena);
	if (sc->vme16_arena)
		vmem_print(sc->vme16_arena);
#endif
}

#ifdef notyet
int
vmedetach(device_t dev)
{
	struct vmebus_softc *sc = device_private(dev);

	if (sc->slaveconfig) {
		/* allow bus master to free its bus resources */
		(*sc->slaveconfig)(device_parent(dev), 0);
	}

	/* extent maps should be empty now */

	if (sc->vme32_arena) {
#ifdef VMEDEBUG
		vmem_print(sc->vme32_arena);
#endif
		vmem_destroy(sc->vme32_arena);
	}
	if (sc->vme24_arena) {
#ifdef VMEDEBUG
		vmem_print(sc->vme24_arena);
#endif
		vmem_destroy(sc->vme24_arena);
	}
	if (sc->vme16_arena) {
#ifdef VMEDEBUG
		vmem_print(sc->vme16_arena);
#endif
		vmem_destroy(sc->vme16_arena);
	}

	return (0);
}
#endif

static vmem_t *
vme_select_map(struct vmebus_softc *sc, vme_am_t ams)
{
	if ((ams & VME_AM_ADRSIZEMASK) == VME_AM_A32)
		return (sc->vme32_arena);
	else if ((ams & VME_AM_ADRSIZEMASK) == VME_AM_A24)
		return (sc->vme24_arena);
	else if ((ams & VME_AM_ADRSIZEMASK) == VME_AM_A16)
		return (sc->vme16_arena);
	else
		return (0);
}

int
_vme_space_alloc(struct vmebus_softc *sc, vme_addr_t addr, vme_size_t len, vme_am_t ams)
{
	vmem_t *vm;

	vm = vme_select_map(sc, ams);
	if (!vm)
		return (EINVAL);

	return vmem_xalloc_addr(vm, addr, len, VM_NOSLEEP);
}

void
_vme_space_free(struct vmebus_softc *sc, vme_addr_t addr, vme_size_t len, vme_am_t ams)
{
	vmem_t *vm;

	vm = vme_select_map(sc, ams);
	if (!vm) {
		panic("vme_space_free: invalid am %x", ams);
		return;
	}

	vmem_xfree(vm, addr, len);
}

int
_vme_space_get(struct vmebus_softc *sc, vme_size_t len, vme_am_t ams, u_long align, vme_addr_t *addr)
{
	vmem_t *vm;
	vmem_addr_t help;
	int res;

	vm = vme_select_map(sc, ams);
	if (!vm)
		return (EINVAL);

	res = vmem_xalloc(vm, len,
			  align,		/* align */
			  0,			/* phase */
			  0,			/* nocross */
			  VMEM_ADDR_MIN,	/* minaddr */
			  VMEM_ADDR_MAX,	/* maxaddr */
			  VM_BESTFIT | VM_NOSLEEP,
			  &help);
	if (!res)
		*addr = help;
	return (res);
}
