/* $NetBSD: mcbus.c,v 1.6.8.1 1999/12/27 18:31:26 wrstuden Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mcbus.c,v 1.6.8.1 1999/12/27 18:31:26 wrstuden Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/mcbus/mcbusreg.h>
#include <alpha/mcbus/mcbusvar.h>

#include "locators.h"

#define KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))
#define	MCPCIA_EXISTS(mid, gid)	\
	(!badaddr((void *)KV(MCPCIA_BRIDGE_ADDR(gid, mid)), sizeof (u_int32_t)))

extern struct cfdriver mcbus_cd;

struct mcbus_cpu_busdep mcbus_primary;

static int	mcbusmatch __P((struct device *, struct cfdata *, void *));
static void	mcbusattach __P((struct device *, struct device *, void *));
static int	mcbusprint __P((void *, const char *));
static int	mcbussbm __P((struct device *, struct cfdata *, void *));
static char	*mcbus_node_type_str __P((u_int8_t));

typedef struct {
	struct device	mcbus_dev;
	u_int8_t	mcbus_types[MCBUS_MID_MAX];
} mcbus_softc_t;

struct cfattach mcbus_ca = {
	sizeof (mcbus_softc_t), mcbusmatch, mcbusattach
};

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

extern void mcpcia_config_cleanup __P((void));

static int
mcbusprint(aux, cp)
	void *aux;
	const char *cp;
{
	struct mcbus_dev_attach_args *tap = aux;
	printf(" mid %d: %s", tap->ma_mid, mcbus_node_type_str(tap->ma_type));
	return (UNCONF);
}

static int
mcbussbm(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mcbus_dev_attach_args *tap = aux;
	if (tap->ma_name != mcbus_cd.cd_name)
		return (0);
	if (cf->cf_loc[MCBUSCF_MID] != MCBUSCF_MID_DEFAULT &&
	    cf->cf_loc[MCBUSCF_MID] != tap->ma_mid)
		return (0);
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

static int
mcbusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
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
mcbusattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	static const char *bcs[8] = {
		"(no bcache)", "1MB BCache", "2MB BCache", "4MB BCache",
		"??(4)??", "??(5)??", "??(6)??", "??(7)??"
	};
	struct mcbus_dev_attach_args ta;
	mcbus_softc_t *mbp = (mcbus_softc_t *)self;
	int i, mid;

	printf("\n");

	mbp->mcbus_types[0] = MCBUS_TYPE_RES;
	for (mid = 1; mid <= MCBUS_MID_MAX; ++mid) {
		mbp->mcbus_types[mid] = MCBUS_TYPE_UNK;
	}

	/*
	 * Find and "configure" memory.
	 */
	ta.ma_name = mcbus_cd.cd_name;
	/*
	 * XXX If we ever support more than one MCBUS, we'll
	 * XXX have to probe for them, and map them to unit
	 * XXX numbers.
	 */
	ta.ma_gid = MCBUS_GID_FROM_INSTANCE(0);
	ta.ma_mid = 1;
	ta.ma_type = MCBUS_TYPE_MEM;
	mbp->mcbus_types[1] = MCBUS_TYPE_MEM;
	(void) config_found_sm(self, &ta, mcbusprint, mcbussbm);

	/*
	 * Now find PCI busses.
	 */
	for (i = 0; i < MCPCIA_PER_MCBUS; i++) {
		mid = mcbus_mcpcia_probe_order[i];
		ta.ma_name = mcbus_cd.cd_name;
		/*
		 * XXX If we ever support more than one MCBUS, we'll
		 * XXX have to probe for them, and map them to unit
		 * XXX numbers.
		 */
		ta.ma_gid = MCBUS_GID_FROM_INSTANCE(0);
		ta.ma_mid = mid;
		ta.ma_type = MCBUS_TYPE_PCI;
		if (MCPCIA_EXISTS(ta.ma_mid, ta.ma_gid)) {
			(void) config_found_sm(self, &ta, mcbusprint, mcbussbm);
		}
	}

	/*
	 * Deal with hooking CPU instances to MCBUS module ids.
	 *
	 * Note that we do this here because it's the read of
	 * stupid MCPCIA WHOAMI register that can get us the
	 * module ID and type of the configuring CPU.
	 */

	if (mcbus_primary.mcbus_valid) {
		mid = mcbus_primary.mcbus_cpu_mid;
		printf("%s mid %d: %s %s\n", self->dv_xname,
		    mid, mcbus_node_type_str(MCBUS_TYPE_CPU),
		    bcs[mcbus_primary.mcbus_bcache & 0x7]);
#if 0
		ta.ma_name = mcbus_cd.cd_name;
		/*
		 * XXX If we ever support more than one MCBUS, we'll
		 * XXX have to probe for them, and map them to unit
		 * XXX numbers.
		 */
		ta.ma_gid = MCBUS_GID_FROM_INSTANCE(0);
		ta.ma_mid = mid;
		ta.ma_type = MCBUS_TYPE_CPU;
		mbp->mcbus_types[mid] = MCBUS_TYPE_CPU;
		(void) config_found_sm(self, &ta, mcbusprint, mcbussbm);
#endif
	}

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

static char *
mcbus_node_type_str(type)
	u_int8_t type;
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
