/*	$NetBSD: tc_subr.c,v 1.26 1999/08/02 12:01:46 mrg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tc_subr.c,v 1.26 1999/08/02 12:01:46 mrg Exp $");


#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>

#define	_PMAX_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/tc/tcvar.h>

#include <pmax/pmax/pmaxtype.h>

#ifndef NULL
#define NULL 0
#endif



/*
 *  TurboChannel autoconfiguration declarations and tables for DECstations.
 */

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/asic.h>

#include <pmax/pmax/turbochannel.h>

#include <machine/fbio.h>
#include <machine/fbvar.h>
#include <pmax/dev/cfbvar.h>
#include <pmax/dev/mfbvar.h>
#include <pmax/dev/sfbvar.h>
#include <pmax/dev/xcfbvar.h>

/*
 * Which system models were configured?
 */
#include "opt_dec_3max.h"
#include "opt_dec_3min.h"
#include "opt_dec_3maxplus.h"
#include "opt_dec_maxine.h"

/*
 * Which TC framebuffers were drivers configured for?
 * Used for configuring a console device.
 */
#include "cfb.h"
#include "mfb.h"
#include "sfb.h"
#include "px.h"
#include "fb.h"
#include "xcfb.h"

/*
 * Tables for table-driven TC and console configuration.
 */
struct fbinfo;
struct tcfbsw {
	const char fbsw_name[TC_ROM_LLEN+1];
	int (*fbsw_initfn) __P((struct fbinfo*, char*, int, int));
};

extern int px_init __P((struct fbinfo*, char*, int, int));

#define tcfbsw_entry(tcname, initfn) { tcname, initfn }

const struct tcfbsw tcfbsw[] = {

#if NMFB > 0
  tcfbsw_entry("PMAG-AA ", mfbinit),
#endif

#if NSFB > 0
  tcfbsw_entry("PMAGB-BA", sfbinit),
#endif

#if NCFB > 0
  tcfbsw_entry("PMAG-BA ", cfbinit),
#endif

#if NPX > 0
  tcfbsw_entry("PMAG-CA ", px_init),
  tcfbsw_entry("PMAG-DA ", px_init),
  tcfbsw_entry("PMAG-FA ", px_init),	/* ?? */
#endif
};
const int ntcfbsw = sizeof(tcfbsw) / sizeof(tcfbsw[0]);


/*
 * Find a tc graphcis console device.
 */
extern int tc_findconsole __P((int prom_slot));


/*
 * Local forward declarations
 */
extern struct tcbus_attach_args *  cpu_tcdesc __P ((int systype));

static int tc_consprobeslot __P((tc_addr_t slotaddr));


/*
 * DECstation tc implementations dont' have a tcasic to handle interrupts,
 * and the mapping to CPU interrupt lines is model-dpeendent.
 * We have to pass TC interrupt establish/disestablish requests up to
 * motherboard-specific code.
 */
void	tc_ds_intr_establish __P((struct device *, void *, tc_intrlevel_t,
				intr_handler_t handler, intr_arg_t arg));
void	tc_ds_intr_disestablish __P((struct device *dev, void *cookie));

bus_dma_tag_t tc_ds_get_dma_tag __P((int));

/* XXX*/
typedef int (*tc_handler_t) __P((void *intr_arg));
extern void (*tc_enable_interrupt)  __P ((u_int slotno, tc_handler_t,
				     void *intr_arg, int on));

/*
 * Map from  systype code to a  tcbus_attach_args struct.
 * XXX make table-driven.
 */
struct tcbus_attach_args *
cpu_tcdesc(cpu)
    int cpu;
{
	switch (cpu) {

#ifdef DEC_3MAXPLUS
	case  DS_3MAXPLUS: {
		extern struct tcbus_attach_args kn03_tc_desc[];

		return &kn03_tc_desc[0];
	}
#endif /* DEC_3MAXPLUS */

#ifdef DEC_3MAX
	case  DS_3MAX: {
		extern struct tcbus_attach_args kn02_tc_desc[];
		return &kn02_tc_desc[0];
	}
#endif /* DEC_3MAX */

#ifdef DEC_3MIN
	case DS_3MIN: {
		extern struct tcbus_attach_args kmin_tc_desc[];
		return &kmin_tc_desc[0];
	}
#endif /*DEC_3MIN*/

#ifdef DEC_MAXINE
	case DS_MAXINE: {
		extern struct tcbus_attach_args xine_tc_desc[];
		return xine_tc_desc;
	}
#endif /*DEC_MAXINE*/

	default:
#ifdef DIAGNOSTIC
		printf("tcattach: systype %d, no turbochannel\n", cpu);
		return NULL;
#endif /*DIAGNOSTIC*/
		panic("cpu_tc: system type 0x%x, no TurboChannel configured\n",
		     cpu);
	}
}


/*
 * We have a TurboChannel bus.  Configure it.
 */
void
config_tcbus(parent, systype, printfn)
     	struct device *parent;
	int systype;
	int	printfn __P((void *, const char *));

