/* $NetBSD: dec_6600.c,v 1.29.2.1 2010/10/22 07:20:55 uebayasi Exp $ */

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

#include "opt_kgdb.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_6600.c,v 1.29.2.1 2010/10/22 07:20:55 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/bus.h>
#include <machine/alpha.h>
#include <machine/logout.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ata/atavar.h>

#include <dev/ic/mlxio.h>
#include <dev/ic/mlxvar.h>

#include <dev/i2o/i2o.h>
#include <dev/i2o/iopio.h>
#include <dev/i2o/iopvar.h>

#include "pckbd.h"

#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif

#define	DR_VERBOSE(f) while (0)

static int comcnrate __attribute__((unused)) = CONSPEED;

void dec_6600_init(void);
static void dec_6600_cons_init(void);
static void dec_6600_device_register(struct device *, void *);
static void dec_6600_mcheck(unsigned long, struct ev6_logout_area *);
static void dec_6600_mcheck_sys(unsigned int, struct ev6_logout_area *);
static void dec_6600_mcheck_handler(unsigned long, struct trapframe *,
				    unsigned long, unsigned long);

#ifdef KGDB
#include <machine/db_machdep.h>

static const char *kgdb_devlist[] = {
	"com",
	NULL,
};
#endif /* KGDB */

void
dec_6600_init()
{

	platform.family = "6600";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		/* XXX Don't know the system variations, yet. */
		platform.model = alpha_unknown_sysname();
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
dec_6600_cons_init()
{
	struct ctb *ctb;
	u_int64_t ctbslot;
	struct tsp_config *tsp;

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);
	ctbslot = ctb->ctb_turboslot;

	/* Console hose defaults to hose 0. */
	tsp_console_hose = 0;

	tsp = tsp_init(0, tsp_console_hose);

	switch (ctb->ctb_term_type) {
	case CTB_PRINTERPORT: 
		/* serial console ... */
		assert(CTB_TURBOSLOT_HOSE(ctbslot) == 0);
		/* XXX */
		{
			/*
			 * Delay to allow PROM putchars to complete.
			 * FIFO depth * character time,
			 * character time = (1000000 / (defaultrate / 10))
			 */
			DELAY(160000000 / comcnrate);

			if(comcnattach(&tsp->pc_iot, 0x3f8, comcnrate,
			    COM_FREQ, COM_TYPE_NORMAL,
			    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8))
				panic("can't init serial console");

			break;
		}

	case CTB_GRAPHICS:
#if NPCKBD > 0
		/* display console ... */
		/* XXX */
		(void) pckbc_cnattach(&tsp->pc_iot, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);

		if (CTB_TURBOSLOT_TYPE(ctbslot) ==
		    CTB_TURBOSLOT_TYPE_ISA)
			isa_display_console(&tsp->pc_iot, &tsp->pc_memt);
		else {
			/* The display PCI might be different */
			tsp_console_hose = CTB_TURBOSLOT_HOSE(ctbslot);
			tsp = tsp_init(0, tsp_console_hose);
			pci_display_console(&tsp->pc_iot, &tsp->pc_memt,
			    &tsp->pc_pc, CTB_TURBOSLOT_BUS(ctbslot),
			    CTB_TURBOSLOT_SLOT(ctbslot), 0);
		}
#else
		panic("not configured to use display && keyboard console");
#endif
		break;

	default:
		printf("ctb_term_type = 0x%lx ctb_turboslot = 0x%lx"
		    " hose = %ld\n", ctb->ctb_term_type, ctbslot,
		    CTB_TURBOSLOT_HOSE(ctbslot));

		panic("consinit: unknown console type %ld",
		    ctb->ctb_term_type);
	}
#ifdef KGDB
	/* Attach the KGDB device. */
	alpha_kgdb_init(kgdb_devlist, &tsp->pc_iot);
#endif /* KGDB */
}

