/*	$NetBSD: cpu.c,v 1.1 1995/02/13 23:06:55 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

/* Definition of the driver for autoconfig. */
static int	cpumatch(struct device *, void *, void *);
static void	cpuattach(struct device *, struct device *, void *);
struct cfdriver cpucd =
    { NULL, "cpu", cpumatch, cpuattach, DV_DULL, sizeof (struct device) };

static int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	void *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;
	struct confargs *ca = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ca->ca_name, cpucd.cd_name) != 0)
		return (0);

	return (1);
}

static void
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{
        struct pcs *p;

        p = (struct pcs*)((char *)hwrpb + hwrpb->rpb_pcs_off +
	    (dev->dv_unit * hwrpb->rpb_pcs_size));
	printf(": 0x%lx 0x%lx, type 0x%lx, var 0x%lx, rev 0x%lx\n",
	    (long)(p->pcs_proc_sn[0]), (long)(p->pcs_proc_sn[8]), 
	    (long)p->pcs_proc_type, *((long *)&p->pcs_proc_var),
	    *((long *)p->pcs_proc_revision));
}
