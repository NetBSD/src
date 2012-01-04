/*	$NetBSD: rmixl_cpunode.c,v 1.1.2.6 2012/01/04 16:17:53 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpunode.c,v 1.1.2.6 2012/01/04 16:17:53 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>

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

static u_int rmixl_nodes;

static int
cpunode_rmixl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	u_int node = ma->ma_node;

	if (!cpu_rmixl(mips_options.mips_cpu))
		return 0;

	if (node == MAINBUSCF_NODE_DEFAULT) {
		node = ffs(rmixl_nodes) - 1;
		if (node == 0)
			return 0;
		ma->ma_node = node;
	}

	/* max of 4 nodes */
	if (node >= 4)
		return 0;

	/* Make sure we haven't attached this node before */
	if (rmixl_nodes & __BIT(node))
		return 0;

	/* Node 0 always is good */
	if (node == 0)
		return 1;

	/* XLR/XLS can only have one node */
	if (cpu_rmixlr(mips_options.mips_cpu)
	    || cpu_rmixls(mips_options.mips_cpu))
		return 0;

	/*
	 * To see if a node is present, check to see if it PCI devices exist.
	 */
	const pcireg_t id =
	    rmixlp_read_4(_RMIXL_PCITAG(0, node * 8, 0), PCI_ID_REG);
	if (id != PCI_ID_CODE(PCI_VENDOR_NETLOGIC,PCI_PRODUCT_NETLOGIC_XLP_SBC))
		return 0;

	return 1;
}

static void
cpunode_rmixl_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const sc = device_private(self);
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct mainbus_attach_args * const ma = aux;
	struct cpunode_attach_args na;
	const u_int cpu_cidflags = mips_options.mips_cpu->cpu_cidflags;
	const u_int rmi_type = cpu_cidflags & MIPS_CIDFL_RMI_TYPE;
	const u_int ncores = rcp->rc_ncores;
	u_int sz;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;
	rmixl_nodes |= __BIT(ma->ma_node);

	switch (rmi_type) {
	case CIDFL_RMI_TYPE_XLR:
	case CIDFL_RMI_TYPE_XLS:
		/*
		 * L2 is unified on XLR, XLS
		 */
		sz = (size_t)MIPS_CIDFL_RMI_L2SZ(cpu_cidflags);
		aprint_normal_dev(self,
			"%uKB/32B %u-banked 8-way set associative"
			" L2 unified cache\n",
			sz/1024, sz/(256 * 1024));
		break;
	case CIDFL_RMI_TYPE_XLP: {
		/*
		 * L3 is unified on XLP.  Why they don't use COP0 Config2 for
		 * this bothers me (except 16-way doesn't have an encoding).
		 */
		sz = (size_t)MIPS_CIDFL_RMI_L3SZ(cpu_cidflags);
		aprint_normal_dev(self,
		    "%u bank%s of %uKB/64B 16-way set associative L3 unified cache\n",
		    ncores, ncores == 1 ? "" : "s", sz/1024);
		break;
	}
	}

	aprint_normal_dev(self, "%u %s on node\n", ncores,
		ncores == 1 ? "core" : "cores");

	/*
	 * Attach cpu (RMI thread) devices
	 */
	for (u_int i = 0; i < ncores; i++) {
		na.na_name = "cpucore";
		na.na_node = ma->ma_node;
		na.na_core = i;
		na.na_dmat29 = ma->ma_dmat29;
		na.na_dmat32 = ma->ma_dmat32;
		na.na_dmat64 = ma->ma_dmat64;
		config_found(self, &na, cpunode_rmixl_print);
	}

	/*
	 * Attach obio (XLR/XLS)
	 */
	switch (rmi_type) {
	case CIDFL_RMI_TYPE_XLR:
	case CIDFL_RMI_TYPE_XLS:
		na.na_name = "obio";
		config_found(self, &na, NULL);
		break;
	case CIDFL_RMI_TYPE_XLP:
		break;
	}
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
