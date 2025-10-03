/* $NetBSD: dec_6600.c,v 1.40 2025/10/03 14:12:03 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996, 1997 Carnegie-Mellon University.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_6600.c,v 1.40 2025/10/03 14:12:03 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/lwp.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/alpha.h>
#include <machine/logout.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#define	DR_VERBOSE(f) while (0)

void dec_6600_init(void);
static void dec_6600_cons_init(void);
static void dec_6600_device_register(device_t, void *);
static void dec_6600_mcheck(unsigned long, struct ev6_logout_area *);
static void dec_6600_mcheck_sys(unsigned int, struct ev6_logout_area *);
static void dec_6600_mcheck_handler(unsigned long, struct trapframe *,
				    unsigned long, unsigned long);

static const struct alpha_variation_table dec_6600_variations[] = {
	{ SV_ST_DP264, "AlphaPC DP264" },
	{ SV_ST_CLIPPER, "AlphaServer ES40 (\"Clipper\")" },
	{ SV_ST_GOLDRUSH, "AlphaServer DS20 (\"GoldRush\")" },
	{ SV_ST_WEBBRICK, "AlphaServer DS10 (\"WebBrick\")" },
	{ SV_ST_SHARK, "AlphaServer DS20L (\"Shark\")" },
	{ 0, NULL },
};

static const struct alpha_variation_table dec_titan_variations[] = {
	{ 0, NULL },
};

void
dec_6600_init(void)
{
	uint64_t variation;

	platform.family = (hwrpb->rpb_type == ST_DEC_TITAN) ? "Titan"
							    : "6600";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		const struct alpha_variation_table *vartab =
		    (hwrpb->rpb_type == ST_DEC_TITAN) ? dec_titan_variations
						      : dec_6600_variations;
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
							   vartab)) == NULL) {
			platform.model = alpha_unknown_sysname();
		}
	}

	platform.iobus = "tsc";
	platform.cons_init = dec_6600_cons_init;
	platform.device_register = dec_6600_device_register;
	platform.mcheck_handler = dec_6600_mcheck_handler;

	/* enable Cchip and Pchip error interrupts */
	STQP(TS_C_DIM0) = 0xe000000000000000;
	STQP(TS_C_DIM1) = 0xe000000000000000;
}

static void
dec_6600_cons_init(void)
{
	struct ctb *ctb;
	uint64_t ctbslot;
	struct tsp_config *tsp;
	bus_space_tag_t isa_iot, isa_memt;

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);
	ctbslot = ctb->ctb_turboslot;

	/* Console hose defaults to hose 0. */
	tsp_console_hose = 0;

	tsp = tsp_init(tsp_console_hose);

	isa_iot = &tsp->pc_iot;
	isa_memt = &tsp->pc_memt;

	switch (ctb->ctb_term_type) {
	case CTB_PRINTERPORT:
		/* serial console ... */
		assert(CTB_TURBOSLOT_HOSE(ctbslot) == 0);
		break;

	case CTB_GRAPHICS:
		/* PCI display might be on a different hose. */
		if (CTB_TURBOSLOT_TYPE(ctbslot) != CTB_TURBOSLOT_TYPE_ISA) {
			tsp_console_hose = CTB_TURBOSLOT_HOSE(ctbslot);
			tsp = tsp_init(tsp_console_hose);
		}
		break;

	default:
		/* Let pci_consinit() handle it. */
		break;
	}

	pci_consinit(&tsp->pc_pc, &tsp->pc_iot, &tsp->pc_memt,
	    isa_iot, isa_memt);
}

