/* $NetBSD: opb.c,v 1.13 2003/07/15 02:54:44 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: opb.c,v 1.13 2003/07/15 02:54:44 lukem Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/ibm405gp.h>

/*
 * The devices on the On-chip Peripheral Bus to the 405GP cpu.
 */
const struct opb_dev {
	int pvr;
	const char *name;
	bus_addr_t addr;
	int irq;
} opb_devs [] = {
	{ IBM405GP,	"com",	IBM405GP_UART0_BASE,	 0 },
	{ IBM405GP,	"com",	IBM405GP_UART1_BASE,	 1 },
	{ IBM405GP,	"emac",	IBM405GP_EMAC0_BASE,	 9 }, /* XXX: really irq 9..15 */
	{ IBM405GP,	"gpio",	IBM405GP_GPIO0_BASE,	-1 },
	{ IBM405GP,	"iic",	IBM405GP_IIC0_BASE,	 2 },
	{ IBM405GP,	"wdog",	-1,	        	-1 },
	{ 0,		 NULL }
};

static int	opb_match(struct device *, struct cfdata *, void *);
static void	opb_attach(struct device *, struct device *, void *);
static int	opb_submatch(struct device *, struct cfdata *, void *);
static int	opb_print(void *, const char *);

CFATTACH_DECL(opb, sizeof(struct device),
    opb_match, opb_attach, NULL, NULL);

/*
 * Probe for the opb; always succeeds.
 */
static int
opb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct opb_attach_args *oaa = aux;

	/* match only opb devices */ 
	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
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

	return (config_match(parent, cf, aux));
}

/*
 * Attach the on-chip peripheral bus.
 */
static void
opb_attach(struct device *parent, struct device *self, void *aux)
{
	struct plb_attach_args *paa = aux;
	struct opb_attach_args oaa;
	int i, pvr;

	printf("\n");
	pvr = mfpvr() >> 16;

	for (i = 0; opb_devs[i].name != NULL; i++) {
		if (opb_devs[i].pvr != pvr)
			continue;
		oaa.opb_name = opb_devs[i].name;
		oaa.opb_addr = opb_devs[i].addr;
		oaa.opb_irq = opb_devs[i].irq;
		oaa.opb_bt = paa->plb_bt;
		oaa.opb_dmat = paa->plb_dmat;

		(void) config_found_sm(self, &oaa, opb_print, opb_submatch);
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

	return (UNCONF);
}
