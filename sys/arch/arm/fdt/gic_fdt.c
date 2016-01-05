/* $NetBSD: gic_fdt.c,v 1.2 2016/01/05 21:53:48 marty Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gic_fdt.c,v 1.2 2016/01/05 21:53:48 marty Exp $");

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

static void *	gic_fdt_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *);
static void	gic_fdt_disestablish(device_t, void *);
static bool	gic_fdt_intrstr(device_t, u_int *, char *, size_t);

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
gic_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	int iflags = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);
	const u_int trig = be32toh(specifier[2]) & 0xf;
	const u_int level = (trig & 0x3) ? IST_EDGE : IST_LEVEL;

	return intr_establish(irq, ipl, level | iflags, func, arg);
}

static void
gic_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
gic_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	if (!specifier)
		return false;
	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	snprintf(buf, buflen, "GIC irq %d", irq);

	return true;
}
