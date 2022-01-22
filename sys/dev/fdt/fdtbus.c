/* $NetBSD: fdtbus.c,v 1.45 2022/01/22 11:49:17 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdtbus.c,v 1.45 2022/01/22 11:49:17 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <dev/fdt/fdtvar.h>

#include <libfdt.h>

#include "locators.h"

#define	FDT_MAX_PATH	256

struct fdt_node {
	device_t	n_bus;
	device_t	n_dev;
	int		n_phandle;
	const char	*n_name;
	struct fdt_attach_args n_faa;

	int		n_cfpass;
	cfdata_t	n_cf;

	u_int		n_order;

	bool		n_pinctrl_init;

	TAILQ_ENTRY(fdt_node) n_nodes;
};

static TAILQ_HEAD(, fdt_node) fdt_nodes =
    TAILQ_HEAD_INITIALIZER(fdt_nodes);
static bool fdt_need_rescan = false;

struct fdt_softc {
	device_t	sc_dev;
	int		sc_phandle;
	struct fdt_attach_args sc_faa;
};

static int	fdt_match(device_t, cfdata_t, void *);
static void	fdt_attach(device_t, device_t, void *);
static int	fdt_rescan(device_t, const char *, const int *);
static void	fdt_childdet(device_t, device_t);

static int	fdt_scan_submatch(device_t, cfdata_t, const int *, void *);
static void	fdt_scan_best(struct fdt_softc *, struct fdt_node *);
static void	fdt_scan(struct fdt_softc *, int);
static void	fdt_add_node(struct fdt_node *);
static u_int	fdt_get_order(int);
static void	fdt_pre_attach(struct fdt_node *);
static void	fdt_post_attach(struct fdt_node *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "simple-bus" },
	{ .compat = "simple-pm-bus" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL2_NEW(simplebus, sizeof(struct fdt_softc),
    fdt_match, fdt_attach, NULL, NULL, fdt_rescan, fdt_childdet);

static int
fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	int match;

	/* Check compatible string */
	match = of_compatible_match(phandle, compat_data);
	if (match)
		return match;

	/* Some nodes have no compatible string */
	if (!of_hasprop(phandle, "compatible")) {
		if (OF_finddevice("/clocks") == phandle)
			return 1;
		if (OF_finddevice("/chosen") == phandle)
			return 1;
	}

	/* Always match the root node */
	return OF_finddevice("/") == phandle;
}

static void
fdt_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_softc *sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	const char *descr, *model;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_faa = *faa;

	aprint_naive("\n");

	descr = fdtbus_get_string(phandle, "model");
	if (descr)
		aprint_normal(": %s\n", descr);
	else
		aprint_normal("\n");

	/* Find all child nodes */
	fdt_add_bus(self, phandle, &sc->sc_faa);

	/* Only the root bus should scan for devices */
	if (OF_finddevice("/") != faa->faa_phandle)
		return;

	/* Set hw.model if available */
	model = fdtbus_get_string(phandle, "compatible");
	if (model)
		cpu_setmodel("%s", model);
	else if (descr)
		cpu_setmodel("%s", descr);

	/* Scan devices */
	fdt_rescan(self, NULL, NULL);
}

static int
fdt_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct fdt_softc *sc = device_private(self);
	struct fdt_node *node;
	int pass;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes)
		fdt_scan_best(sc, node);

	pass = 0;
	fdt_need_rescan = false;
	do {
		fdt_scan(sc, pass);
		if (fdt_need_rescan == true) {
			pass = 0;
			TAILQ_FOREACH(node, &fdt_nodes, n_nodes) {
				if (node->n_cfpass == -1)
					fdt_scan_best(sc, node);
			}
			fdt_need_rescan = false;
		} else {
			pass++;
		}
	} while (pass <= FDTCF_PASS_DEFAULT);

	return 0;
}

static void
fdt_childdet(device_t parent, device_t child)
{
	struct fdt_node *node;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes)
		if (node->n_dev == child) {
			node->n_dev = NULL;
			break;
		}
}

static void
fdt_init_attach_args(const struct fdt_attach_args *faa_tmpl, struct fdt_node *node,
    bool quiet, struct fdt_attach_args *faa)
{
	*faa = *faa_tmpl;
	faa->faa_phandle = node->n_phandle;
	faa->faa_name = node->n_name;
	faa->faa_quiet = quiet;
	faa->faa_bst = node->n_faa.faa_bst;
	faa->faa_dmat = fdtbus_iommu_map(node->n_phandle, 0,
	   node->n_faa.faa_dmat);
}

static bool
fdt_add_bus_stdmatch(void *arg, int child)
{
	return fdtbus_status_okay(child);
}

void
fdt_add_bus(device_t bus, const int phandle, struct fdt_attach_args *faa)
{
	fdt_add_bus_match(bus, phandle, faa, fdt_add_bus_stdmatch, NULL);
}

