/* $NetBSD: octeon_intc.c,v 1.7 2021/05/05 06:47:29 simonb Exp $ */

/*-
 * Copyright (c) 2020 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: octeon_intc.c,v 1.7 2021/05/05 06:47:29 simonb Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arch/mips/cavium/octeonvar.h>

static int	octeon_intc_match(device_t, cfdata_t, void *);
static void	octeon_intc_attach(device_t, device_t, void *);

static void *	octeon_intc_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *, const char *);
static void	octeon_intc_disestablish(device_t, void *);
static bool	octeon_intc_intrstr(device_t, u_int *, char *, size_t);

static struct fdtbus_interrupt_controller_func octeon_intc_funcs = {
	.establish = octeon_intc_establish,
	.disestablish = octeon_intc_disestablish,
	.intrstr = octeon_intc_intrstr
};

enum octeon_intc_type {
	OCTEON_INTC_CIU,
};

struct octeon_intc_softc {
	device_t		sc_dev;
	int			sc_phandle;
	enum octeon_intc_type	sc_type;
	const char		*sc_descr;
};

CFATTACH_DECL_NEW(octintc, sizeof(struct octeon_intc_softc),
	octeon_intc_match, octeon_intc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cavium,octeon-3860-ciu",	.value = OCTEON_INTC_CIU },
	DEVICE_COMPAT_EOL
};

static int
octeon_intc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
octeon_intc_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_intc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_type = of_compatible_lookup(phandle, compat_data)->value;

	switch (sc->sc_type) {
	case OCTEON_INTC_CIU:
		sc->sc_descr = "CIU";
		break;
	}

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &octeon_intc_funcs);
	if (error != 0) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_descr);
}

static void *
octeon_intc_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct octeon_intc_softc * const sc = device_private(dev);

	/* 1st cell is the controller register (0 or 1) */
	/* 2nd cell is the bit within the register (0..63) */

	const u_int reg = be32toh(specifier[0]);
	const u_int bit = be32toh(specifier[1]);
	const u_int irq = (reg * 64) + bit;

	if (irq >= NIRQS) {
		aprint_error_dev(dev, "%s irq %d (%d, %d) out of range\n",
		    sc->sc_descr, irq, reg, bit);
		return NULL;
	}

	return octeon_intr_establish(irq, ipl, func, arg);
}

static void
octeon_intc_disestablish(device_t dev, void *ih)
{
	octeon_intr_disestablish(ih);
}

static bool
octeon_intc_intrstr(device_t dev, u_int *specifier, char *buf,
    size_t buflen)
{
	struct octeon_intc_softc * const sc = device_private(dev);

	/* 1st cell is the controller register (0 or 1) */
	/* 2nd cell is the bit within the register (0..63) */

	const u_int reg = be32toh(specifier[0]);
	const u_int bit = be32toh(specifier[1]);
	const u_int irq = (reg * 64) + bit;

	snprintf(buf, buflen, "%s irq %d", sc->sc_descr, irq);

	return true;
}
