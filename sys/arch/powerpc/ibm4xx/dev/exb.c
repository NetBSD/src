/*	$Id: exb.c,v 1.2 2010/11/06 16:30:15 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Masao Uebayashi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exb.c,v 1.2 2010/11/06 16:30:15 uebayasi Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/dev/exbvar.h>

struct exb_softc {
};

extern const struct exb_conf exb_confs[];

static int exb_match(device_t, struct cfdata *, void *);
static void exb_attach(device_t, struct device *, void *);
static int exb_print(void *, const char *);

CFATTACH_DECL_NEW(exb, sizeof(struct exb_softc), exb_match, exb_attach,
    NULL, NULL);

static struct powerpc_bus_space exb_bus_space_tag =
    { _BUS_SPACE_BIG_ENDIAN | _BUS_SPACE_MEM_TYPE, 0 };

static int
exb_match(device_t parent, struct cfdata *cf, void *aux)
{

	return 1;
}

static void
exb_attach(device_t parent, device_t self, void *aux)
{
	const struct exb_conf *ec = exb_confs;
	uint32_t ebc0_bNcr;
	bus_addr_t base, size;
	int locs[EXBCF_NLOCS];

	printf("\n");

	/* mtdcr needs a constant */
	switch (device_unit(self)) {
	case 0: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B0CR);	break;
	case 1: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B1CR);	break;
	case 2: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B2CR);	break;
	case 3: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B3CR);	break;
	case 4: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B4CR);	break;
	case 5: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B5CR);	break;
	case 6: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B6CR);	break;
	case 7: mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B7CR);	break;
	}
	ebc0_bNcr = mfdcr(DCR_EBC0_CFGDATA);
	base = ebc0_bNcr & 0xfff00000;
	size = 0x00100000 << ((ebc0_bNcr & 0x000e0000) >> 17);

	exb_bus_space_tag.pbs_base = base;
	exb_bus_space_tag.pbs_limit = base + size - 1;

	while (ec->ec_name) {
		struct exb_conf ec1;

		locs[EXBCF_ADDR] = ec->ec_addr;

		ec1 = *ec;
		ec1.ec_bust = exb_get_bus_space_tag();

		config_found_sm_loc(self, "exb", locs, &ec1, exb_print,
		    config_stdsubmatch);
		ec++;
	}
}

static int
exb_print(void *aux, const char *bus)
{
	struct exb_conf *ec = aux;

	if (bus)
		return QUIET;

	if (ec->ec_addr)
		printf(" addr %"PRIxBUSADDR, ec->ec_addr);

	return UNCONF;
}

bus_space_tag_t
exb_get_bus_space_tag(void)
{
	static char ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
	    __attribute__((aligned(8)));
	static int exb_bus_space_tag_inited;

	if (exb_bus_space_tag_inited == 0) {
		if (bus_space_init(&exb_bus_space_tag, "exbtag",
		    ex_storage, sizeof(ex_storage)))
			panic("exb_attach: Failed to initialise exb_bus_space_tag");
		exb_bus_space_tag_inited = 1;
	}
	return &exb_bus_space_tag;
}