void
fdt_add_bus_match(device_t bus, const int phandle, struct fdt_attach_args *faa,
    bool (*fn)(void *, int), void *fnarg)
{
	int child;

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (fn && !fn(fnarg, child))
			continue;

		fdt_add_child(bus, child, faa, fdt_get_order(child));
	}
}

static int
fdt_dma_translate(int phandle, struct fdt_dma_range **ranges, u_int *nranges)
{
	const uint8_t *data;
	int len, n;

	const int parent = OF_parent(phandle);
	if (parent == -1)
		return 1;	/* done searching */

	data = fdtbus_get_prop(phandle, "dma-ranges", &len);
	if (data == NULL)
		return 1;	/* no dma-ranges property, stop searching */

	if (len == 0)
		return 0;	/* dma-ranges property is empty, keep going */

	const int addr_cells = fdtbus_get_addr_cells(phandle);
	const int size_cells = fdtbus_get_size_cells(phandle);
	const int paddr_cells = fdtbus_get_addr_cells(parent);
	if (addr_cells == -1 || size_cells == -1 || paddr_cells == -1)
		return 1;

	const int entry_size = (addr_cells + paddr_cells + size_cells) * 4;

	*nranges = len / entry_size;
	*ranges = kmem_alloc(sizeof(struct fdt_dma_range) * *nranges, KM_SLEEP);
	for (n = 0; len >= entry_size; n++, len -= entry_size) {
		const uint64_t cba = fdtbus_get_cells(data, addr_cells);
		data += addr_cells * 4;
		const uint64_t pba = fdtbus_get_cells(data, paddr_cells);
		data += paddr_cells * 4;
		const uint64_t cl = fdtbus_get_cells(data, size_cells);
		data += size_cells * 4;

		(*ranges)[n].dr_sysbase = pba;
		(*ranges)[n].dr_busbase = cba;
		(*ranges)[n].dr_len = cl;
	}

	return 1;
}

static bus_dma_tag_t
fdt_get_dma_tag(struct fdt_node *node)
{
	struct fdt_dma_range *ranges = NULL;
	u_int nranges = 0;
	int parent;

	parent = OF_parent(node->n_phandle);
	while (parent != -1) {
		if (fdt_dma_translate(parent, &ranges, &nranges) != 0)
			break;
		parent = OF_parent(parent);
	}

	return fdtbus_dma_tag_create(node->n_phandle, ranges, nranges);
}

static uint32_t
fdt_bus_flags(int phandle, uint32_t *flags)
{
	if (of_hasprop(phandle, "nonposted-mmio")) {
		*flags |= FDT_BUS_SPACE_FLAG_NONPOSTED_MMIO;
		return 1;
	}

	return 0;
}

static bus_space_tag_t
fdt_get_bus_tag(struct fdt_node *node)
{
	uint32_t flags = 0;
	int parent;

	parent = OF_parent(node->n_phandle);
	while (parent != -1) {
		if (fdt_bus_flags(parent, &flags) != 0) {
			break;
		}
		parent = OF_parent(parent);
	}

	return fdtbus_bus_tag_create(node->n_phandle, flags);
}

void
fdt_add_child(device_t bus, const int child, struct fdt_attach_args *faa,
    u_int order)
{
	struct fdt_node *node;

	/* Add the node to our device list */
	node = kmem_zalloc(sizeof(*node), KM_SLEEP);
	node->n_bus = bus;
	node->n_dev = NULL;
	node->n_phandle = child;
	node->n_name = fdtbus_get_string(child, "name");
	node->n_cfpass = -1;
	node->n_cf = NULL;
	node->n_order = order;
	node->n_faa = *faa;
	node->n_faa.faa_phandle = child;
	node->n_faa.faa_name = node->n_name;
	node->n_faa.faa_bst = fdt_get_bus_tag(node);
	node->n_faa.faa_dmat = fdt_get_dma_tag(node);

	fdt_add_node(node);
	fdt_need_rescan = true;
}

static int
fdt_scan_submatch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	if (locs[FDTCF_PASS] != FDTCF_PASS_DEFAULT &&
	    locs[FDTCF_PASS] != cf->cf_loc[FDTCF_PASS])
		return 0;

	return config_stdsubmatch(parent, cf, locs, aux);
}

static void
fdt_scan_best(struct fdt_softc *sc, struct fdt_node *node)
{
	struct fdt_attach_args faa;
	cfdata_t cf, best_cf;
	int match, best_match, best_pass;

	best_cf = NULL;
	best_match = 0;
	best_pass = FDTCF_PASS_DEFAULT;

	for (int pass = 0; pass <= FDTCF_PASS_DEFAULT; pass++) {
		const int locs[FDTCF_NLOCS] = {
			[FDTCF_PASS] = pass
		};
		fdt_init_attach_args(&sc->sc_faa, node, true, &faa);
		cf = config_search(node->n_bus, &faa,
		    CFARGS(.submatch = fdt_scan_submatch,
			   .iattr = "fdt",
			   .locators = locs));
		if (cf == NULL)
			continue;
		match = config_match(node->n_bus, cf, &faa);
		if (match > best_match) {
			best_match = match;
			best_cf = cf;
			best_pass = pass;
		}
	}

	node->n_cf = best_cf;
	node->n_cfpass = best_pass;
}

