/* $NetBSD: gic_fdt.c,v 1.1.2.2 2015/12/27 12:09:30 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gic_fdt.c,v 1.1.2.2 2015/12/27 12:09:30 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/cortex/gic_intr.h>

#include <dev/fdt/fdtvar.h>

static int	gic_fdt_match(device_t, cfdata_t, void *);
static void	gic_fdt_attach(device_t, device_t, void *);

static void *	gic_fdt_establish(device_t, int, u_int, int, int,
		    int (*)(void *), void *);
static void	gic_fdt_disestablish(device_t, void *);
static bool	gic_fdt_intrstr(device_t, int, u_int, char *, size_t);

struct fdtbus_interrupt_controller_func gic_fdt_funcs = {
	.establish = gic_fdt_establish,
	.disestablish = gic_fdt_disestablish,
	.intrstr = gic_fdt_intrstr
};

struct gic_fdt_softc {
	device_t		sc_dev;
	int			sc_phandle;
};

CFATTACH_DECL_NEW(gic_fdt, sizeof(struct gic_fdt_softc),
	gic_fdt_match, gic_fdt_attach, NULL, NULL);

static int
gic_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"arm,gic-400",
		"arm,cortex-a15-gic",
		"arm,cortex-a9-gic",
		"arm,cortex-a7-gic",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_compatible(faa->faa_phandle, compatible) >= 0;
}

static void
gic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct gic_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;

	error = fdtbus_register_interrupt_controller(self, faa->faa_phandle,
	    &gic_fdt_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GIC\n");
}

static void *
gic_fdt_establish(device_t dev, int phandle, u_int index, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	struct gic_fdt_softc * const sc = device_private(dev);
	int iflags = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;
	u_int *interrupts;
	int interrupt_cells, len;

	len = OF_getprop(sc->sc_phandle, "#interrupt-cells", &interrupt_cells,
	    sizeof(interrupt_cells));
	if (len != sizeof(interrupt_cells) || interrupt_cells <= 0)
		return NULL;
	interrupt_cells = be32toh(interrupt_cells);

	len = OF_getproplen(phandle, "interrupts");
	if (len <= 0)
		return NULL;

	const u_int clen = interrupt_cells * 4;
	const u_int nintr = len / interrupt_cells;

	if (index >= nintr)
		return NULL;

	interrupts = kmem_alloc(len, KM_SLEEP);

	if (OF_getprop(phandle, "interrupts", interrupts, len) != len) {
		kmem_free(interrupts, len);
		return NULL;
	}

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(interrupts[index * clen + 0]);
	const u_int intr = be32toh(interrupts[index * clen + 1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);
	const u_int trig = be32toh(interrupts[index * clen + 2]) & 0xf;
	const u_int level = (trig & 0x3) ? IST_EDGE : IST_LEVEL;

	kmem_free(interrupts, len);

	return intr_establish(irq, ipl, level | iflags, func, arg);
}

static void
gic_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
gic_fdt_intrstr(device_t dev, int phandle, u_int index, char *buf,
    size_t buflen)
{
	struct gic_fdt_softc * const sc = device_private(dev);
	u_int *interrupts;
	int interrupt_cells, len;

	len = OF_getprop(sc->sc_phandle, "#interrupt-cells", &interrupt_cells,
	    sizeof(interrupt_cells));
	if (len != sizeof(interrupt_cells) || interrupt_cells <= 0) {
		return false;
	}
	interrupt_cells = be32toh(interrupt_cells);

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

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(interrupts[index * clen + 0]);
	const u_int intr = be32toh(interrupts[index * clen + 1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	kmem_free(interrupts, len);

	snprintf(buf, buflen, "GIC irq %d", irq);

	return true;
}
