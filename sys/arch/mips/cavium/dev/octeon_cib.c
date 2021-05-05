/* $NetBSD: octeon_cib.c,v 1.7 2021/05/05 06:47:29 simonb Exp $ */

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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_cib.c,v 1.7 2021/05/05 06:47:29 simonb Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arch/mips/cavium/octeonvar.h>

static int	octeon_cib_match(device_t, cfdata_t, void *);
static void	octeon_cib_attach(device_t, device_t, void *);

static void *	octeon_cib_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *, const char *);
static void	octeon_cib_disestablish(device_t, void *);
static bool	octeon_cib_intrstr(device_t, u_int *, char *, size_t);

static int	octeon_cib_intr(void *);

static struct fdtbus_interrupt_controller_func octeon_cib_funcs = {
	.establish = octeon_cib_establish,
	.disestablish = octeon_cib_disestablish,
	.intrstr = octeon_cib_intrstr
};

struct octeon_cib_intr {
	bool			ih_mpsafe;
	int			ih_type;
	int			(*ih_func)(void *);
	void			*ih_arg;
	uint64_t		ih_mask;
};

struct octeon_cib_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh_raw;
	bus_space_handle_t	sc_bsh_en;

	struct octeon_cib_intr	*sc_intr;
	u_int			sc_nintr;
};

#define	CIB_READ_RAW(sc)		\
	bus_space_read_8((sc)->sc_bst, (sc)->sc_bsh_raw, 0)
#define	CIB_WRITE_RAW(sc, val)		\
	bus_space_write_8((sc)->sc_bst, (sc)->sc_bsh_raw, 0, (val))
#define	CIB_READ_EN(sc)			\
	bus_space_read_8((sc)->sc_bst, (sc)->sc_bsh_en, 0)
#define	CIB_WRITE_EN(sc, val)		\
	bus_space_write_8((sc)->sc_bst, (sc)->sc_bsh_en, 0, (val))

CFATTACH_DECL_NEW(octcib, sizeof(struct octeon_cib_softc),
	octeon_cib_match, octeon_cib_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cavium,octeon-7130-cib" },
	DEVICE_COMPAT_EOL
};

static int
octeon_cib_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
octeon_cib_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_cib_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int max_bits;
	int error;
	void *ih;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0 ||
	    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_raw) != 0) {
		aprint_error(": couldn't map RAW register\n");
		return;
	}
	if (fdtbus_get_reg(phandle, 1, &addr, &size) != 0 ||
	    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_en) != 0) {
		aprint_error(": couldn't map EN register\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if (of_getprop_uint32(phandle, "cavium,max-bits", &max_bits) != 0) {
		aprint_error(": missing 'cavium,max-bits' property\n");
		return;
	}
	if (max_bits == 0 || max_bits > 64) {
		aprint_error(": 'cavium,max-bits' value out of range\n");
		return;
	}

	sc->sc_intr = kmem_zalloc(sizeof(*sc->sc_intr) * max_bits, KM_SLEEP);
	sc->sc_nintr = max_bits;

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &octeon_cib_funcs);
	if (error != 0) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": CIB\n");

	CIB_WRITE_EN(sc, 0);
	CIB_WRITE_RAW(sc, ~0ULL);

	ih = fdtbus_intr_establish(phandle, 0, IPL_SCHED, FDT_INTR_MPSAFE,
	    octeon_cib_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

static void *
octeon_cib_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct octeon_cib_softc * const sc = device_private(dev);
	struct octeon_cib_intr *ih;
	uint64_t val;

	/* 1st cell is the bit number in the CIB* registers */
	/* 2nd cell is the triggering setting */
	const int bit = be32toh(specifier[0]);
	const int type = (be32toh(specifier[1]) & 0x3) ? IST_EDGE : IST_LEVEL;

	if (bit > sc->sc_nintr) {
		aprint_error_dev(dev, "bit %d out of range\n", bit);
		return NULL;
	}

	ih = &sc->sc_intr[bit];
	ih->ih_mpsafe = (flags & FDT_INTR_MPSAFE) != 0;
	ih->ih_type = type;
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_mask = __BIT(bit);

	val = CIB_READ_EN(sc);
	val |= ih->ih_mask;
	CIB_WRITE_EN(sc, val);

	return ih;
}

static void
octeon_cib_disestablish(device_t dev, void *ih_cookie)
{
	struct octeon_cib_softc * const sc = device_private(dev);
	struct octeon_cib_intr *ih = ih_cookie;
	uint64_t val;

	val = CIB_READ_EN(sc);
	val &= ~ih->ih_mask;
	CIB_WRITE_EN(sc, val);
}

static bool
octeon_cib_intrstr(device_t dev, u_int *specifier, char *buf,
    size_t buflen)
{
	/* 1st cell is the bit number in the CIB* registers */
	const int bit = be32toh(specifier[0]);

	snprintf(buf, buflen, "%s intr %d", device_xname(dev), bit);

	return true;
}

static int
octeon_cib_intr(void *priv)
{
	struct octeon_cib_softc * const sc = priv;
	struct octeon_cib_intr *ih;
	uint64_t pend;
	int n, rv = 0;

	pend = CIB_READ_RAW(sc);
	pend &= CIB_READ_EN(sc);

	while ((n = ffs64(pend)) != 0) {
		ih = &sc->sc_intr[n - 1];
		KASSERT(ih->ih_mask == __BIT(n - 1));

		if (ih->ih_type == IST_EDGE)
			CIB_WRITE_RAW(sc, ih->ih_mask);	/* ack */

#ifdef MULTIPROCESSOR
		if (!ih->ih_mpsafe) {
			KERNEL_LOCK(1, NULL);
			rv |= ih->ih_func(ih->ih_arg);
			KERNEL_UNLOCK_ONE(NULL);
		} else
#endif
			rv |= ih->ih_func(ih->ih_arg);

		pend &= ~ih->ih_mask;
	}

	return rv;
}
