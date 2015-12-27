/* $NetBSD: dec_kn300.c,v 1.40.14.1 2015/12/27 12:09:27 skrll Exp $ */

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

#include "opt_kgdb.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_kn300.c,v 1.40.14.1 2015/12/27 12:09:27 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/alpha.h>
#include <machine/autoconf.h>
#include <machine/frame.h>
#include <machine/cpuconf.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <uvm/uvm_extern.h>

#include <alpha/mcbus/mcbusreg.h>
#include <alpha/mcbus/mcbusvar.h>
#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>
#include <alpha/pci/pci_kn300.h>
#include <machine/logout.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/mlxio.h>
#include <dev/ic/mlxvar.h>


#include "pckbd.h"

#ifndef	CONSPEED
#define	CONSPEED	TTYDEF_SPEED
#endif
static int comcnrate = CONSPEED;

void dec_kn300_init(void);
void dec_kn300_cons_init(void);
static void dec_kn300_device_register(device_t, void *);
static void dec_kn300_mcheck_handler
(unsigned long, struct trapframe *, unsigned long, unsigned long);

#ifdef KGDB
#include <machine/db_machdep.h>

static const char *kgdb_devlist[] = {
	"com",
	NULL,
};
#endif /* KGDB */

#define	ALPHASERVER_4100	"AlphaServer 4100"

const struct alpha_variation_table dec_kn300_variations[] = {
	{ 0, ALPHASERVER_4100 },
	{ 0, NULL },
};

void
dec_kn300_init(void)
{
	uint64_t variation;
	int cachesize;

	platform.family = ALPHASERVER_4100;

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_kn300_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "mcbus";
	platform.cons_init = dec_kn300_cons_init;
	platform.device_register = dec_kn300_device_register;
	platform.mcheck_handler = dec_kn300_mcheck_handler;

	/*
	 * Determine B-cache size by looking at the primary (console)
	 * MCPCIA's WHOAMI register.
	 */
	mcpcia_init();

	if (mcbus_primary.mcbus_valid) {
		switch (mcbus_primary.mcbus_bcache) {
		default:
		case CPU_BCache_0MB:
			/* No B-cache or invalid; default to 1MB. */
			/* FALLTHROUGH */

		case CPU_BCache_1MB:
			cachesize = (1 * 1024 * 1024);
			break;

		case CPU_BCache_2MB:
			cachesize = (2 * 1024 * 1024);
			break;

		case CPU_BCache_4MB:
			cachesize = (4 * 1024 * 1024);
			break;
		}
	} else {
		/* Default to 1MB. */
		cachesize = (1 * 1024 * 1024);
	}

	uvmexp.ncolors = atop(cachesize);
}

void
dec_kn300_cons_init(void)
{
	struct ctb *ctb;
	struct mcpcia_config *ccp;
	extern struct mcpcia_config mcpcia_console_configuration;

	ccp = &mcpcia_console_configuration;
	/* It's already initialized. */

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);

	switch (ctb->ctb_term_type) {
	case CTB_PRINTERPORT:
		/* serial console ... */
		/*
		 * Delay to allow PROM putchars to complete.
		 * FIFO depth * character time,
		 * character time = (1000000 / (defaultrate / 10))
		 */
		DELAY(160000000 / comcnrate);
		if (comcnattach(&ccp->cc_iot, 0x3f8, comcnrate,
		    COM_FREQ, COM_TYPE_NORMAL,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)) {
			panic("can't init serial console");

		}
		break;

	case CTB_GRAPHICS:
#if NPCKBD > 0
		/* display console ... */
		/* XXX */
		(void) pckbc_cnattach(&ccp->cc_iot, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT, 0);

		if (CTB_TURBOSLOT_TYPE(ctb->ctb_turboslot) ==
		    CTB_TURBOSLOT_TYPE_ISA)
			isa_display_console(&ccp->cc_iot, &ccp->cc_memt);
		else
			pci_display_console(&ccp->cc_iot, &ccp->cc_memt,
			    &ccp->cc_pc, CTB_TURBOSLOT_BUS(ctb->ctb_turboslot),
			    CTB_TURBOSLOT_SLOT(ctb->ctb_turboslot), 0);
#else
		panic("not configured to use display && keyboard console");
#endif
		break;

	default:
		printf("ctb->ctb_term_type = 0x%lx\n", ctb->ctb_term_type);
		printf("ctb->ctb_turboslot = 0x%lx\n", ctb->ctb_turboslot);

		panic("consinit: unknown console type %ld",
		    ctb->ctb_term_type);
	}
#ifdef KGDB
	/* Attach the KGDB device. */
	alpha_kgdb_init(kgdb_devlist, &ccp->cc_iot);
#endif /* KGDB */
}

