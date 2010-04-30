/* $NetBSD: opb.c,v 1.23.88.1 2010/04/30 14:39:42 uebayasi Exp $ */

/*
 * Copyright 2001,2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: opb.c,v 1.23.88.1 2010/04/30 14:39:42 uebayasi Exp $");

#include "locators.h"
#include "opt_emac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/opbreg.h>
#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/dev/rgmiireg.h>
#include <powerpc/ibm4xx/dev/zmiireg.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/spr.h>

#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/amcc405ex.h>

static int opb_get_frequency_405gp(void);
static int opb_get_frequency_405ex(void);


/*
 * The devices on the On-chip Peripheral Bus to the 405GP/EX CPU.
 */
const struct opb_dev {
	int pvr;
	const char *name;
	bus_addr_t addr;
	int instance;
	int irq;
	int flags;
} opb_devs [] = {
	/* IBM405GP */
	{ IBM405GP,	"com",	IBM405GP_UART0_BASE,	0,  0, 0 },
	{ IBM405GP,	"com",	IBM405GP_UART1_BASE,	1,  1, 0 },
	{ IBM405GP,	"emac",	IBM405GP_EMAC0_BASE,	0, 15, 0 },
	{ IBM405GP,	"opbgpio",	IBM405GP_GPIO0_BASE,	0, -1, 0 },
	{ IBM405GP,	"gpiic",IBM405GP_IIC0_BASE,	0,  2, 0 },
	{ IBM405GP,	"wdog",	-1,	        	0, -1, 0 },

	/* IBM405GPR */
	{ IBM405GPR,	"com",	IBM405GP_UART0_BASE,	0,  0, 0 },
	{ IBM405GPR,	"com",	IBM405GP_UART1_BASE,	1,  1, 0 },
	{ IBM405GPR,	"emac",	IBM405GP_EMAC0_BASE,	0, 15, 0 },
	{ IBM405GPR,	"opbgpio",	IBM405GP_GPIO0_BASE,	0, -1, 0 },
	{ IBM405GPR,	"gpiic",IBM405GP_IIC0_BASE,	0,  2, 0 },
	{ IBM405GPR,	"wdog",	-1,	        	0, -1, 0 },

	/* AMCC405EX */
	{ AMCC405EX,	"gpt",	AMCC405EX_GPT0_BASE,	 0, -1, 0 },
	{ AMCC405EX,	"com",	AMCC405EX_UART0_BASE,	 0, 26, 0 },
	{ AMCC405EX,	"com",	AMCC405EX_UART1_BASE,	 1,  1, 0 },
	{ AMCC405EX,	"gpiic",AMCC405EX_IIC0_BASE,	 0,  2, 0 },
	{ AMCC405EX,	"gpiic",AMCC405EX_IIC1_BASE,	 1,  7, 0 },
	{ AMCC405EX,	"scp",	AMCC405EX_SCP0_BASE,	 0,  8, 0 }, /* SPI */
	{ AMCC405EX,	"opbgpio",	AMCC405EX_GPIO0_BASE,	-1, -1, 0 },
	{ AMCC405EX,	"emac",	AMCC405EX_EMAC0_BASE,	 0, 24,
	    OPB_FLAGS_EMAC_GBE | OPB_FLAGS_EMAC_STACV2 | OPB_FLAGS_EMAC_HT256 |\
	    OPB_FLAGS_EMAC_RMII_RGMII },
	{ AMCC405EX,	"emac",	AMCC405EX_EMAC1_BASE,	 1, 25,
	    OPB_FLAGS_EMAC_GBE | OPB_FLAGS_EMAC_STACV2 | OPB_FLAGS_EMAC_HT256 |\
	    OPB_FLAGS_EMAC_RMII_RGMII },
	{ AMCC405EX,	"wdog",	-1,			 0, -1, 0 },

	{ 0,		 NULL }
};

int (*opb_get_frequency)(void);
const struct opb_param {
	int pvr;
	bus_addr_t base;
	bus_addr_t limit;
	int (*opb_get_frequency)(void);
	bus_addr_t zmii_base;
	bus_addr_t rgmii_base;
} opb_params[] = {
	{ IBM405GP,
	    IBM405GP_IP_BASE,	IBM405GP_IP_BASE + OPBREG_SIZE,
	    opb_get_frequency_405gp,
	    0,				0 },
	{ IBM405GPR,
	    IBM405GP_IP_BASE,	IBM405GP_IP_BASE + OPBREG_SIZE,
	    opb_get_frequency_405gp,
	    0,				0 },
	{ AMCC405EX,
	    AMCC405EX_OPB_BASE,	AMCC405EX_OPB_BASE + OPBREG_SIZE,
	    opb_get_frequency_405ex,
	    0,				AMCC405EX_RGMIIB0_BASE},

	{ 0 }
};

static int	opb_match(device_t, cfdata_t, void *);
static void	opb_attach(device_t, device_t, void *);
static int	opb_submatch(device_t, cfdata_t, const int *, void *);
static int	opb_print(void *, const char *);

