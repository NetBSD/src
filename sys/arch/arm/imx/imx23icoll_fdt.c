/* $NetBSD: imx23icoll_fdt.c,v 1.1 2025/10/09 06:15:16 skrll Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx23icoll_fdt.c,v 1.1 2025/10/09 06:15:16 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>
#include <arm/imx/imx23var.h>
#include <arm/imx/imx23_icollvar.h>

static int imx23icoll_fdt_match(device_t, cfdata_t, void *);
static void imx23icoll_fdt_attach(device_t, device_t, void *);

static void *	imx23icoll_fdt_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *, const char *);
static void	imx23icoll_fdt_disestablish(device_t, void *);
static bool	imx23icoll_fdt_intrstr(device_t, u_int *, char *, size_t);

struct imx23icoll_fdt_softc {
	struct icoll_softc sc_icoll;
};

CFATTACH_DECL_NEW(imx23icoll_fdt, sizeof(struct imx23icoll_fdt_softc),
		  imx23icoll_fdt_match, imx23icoll_fdt_attach, NULL, NULL);

struct fdtbus_interrupt_controller_func imx23icoll_fdt_funcs = {
	.establish = imx23icoll_fdt_establish,
	.disestablish = imx23icoll_fdt_disestablish,
	.intrstr = imx23icoll_fdt_intrstr
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-icoll" },
	{ .compat = "fsl,icoll" },
	DEVICE_COMPAT_EOL
};

static int
imx23icoll_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23icoll_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct imx23icoll_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	/*
	 * Initialize icoll controller
	 */
	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_icoll.sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	imx23icoll_init(&sc->sc_icoll, self, faa->faa_bst);

	/*
	 * Register fdt interrupt controller
	 */
	int error = fdtbus_register_interrupt_controller(self, phandle,
							 &imx23icoll_fdt_funcs);
	if (error) {
		aprint_error(
		    "imx23icoll_fdt: couldn't register with fdtbus: %d\n",
		    error);
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	struct apb_attach_args apbaa = {
		.aa_name = "imx23icoll",
		.aa_iot = faa->faa_bst,
		.aa_dmat = faa->faa_dmat,
		.aa_addr = addr,
		.aa_size = size,
		.aa_irq = -1
	};

	config_found(self, &apbaa, NULL, CFARGS_NONE);

	arm_fdt_irq_set_handler((void (*)(void *))imx23_intr_dispatch);
}

static bool
imx23icoll_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	const u_int irq = be32toh(*specifier);

	snprintf(buf, buflen, "icoll irq %d", irq);

	return true;
}

static void *
imx23icoll_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
			 int (*func)(void *), void *arg, const char *xname)
{
	const u_int irq = be32toh(*specifier);
	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	return intr_establish_xname(irq, ipl, IST_LEVEL | mpsafe, func, arg,
				    xname);
}

static void
imx23icoll_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}
