/*	$NetBSD: sram.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

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

/*
 * Driver and allocator for the MPC5200B on-chip SRAM (16KB at MBAR+0x8000).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sram.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/vmem.h>
#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/sramvar.h>

static int	sram_match(device_t, cfdata_t, void *);
static void	sram_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sram, sizeof(struct sram_softc),
    sram_match, sram_attach, NULL, NULL);

/* Single instance; the allocator API is global for BestComm's use. */
static struct sram_softc *sram_sc;

static int
sram_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	char compat[32];
	int len;

	if (strcmp(oba->obio_name, "sram") == 0)
		return 1;

	len = OF_getprop(oba->obio_node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-sram") == 0 ||
	     strcmp(compat, "mpc5200b-sram") == 0))
		return 1;

	return 0;
}

static void
sram_attach(device_t parent, device_t self, void *aux)
{
	struct sram_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_iot = oba->obio_bst;

	size = oba->obio_size != 0 ? oba->obio_size : MPC5200_SRAM_SIZE;
	if (bus_space_map(sc->sc_iot, oba->obio_addr, size,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	sc->sc_pa = oba->obio_addr;
	sc->sc_kva = (vaddr_t)bus_space_vaddr(sc->sc_iot, sc->sc_ioh);
	sc->sc_size = size;

	/*
	 * Arena addresses are the physical SRAM addresses, so an allocation
	 * is directly the value a DMA descriptor or the TaskBar wants.
	 */
	sc->sc_arena = vmem_create(device_xname(self), sc->sc_pa, size,
	    sizeof(uint32_t), NULL, NULL, NULL, 0, VM_SLEEP, IPL_VM);
	if (sc->sc_arena == NULL) {
		aprint_error(": can't create arena\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		return;
	}

	sram_sc = sc;

	aprint_normal(": %ju KB on-chip SRAM\n", (uintmax_t)(size / 1024));
}

bool
sram_available(void)
{
	return sram_sc != NULL;
}

/*
 * Allocate "size" bytes of SRAM with the given power-of-two alignment (0 or 1
 * means word alignment).
 */
bus_addr_t
sram_alloc(size_t size, size_t align)
{
	vmem_addr_t addr;

	if (sram_sc == NULL)
		return 0;

	if (vmem_xalloc(sram_sc->sc_arena, size, align, 0, 0,
	    VMEM_ADDR_MIN, VMEM_ADDR_MAX, VM_BESTFIT | VM_NOSLEEP, &addr) != 0)
		return 0;

	return (bus_addr_t)addr;
}

void
sram_free(bus_addr_t addr, size_t size)
{
	if (sram_sc == NULL || addr == 0)
		return;

	vmem_xfree(sram_sc->sc_arena, (vmem_addr_t)addr, size);
}

/*
 * Translate a physical SRAM address to a CPU pointer in mapped window.
 */
void *
sram_kva(bus_addr_t addr)
{
	if (sram_sc == NULL ||
	    addr < sram_sc->sc_pa || addr >= sram_sc->sc_pa + sram_sc->sc_size)
		return NULL;

	return (void *)(sram_sc->sc_kva + (addr - sram_sc->sc_pa));
}
