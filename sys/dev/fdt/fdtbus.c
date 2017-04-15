/* $NetBSD: fdtbus.c,v 1.7 2017/04/15 00:34:29 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdtbus.c,v 1.7 2017/04/15 00:34:29 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <dev/fdt/fdtvar.h>

#define	FDT_MAX_PATH	256

struct fdt_node {
	device_t	n_dev;
	int		n_phandle;
	char		*n_name;

	TAILQ_ENTRY(fdt_node) n_nodes;
};

struct fdt_softc {
	device_t	sc_dev;
	int		sc_phandle;
	struct fdt_attach_args sc_faa;

	TAILQ_HEAD(, fdt_node) sc_nodes;
};

static int	fdt_match(device_t, cfdata_t, void *);
static void	fdt_attach(device_t, device_t, void *);
static void	fdt_scan(struct fdt_softc *, const char *, bool);

static const char * const fdtbus_compatible[] =
    { "simple-bus", NULL };

CFATTACH_DECL_NEW(fdt, sizeof(struct fdt_softc),
    fdt_match, fdt_attach, NULL, NULL);

static int	fdt_print(void *, const char *);

static int
fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	int match;

	if (!OF_child(faa->faa_phandle))
		return 0;

	match = of_match_compatible(faa->faa_phandle, fdtbus_compatible);
	if (match)
		return match;

	return OF_finddevice("/") == faa->faa_phandle;
}

static void
fdt_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_softc *sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdt_node *node;
	char *model, *name, *status;
	int len, n, child;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_faa = *faa;
	TAILQ_INIT(&sc->sc_nodes);

	aprint_naive("\n");
	len = OF_getproplen(phandle, "model");
	if (len > 0) {
		model = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "model", model, len) == len) {
			aprint_normal(": %s\n", model);
		} else {
			aprint_normal("\n");
		}
		kmem_free(model, len);
	} else {
		aprint_normal("\n");
	}

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		/* If there is a "status" property, make sure it is "okay" */
		len = OF_getproplen(child, "status");
		if (len > 0) {
			status = kmem_zalloc(len, KM_SLEEP);
			int alen __diagused = OF_getprop(child, "status", status, len);
			KASSERT(alen == len);
			const bool okay_p = strcmp(status, "okay") == 0 ||
					    strcmp(status, "ok") == 0;
			kmem_free(status, len);
			if (!okay_p) {
				continue;
			}
		}

		len = OF_getproplen(child, "name");
		if (len <= 0) {
			continue;
		}
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(child, "name", name, len) != len) {
			continue;
		}

		/* Add the node to our device list */
		node = kmem_alloc(sizeof(*node), KM_SLEEP);
		node->n_dev = NULL;
		node->n_phandle = child;
		node->n_name = name;
		TAILQ_INSERT_TAIL(&sc->sc_nodes, node, n_nodes);
	}

	/* Scan and attach all known busses in the tree. */
	for (n = 0; fdtbus_compatible[n] != NULL; n++)
		fdt_scan(sc, fdtbus_compatible[n], true);

	/* Only the root bus should scan for devices */
	if (OF_finddevice("/") != faa->faa_phandle)
		return;

	/* Scan the tree for "early init" devices */
	for (n = 0; n < faa->faa_ninit; n++)
		fdt_scan(sc, faa->faa_init[n], false);

	/* Finally, scan the tree for all other devices */
	fdt_scan(sc, NULL, false);
}

static void
fdt_scan(struct fdt_softc *sc, const char *compat, bool bus)
{
	struct fdt_node *node;
	struct fdt_attach_args faa;
	cfdata_t cf;

	TAILQ_FOREACH(node, &sc->sc_nodes, n_nodes) {
		if (node->n_dev != NULL) {
			/*
			 * Child is already attached. If it is a bus,
			 * recursively scan.
			 */
			if (device_is_a(node->n_dev, "fdt"))
				fdt_scan(device_private(node->n_dev), compat,
				    bus);
			continue;
		}

		faa = sc->sc_faa;
		faa.faa_phandle = node->n_phandle;
		faa.faa_name = node->n_name;

		/*
		 * Only attach busses to nodes where this driver is the best
		 * match.
		 */
		if (compat && bus) {
			cf = config_search_loc(NULL, sc->sc_dev, NULL, NULL,
			    &faa);
			if (cf == NULL || strcmp(cf->cf_name, "fdt") != 0)
				continue;
		}

		/*
		 * Attach the child device.
		 */
		const char * const compats[] = { compat, NULL };
		if (compat == NULL ||
		    of_match_compatible(node->n_phandle, compats)) {
			node->n_dev = config_found(sc->sc_dev, &faa, fdt_print);
		}
	}
}

static int
fdt_print(void *aux, const char *pnp)
{
	const struct fdt_attach_args * const faa = aux;
	char buf[FDT_MAX_PATH];
	const char *name = buf;

	/* Skip "not configured" for nodes w/o compatible property */
	if (OF_getproplen(faa->faa_phandle, "compatible") <= 0)
		return QUIET;

	if (pnp) {
		if (!fdtbus_get_path(faa->faa_phandle, buf, sizeof(buf)))
			name = faa->faa_name;
		aprint_normal("%s at %s", name, pnp);
	}

	return UNCONF;
}