static void
fdt_scan(struct fdt_softc *sc, int pass)
{
	struct fdt_node *node;
	struct fdt_attach_args faa;
	const int locs[FDTCF_NLOCS] = {
		[FDTCF_PASS] = pass
	};
	bool quiet = pass != FDTCF_PASS_DEFAULT;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes) {
		if (node->n_cfpass != pass || node->n_dev != NULL)
			continue;

		fdt_init_attach_args(&sc->sc_faa, node, quiet, &faa);

		if (quiet && node->n_cf == NULL) {
			/*
			 * No match for this device, skip it.
			 */
			continue;
		}

		/*
		 * Attach the device.
		 */
		fdt_pre_attach(node);

		devhandle_t nodeh = device_handle(node->n_bus);

		if (quiet) {
			node->n_dev = config_attach(node->n_bus, node->n_cf,
			    &faa, fdtbus_print,
			    CFARGS(.locators = locs,
				   .devhandle =
				       devhandle_from_of(nodeh,
							 node->n_phandle)));
		} else {
			/*
			 * Default pass.
			 */
			node->n_dev = config_found(node->n_bus, &faa,
			    fdtbus_print,
			    CFARGS(.submatch = fdt_scan_submatch,
				   .iattr = "fdt",
				   .locators = locs,
				   .devhandle =
				       devhandle_from_of(nodeh,
							 node->n_phandle)));
		}

		if (node->n_dev != NULL)
			fdt_post_attach(node);
	}
}

static void
fdt_pre_attach(struct fdt_node *node)
{
	const char *cfgname;
	int error;

	node->n_pinctrl_init = fdtbus_pinctrl_has_config(node->n_phandle, "init");

	cfgname = node->n_pinctrl_init ? "init" : "default";

	aprint_debug_dev(node->n_bus, "set %s config for %s\n", cfgname, node->n_name);

	error = fdtbus_pinctrl_set_config(node->n_phandle, cfgname);
	if (error != 0 && error != ENOENT)
		aprint_debug_dev(node->n_bus,
		    "failed to set %s config on %s: %d\n",
		    cfgname, node->n_name, error);
}

static void
fdt_post_attach(struct fdt_node *node)
{
	char buf[FDT_MAX_PATH];
	prop_dictionary_t dict;
	int error;

	dict = device_properties(node->n_dev);
	if (fdtbus_get_path(node->n_phandle, buf, sizeof(buf)))
		prop_dictionary_set_string(dict, "fdt-path", buf);

	if (node->n_pinctrl_init) {
		aprint_debug_dev(node->n_bus, "set default config for %s\n", node->n_name);
		error = fdtbus_pinctrl_set_config(node->n_phandle, "default");
		if (error != 0 && error != ENOENT)
			aprint_debug_dev(node->n_bus,
			    "failed to set default config on %s: %d\n",
			    node->n_name, error);
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

void
fdt_remove_byhandle(int phandle)
{
	struct fdt_node *node;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes) {
		if (node->n_phandle == phandle) {
			TAILQ_REMOVE(&fdt_nodes, node, n_nodes);
			return;
		}
	}
}

void
fdt_remove_bycompat(const char *compatible[])
{
	struct fdt_node *node, *next;

	TAILQ_FOREACH_SAFE(node, &fdt_nodes, n_nodes, next) {
		if (of_compatible(node->n_phandle, compatible)) {
			TAILQ_REMOVE(&fdt_nodes, node, n_nodes);
		}
	}
}

int
fdt_find_with_property(const char *prop, int *pindex)
{
	struct fdt_node *node;
	int index = 0;

	TAILQ_FOREACH(node, &fdt_nodes, n_nodes) {
		if (index++ < *pindex)
			continue;
		if (of_hasprop(node->n_phandle, prop)) {
			*pindex = index;
			return node->n_phandle;
		}
	}

	return -1;
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

int
fdtbus_print(void *aux, const char *pnp)
{
	const struct fdt_attach_args * const faa = aux;
	char buf[FDT_MAX_PATH];
	const char *name = buf;
	int len;

	if (pnp && faa->faa_quiet)
		return QUIET;

	/* Skip "not configured" for nodes w/o compatible property */
	if (pnp && OF_getproplen(faa->faa_phandle, "compatible") <= 0)
		return QUIET;

	if (!fdtbus_get_path(faa->faa_phandle, buf, sizeof(buf)))
		name = faa->faa_name;

	if (pnp) {
		aprint_normal("%s at %s", name, pnp);
		const char *compat = fdt_getprop(fdtbus_get_data(),
		    fdtbus_phandle2offset(faa->faa_phandle), "compatible",
		    &len);
		while (len > 0) {
			aprint_debug(" <%s>", compat);
			len -= (strlen(compat) + 1);
			compat += (strlen(compat) + 1);
		}
	} else
		aprint_debug(" (%s)", name);

	return UNCONF;
}