CFATTACH_DECL_NEW(opb, sizeof(struct opb_softc),
    opb_match, opb_attach, NULL, NULL);

static struct powerpc_bus_space opb_tag = {
	_BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000,
};
static char ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));
static int opb_tag_init_done;

/*
 * Probe for the opb; always succeeds.
 */
static int
opb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only opb devices */
	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static int
opb_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct opb_attach_args *oaa = aux;

	if (cf->cf_loc[OPBCF_ADDR] != OPBCF_ADDR_DEFAULT &&
	    cf->cf_loc[OPBCF_ADDR] != oaa->opb_addr)
		return 0;

	return config_match(parent, cf, aux);
}

/*
 * Attach the on-chip peripheral bus.
 */
static void
opb_attach(device_t parent, device_t self, void *aux)
{
	struct opb_softc *sc = device_private(self);
	struct plb_attach_args *paa = aux;
	struct opb_attach_args oaa;
	int i, pvr;

	aprint_naive("\n");
	aprint_normal("\n");
	pvr = mfpvr() >> 16;

	sc->sc_dev = self;
	sc->sc_iot = opb_get_bus_space_tag();

	for (i = 0; opb_params[i].pvr != 0 && opb_params[i].pvr != pvr; i++)
		;
	if (opb_params[i].pvr == 0)
		panic("opb_get_bus_space_tag: no params for this CPU!");
	opb_get_frequency = opb_params[i].opb_get_frequency;
#ifdef EMAC_ZMII_PHY
	if (opb_params[i].zmii_base != 0)
		bus_space_map(sc->sc_iot, opb_params[i].zmii_base, ZMII0_SIZE,
		    0, &sc->sc_zmiih);
#endif
#ifdef EMAC_RGMII_PHY
	if (opb_params[i].rgmii_base != 0)
		bus_space_map(sc->sc_iot, opb_params[i].rgmii_base, RGMII0_SIZE,
		    0, &sc->sc_rgmiih);
#endif

	for (i = 0; opb_devs[i].name != NULL; i++) {
		if (opb_devs[i].pvr != pvr)
			continue;
		oaa.opb_name = opb_devs[i].name;
		oaa.opb_addr = opb_devs[i].addr;
		oaa.opb_instance = opb_devs[i].instance;
		oaa.opb_irq = opb_devs[i].irq;
		oaa.opb_bt = sc->sc_iot;
		oaa.opb_dmat = paa->plb_dmat;
		oaa.opb_flags = opb_devs[i].flags;

		(void) config_found_sm_loc(self, "opb", NULL, &oaa, opb_print,
					   opb_submatch);
	}
}

static int
opb_print(void *aux, const char *pnp)
{
	struct opb_attach_args *oaa = aux;

	if (pnp)
		aprint_normal("%s at %s", oaa->opb_name, pnp);

	if (oaa->opb_addr != OPBCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%08lx", oaa->opb_addr);
	if (oaa->opb_irq != OPBCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", oaa->opb_irq);

	return UNCONF;
}

bus_space_tag_t
opb_get_bus_space_tag(void)
{
	int i, pvr;

	if (!opb_tag_init_done) {
		pvr = mfpvr() >> 16;

		for (i = 0; opb_params[i].pvr != 0 && opb_params[i].pvr != pvr;
		    i++)
			;
		if (opb_params[i].pvr == 0)
			panic("opb_get_bus_space_tag: no params for this CPU!");

		opb_tag.pbs_base = opb_params[i].base;
		opb_tag.pbs_limit = opb_params[i].limit;

		if (bus_space_init(&opb_tag, "opbtag",
		    ex_storage, sizeof(ex_storage)))
			panic("opb_attach: Failed to initialise opb_tag");
		opb_tag_init_done = 1;
	}

	return &opb_tag;
}

static int
opb_get_frequency_405gp(void)
{
	prop_number_t pn;
	uint32_t pllmr;
	unsigned int processor_freq, plb_freq, opb_freq;

	pn = prop_dictionary_get(board_properties, "processor-frequency");
	KASSERT(pn != NULL);
	processor_freq = (unsigned int) prop_number_integer_value(pn);
	pllmr = mfdcr(DCR_CPC0_PLLMR);
	plb_freq = processor_freq / CPC0_PLLMR_CBDV(pllmr);
	opb_freq = plb_freq / CPC0_PLLMR_OPDV(pllmr);

	return opb_freq;
}

static int
opb_get_frequency_405ex(void)
{
	prop_number_t pn;
	unsigned int processor_freq, plb_freq, opb_freq;

	pn = prop_dictionary_get(board_properties, "processor-frequency");
	KASSERT(pn != NULL);
	processor_freq = (unsigned int) prop_number_integer_value(pn);
	plb_freq = processor_freq / CPR0_PLBDV0(mfcpr(DCR_CPR0_PLBD));
	opb_freq = plb_freq / CPR0_OPBDV0(mfcpr(DCR_CPR0_OPBD));

	return opb_freq;
}
