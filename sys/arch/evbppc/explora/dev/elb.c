/*	$NetBSD: elb.c,v 1.1 2003/03/11 10:57:57 hannken Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/explora.h>
#define _IBM4XX_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <powerpc/ibm4xx/dcr403cgx.h>

#include <evbppc/explora/dev/elbvar.h>

struct elb_dev {
	const char *elb_name;
	int elb_addr;
	int elb_addr2;
	int elb_irq;
};

static int	elb_match(struct device *, struct cfdata *, void *);
static void	elb_attach(struct device *, struct device *, void *);
static int	elb_print(void *, const char *);

static struct elb_dev elb_devs[] = {
	{ "cpu",	0,		0,		-1 },
	{ "pckbc",	BASE_PCKBC,	BASE_PCKBC2,	31 },
	{ "com",	BASE_COM,	0,		30 },
	{ "lpt",	BASE_LPT,	0,		-1 },
	{ "fb",		BASE_FB,	BASE_FB2,	-1 },
	{ "le",		BASE_LE,	0,		28 },
};

CFATTACH_DECL(elb, sizeof(struct device),
    elb_match, elb_attach, NULL, NULL);

/*
 * Probe for the elb; always succeeds.
 */
static int
elb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

/*
 * Attach the Explora local bus.
 */
static void
elb_attach(struct device *parent, struct device *self, void *aux)
{
	struct elb_attach_args eaa;
	int i;

	printf("\n");
	for (i = 0; i < sizeof(elb_devs)/sizeof(elb_devs[0]); i++) {
		eaa.elb_name = elb_devs[i].elb_name;
		eaa.elb_bt = MAKE_BUS_TAG(elb_devs[i].elb_addr);
		eaa.elb_dmat = &ibm4xx_default_bus_dma_tag;
		eaa.elb_base = elb_devs[i].elb_addr;
		eaa.elb_base2 = elb_devs[i].elb_addr2;
		eaa.elb_irq = elb_devs[i].elb_irq;

		(void) config_found_sm(self, &eaa, elb_print, NULL);
	}
}

static int
elb_print(void *aux, const char *pnp)
{
	struct elb_attach_args *eaa = aux;

	if (pnp)
		aprint_normal("%s at %s", eaa->elb_name, pnp);
	if (eaa->elb_irq != -1)
		aprint_normal(" irq %d", eaa->elb_irq);

	return (UNCONF);
}