static void
dec_6600_device_register(struct device *dev, void *aux)
{
	static int found, initted, diskboot, netboot;
	static struct device *primarydev, *pcidev, *ctrlrdev;
	struct bootdev_data *b = bootdev_data;
	struct device *parent = device_parent(dev);

	if (found)
		return;

	if (!initted) {
		diskboot = (strcasecmp(b->protocol, "SCSI") == 0) ||
		    (strcasecmp(b->protocol, "RAID") == 0) ||
		    (strcasecmp(b->protocol, "I2O") == 0) ||
		    (strcasecmp(b->protocol, "IDE") == 0);
		netboot = (strcasecmp(b->protocol, "BOOTP") == 0) ||
		    (strcasecmp(b->protocol, "MOP") == 0);
		DR_VERBOSE(printf("diskboot = %d, netboot = %d\n", diskboot,
		    netboot));
		initted = 1;
	}

	if (primarydev == NULL) {
		if (!device_is_a(dev, "tsp"))
			return;
		else {
			struct tsp_attach_args *tsp = aux;

			if (b->bus != tsp->tsp_slot)
				return;
			primarydev = dev;
			DR_VERBOSE(printf("\nprimarydev = %s\n",
			    dev->dv_xname));
			return;
		}
	}

	if (pcidev == NULL) {
		if (!device_is_a(dev, "pci"))
			return;
		/*
		 * Try to find primarydev anywhere in the ancestry.  This is
		 * necessary if the PCI bus is hidden behind a bridge.
		 */
		while (parent) {
			if (parent == primarydev)
				break;
			parent = device_parent(parent);
		}
		if (!parent)
			return;
		else {
			struct pcibus_attach_args *pba = aux;

			if ((b->slot / 1000) != pba->pba_bus)
				return;
	
			pcidev = dev;
			DR_VERBOSE(printf("\npcidev = %s\n", dev->dv_xname));
			return;
		}
	}

	if (ctrlrdev == NULL) {
		if (parent != pcidev)
			return;
		else {
			struct pci_attach_args *pa = aux;
			int slot;

			slot = pa->pa_bus * 1000 + pa->pa_function * 100 +
			    pa->pa_device;
			if (b->slot != slot)
				return;
	
			if (netboot) {
				booted_device = dev;
				DR_VERBOSE(printf("\nbooted_device = %s\n",
				    dev->dv_xname));
				found = 1;
			} else {
				ctrlrdev = dev;
				DR_VERBOSE(printf("\nctrlrdev = %s\n",
				    dev->dv_xname));
			}
			return;
		}
	}

	if (!diskboot)
		return;

	if (device_is_a(dev, "sd") ||
	    device_is_a(dev, "st") ||
	    device_is_a(dev, "cd")) {
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;
		int unit;

		if (device_parent(parent) != ctrlrdev)
			return;

		unit = periph->periph_target * 100 + periph->periph_lun;
		if (b->unit != unit)
			return;
		if (b->channel != periph->periph_channel->chan_channel)
			return;

		/* we've found it! */
		booted_device = dev;
		DR_VERBOSE(printf("\nbooted_device = %s\n", dev->dv_xname));
		found = 1;
	}

	if (device_is_a(dev, "ld") && device_is_a(parent, "iop")) {
		/*
		 * Argh!  The attach arguments for ld devices is not
		 * consistent, so each supported raid controller requires
		 * different checks.
		 */
		struct iop_attach_args *iopa = aux;

		if (parent != ctrlrdev)
			return;

		if (b->unit != iopa->ia_tid)
			return;
		/* we've found it! */
		booted_device = dev;
		DR_VERBOSE(printf("\nbooted_device = %s\n", dev->dv_xname));
		found = 1;
	}

	if (device_is_a(dev, "ld") && device_is_a(parent, "mlx")) {
		/*
		 * Argh!  The attach arguments for ld devices is not
		 * consistent, so each supported raid controller requires
		 * different checks.
		 */
		struct mlx_attach_args *mlxa = aux;

		if (parent != ctrlrdev)
			return;

		if (b->unit != mlxa->mlxa_unit)
			return;
		/* we've found it! */
		booted_device = dev;
		DR_VERBOSE(printf("\nbooted_device = %s\n", dev->dv_xname));
		found = 1;
	}

	/*
	 * Support to boot from IDE drives.
	 */
	if (device_is_a(dev, "wd")) {
		struct ata_device *adev = aux;

		if (!device_is_a(parent, "atabus"))
			return;
		if (device_parent(parent) != ctrlrdev)
			return;

		DR_VERBOSE(printf("\nAtapi info: drive: %d, channel %d\n",
		    adev->adev_drv_data->drive, adev->adev_channel));
		DR_VERBOSE(printf("Bootdev info: unit: %d, channel: %d\n",
		    b->unit, b->channel));
		if (b->unit != adev->adev_drv_data->drive ||
		    b->channel != adev->adev_channel)
			return;

		/* we've found it! */
		booted_device = dev;
		DR_VERBOSE(printf("booted_device = %s\n", dev->dv_xname));
		found = 1;
	}
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
