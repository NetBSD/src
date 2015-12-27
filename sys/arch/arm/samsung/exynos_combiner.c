/*	$NetBSD: exynos_combiner.c,v 1.3.2.2 2015/12/27 12:09:32 skrll Exp $ */

/*-
* Copyright (c) 2015 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Marty Fouts
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

#include "opt_exynos.h"
#include "opt_arm_debug.h"
#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_combiner.c,v 1.3.2.2 2015/12/27 12:09:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <arm/cortex/gic_intr.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_intr.h>

#include <dev/fdt/fdtvar.h>

#define COMBINER_IESR_OFFSET  0x00
#define COMBINER_IECR_OFFSET  0x04
#define COMBINER_ISTR_OFFSET  0x08
#define COMBINER_IMSR_OFFSET  0x0C
#define COMBINER_BLOCK_SIZE   0x10

struct exynos_combiner_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

};

static int exynos_combiner_match(device_t, cfdata_t, void *);
static void exynos_combiner_attach(device_t, device_t, void *);

static void *	exynos_combiner_establish(device_t, int, u_int, int, int,
		    int (*)(void *), void *);
static void	exynos_combiner_disestablish(device_t, void *);
static bool	exynos_combiner_intrstr(device_t, int, u_int, char *, size_t);

struct fdtbus_interrupt_controller_func exynos_combiner_funcs = {
	.establish = exynos_combiner_establish,
	.disestablish = exynos_combiner_disestablish,
	.intrstr = exynos_combiner_intrstr
};

CFATTACH_DECL_NEW(exynos_intr, sizeof(struct exynos_combiner_softc),
	exynos_combiner_match, exynos_combiner_attach, NULL, NULL);

static int
exynos_combiner_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos4210-combiner",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_combiner_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_combiner_softc * const sc
		= kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	error = fdtbus_register_interrupt_controller(self, faa->faa_phandle,
	    &exynos_combiner_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_normal(" @ 0x%08x: interrupt combiner", (uint)addr);
	aprint_naive("\n");
	aprint_normal("\n");

}

static void *
exynos_combiner_establish(device_t dev, int phandle, u_int index, int ipl,
			  int flags,
			  int (*func)(void *), void *arg)
{
	struct exynos_combiner_softc * const sc = device_private(dev);
	int iflags = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;
	int iblock = index >> 3;
	int ioffset = index & 0x07;
	int block_offset =
		iblock * COMBINER_BLOCK_SIZE + COMBINER_IESR_OFFSET;
	int istatus =
		bus_space_read_4(sc->sc_bst, sc->sc_bsh, block_offset);
	printf("Establishing irq %d (0x%x) @ iblock = %d, ioffset = %d\n",
	       index, index, iblock, ioffset);
	istatus |= 1 << ioffset;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, block_offset, istatus);
	return intr_establish(index, ipl, iflags, func, arg);
}

static void
exynos_combiner_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
exynos_combiner_intrstr(device_t dev, int phandle, u_int index, char *buf,
    size_t buflen)
{
	struct exynos_combiner_softc * const sc = device_private(dev);
	u_int *interrupts;
	int interrupt_cells, len;

	if (of_getprop_uint32(sc->sc_phandle, "#interrupt-cells",
	    &interrupt_cells)) {
		return false;
	}

	len = OF_getproplen(phandle, "interrupts");
	if (len <= 0) {
		return false;
	}

	const u_int clen = interrupt_cells * 4;
	const u_int nintr = len / interrupt_cells;

	if (index >= nintr) {
		return false;
	}

	interrupts = kmem_alloc(len, KM_SLEEP);

	if (OF_getprop(phandle, "interrupts", interrupts, len) != len) {
		kmem_free(interrupts, len);
		return false;
	}

	/* 1st cell is the interrupt type; */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(interrupts[index * clen + 0]);
	const u_int intr = be32toh(interrupts[index * clen + 1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	kmem_free(interrupts, len);

	snprintf(buf, buflen, "LIC irq %d", irq);

	return true;
}
