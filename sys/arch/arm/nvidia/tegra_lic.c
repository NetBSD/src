/* $NetBSD: tegra_lic.c,v 1.5.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_lic.c,v 1.5.8.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <arm/cortex/gic_intr.h>

#include <dev/fdt/fdtvar.h>

#define	LIC_CPU_IER_CLR_REG	0x28
#define	LIC_CPU_IEP_CLASS_REG	0x2c

static int	tegra_lic_match(device_t, cfdata_t, void *);
static void	tegra_lic_attach(device_t, device_t, void *);

static void *	tegra_lic_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *);
static void	tegra_lic_disestablish(device_t, void *);
static bool	tegra_lic_intrstr(device_t, u_int *, char *, size_t);

struct fdtbus_interrupt_controller_func tegra_lic_funcs = {
	.establish = tegra_lic_establish,
	.disestablish = tegra_lic_disestablish,
	.intrstr = tegra_lic_intrstr
};

struct tegra_lic_softc {
	device_t		sc_dev;
	int			sc_phandle;
};

CFATTACH_DECL_NEW(tegra_lic, sizeof(struct tegra_lic_softc),
	tegra_lic_match, tegra_lic_attach, NULL, NULL);

static int
tegra_lic_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-ictlr",
		"nvidia,tegra124-ictlr",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_lic_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_lic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	int error, index;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;

	error = fdtbus_register_interrupt_controller(self, faa->faa_phandle,
	    &tegra_lic_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": LIC\n");

	bst = faa->faa_bst;
	for (index = 0; ; index++) {
		error = fdtbus_get_reg(faa->faa_phandle, index, &addr, &size);
		if (error != 0)
			break;
		error = bus_space_map(bst, addr, size, 0, &bsh);
		if (error) {
			aprint_error_dev(self, "can't map IC#%d: %d\n",
			    index, error);
			continue;
		}

		/* Clear interrupt enable for CPU */
		bus_space_write_4(bst, bsh, LIC_CPU_IER_CLR_REG, 0xffffffff);

		/* Route to IRQ */
		bus_space_write_4(bst, bsh, LIC_CPU_IEP_CLASS_REG, 0);

		bus_space_unmap(bst, bsh, size);
	}
}

static void *
tegra_lic_establish(device_t dev, u_int *specifier, int ipl, int flags,
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
tegra_lic_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
tegra_lic_intrstr(device_t dev, u_int *specifier, char *buf,
    size_t buflen)
{
	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	snprintf(buf, buflen, "irq %d", irq);

	return true;
}
