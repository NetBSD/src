/*	$NetBSD: bootbus.c,v 1.24 2022/01/22 11:49:16 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Autoconfiguration support for Sun4d "bootbus".
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bootbus.c,v 1.24 2022/01/22 11:49:16 thorpej Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <sparc/sparc/cpuunitvar.h>
#include <sparc/dev/bootbusvar.h>

#include "locators.h"

struct bootbus_softc {
	int sc_node;				/* our OBP node */

	bus_space_tag_t sc_st;			/* ours */
	bus_space_tag_t sc_bustag;		/* passed on to children */
};

static int bootbus_match(device_t, cfdata_t, void *);
static void bootbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bootbus, sizeof(struct bootbus_softc),
    bootbus_match, bootbus_attach, NULL, NULL);

static int bootbus_submatch(device_t, cfdata_t, const int *, void *);
static int bootbus_print(void *, const char *);

static int bootbus_setup_attach_args(struct bootbus_softc *, bus_space_tag_t,
    int, struct bootbus_attach_args *);
static void bootbus_destroy_attach_args(struct bootbus_attach_args *);

static int
bootbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpuunit_attach_args *cpua = aux;

	if (strcmp(cpua->cpua_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static void
bootbus_attach(device_t parent, device_t self, void *aux)
{
	struct bootbus_softc *sc = device_private(self);
	struct cpuunit_attach_args *cpua = aux;
	int node, error;

	sc->sc_node = cpua->cpua_node;
	sc->sc_st = cpua->cpua_bustag;

	printf("\n");

	/*
	 * Initialize the bus space tag we pass on to our children.
	 */
	sc->sc_bustag = kmem_zalloc(sizeof(*sc->sc_bustag), KM_SLEEP);
	sc->sc_bustag->cookie = sc;
	sc->sc_bustag->parent = sc->sc_st;
	sc->sc_bustag->sparc_bus_map = sc->sc_st->sparc_bus_map;
	sc->sc_bustag->sparc_bus_mmap = sc->sc_st->sparc_bus_mmap;

	/*
	 * Collect address translations from the OBP.
	 */
	error = prom_getprop(sc->sc_node, "ranges",
	    sizeof(struct openprom_range), &sc->sc_bustag->nranges,
	    &sc->sc_bustag->ranges);
	if (error) {
		printf("%s: error %d getting \"ranges\" property\n",
		    device_xname(self), error);
		panic("bootbus_attach");
	}

	/* Attach the CPU (and possibly bootbus) child nodes. */
	devhandle_t selfh = device_handle(self);
	for (node = firstchild(sc->sc_node); node != 0;
	     node = nextsibling(node)) {
		struct bootbus_attach_args baa;

		if (bootbus_setup_attach_args(sc, sc->sc_bustag, node, &baa))
			panic("bootbus_attach: failed to set up attach args");

		config_found(self, &baa, bootbus_print,
		    CFARGS(.devhandle = prom_node_to_devhandle(selfh, node),
			   .submatch = bootbus_submatch));

		bootbus_destroy_attach_args(&baa);
	}
}

static int
bootbus_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct bootbus_attach_args *baa = aux;

	if (cf->cf_loc[BOOTBUSCF_SLOT] != BOOTBUSCF_SLOT_DEFAULT &&
	    cf->cf_loc[BOOTBUSCF_SLOT] != baa->ba_slot)
		return (0);

	if (cf->cf_loc[BOOTBUSCF_OFFSET] != BOOTBUSCF_OFFSET_DEFAULT &&
	    cf->cf_loc[BOOTBUSCF_OFFSET] != baa->ba_offset)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
bootbus_print(void *aux, const char *pnp)
{
	struct bootbus_attach_args *baa = aux;
	int i;

	if (pnp)
		aprint_normal("%s at %s", baa->ba_name, pnp);
	aprint_normal(" slot %d offset 0x%x", baa->ba_slot, baa->ba_offset);
	for (i = 0; i < baa->ba_nintr; i++)
		aprint_normal(" ipl %d", baa->ba_intr[i].oi_pri);

	return (UNCONF);
}

static int
bootbus_setup_attach_args(struct bootbus_softc *sc, bus_space_tag_t bustag,
    int node, struct bootbus_attach_args *baa)
{
	int n, error;

	memset(baa, 0, sizeof(*baa));

	error = prom_getprop(node, "name", 1, &n, &baa->ba_name);
	if (error)
		return (error);
	baa->ba_name[n] = '\0';

	baa->ba_bustag = bustag;
	baa->ba_node = node;

	error = prom_getprop(node, "reg", sizeof(struct openprom_addr),
	    &baa->ba_nreg, &baa->ba_reg);
	if (error) {
		bootbus_destroy_attach_args(baa);
		return (error);
	}

	error = prom_getprop(node, "intr", sizeof(struct openprom_intr),
	    &baa->ba_nintr, &baa->ba_intr);
	if (error != 0 && error != ENOENT) {
		bootbus_destroy_attach_args(baa);
		return (error);
	}

	error = prom_getprop(node, "address", sizeof(uint32_t),
	    &baa->ba_npromvaddrs, &baa->ba_promvaddrs);
	if (error != 0 && error != ENOENT) {
		bootbus_destroy_attach_args(baa);
		return (error);
	}

	return (0);
}

static void
bootbus_destroy_attach_args(struct bootbus_attach_args *baa)
{

	if (baa->ba_name != NULL)
		free(baa->ba_name, M_DEVBUF);

	if (baa->ba_reg != NULL)
		free(baa->ba_reg, M_DEVBUF);

	if (baa->ba_intr != NULL)
		free(baa->ba_intr, M_DEVBUF);

	if (baa->ba_promvaddrs != NULL)
		free(baa->ba_promvaddrs, M_DEVBUF);
}
