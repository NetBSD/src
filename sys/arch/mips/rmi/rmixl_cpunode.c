/*	$NetBSD: rmixl_cpunode.c,v 1.1.2.3 2010/01/20 20:48:12 matt Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * mainbus.c
 *
 * cpunode configuration
 *
 * Created      : 15/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpunode.c,v 1.1.2.3 2010/01/20 20:48:12 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <evbmips/rmixl/autoconf.h>

#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_cpunodevar.h>

#include <machine/bus.h>
#include "locators.h"

static int  cpunode_rmixl_match(device_t, cfdata_t, void *);
static void cpunode_rmixl_attach(device_t, device_t, void *);
static int  cpunode_rmixl_print(void *, const char *);

CFATTACH_DECL_NEW(cpunode_rmixl, sizeof(struct cpunode_softc),
	cpunode_rmixl_match, cpunode_rmixl_attach, NULL, NULL);

static int
cpunode_rmixl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (!cpu_rmixl(mips_options.mips_cpu))
		return 0;

	/* XXX for now attach one node only */
	if (ma->ma_node != 0)
		return 0;

	return 1;
}

static void
cpunode_rmixl_attach(device_t parent, device_t self, void *aux)
{
	u_int sz;
	u_int ncores;
	struct cpunode_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	struct cpunode_attach_args na;
	const u_int cpu_cidflags = mips_options.mips_cpu->cpu_cidflags;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;
	sc->sc_attached = 1;

	switch (cpu_cidflags & MIPS_CIDFL_RMI_TYPE) {
	case CIDFL_RMI_TYPE_XLR:
	case CIDFL_RMI_TYPE_XLS:
		/*
		 * L2 is unified on XLR, XLS
		 */
		sz = (size_t)MIPS_CIDFL_RMI_L2SZ(cpu_cidflags);
		aprint_normal("%s: %dKB/32B %d-banked 8-way set associative"
			" unified L2 cache\n", device_xname(self),
			sz/1024, sz/(256 * 1024));
		break;
	case CIDFL_RMI_TYPE_XLP:
		/* TBD */
		break;
	}

	ncores = MIPS_CIDFL_RMI_NTHREADS(cpu_cidflags);
	aprint_normal("%s: %d %s on node\n", device_xname(self), ncores,
		ncores == 1 ? "core" : "cores");

	/*
	 * Attach cpu (RMI thread) devices
	 */
	for (int i=0; i < ncores; i++) {
		na.na_name = "cpucore";
		na.na_node = ma->ma_node;
		na.na_core = i;
		config_found(self, &na, cpunode_rmixl_print);
	}

	/*
	 * Attach obio
	 */
	na.na_name = "obio";
	config_found(self, &na, NULL);
}

static int
cpunode_rmixl_print(void *aux, const char *pnp)
{
	struct cpunode_attach_args *na = aux;

	if (pnp != NULL)
		aprint_normal("%s:", pnp);
	aprint_normal(" core %d", na->na_core);

	return (UNCONF);
}
