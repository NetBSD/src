/* $NetBSD: tcasic.c,v 1.17.2.2 1997/06/06 02:41:17 thorpej Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tcasic.c,v 1.17.2.2 1997/06/06 02:41:17 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/tc/tcvar.h>
#include <alpha/tc/tc_conf.h>

/* Definition of the driver for autoconfig. */
int	tcasicmatch(struct device *, struct cfdata *, void *);
void	tcasicattach(struct device *, struct device *, void *);

struct cfattach tcasic_ca = {
	sizeof (struct device), tcasicmatch, tcasicattach,
};

struct cfdriver tcasic_cd = {
	NULL, "tcasic", DV_DULL,
};

int	tcasicprint __P((void *, const char *));

extern int cputype;

/* There can be only one. */
int	tcasicfound;

int
tcasicmatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct confargs *ca = aux;

        /* Make sure that we're looking for a TurboChannel ASIC. */
        if (strcmp(ca->ca_name, tcasic_cd.cd_name))
                return (0);

        /* Make sure that the system supports a TurboChannel ASIC. */
	if ((cputype != ST_DEC_3000_500) && (cputype != ST_DEC_3000_300))
		return (0);

	if (tcasicfound)
		return (0);

	return (1);
}

void
tcasicattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tcbus_attach_args tba;
	void (*intr_setup) __P((void));
	void (*iointr) __P((void *, unsigned long));

	printf("\n");
	tcasicfound = 1;

	switch (cputype) {
#ifdef DEC_3000_500
	case ST_DEC_3000_500:

		intr_setup = tc_3000_500_intr_setup;
		iointr = tc_3000_500_iointr;

		tc_dma_get_tag_func = tc_dma_get_tag_3000_500;

		tba.tba_busname = "tc";
		tba.tba_speed = TC_SPEED_25_MHZ;
		tba.tba_nslots = tc_3000_500_nslots;
		tba.tba_slots = tc_3000_500_slots;
		if (hwrpb->rpb_variation & SV_GRAPHICS) {
			tba.tba_nbuiltins = tc_3000_500_graphics_nbuiltins;
			tba.tba_builtins = tc_3000_500_graphics_builtins;
		} else {
			tba.tba_nbuiltins = tc_3000_500_nographics_nbuiltins;
			tba.tba_builtins = tc_3000_500_nographics_builtins;
		}
		tba.tba_intr_establish = tc_3000_500_intr_establish;
		tba.tba_intr_disestablish = tc_3000_500_intr_disestablish;

		/* Do 3000/500-specific DMA setup now. */
		tc_dma_init_3000_500(tc_3000_500_nslots);
		break;
#endif /* DEC_3000_500 */

#ifdef DEC_3000_300
	case ST_DEC_3000_300:

		intr_setup = tc_3000_300_intr_setup;
		iointr = tc_3000_300_iointr;

		tc_dma_get_tag_func = tc_dma_get_tag_3000_300;

		tba.tba_busname = "tc";
		tba.tba_speed = TC_SPEED_12_5_MHZ;
		tba.tba_nslots = tc_3000_300_nslots;
		tba.tba_slots = tc_3000_300_slots;
		tba.tba_nbuiltins = tc_3000_300_nbuiltins;
		tba.tba_builtins = tc_3000_300_builtins;
		tba.tba_intr_establish = tc_3000_300_intr_establish;
		tba.tba_intr_disestablish = tc_3000_300_intr_disestablish;
		break;
#endif /* DEC_3000_300 */

	default:
		panic("tcasicattach: bad cputype");
	}

	tba.tba_memt = tc_bus_mem_init(NULL);
	
	tc_dma_init();

	(*intr_setup)();
	set_iointr(iointr);

	config_found(self, &tba, tcasicprint);
}

int
tcasicprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	/* only TCs can attach to tcasics; easy. */
	if (pnp)
		printf("tc at %s", pnp);
	return (UNCONF);
}
