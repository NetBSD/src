/* $NetBSD: tlsb.c,v 1.31.26.1 2007/03/12 05:46:15 rmind Exp $ */
/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Based in part upon a prototype version by Jason Thorpe
 * Copyright (c) 1996, 1998 by Jason Thorpe.
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
 * Autoconfiguration and support routines for the TurboLaser System Bus
 * found on AlphaServer 8200 and 8400 systems.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tlsb.c,v 1.31.26.1 2007/03/12 05:46:15 rmind Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/cpuvar.h>
#include <machine/rpb.h>
#include <machine/pte.h>
#include <machine/alpha.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>

#include "locators.h"

#define KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))

static int	tlsbmatch __P((struct device *, struct cfdata *, void *));
static void	tlsbattach __P((struct device *, struct device *, void *));

CFATTACH_DECL(tlsb, sizeof (struct device),
    tlsbmatch, tlsbattach, NULL, NULL);

extern struct cfdriver tlsb_cd;

static int	tlsbprint __P((void *, const char *));
static const char *tlsb_node_type_str __P((u_int32_t));

/*
 * There can be only one TurboLaser, and we'll overload it
 * with a bitmap of found turbo laser nodes. Note that
 * these are just the actual hard TL node IDS that we
 * discover here, not the virtual IDs that get assigned
 * to CPUs. During TLSB specific error handling we
 * only need to know which actual TLSB slots have boards
 * in them (irrespective of how many CPUs they have).
 */
int	tlsb_found;

static int
tlsbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct tlsb_dev_attach_args *tap = aux;

	if (pnp)
		aprint_normal("%s at %s node %d",
		    tlsb_node_type_str(tap->ta_dtype), pnp, tap->ta_node);
	else
		aprint_normal(" node %d: %s", tap->ta_node,
		    tlsb_node_type_str(tap->ta_dtype));

	return (UNCONF);
}

static int
tlsbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/* Make sure we're looking for a TurboLaser. */
	if (strcmp(ma->ma_name, tlsb_cd.cd_name) != 0)
		return (0);

	/*
	 * Only one instance of TurboLaser allowed,
	 * and only available on 21000 processor type
	 * platforms.
	 */
	if ((cputype != ST_DEC_21000) || tlsb_found)
		return (0);

	return (1);
}

static void
tlsbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tlsb_dev_attach_args ta;
	u_int32_t tldev;
	int node;
	int locs[TLSBCF_NLOCS];

	printf("\n");

	/*
	 * Attempt to find all devices on the bus, including
	 * CPUs, memory modules, and I/O modules.
	 */

	/*
	 * Sigh. I would like to just start off nicely,
	 * but I need to treat I/O modules differently-
	 * The highest priority I/O node has to be in
	 * node #8, and I want to find it *first*, since
	 * it will have the primary disks (most likely)
	 * on it.
	 */
	for (node = 0; node <= TLSB_NODE_MAX; ++node) {
		/*
		 * Check for invalid address.  This may not really
		 * be necessary, but what the heck...
		 */
		if (badaddr(TLSB_NODE_REG_ADDR(node, TLDEV), sizeof(u_int32_t)))
			continue;
		tldev = TLSB_GET_NODEREG(node, TLDEV);
		if (tldev == 0) {
			/* Nothing at this node. */
			continue;
		}
		/*
		 * Store up that we found something at this node.
		 * We do this so that we don't have to do something
		 * silly at fault time like try a 'baddadr'...
		 */
		tlsb_found |= (1 << node);
		if (TLDEV_ISIOPORT(tldev))
			continue;	/* not interested right now */
		ta.ta_node = node;
		ta.ta_dtype = TLDEV_DTYPE(tldev);
		ta.ta_swrev = TLDEV_SWREV(tldev);
		ta.ta_hwrev = TLDEV_HWREV(tldev);

		/*
		 * Deal with hooking CPU instances to TurboLaser nodes.
		 */
		if (TLDEV_ISCPU(tldev)) {
			printf("%s node %d: %s\n", self->dv_xname,
			    node, tlsb_node_type_str(tldev));
		}
		/*
		 * Attach any children nodes, including a CPU's GBus
		 */
		locs[TLSBCF_NODE] = node;
		locs[TLSBCF_OFFSET] = 0; /* XXX unused? */

		config_found_sm_loc(self, "tlsb", locs, &ta,
				    tlsbprint, config_stdsubmatch);
	}
	/*
	 * *Now* search for I/O nodes (in descending order)
	 */
	while (--node > 0) {
		if (badaddr(TLSB_NODE_REG_ADDR(node, TLDEV), sizeof(u_int32_t)))
			continue;
		tldev = TLSB_GET_NODEREG(node, TLDEV);
		if (tldev == 0) {
			continue;
		}
		if (TLDEV_ISIOPORT(tldev)) {
#if defined(MULTIPROCESSOR)
			/*
			 * XXX Eventually, we want to select a secondary
			 * XXX processor on which to field interrupts for
			 * XXX this node.  However, we just send them to
			 * XXX the primary CPU for now.
			 *
			 * XXX Maybe multiple CPUs?  Does the hardware
			 * XXX round-robin, or check the length of the
			 * XXX per-CPU interrupt queue?
			 */
			printf("%s node %d: routing interrupts to %s\n",
			  self->dv_xname, node,
			  cpu_info[hwrpb->rpb_primary_cpu_id]->ci_softc->sc_dev.dv_xname);
			TLSB_PUT_NODEREG(node, TLCPUMASK,
			    (1UL << hwrpb->rpb_primary_cpu_id));
#else
			/*
			 * Make sure interrupts are sent to the primary CPU.
			 */
			TLSB_PUT_NODEREG(node, TLCPUMASK,
			    (1UL << hwrpb->rpb_primary_cpu_id));
#endif /* MULTIPROCESSOR */

			ta.ta_node = node;
			ta.ta_dtype = TLDEV_DTYPE(tldev);
			ta.ta_swrev = TLDEV_SWREV(tldev);
			ta.ta_hwrev = TLDEV_HWREV(tldev);

			locs[TLSBCF_NODE] = node;
			locs[TLSBCF_OFFSET] = 0; /* XXX unused? */

			config_found_sm_loc(self, "tlsb", locs, &ta,
					    tlsbprint, config_stdsubmatch);
		}
	}
}

static const char *
tlsb_node_type_str(dtype)
	u_int32_t dtype;
{
	static char	tlsb_line[64];

	switch (dtype & TLDEV_DTYPE_MASK) {
	case TLDEV_DTYPE_KFTHA:
		return ("KFTHA I/O interface");

	case TLDEV_DTYPE_KFTIA:
		return ("KFTIA I/O interface");

	case TLDEV_DTYPE_MS7CC:
		return ("MS7CC Memory Module");

	case TLDEV_DTYPE_SCPU4:
		return ("Single CPU, 4MB cache");

	case TLDEV_DTYPE_SCPU16:
		return ("Single CPU, 16MB cache");

	case TLDEV_DTYPE_DCPU4:
		return ("Dual CPU, 4MB cache");

	case TLDEV_DTYPE_DCPU16:
		return ("Dual CPU, 16MB cache");

	default:
		memset(tlsb_line, 0, sizeof(tlsb_line));
		sprintf(tlsb_line, "unknown, dtype 0x%x", dtype);
		return (tlsb_line);
	}
	/* NOTREACHED */
}
