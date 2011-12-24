/*	$NetBSD: rmixl_cpunode.c,v 1.1.2.5 2011/12/24 01:57:54 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpunode.c,v 1.1.2.5 2011/12/24 01:57:54 matt Exp $");

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

const char *rmixl_cpuname;
static char xlpxxx_cpuname[8];

static int  cpunode_rmixl_match(device_t, cfdata_t, void *);
static void cpunode_rmixl_attach(device_t, device_t, void *);
static int  cpunode_rmixl_print(void *, const char *);

CFATTACH_DECL_NEW(cpunode_rmixl, sizeof(struct cpunode_softc),
	cpunode_rmixl_match, cpunode_rmixl_attach, NULL, NULL);

rmixlp_variant_t rmixl_xlp_variant;
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
	struct mainbus_attach_args * const ma = aux;
	struct cpunode_attach_args na;
	const u_int cpu_cidflags = mips_options.mips_cpu->cpu_cidflags;
	u_int sz;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;
	rmixl_nodes |= __BIT(ma->ma_node);

	u_int ncores = MIPS_CIDFL_RMI_NCORES(cpu_cidflags);
	const u_int rmi_type = cpu_cidflags & MIPS_CIDFL_RMI_TYPE;

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
		uint32_t cfg_status0 = rmixlp_read_4(RMIXLP_SM_PCITAG,
		    RMIXLP_SM_EFUSE_DEVICE_CFG_STATUS0);
		const char *sfx = "";
		char msd;
		if (mips_options.mips_cpu->cpu_pid == MIPS_XLP8XX) {
			/*
			 * From XLP 8xx-4XX PRM 1.41 (2011-12-16)
			 * 31.4 System Configuration
			 */
			uint32_t cfg_status1 = rmixlp_read_4(RMIXLP_SM_PCITAG,
			    RMIXLP_SM_EFUSE_DEVICE_CFG_STATUS1);
			uint8_t id0 = cfg_status0; /* bits 7:0 */
			for (ncores = 8; id0 != 0; id0 >>= 1) {
				ncores -= id0 & 1;
			}
			if ((cfg_status1 & 7) == 0) {
				msd = '8';
				rmixl_xlp_variant = RMIXLP_8XX;
			} else {
				msd = '4';
				rmixl_xlp_variant = RMIXLP_4XX;
			}
		} else if (mips_options.mips_cpu->cpu_pid == MIPS_XLP3XX) {
			/*
			 * From XLP3xx_3xx-Lite PRM 1.41 (2011-12-16)
			 * 30.4 System Configuration
			 */
			uint8_t idl = (cfg_status0 >> 8) & 0xf;
			uint8_t idh = (cfg_status0 >> 16) & 0xff;
			size_t variant = (cfg_status0 >> 4) & 0x3;
			static const char sfxs[4][2] = { "", "L", "H", "Q" };
			sfx = sfxs[variant];
			if (idh == 0xff) {
				ncores = 1;
			} else if (idl != 0) {
				ncores = 2;
			} else {
				ncores = 4;
			}
			msd = '3';
			rmixl_xlp_variant = RMIXLP_3XX + variant;
		} else {
			panic("%s: unknown RMI XLP variant %#x!",
			    __func__, mips_options.mips_cpu->cpu_pid);
		}

		snprintf(xlpxxx_cpuname, sizeof(xlpxxx_cpuname),
		    "XLP%c%02u%s", msd, ncores * 4, sfx);
		rmixl_cpuname = xlpxxx_cpuname;

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