static void
dec_6600_device_register(device_t dev, void *aux)
{
	static device_t primarydev;
	struct bootdev_data *b = bootdev_data;
	device_t parent = device_parent(dev);

	/*
	 * First section: Deal with system-specific quirks.
	 */

	if ((hwrpb->rpb_variation & SV_ST_MASK) == SV_ST_WEBBRICK) {
		/*
		 * DMA on the on-board ALI IDE controller is not
		 * working correctly; disable it for now to let
		 * the systems at least hobble along.
		 *
		 * N.B. There's only one Pchip on a DS10, do there
		 * is not need to determine which hose we have here.
		 *
		 * XXX This is meant to be temporary until we can find
		 * XXX and fix the issue with bus-master DMA.
		 */
		if (device_is_a(parent, "pci") && device_is_a(dev, "aceride")) {
			struct pci_attach_args *pa = aux;

			if (pa->pa_bus == 0 && pa->pa_device == 13 &&
			    pa->pa_function == 0) {
				device_setprop_bool(dev,
				    "pciide-disable-dma", true);
			}
		}
	}

	/*
	 * Second section: Boot device detection.
	 */

	if (booted_device != NULL || b == NULL) {
		return;
	}

	if (primarydev == NULL) {
		if (device_is_a(dev, "tsp")) {
			struct tsp_attach_args *tsp = aux;

			if (b->bus == tsp->tsp_slot) {
				primarydev = dev;
				DR_VERBOSE(printf("\nprimarydev = %s\n",
				    device_xname(dev)));
			}
		}
		return;
	}

	pci_find_bootdev(primarydev, dev, aux);
}

static void
dec_6600_mcheck(unsigned long vector, struct ev6_logout_area *la)
{
	const char *t = "Unknown", *c = "";

	if (vector == ALPHA_SYS_ERROR || vector == ALPHA_PROC_ERROR)
		c = " Correctable";

	switch (vector) {
	case ALPHA_SYS_ERROR:
	case ALPHA_SYS_MCHECK:
		t = "System";
		break;

	case ALPHA_PROC_ERROR:
	case ALPHA_PROC_MCHECK:
		t = "Processor";
		break;

	case ALPHA_ENV_MCHECK:
		t = "Environmental";
		break;
	}

	printf("\n%s%s Machine Check (%lx): "
	       "Rev 0x%x, Code 0x%x, Flags 0x%x\n\n",
	       t, c, vector, la->mchk_rev, la->mchk_code, la->la.la_flags);
}

static void
dec_6600_mcheck_sys(unsigned int indent, struct ev6_logout_area *la)
{
	struct ev6_logout_sys *ls =
		(struct ev6_logout_sys *)ALPHA_LOGOUT_SYSTEM_AREA(&la->la);

#define FMT	"%-30s = 0x%016lx\n"

	IPRINTF(indent, FMT, "Software Error Summary Flags", ls->flags);

	IPRINTF(indent, FMT, "CPU Device Interrupt Requests", ls->dir);
	tsc_print_dir(indent + 1, ls->dir);

	IPRINTF(indent, FMT, "Cchip Miscellaneous Register", ls->misc);
	tsc_print_misc(indent + 1, ls->misc);

	IPRINTF(indent, FMT, "Pchip 0 Error Register", ls->p0_error);
	if (ls->flags & 0x5)
		tsp_print_error(indent + 1, ls->p0_error);

	IPRINTF(indent, FMT, "Pchip 1 Error Register", ls->p1_error);
	if (ls->flags & 0x6)
		tsp_print_error(indent + 1, ls->p1_error);
}

static void
dec_6600_mcheck_handler(unsigned long mces, struct trapframe *framep,
			unsigned long vector, unsigned long param)
{
	struct mchkinfo *mcp;
	struct ev6_logout_area *la = (struct ev6_logout_area *)param;

	/*
	 * If we expected a machine check, just go handle it in common code.
	 */
	mcp = &curcpu()->ci_mcinfo;
	if (mcp->mc_expected)
		machine_check(mces, framep, vector, param);

	dec_6600_mcheck(vector, la);

	switch (vector) {
	case ALPHA_SYS_ERROR:
	case ALPHA_SYS_MCHECK:
		dec_6600_mcheck_sys(1, la);
		break;

	}

	machine_check(mces, framep, vector, param);
}