/* #define	BDEBUG	1 */
static void
dec_kn300_device_register(device_t dev, void *aux)
{
	static int found, initted, diskboot, netboot;
	static device_t primarydev, pcidev, ctrlrdev;
	struct bootdev_data *b = bootdev_data;
	device_t parent = device_parent(dev);

	if (found)
		return;

	if (!initted) {
		diskboot = (strcasecmp(b->protocol, "SCSI") == 0) ||
		    (strcasecmp(b->protocol, "RAID") == 0);
		netboot = (strcasecmp(b->protocol, "BOOTP") == 0) ||
		    (strcasecmp(b->protocol, "MOP") == 0);
#ifdef BDEBUG
		printf("proto:%s bus:%d slot:%d chan:%d", b->protocol,
		    b->bus, b->slot, b->channel);
		if (b->remote_address)
			printf(" remote_addr:%s", b->remote_address);
		printf(" un:%d bdt:%d", b->unit, b->boot_dev_type);
		if (b->ctrl_dev_type)
			printf(" cdt:%s\n", b->ctrl_dev_type);
		else
			printf("\n");
		printf("diskboot = %d, netboot = %d\n", diskboot, netboot);
#endif
		initted = 1;
	}

	if (primarydev == NULL) {
		if (!device_is_a(dev, "mcpcia"))
			return;
		else {
			struct mcbus_dev_attach_args *ma = aux;

			if (b->bus != ma->ma_mid - 4)
				return;
			primarydev = dev;
#ifdef BDEBUG
			printf("\nprimarydev = %s\n", device_xname(dev));
#endif
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
#ifdef BDEBUG
			printf("\npcidev = %s\n", device_xname(dev));
#endif
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
#ifdef BDEBUG
				printf("\nbooted_device = %s\n", device_xname(dev));
#endif
				found = 1;
			} else {
				ctrlrdev = dev;
#ifdef BDEBUG
				printf("\nctrlrdev = %s\n", device_xname(dev));
#endif
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
#ifdef BDEBUG
		printf("\nbooted_device = %s\n", device_xname(dev));
#endif
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
#if 0
		printf("\nbooted_device = %s\n", device_xname(dev));
#endif
		found = 1;
	}
}


/*
 * KN300 Machine Check Handlers.
 */
static void kn300_softerr(unsigned long, unsigned long,
    unsigned long, struct trapframe *);

static void kn300_mcheck(unsigned long, unsigned long,
    unsigned long, struct trapframe *);

/*
 * "soft" error structure in system area for KN300 processor.
 * It differs from the EV5 'common' structure in a minor but
 * exceedingly stupid and annoying fashion.
 */

typedef struct {
	/*
	 * Should be mc_cc_ev5 structure. Contents are the same,
	 * just in different places.
	 */
	uint64_t	ei_stat;
	uint64_t	ei_addr;
	uint64_t	fill_syndrome;
	uint64_t	isr;
	/*
	 * Platform Specific Area
	 */
	uint32_t	whami;
	uint32_t	sys_env;
	uint64_t	mcpcia_regs;
	uint32_t	pci_rev;
	uint32_t	mc_err0;
	uint32_t	mc_err1;
	uint32_t	cap_err;
	uint32_t	mdpa_stat;
	uint32_t	mdpa_syn;
	uint32_t	mdpb_stat;
	uint32_t	mdpb_syn;
	uint64_t	end_rsvd;
} mc_soft300;
#define	CAP_ERR_CRDX	204

static void
kn300_softerr(unsigned long mces, unsigned long type, unsigned long logout, struct trapframe *framep)
{
	static const char *sys = "system";
	static const char *proc = "processor";
	int whami;
	mc_hdr_ev5 *hdr;
	mc_soft300 *ptr;
	static const char *fmt1 = "        %-25s = 0x%l016x\n";

	hdr = (mc_hdr_ev5 *) logout;
	ptr = (mc_soft300 *) (logout + sizeof (*hdr));
	whami = alpha_pal_whami();

	printf("kn300: CPU ID %d %s correctable error corrected by %s\n", whami,
	    (type == ALPHA_SYS_ERROR)?  sys : proc,
	    ((hdr->mcheck_code & 0xff00) == (EV5_CORRECTED << 8))? proc :
	    (((hdr->mcheck_code & 0xff00) == (CAP_ERR_CRDX << 8)) ?
		"I/O Bridge Module" : sys));

	printf("    Machine Check Code 0x%lx\n", hdr->mcheck_code);
	printf("    Physical Address of Error 0x%lx\n", ptr->ei_addr);
	if (ptr->ei_stat & 0x80000000L)
		printf("    Corrected ECC Error ");
	else
		printf("    Other Error");
	if (ptr->ei_stat & 0x40000000L)
		printf("in Memory ");
	else
		printf("in B-Cache ");
	if (ptr->ei_stat & 0x400000000L)
		printf("during I-Cache fill\n");
	else
		printf("during D-Cache fill\n");

	printf(fmt1, "EI Status", ptr->ei_stat);
	printf(fmt1, "Fill Syndrome", ptr->fill_syndrome);
	printf(fmt1, "Interrupt Status Reg.", ptr->isr);
	printf("\n");
	printf(fmt1, "Whami Reg.", ptr->whami);
	printf(fmt1, "Sys. Env. Reg.", ptr->sys_env);
	printf(fmt1, "MCPCIA Regs.", ptr->mcpcia_regs);
	printf(fmt1, "PCI Rev. Reg.", ptr->pci_rev);
	printf(fmt1, "MC_ERR0 Reg.", ptr->mc_err0);
	printf(fmt1, "MC_ERR1 Reg.", ptr->mc_err1);
	printf(fmt1, "CAP_ERR Reg.", ptr->cap_err);
	printf(fmt1, "MDPA_STAT Reg.", ptr->mdpa_stat);
	printf(fmt1, "MDPA_SYN Reg.", ptr->mdpa_syn);
	printf(fmt1, "MDPB_STAT Reg.", ptr->mdpb_stat);
	printf(fmt1, "MDPB_SYN Reg.", ptr->mdpb_syn);

	/*
	 * Clear error by rewriting register.
	 */
	alpha_pal_wrmces(mces);
}

/*
 * KN300 specific machine check handler
 */

static void
kn300_mcheck(unsigned long mces, unsigned long type, unsigned long logout, struct trapframe *framep)
{
	struct mchkinfo *mcp;
	static const char *fmt1 = "        %-25s = 0x%l016x\n";
	int i;	
	mc_hdr_ev5 *hdr;
	mc_uc_ev5 *ptr;
	struct mcpcia_iodsnap *iodsnp;

	/*
	 * If we expected a machine check, just go handle it in common code.
	 */
	mcp = &curcpu()->ci_mcinfo;
	if (mcp->mc_expected) {
		machine_check(mces, framep, type, logout);
		return;
	}

	hdr = (mc_hdr_ev5 *) logout;
	ptr = (mc_uc_ev5 *) (logout + sizeof (*hdr));
	ev5_logout_print(hdr, ptr);

	iodsnp = (struct mcpcia_iodsnap *) ((unsigned long) hdr +
	    (unsigned long) hdr->la_system_offset);
	for (i = 0; i < MCPCIA_PER_MCBUS; i++, iodsnp++) {
		if (!IS_MCPCIA_MAGIC(iodsnp->pci_rev)) {
			continue;
		}
		printf("        IOD %d register dump:\n", i);
		printf(fmt1, "Base Addr of PCI bridge", iodsnp->base_addr);
		printf(fmt1, "Whami Reg.", iodsnp->whami);
		printf(fmt1, "Sys. Env. Reg.", iodsnp->sys_env);
		printf(fmt1, "PCI Rev. Reg.", iodsnp->pci_rev);
		printf(fmt1, "CAP_CTL Reg.", iodsnp->cap_ctrl);
		printf(fmt1, "HAE_MEM Reg.", iodsnp->hae_mem);
		printf(fmt1, "HAE_IO Reg.", iodsnp->hae_io);
		printf(fmt1, "INT_CTL Reg.", iodsnp->int_ctl);
		printf(fmt1, "INT_REG Reg.", iodsnp->int_reg);
		printf(fmt1, "INT_MASK0 Reg.", iodsnp->int_mask0);
		printf(fmt1, "INT_MASK1 Reg.", iodsnp->int_mask1);
		printf(fmt1, "MC_ERR0 Reg.", iodsnp->mc_err0);
		printf(fmt1, "MC_ERR1 Reg.", iodsnp->mc_err1);
		printf(fmt1, "CAP_ERR Reg.", iodsnp->cap_err);
		printf(fmt1, "PCI_ERR1 Reg.", iodsnp->pci_err1);
		printf(fmt1, "MDPA_STAT Reg.", iodsnp->mdpa_stat);
		printf(fmt1, "MDPA_SYN Reg.", iodsnp->mdpa_syn);
		printf(fmt1, "MDPB_STAT Reg.", iodsnp->mdpb_stat);
		printf(fmt1, "MDPB_SYN Reg.", iodsnp->mdpb_syn);

	}
	/*
	 * Now that we've printed all sorts of useful information
	 * and have decided that we really can't do any more to
	 * respond to the error, go on to the common code for
	 * final disposition. Usually this means that we die.
	 */
	/*
	 * XXX: HANDLE PCI ERRORS HERE?
	 */
	machine_check(mces, framep, type, logout);
}

static void
dec_kn300_mcheck_handler(unsigned long mces, struct trapframe *framep, unsigned long vector, unsigned long param)
{
	switch (vector) {
	case ALPHA_SYS_ERROR:
	case ALPHA_PROC_ERROR:
		kn300_softerr(mces, vector, param, framep);
		break;

	case ALPHA_SYS_MCHECK:
	case ALPHA_PROC_MCHECK:
		kn300_mcheck(mces, vector, param, framep);
		break;
	default:
		printf("KN300_MCHECK: unknown check vector 0x%lx\n", vector);
		machine_check(mces, framep, vector, param);
		break;
	}
}
