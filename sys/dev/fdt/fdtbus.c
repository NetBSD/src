/* $NetBSD: fdtbus.c,v 1.2.6.1 2017/04/21 16:53:45 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdtbus.c,v 1.2.6.1 2017/04/21 16:53:45 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <dev/fdt/fdtvar.h>

#define	FDT_MAX_PATH	256

struct fdt_node {
	device_t	n_bus;
	device_t	n_dev;
	int		n_phandle;
	char		*n_name;

	u_int		n_order;

	TAILQ_ENTRY(fdt_node) n_nodes;
};

static TAILQ_HEAD(, fdt_node) fdt_nodes =
    TAILQ_HEAD_INITIALIZER(fdt_nodes);

struct fdt_softc {
	device_t	sc_dev;
	int		sc_phandle;
	struct fdt_attach_args sc_faa;
};

static int	fdt_match(device_t, cfdata_t, void *);
static void	fdt_attach(device_t, device_t, void *);
static void	fdt_scan_bus(struct fdt_softc *);
static void	fdt_scan(struct fdt_softc *);
static void	fdt_add_node(struct fdt_node *);
static u_int	fdt_get_order(int);

static int	fdt_print(void *, const char *);

static const char * const fdtbus_compatible[] =
    { "simple-bus", NULL };

CFATTACH_DECL_NEW(fdt, sizeof(struct fdt_softc),
    fdt_match, fdt_attach, NULL, NULL);

static int
fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	int match;

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
	int len, child;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_faa = *faa;

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
		node->n_bus = self;
		node->n_dev = NULL;
		node->n_phandle = child;
		node->n_name = name;
		node->n_order = fdt_get_order(node->n_phandle);
		fdt_add_node(node);
	}

	/* Scan and attach all known busses in the tree. */
	fdt_scan_bus(sc);

	/* Only the root bus should scan for devices */
	if (OF_finddevice("/") != faa->faa_phandle)
		return;

	/* Scan devices */
	fdt_scan(sc);
}

static void
fdt_init_attach_args(struct fdt_softc *sc, struct fdt_node *node,
    struct fdt_attach_args *faa)
{
	*faa = sc->sc_faa;
	faa->faa_phandle = node->n_phandle;
	faa->faa_name = node->n_name;
}

static void
fdt_scan_bus(struct fdt_softc *sc)
{
	struct fdt_node *node;
	struct fdt_attach_args faa;
	cfdata_t cf;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes) {
		if (node->n_bus != sc->sc_dev)
			continue;
		if (node->n_dev != NULL)
			continue;

		fdt_init_attach_args(sc, node, &faa);

		/*
		 * Only attach busses to nodes where this driver is the best
		 * match.
		 */
		cf = config_search_loc(NULL, node->n_bus, NULL, NULL, &faa);
		if (cf == NULL || strcmp(cf->cf_name, "fdt") != 0)
			continue;

		/*
		 * Attach the bus.
		 */
		node->n_dev = config_found(node->n_bus, &faa, fdt_print);
	}
}

static void
fdt_scan(struct fdt_softc *sc)
{
	struct fdt_node *node;
	struct fdt_attach_args faa;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes) {
		if (node->n_dev != NULL)
			continue;

		fdt_init_attach_args(sc, node, &faa);

		/*
		 * Attach the device.
		 */
		node->n_dev = config_found(node->n_bus, &faa, fdt_print);
	}
}

static void
fdt_add_node(struct fdt_node *new_node)
{
	struct fdt_node *node;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes)
		if (node->n_order > new_node->n_order) {
			TAILQ_INSERT_BEFORE(node, new_node, n_nodes);
			return;
		}
	TAILQ_INSERT_TAIL(&fdt_nodes, new_node, n_nodes);
}

static u_int
fdt_get_order(int phandle)
{
	u_int val = UINT_MAX;
	int child;

	of_getprop_uint32(phandle, "phandle", &val);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		u_int child_val = fdt_get_order(child);
		if (child_val < val)
			val = child_val;
	}

	return val;
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
