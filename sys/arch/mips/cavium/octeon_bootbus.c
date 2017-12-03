/*	$NetBSD: octeon_bootbus.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_bootbus.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _MIPS_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/bootbusvar.h>

static int	bootbus_match(device_t, struct cfdata *, void *);
static void	bootbus_attach(device_t, device_t, void *);
static int      bootbus_submatch(device_t, struct cfdata *,
		    const int *, void *);
static int	bootbus_print(void *, const char *);
static void	bootbus_init(void);

static void	bootbus_bus_io_init(bus_space_tag_t, void *);

static struct mips_bus_space	*bootbus_bust;
static struct mips_bus_dma_tag	*bootbus_dmat;

void
bootbus_bootstrap(struct octeon_config *mcp)
{

	bootbus_bus_io_init(&mcp->mc_bootbus_bust, mcp);

	bootbus_bust = &mcp->mc_bootbus_bust;
	bootbus_dmat = &mcp->mc_bootbus_dmat;
}

/* ---- autoconf */

CFATTACH_DECL_NEW(bootbus, sizeof(device_t), bootbus_match, bootbus_attach, NULL,
    NULL);

static int
bootbus_match(device_t parent, struct cfdata *match, void *aux)
{

	return 1;
}

static void
bootbus_attach(device_t parent, device_t self, void *aux)
{
	const struct bootbus_dev *dev;
	struct bootbus_attach_args aa;
	int i, j;

	aprint_normal("\n");

	bootbus_init();

	for (i = 0; i < (int)bootbus_ndevs; i++) {
		dev = bootbus_devs[i];
		for (j = 0; j < dev->nunits; j++) {
			aa.aa_name = dev->name;
			aa.aa_unitno = j;
			aa.aa_unit = &dev->units[j];
			aa.aa_bust = bootbus_bust;
			aa.aa_dmat = bootbus_dmat;

			(void)config_found_sm_loc(
				self,
				"bootbus",
				NULL,
				&aa,
				bootbus_print,
				bootbus_submatch);
		}
	}
}

static int
bootbus_submatch(device_t parent, struct cfdata *cf,
    const int *ldesc, void *aux)
{

	return config_match(parent, cf, aux);
}

static int
bootbus_print(void *aux, const char *pnp)
{
	struct bootbus_attach_args *aa = aux;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);

	aprint_normal(": address=0x%016" PRIx64, aa->aa_unit->addr);

	return UNCONF;
}

static void
bootbus_init(void)
{

}


/* ---- bus_space(9) */

#define	CHIP	bootbus
#define	CHIP_IO
#define	CHIP_ACCESS_SIZE	8

#define	CHIP_W1_BUS_START(v)	0x0000000000000000ULL
#define	CHIP_W1_BUS_END(v)	0x000000001fffffffULL
#define	CHIP_W1_SYS_START(v)	0x00000000a0000000ULL
#define	CHIP_W1_SYS_END(v)	0x00000000bfffffffULL

#include <mips/mips/bus_space_alignstride_chipdep.c>
