/*	$NetBSD: tc_subr.c,v 1.21.4.2 1999/03/15 03:58:28 nisimura Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: tc_subr.c,v 1.21.4.2 1999/03/15 03:58:28 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>

#define	_PMAX_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/tc/tcvar.h>
#include <pmax/pmax/pmaxtype.h>

/*
 * Which system models were configured?
 */
#include "opt_dec_3max.h"
#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

bus_dma_tag_t tc_ds_get_dma_tag __P((int));

extern struct tcbus_attach_args kn02_tc_desc[];
extern struct tcbus_attach_args kmin_tc_desc[];
extern struct tcbus_attach_args xine_tc_desc[];
extern struct tcbus_attach_args kn03_tc_desc[];

/*
 * We have a TURBOchannel bus.  Configure it.
 */
void
config_tcbus(parent, cpu, printfn)
     	struct device *parent;
	int cpu;
	int printfn __P((void *, const char *));

{
	struct tcbus_attach_args tba;
	struct tcbus_attach_args *tcbus;

	switch (cpu /* systype */) {
#ifdef DEC_3MAX
	case DS_3MAX:
		tcbus = &kn02_tc_desc[0]; break;
#endif
#ifdef DEC_3MIN
	case DS_3MIN:
		tcbus = &kmin_tc_desc[0]; break;
#endif
#ifdef DEC_MAXINE
	case DS_MAXINE:
		tcbus = &xine_tc_desc[0]; break;
#endif
#ifdef DEC_3MAXPLUS
	case DS_3MAXPLUS:
		tcbus = &kn03_tc_desc[0]; break;
#endif
	default:
		panic("config_tcbus: no TURBOchannel configured for systype %d",
		systype);
	}

	tba.tba_busname = "tc";
	tba.tba_memt = 0;

	tba.tba_speed = tcbus->tba_speed;
	tba.tba_nslots = tcbus->tba_nslots;
	tba.tba_slots = tcbus->tba_slots;

	tba.tba_nbuiltins = tcbus->tba_nbuiltins;
	tba.tba_builtins = tcbus->tba_builtins;
	tba.tba_intr_establish = tcbus->tba_intr_establish;
	tba.tba_intr_disestablish = tcbus->tba_intr_disestablish;
	tba.tba_get_dma_tag = tc_ds_get_dma_tag;

	config_found(parent, &tba, printfn);
}

/*
 * Return the DMA tag for use by the specified TURBOchannel slot.
 */
bus_dma_tag_t
tc_ds_get_dma_tag(slot)
	int slot;
{
	/*
	 * All DECstations use the default DMA tag.
	 */
	return (&pmax_default_bus_dma_tag);
}

#include "wsdisplay.h"

#if NWSDISPLAY > 0

#include "mfb.h"
#include "cfb.h"
#include "sfb.h"
#include "tfb.h"

#include <pmax/stand/dec_prom.h>

int tc_fb_cnattach __P((int));

int
tc_fb_cnattach(slotno)
	int slotno;
{
	tc_addr_t tcaddr;
	char tcname[TC_ROM_LLEN];
	extern void sfb_cnattach __P((tc_addr_t));
	extern void cfb_cnattach __P((tc_addr_t));
	extern void mfb_cnattach __P((tc_addr_t));
	extern void tfb_cnattach __P((tc_addr_t));

	tcaddr = (*callv->_slot_address)(slotno);
	if (tc_badaddr(tcaddr) || tc_checkslot(tcaddr, tcname) == 0)
		panic("TC console designed by PROM does not exist!?");

#if NSFB > 0
	if (strncmp("PMAGB-BA", tcname, TC_ROM_LLEN) == 0) {
		sfb_cnattach(tcaddr);
		return 1;
	}
#endif
#if NCFB > 0
	if (strncmp("PMAG-BA ", tcname, TC_ROM_LLEN) == 0) {
		cfb_cnattach(tcaddr);
		return 1;
	}
#endif
#if NMFB > 0
	if (strncmp("PMAG-AA ", tcname, TC_ROM_LLEN) == 0) {
		mfb_cnattach(tcaddr);
		return 1;
	}
#endif
#if NTFB > 0
	if (strncmp("PMAG-RO ", tcname, TC_ROM_LLEN) == 0
	    || strncmp("PMAG-JA ", tcname, TC_ROM_LLEN) == 0) {
		tfb_cnattach(tcaddr);
		return 1;
	}
#endif
	return 0;
}

#endif
