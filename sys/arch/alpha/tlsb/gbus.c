/* $NetBSD: gbus.c,v 1.21.10.1 2011/06/23 14:18:55 cherry Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Autoconfiguration and support routines for the Gbus: the internal
 * bus on AlphaServer CPU modules.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: gbus.c,v 1.21.10.1 2011/06/23 14:18:55 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/tlsb/gbusreg.h>
#include <alpha/tlsb/gbusvar.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>

#include "locators.h"

#define KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))

struct gbus_softc {
	device_t	sc_dev;
	int		sc_tlsbnode;	/* node on the TurboLaser */
};

static int	gbusmatch(device_t, cfdata_t, void *);
static void	gbusattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gbus, sizeof(struct gbus_softc),
    gbusmatch, gbusattach, NULL, NULL);

static int	gbusprint(void *, const char *);

const struct gbus_attach_args gbus_children[] = {
	{ "zsc",	GBUS_DUART0_OFFSET },
	{ "zsc",	GBUS_DUART1_OFFSET },
	{ "mcclock",	GBUS_CLOCK_OFFSET },
	{ NULL,		0 },
};

static int
gbusprint(void *aux, const char *pnp)
{
	struct gbus_attach_args *ga = aux;

	if (pnp)
		aprint_normal("%s at %s", ga->ga_name, pnp);
	aprint_normal(" offset 0x%lx", ga->ga_offset);
	return (UNCONF);
}

static int
gbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct tlsb_dev_attach_args *ta = aux;

	/*
	 * Make sure we're looking for a Gbus.  The Gbus only
	 * "exists" on the CPU module that holds the primary CPU.
	 *
	 * Compute which node this should exist on by dividing the
	 * primary CPU by 2 (since there are up to 2 CPUs per CPU
	 * module).
	 */
	if (TLDEV_ISCPU(ta->ta_dtype) &&
	    ta->ta_node == (hwrpb->rpb_primary_cpu_id / 2))
		return (1);

	return (0);
}

static void
gbusattach(device_t parent, device_t self, void *aux)
{
	struct gbus_softc *sc = device_private(self);
	struct tlsb_dev_attach_args *ta = aux;
	const struct gbus_attach_args *ga;
	int locs[GBUSCF_NLOCS];

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_tlsbnode = ta->ta_node;

	/* Attach the children. */
	for (ga = gbus_children; ga->ga_name != NULL; ga++) {
		struct gbus_attach_args gaa = *ga;
		locs[GBUSCF_OFFSET] = gaa.ga_offset;
		(void) config_found_sm_loc(self, "gbus", locs, &gaa,
			   gbusprint, config_stdsubmatch);
	}
}
