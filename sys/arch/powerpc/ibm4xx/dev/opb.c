/* $NetBSD: opb.c,v 1.1 2002/08/12 02:06:20 simonb Exp $ */

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

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _GALAXY_BUS_DMA_PRIVATE
#include <machine/walnut.h>

#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/ibm405gp.h>

/*
 * The devices on the On-chip Peripheral Bus to the 405GP cpu.
 */
const struct opb_dev {
	const char *name;
	bus_addr_t addr;
	int irq;
} opb_devs [] = {
	{ "com",	UART0_BASE,	 0 },
	{ "com",	UART1_BASE,	 1 },
	{ "emac",	EMAC0_BASE,	 9 }, /* XXX: really irq 9..15 */
	{ "gpio",	GPIO0_BASE,	-1 },
	{ "iic",	IIC0_BASE,	 2 },
	{ "wdog",	-1,        	-1 },
	{ NULL }
};

static int	opb_match(struct device *, struct cfdata *, void *);
static void	opb_attach(struct device *, struct device *, void *);
static int	opb_submatch(struct device *, struct cfdata *, void *);
static int	opb_print(void *, const char *);

struct cfattach opb_ca = {
	sizeof(struct device), opb_match, opb_attach
};


/*
 * Probe for the opb; always succeeds.
 */
static int
opb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only opb devices */ 
	if (strcmp(oaa->opb_name, cf->cf_driver->cd_name) != 0)
		return (0);

	return (1);
}

static int
opb_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	if (cf->cf_loc[OPBCF_ADDR] != OPBCF_ADDR_DEFAULT &&
	    cf->cf_loc[OPBCF_ADDR] != oaa->opb_addr)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

/*
 * Attach the on-chip peripheral bus.
 */
static void
opb_attach(struct device *parent, struct device *self, void *aux)
{
	struct opb_attach_args oaa;
	int i;

	printf("\n");

	for (i = 0; opb_devs[i].name != NULL; i++) {
		oaa.opb_name = opb_devs[i].name;
		oaa.opb_addr = opb_devs[i].addr;
		oaa.opb_irq = opb_devs[i].irq;
		oaa.opb_bt = galaxy_make_bus_space_tag(0, 0);
		oaa.opb_dmat = &galaxy_default_bus_dma_tag;

		(void) config_found_sm(self, &oaa, opb_print, opb_submatch);
	}
}

static int
opb_print(void *aux, const char *pnp)
{
	struct opb_attach_args *oaa = aux;

	if (pnp)
		printf("%s at %s", oaa->opb_name, pnp);

	if (oaa->opb_addr != OPBCF_ADDR_DEFAULT)
		printf(" addr 0x%08lx", oaa->opb_addr);
	if (oaa->opb_irq != OPBCF_IRQ_DEFAULT)
		printf(" irq %d", oaa->opb_irq);

	return (UNCONF);
}
