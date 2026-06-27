/*	$NetBSD: obio.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

/* Expose the powerpc _bus_dma* back-end functions for our DMA tag. */
#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_print(void *, const char *);

CFATTACH_DECL_NEW(mpcobio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

/*
 * DMA tag shared by all on-chip devices. 
 */
static struct powerpc_bus_dma_tag obio_bus_dma_tag = {
	._dmamap_create = _bus_dmamap_create,
	._dmamap_destroy = _bus_dmamap_destroy,
	._dmamap_load = _bus_dmamap_load,
	._dmamap_load_mbuf = _bus_dmamap_load_mbuf,
	._dmamap_load_uio = _bus_dmamap_load_uio,
	._dmamap_load_raw = _bus_dmamap_load_raw,
	._dmamap_unload = _bus_dmamap_unload,
	._dmamap_sync = _bus_dmamap_sync,
	._dmamem_alloc = _bus_dmamem_alloc,
	._dmamem_free = _bus_dmamem_free,
	._dmamem_map = _bus_dmamem_map,
	._dmamem_unmap = _bus_dmamem_unmap,
	._dmamem_mmap = _bus_dmamem_mmap,
};

/*
 * Essential on-chip blocks that have no node of their own in some firmware
 * builds.
 */
static const struct obio_builtin {
	const char	*ob_name;
	bus_size_t	ob_offset;
	bus_size_t	ob_size;
} obio_builtins[] = {
	{ "cdm", MPC5200_REG_CDM, 0x38 },
};

static int
obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "builtin") == 0)
		return 1;

	return 0;
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	struct confargs *ca = aux;
	struct obio_attach_args oba;
	uint32_t reg[2];
	bus_addr_t mbar;
	bus_size_t mbarsz;
	int node, len;
	char name[32];
	bool found[__arraycount(obio_builtins)];
	size_t i;

	memset(found, 0, sizeof(found));

	sc->sc_dev = self;
	sc->sc_node = ca->ca_node;

	/*
	 * The "/builtin" node's "reg" describes the MBAR peripheral window.
	 * Fall back to the architectural default if the firmware omits it.
	 */
	mbar = MPC5200_MBAR_DEFAULT;
	mbarsz = MPC5200_MBAR_SIZE;
	len = OF_getprop(ca->ca_node, "reg", reg, sizeof(reg));
	if (len >= (int)sizeof(reg)) {
		mbar = reg[0];
		mbarsz = reg[1];
	}

	aprint_normal(": MPC5200B on-chip peripherals at 0x%08jx\n",
	    (uintmax_t)mbar);

	/* Build the shared big-endian register-space tag over the window. */
	sc->sc_bst.pbs_flags = _BUS_SPACE_BIG_ENDIAN | _BUS_SPACE_MEM_TYPE;
	sc->sc_bst.pbs_offset = 0;
	sc->sc_bst.pbs_base = mbar;
	sc->sc_bst.pbs_limit = mbar + mbarsz;
	sc->sc_bst.pbs_extent = NULL;
	if (bus_space_init(&sc->sc_bst, "obio", NULL, 0) != 0) {
		aprint_error_dev(self, "can't init bus space tag\n");
		return;
	}

	/* Enumerate and attach the on-chip devices from the OF tree. */
	for (node = OF_child(ca->ca_node); node != 0; node = OF_peer(node)) {
		memset(name, 0, sizeof(name));
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0)
			continue;

		oba.obio_name = name;
		oba.obio_node = node;
		oba.obio_bst = &sc->sc_bst;
		oba.obio_dmat = &obio_bus_dma_tag;
		oba.obio_addr = 0;
		oba.obio_size = 0;
		len = OF_getprop(node, "reg", reg, sizeof(reg));
		if (len >= (int)sizeof(reg)) {
			oba.obio_addr = reg[0];
			oba.obio_size = reg[1];
		}

		for (i = 0; i < __arraycount(obio_builtins); i++)
			if (strcmp(name, obio_builtins[i].ob_name) == 0)
				found[i] = true;

		config_found(self, &oba, obio_print, CFARGS_NONE);
	}

	/* Attach any essential built-in block the OF tree did not list. */
	for (i = 0; i < __arraycount(obio_builtins); i++) {
		if (found[i])
			continue;
		oba.obio_name = obio_builtins[i].ob_name;
		oba.obio_node = sc->sc_node;	/* no node of its own */
		oba.obio_bst = &sc->sc_bst;
		oba.obio_dmat = &obio_bus_dma_tag;
		oba.obio_addr = mbar + obio_builtins[i].ob_offset;
		oba.obio_size = obio_builtins[i].ob_size;
		config_found(self, &oba, obio_print, CFARGS_NONE);
	}
}

static int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *oba = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s", oba->obio_name, pnp);
	if (oba->obio_size != 0)
		aprint_normal(" addr 0x%08jx", (uintmax_t)oba->obio_addr);

	return UNCONF;
}
