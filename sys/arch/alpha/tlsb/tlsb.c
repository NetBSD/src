/* $NetBSD: tlsb.c,v 1.5 1997/07/17 01:27:22 jtk Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Based in part upon a prototype version by Jason Thorpe
 * Copyright (c) 1996 by Jason Thorpe.
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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tlsb.c,v 1.5 1997/07/17 01:27:22 jtk Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>

#include "locators.h"

extern int	cputype;
int		cpu_node_id;
extern struct cfdriver cpu_cd;

#define KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))

static int	tlsbmatch __P((struct device *, struct cfdata *, void *));
static void	tlsbattach __P((struct device *, struct device *, void *));
struct cfattach tlsb_ca = {
	sizeof (struct device), tlsbmatch, tlsbattach
};

struct cfdriver	tlsb_cd = {
	NULL, "tlsb", DV_DULL,
};

static int	tlsbprint __P((void *, const char *));
static int	tlsbsubmatch __P((struct device *, struct cfdata *, void *));
static char	*tlsb_node_type_str __P((u_int32_t));

static int
tlsbprint(aux, cp)
	void *aux;
	const char *cp;
{
	struct tlsb_dev_attach_args *tap = aux;
	printf(" node %d: %s", tap->ta_node,
	    tlsb_node_type_str(tap->ta_dtype));
	return (UNCONF);
}

static int
tlsbsubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct tlsb_dev_attach_args *tap = aux;
	if (tap->ta_name != tlsb_cd.cd_name)
		return (0);
	if (cf->cf_loc[TLSBCF_NODE] != TLSBCF_NODE_DEFAULT &&
	    cf->cf_loc[TLSBCF_NODE] != tap->ta_node)
		return (0);
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

static int
tlsbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	/* Make sure we're looking for a TurboLaser. */
	if (strcmp(ca->ca_name, tlsb_cd.cd_name) != 0)
		return (0);

	/*
	 * Only one instance of TurboLaser allowed,
	 * and only available on 21000 processor type
	 * platforms.
	 */
	if ((cputype != ST_DEC_21000) || (cf->cf_unit != 0))
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
	u_int8_t vid;
	int node;
	struct tlsb_cpu_busdep *tcpu;
	struct cfdriver *cfd = &cpu_cd;

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
		if (TLDEV_ISIOPORT(tldev))
			continue;	/* not interested right now */
		ta.ta_name = tlsb_cd.cd_name;
		ta.ta_node = node;
		ta.ta_dtype = TLDEV_DTYPE(tldev);
		ta.ta_swrev = TLDEV_SWREV(tldev);
		ta.ta_hwrev = TLDEV_HWREV(tldev);

		/*
		 * Deal with hooking CPU instances to TurboLaser nodes.
		 */
		if (TLDEV_ISCPU(tldev)) {
			printf("%s node %d: %s", self->dv_xname,
			    node, tlsb_node_type_str(tldev));

			/*
			 * Hook in the first CPU unit.
			 */
			vid = (TLSB_GET_NODEREG(node, TLVID) &
			    TLVID_VIDA_MASK) >> TLVID_VIDA_SHIFT;

			if ((tcpu = malloc(sizeof(struct tlsb_cpu_busdep),
			    M_DEVBUF, M_NOWAIT)) == NULL)
				panic("\nno memory for cpu busdep info");

			tcpu->tcpu_vid = vid;
			tcpu->tcpu_node = node;
			cpu_node_id = node;
			printf(", VID %d -> %s", tcpu->tcpu_vid, cfd->cd_name);

			/*
			 * Do 2nd CPU (if available) here.
			 */
			printf("\n");
			TLSB_PUT_NODEREG(node, TLCPUMASK, (1<<vid));

			/*
			 * XXX: Check to make sure that INTRMASK has ints
			 * XXX: enabled for this CPU.
			 */
		}
		/*
		 * Attach any  children nodes, including a CPU's GBus
		 */
		config_found_sm(self, &ta, tlsbprint, tlsbsubmatch);
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
			ta.ta_name = tlsb_cd.cd_name;
			ta.ta_node = node;
			ta.ta_dtype = TLDEV_DTYPE(tldev);
			ta.ta_swrev = TLDEV_SWREV(tldev);
			ta.ta_hwrev = TLDEV_HWREV(tldev);
			config_found_sm(self, &ta, tlsbprint, tlsbsubmatch);
		}
	}
}

static char *
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
		bzero(tlsb_line, sizeof(tlsb_line));
		sprintf(tlsb_line, "unknown, dtype 0x%x", dtype);
		return (tlsb_line);
	}
	/* NOTREACHED */
}