{
	struct tcbus_attach_args tcb;

	struct tcbus_attach_args * tcbus = cpu_tcdesc(systype);

	if (tcbus == NULL) {
		printf("no TurboChannel configuration info for this machine\n");
		return;
	}

	/*
	 * Set up important CPU/chipset information.
	 */
	tcb.tba_busname =  tcbus->tba_busname;
	tcb.tba_memt = 0;			/* XXX ignored for now */

	tcb.tba_speed = tcbus->tba_speed;
	tcb.tba_nslots = tcbus->tba_nslots;
	tcb.tba_slots = tcbus->tba_slots;

	tcb.tba_nbuiltins = tcbus->tba_nbuiltins;
	tcb.tba_builtins = tcbus->tba_builtins;
	tcb.tba_intr_establish = tc_ds_intr_establish;
	tcb.tba_intr_disestablish = tc_ds_intr_disestablish;
	tcb.tba_get_dma_tag = tc_ds_get_dma_tag;

	config_found(parent, (struct confargs*)&tcb, printfn);
}

/*
 * Establish an interrupt handler.
 * For both TC and IOCTL asic, we must upcall to  motherboard-specific
 * interrupt-hanlder functions,  in case we need to recompute masks for
 * CPU interrupt lines.
 */
void
tc_ds_intr_establish(dev, cookie, level, handler, val)
    struct device *dev;
    void *cookie;
    tc_intrlevel_t level;
    intr_handler_t handler;
    void *val;
{

#ifdef DIAGNOSTIC
	if (tc_enable_interrupt == NULL)
	    panic("tc_intr_establish: tc_enable not set\n");
#endif

#ifdef DEBUG
	printf("tc_intr_establish: slot %d level %d handler %p sc %p on\n",
		(int) cookie, (int) level, handler,  val);
#endif

	 /*
	  * Enable the interrupt from tc (or ioctl asic) slot with NetBSD/pmax
	  * sw-convention name ``cookie'' on this CPU.
	  * XXX store the level somewhere for selective enabling of
	  * interrupts from TC option slots.
	  */
	 (*tc_enable_interrupt) ((int)cookie, handler, val, 1);
}

void
tc_ds_intr_disestablish(dev, arg)
    struct device *dev;
    void *arg;
{
	/*(*tc_enable_interrupt) (ca->ca_slot, handler, 0);*/
    	printf("cannot dis-establish IOASIC interrupts\n");
}

/*
 * Return the DMA tag for use by the specified TurboChannel slot.
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

/*
 * Console initialization --
 * Called before autoconfiguration, to find a system console.
 */


/*
 * Probe the turbochannel for a framebuffer option card, starting at
 * the preferred slot and then scanning all slots. Configure the first
 * supported framebuffer device found, if any, as the console, and
 * return 1 if found.
 * XXX knows about internals of TurboChannel bus autoconfig descriptor,
 * which needs to be fixed badly.
 */
int
tc_findconsole(preferred_slot)
	int preferred_slot;
{
	int slot;

	struct tcbus_attach_args *tc_desc;

	/* First, try the slot configured as console in NVRAM. */
	 /* if (consprobeslot(preferred_slot)) return (1); */

	 if ((tc_desc = cpu_tcdesc(systype)) == NULL)
		return 0;

	/*
	 * Try to configure each turbochannel option or builtin framebuffer.
	 */
	for (slot = 0; slot < tc_desc->tba_nslots; slot++) {
		if (tc_consprobeslot(tc_desc->tba_slots[slot].tcs_addr))
			return (1);
	}
	return (0);
}


/*
 * Look in a single TC option slot to see if it contains a possible
 * framebuffer console device.
 *
 * Configure only the framebuffers for which driver are configured
 * into the kernel.  If a suitable framebuffer is found, initialize
 * it, and set up glass-tty emulation.
 */
static int
tc_consprobeslot(tc_slotaddr)
	tc_addr_t tc_slotaddr;
{
#if NCFB > 0 || NFB > 0 || NMFB > 0 || NSFB > 0 || NXCFB > 0 || NPX > 0
	void *slotaddr = (void *) tc_slotaddr;
	struct fbinfo *fi;
	char name[20];
	int i;

	/*
	 * check specified TC slot address
	 */
	if (tc_badaddr(slotaddr))
		return (0);

	/* check slot matches name */
	if (tc_checkslot(tc_slotaddr, name) == 0)
		return (0);

	if (fballoc((caddr_t)tc_slotaddr, &fi))
		return (0);

	/*
	 * We found an device in the given slot. Now see if it's a
	 * framebuffer for which we have a driver.
	 */
	for (i = 0; i < ntcfbsw; i++) {
		if (tcfbsw[i].fbsw_initfn == 0)
			break;
		if (strcmp(name, tcfbsw[i].fbsw_name) == 0) {
			if (tcfbsw[i].fbsw_initfn(fi, slotaddr, 0, 1))
				return (1);
		}
	}
#endif

	return (0);
}
