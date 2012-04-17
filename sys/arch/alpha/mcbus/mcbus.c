/* $NetBSD: mcbus.c,v 1.21.2.1 2012/04/17 00:05:56 yamt Exp $ */

/*
 * Copyright (c) 1998 by Matthew Jacob
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
 * Autoconfiguration routines for the MCBUS system
 * bus found on AlphaServer 4100 systems.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcbus.c,v 1.21.2.1 2012/04/17 00:05:56 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/mcbus/mcbusreg.h>
#include <alpha/mcbus/mcbusvar.h>

#include <alpha/pci/mcpciareg.h>

#include "locators.h"

#define KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))
#define	MCPCIA_EXISTS(mid, gid)	\
	(!badaddr((void *)KV(MCPCIA_BRIDGE_ADDR(gid, mid)), sizeof (uint32_t)))

extern struct cfdriver mcbus_cd;

struct mcbus_cpu_busdep mcbus_primary;

static int	mcbusmatch(device_t, cfdata_t, void *);
static void	mcbusattach(device_t, device_t, void *);
static int	mcbusprint(void *, const char *);
static const char *mcbus_node_type_str(uint8_t);

typedef struct {
	uint8_t	mcbus_types[MCBUS_MID_MAX];
} mcbus_softc_t;

CFATTACH_DECL_NEW(mcbus, sizeof (mcbus_softc_t),
    mcbusmatch, mcbusattach, NULL, NULL);

/*
 * Tru64 UNIX (formerly Digital UNIX (formerly DEC OSF/1)) probes for MCPCIAs
 * in the following order:
 *
 *	5, 4, 7, 6
 *
 * This is so that the built-in CD-ROM on the internal 53c810 is always
 * dka500.  We probe them in the same order, for consistency.
 */
const int mcbus_mcpcia_probe_order[] = { 5, 4, 7, 6 };

extern void mcpcia_config_cleanup(void);

static int
mcbusprint(void *aux, const char *cp)
{
	struct mcbus_dev_attach_args *tap = aux;
	aprint_normal(" mid %d: %s", tap->ma_mid,
	    mcbus_node_type_str(tap->ma_type));
	return (UNCONF);
}

static int
mcbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* Make sure we're looking for a MCBUS. */
	if (strcmp(ma->ma_name, mcbus_cd.cd_name) != 0)
		return (0);

	/*
	 * Only available on 4100 processor type platforms.
	 */
	if (cputype != ST_DEC_4100)
		return (0);
	return (1);
}

static void
mcbusattach(device_t parent, device_t self, void *aux)
{
	static const char * const bcs[CPU_BCacheMask + 1] = {
		"No", "1MB", "2MB", "4MB",
	};
	struct mcbus_dev_attach_args ta;
	mcbus_softc_t *mbp = device_private(self);
	int i, mid;
	int locs[MCBUSCF_NLOCS];

	printf(": %s BCache\n", mcbus_primary.mcbus_valid ?
	    bcs[mcbus_primary.mcbus_bcache] : "Unknown");

	mbp->mcbus_types[0] = MCBUS_TYPE_RES;
	for (mid = 1; mid < MCBUS_MID_MAX; ++mid)
		mbp->mcbus_types[mid] = MCBUS_TYPE_UNK;

	/*
	 * Find and "configure" memory.
	 */

	/*
	 * XXX If we ever support more than one MCBUS, we'll
	 * XXX have to probe for them, and map them to unit
	 * XXX numbers.
	 */
	ta.ma_gid = MCBUS_GID_FROM_INSTANCE(0);
	ta.ma_mid = 1;
	ta.ma_type = MCBUS_TYPE_MEM;
	mbp->mcbus_types[1] = MCBUS_TYPE_MEM;
	locs[MCBUSCF_MID] = 1;
	(void) config_found_sm_loc(self, "mcbus", locs, &ta,
				   mcbusprint, config_stdsubmatch);

	/*
	 * Now find PCI busses.
	 */
	for (i = 0; i < MCPCIA_PER_MCBUS; i++) {
		mid = mcbus_mcpcia_probe_order[i];
		/*
		 * XXX If we ever support more than one MCBUS, we'll
		 * XXX have to probe for them, and map them to unit
		 * XXX numbers.
		 */
		ta.ma_gid = MCBUS_GID_FROM_INSTANCE(0);
		ta.ma_mid = mid;
		ta.ma_type = MCBUS_TYPE_PCI;
		locs[MCBUSCF_MID] = mid;
		if (MCPCIA_EXISTS(ta.ma_mid, ta.ma_gid))
			(void) config_found_sm_loc(self, "mcbus", locs, &ta,
						   mcbusprint,
						   config_stdsubmatch);
	}

#if 0
	/*
	 * Deal with hooking CPU instances to MCBUS module ids.
	 *
	 * Note that we do this here because it's the read of
	 * stupid MCPCIA WHOAMI register that can get us the
	 * module ID and type of the configuring CPU.
	 */

	if (mcbus_primary.mcbus_valid) {
		mid = mcbus_primary.mcbus_cpu_mid;
		printf("%s mid %d: %s %s\n", device_xname(self),
		    mid, mcbus_node_type_str(MCBUS_TYPE_CPU),
		    bcs[mcbus_primary.mcbus_bcache & 0x7]);
		/*
		 * XXX If we ever support more than one MCBUS, we'll
		 * XXX have to probe for them, and map them to unit
		 * XXX numbers.
		 */
		ta.ma_gid = MCBUS_GID_FROM_INSTANCE(0);
		ta.ma_mid = mid;
		ta.ma_type = MCBUS_TYPE_CPU;
		mbp->mcbus_types[mid] = MCBUS_TYPE_CPU;
		locs[MCBUSCF_MID] = mid;
		(void) config_found_sm_loc(self, "mcbus", locs, &ta,
					   mcbusprint, config_stdsubmatch);
	}
#endif

	/*
	 * Now clean up after configuring everything.
	 *
	 * This is an unfortunate layering violation- but
	 * we can't enable interrupts until *all* probing
	 * is done, but the code and knowledge to clean
	 * up after probing and to enable interrupts is
	 * down in the MCPCIA layer.
	 */
	mcpcia_config_cleanup();
}

static const char *
mcbus_node_type_str(uint8_t type)
{
	switch (type) {
	case MCBUS_TYPE_RES:
		panic ("RESERVED TYPE IN MCBUS_NODE_TYPE_STR");
		break;
	case MCBUS_TYPE_UNK:
		panic ("UNKNOWN TYPE IN MCBUS_NODE_TYPE_STR");
		break;
	case MCBUS_TYPE_MEM:
		return ("Memory");
	case MCBUS_TYPE_CPU:
		return ("CPU");
	case MCBUS_TYPE_PCI:
		return ("PCI Bridge");
	default:
		panic("REALLY UNKNWON (%x) TYPE IN MCBUS_NODE_TYPE_STR", type);
		break;
	}
}
