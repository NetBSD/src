/*	$NetBSD: cpuunit.c,v 1.15 2011/07/17 23:18:23 mrg Exp $	*/

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
 * Autoconfiguration support for Sun4d "cpu units".
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpuunit.c,v 1.15 2011/07/17 23:18:23 mrg Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <sparc/sparc/cpuunitvar.h>

struct cpuunit_softc {
	int sc_node;				/* our OBP node */

	bus_space_tag_t sc_st;			/* ours */
	bus_space_tag_t sc_bustag;		/* passed on to children */

	int sc_device_id;			/* device-id */
	int sc_board;				/* board number */
};

static int cpuunit_match(device_t, cfdata_t, void *);
static void cpuunit_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpuunit, sizeof(struct cpuunit_softc),
    cpuunit_match, cpuunit_attach, NULL, NULL);

static int cpuunit_print(void *, const char *);

static int cpuunit_setup_attach_args(struct cpuunit_softc *, bus_space_tag_t,
    int, struct cpuunit_attach_args *);
static void cpuunit_destroy_attach_args(struct cpuunit_attach_args *);

static int
cpuunit_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, "cpu-unit") == 0)
		return (1);

	return (0);
}

static void
cpuunit_attach(device_t parent, device_t self, void *aux)
{
	struct cpuunit_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int node, error;
	bus_space_tag_t sbt;

	sc->sc_node = ma->ma_node;
	sc->sc_st = ma->ma_bustag;

	sc->sc_device_id = prom_getpropint(sc->sc_node, "device-id", -1);
	sc->sc_board = prom_getpropint(sc->sc_node, "board#", -1);

	printf(": board #%d, ID %d\n", sc->sc_board, sc->sc_device_id);

	/*
	 * Initialize the bus space tag we pass on to our children.
	 */
	sbt = sc->sc_bustag = malloc(sizeof(*sbt), M_DEVBUF, M_WAITOK);
	memcpy(sbt, sc->sc_st, sizeof(*sbt));
	sbt->cookie = sc;
	sbt->parent = sc->sc_st;
	sbt->nranges = 0;
	sbt->ranges = NULL;

	/*
	 * Collect address translations from the OBP.
	 */
	error = prom_getprop(sc->sc_node, "ranges",
	    sizeof(struct openprom_range), &sbt->nranges, &sbt->ranges);
	if (error) {
		printf("%s: error %d getting \"ranges\" property\n",
		    device_xname(self), error);
		panic("cpuunit_attach");
	}

	/* Attach the CPU (and possibly bootbus) child nodes. */
	for (node = firstchild(sc->sc_node); node != 0;
	     node = nextsibling(node)) {
		struct cpuunit_attach_args cpua;

		if (cpuunit_setup_attach_args(sc, sbt, node, &cpua))
			panic("cpuunit_attach: failed to set up attach args");

		(void) config_found(self, &cpua, cpuunit_print);

		cpuunit_destroy_attach_args(&cpua);
	}
}

static int
cpuunit_print(void *aux, const char *pnp)
{
	struct cpuunit_attach_args *cpua = aux;

	if (pnp)
		aprint_normal("%s at %s", cpua->cpua_name, pnp);

	return (UNCONF);
}

static int
cpuunit_setup_attach_args(struct cpuunit_softc *sc, bus_space_tag_t bustag,
    int node, struct cpuunit_attach_args *cpua)
{
	int n, error;

	memset(cpua, 0, sizeof(*cpua));

	error = prom_getprop(node, "name", 1, &n, &cpua->cpua_name);
	if (error)
		return (error);
	cpua->cpua_name[n] = '\0';

	error = prom_getprop(node, "device_type", 1, &n,
	    &cpua->cpua_type);
	if (error) {
		free(cpua->cpua_name, M_DEVBUF);
		return (error);
	}

	cpua->cpua_bustag = bustag;
	cpua->cpua_node = node;
	cpua->cpua_device_id = sc->sc_device_id;

	return (0);
}

static void
cpuunit_destroy_attach_args(struct cpuunit_attach_args *cpua)
{

	if (cpua->cpua_name != NULL)
		free(cpua->cpua_name, M_DEVBUF);

	if (cpua->cpua_type != NULL)
		free(cpua->cpua_type, M_DEVBUF);
}
