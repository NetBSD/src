/*	$NetBSD: octeon_iobus.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2007
 *      Internet Initiative Japan, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_iobus.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _MIPS_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <mips/cavium/include/iobusvar.h>

struct iobus_softc {
	device_t		sc_dev;

	/* XXX load/IOBDMA/store operations */
	bus_space_handle_t	sc_ops_bush;
};

static int		iobus_match(device_t, struct cfdata *, void *);
static void		iobus_attach(device_t, device_t, void *);
static int		iobus_submatch(device_t, struct cfdata *,
			    const int *, void *);
static int		iobus_print(void *, const char *);
static void		iobus_init(struct iobus_softc *);
static void		iobus_init_map(struct iobus_softc *);
static void		iobus_init_local(struct iobus_softc *);
static void		iobus_init_local_pow(struct iobus_softc *);
static void		iobus_init_local_fpa(struct iobus_softc *);

static void		iobus_bus_io_init(bus_space_tag_t, void *);

static struct mips_bus_space	*iobus_bust;
static struct mips_bus_dma_tag	*iobus_dmat;

void
iobus_bootstrap(struct octeon_config *mcp)
{
	iobus_bus_io_init(&mcp->mc_iobus_bust, mcp);

	iobus_bust = &mcp->mc_iobus_bust;
	iobus_dmat = &mcp->mc_iobus_dmat;
}

/* ---- autoconf */

CFATTACH_DECL_NEW(iobus, sizeof(struct iobus_softc), iobus_match, iobus_attach, NULL,
    NULL);

static int
iobus_match(device_t parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
iobus_attach(device_t parent, device_t self, void *aux)
{
	struct iobus_softc *sc = device_private(self);
	const struct iobus_dev *dev;
	struct iobus_attach_args aa;
	int i, j;

	sc->sc_dev = self;

	aprint_normal("\n");

	iobus_init(sc);

	for (i = 0; i < (int)iobus_ndevs; i++) {
		dev = iobus_devs[i];
		for (j = 0; j < dev->nunits; j++) {
			aa.aa_name = dev->name;
			aa.aa_unitno = j;
			aa.aa_unit = &dev->units[j];
			aa.aa_bust = iobus_bust;
			aa.aa_dmat = iobus_dmat;

			(void)config_found_sm_loc(
				self,
				"iobus",
				NULL,
				&aa,
				iobus_print,
				iobus_submatch);
		}
	}
}

static int
iobus_submatch(device_t parent, struct cfdata *cf,
    const int *ldesc, void *aux)
{
	return config_match(parent, cf, aux);
}

static int
iobus_print(void *aux, const char *pnp)
{
	struct iobus_attach_args *aa = aux;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);

	aprint_normal(" address 0x%016" PRIx64, aa->aa_unit->addr);

	return UNCONF;
}

/* ---- */

void
iobus_init(struct iobus_softc *sc)
{
	iobus_init_map(sc);
	iobus_init_local(sc);
}

void
iobus_init_map(struct iobus_softc *sc)
{
	/* XXX map all ``operations'' space at once */
	bus_space_map(
		iobus_bust,
		0x0001280000000000ULL,
		0x0001800000000000ULL - 0x0001280000000000ULL,
		0,
		&sc->sc_ops_bush);
}

void
iobus_init_local(struct iobus_softc *sc)
{
	iobus_init_local_pow(sc);
	iobus_init_local_fpa(sc);
}

extern struct octeon_config octeon_configuration;

void
iobus_init_local_pow(struct iobus_softc *sc)
{
	void octeon_pow_bootstrap(struct octeon_config *);

	aprint_normal("%s: initializing POW\n", device_xname(sc->sc_dev));

	octeon_pow_bootstrap(&octeon_configuration);
}

void
iobus_init_local_fpa(struct iobus_softc *sc)
{
	void octeon_fpa_bootstrap(struct octeon_config *);

	aprint_normal("%s: initializing FPA\n", device_xname(sc->sc_dev));

	octeon_fpa_bootstrap(&octeon_configuration);
}

/* ---- bus_space(9) */

#define	CHIP	iobus
#define	CHIP_IO
#define	CHIP_ACCESS_SIZE	8

/* CIU and GPIO NCB type CSRs */
#define	CHIP_W1_BUS_START(v)	0x0001070000000000ULL
#define	CHIP_W1_BUS_END(v)	0x00010700ffffffffULL
#define	CHIP_W1_SYS_START(v)	0x8001070000000000ULL
#define	CHIP_W1_SYS_END(v)	0x80010700ffffffffULL

/* a number of RSL type CSRs */
#define	CHIP_W2_BUS_START(v)	0x0001180000000000ULL
#define	CHIP_W2_BUS_END(v)	0x000118ffffffffffULL
#define	CHIP_W2_SYS_START(v)	0x8001180000000000ULL
#define	CHIP_W2_SYS_END(v)	0x800118ffffffffffULL

/* load/IOBDMA/store operations */
#define	CHIP_W3_BUS_START(v)	0x0001280000000000ULL
#define	CHIP_W3_BUS_END(v)	0x00017fffffffffffULL
#define	CHIP_W3_SYS_START(v)	0x8001280000000000ULL
#define	CHIP_W3_SYS_END(v)	0x80017fffffffffffULL

#include <mips/mips/bus_space_alignstride_chipdep.c>
