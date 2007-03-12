/* $NetBSD: dec_kn8ae.c,v 1.36.2.1 2007/03/12 05:45:50 rmind Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_kn8ae.c,v 1.36.2.1 2007/03/12 05:45:50 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/frame.h>
#include <machine/alpha.h>
#include <machine/logout.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>
#include <alpha/tlsb/kftxxreg.h>
#include <alpha/tlsb/kftxxvar.h>
#define	KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))


void dec_kn8ae_init __P((void));
void dec_kn8ae_cons_init __P((void));
static void dec_kn8ae_device_register __P((struct device *, void *));

static void dec_kn8ae_mcheck_handler
    __P((unsigned long, struct trapframe *, unsigned long, unsigned long));

const struct alpha_variation_table dec_kn8ae_variations[] = {
	{ 0, "AlphaServer 8400" },
	{ 0, NULL },
};

void
dec_kn8ae_init()
{
	u_int64_t variation;

	platform.family = "AlphaServer 8400";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_kn8ae_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "tlsb";
	platform.cons_init = dec_kn8ae_cons_init;
	platform.device_register = dec_kn8ae_device_register;
	platform.mcheck_handler = dec_kn8ae_mcheck_handler;
}

void
dec_kn8ae_cons_init()
{

	/*
	 * Info to retain:
	 *
	 *	The AXP 8X00 seems to encode the
	 *	type of console in the ctb_type field,
	 *	not the ctb_term_type field.
	 *
	 *	XXX Not Type 4 CTB?
	 */
}

/* #define	BDEBUG	1 */
static void
dec_kn8ae_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	static int found, initted, diskboot, netboot;
	static struct device *primarydev, *pcidev, *ctrlrdev;
	struct bootdev_data *b = bootdev_data;
	struct device *parent = device_parent(dev);

	if (found)
		return;

	if (!initted) {
		diskboot = (strcasecmp(b->protocol, "SCSI") == 0);
		netboot = (strcasecmp(b->protocol, "BOOTP") == 0) ||
		    (strcasecmp(b->protocol, "MOP") == 0);
#if	BDEBUG
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
		if (!device_is_a(dev, "dwlpx"))
			return;
		else {
			struct kft_dev_attach_args *ka = aux;

			if (b->bus != ka->ka_hosenum)
				return;
			primarydev = dev;
#ifdef BDEBUG
			printf("\nprimarydev = %s\n", dev->dv_xname);
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
#if	BDEBUG
			printf("\npcidev = %s\n", dev->dv_xname);
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
				printf("\nbooted_device = %s\n", dev->dv_xname);
#endif
				found = 1;
			} else {
				ctrlrdev = dev;
#if	BDEBUG
				printf("\nctrlrdev = %s\n", dev->dv_xname);
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
#if	BDEBUG
		printf("\nbooted_device = %s\n", dev->dv_xname);
#endif
		found = 1;
	}
}

/*
 * KN8AE Machine Check Handlers.
 */
void kn8ae_harderr __P((unsigned long, unsigned long,
    unsigned long, struct trapframe *));

static void kn8ae_softerr __P((unsigned long, unsigned long,
    unsigned long, struct trapframe *));

void kn8ae_mcheck __P((unsigned long, unsigned long,
    unsigned long, struct trapframe *));

/*
 * Support routine for clearing errors
 */
static void clear_tlsb_ebits __P((int));

static void
clear_tlsb_ebits(cpuonly)
	int cpuonly;
{
	int node;
	u_int32_t tldev;

	for (node = 0; node <= TLSB_NODE_MAX; ++node) {
		if ((tlsb_found & (1 << node)) == 0)
			continue;
		tldev = TLSB_GET_NODEREG(node, TLDEV);
		if (tldev == 0) {
			/* "cannot happen" */
			continue;
		}
		/*
		 * Registers to clear for all nodes.
		 */
		if (TLSB_GET_NODEREG(node, TLBER) &
		    (TLBER_UDE|TLBER_CWDE|TLBER_CRDE)) {
			TLSB_PUT_NODEREG(node, TLESR0,
			    TLSB_GET_NODEREG(node, TLESR0));
			TLSB_PUT_NODEREG(node, TLESR1,
			    TLSB_GET_NODEREG(node, TLESR1));
			TLSB_PUT_NODEREG(node, TLESR2,
			    TLSB_GET_NODEREG(node, TLESR2));
			TLSB_PUT_NODEREG(node, TLESR3,
			    TLSB_GET_NODEREG(node, TLESR3));
		}
		TLSB_PUT_NODEREG(node, TLBER,
		    TLSB_GET_NODEREG(node, TLBER));
		TLSB_PUT_NODEREG(node, TLFADR0,
		    TLSB_GET_NODEREG(node, TLFADR0));
		TLSB_PUT_NODEREG(node, TLFADR1,
		    TLSB_GET_NODEREG(node, TLFADR1));

		if (TLDEV_ISCPU(tldev)) {
			TLSB_PUT_NODEREG(node, TLEPAERR,
			    TLSB_GET_NODEREG(node, TLEPAERR));
			TLSB_PUT_NODEREG(node, TLEPDERR,
			    TLSB_GET_NODEREG(node, TLEPDERR));
			TLSB_PUT_NODEREG(node, TLEPMERR,
			    TLSB_GET_NODEREG(node, TLEPMERR));
			continue;
		}
		/*
		 * If we're only doing CPU nodes, or this was a memory
		 * node, we're done. Onwards.
		 */
		if (cpuonly || TLDEV_ISMEM(tldev)) {
			continue;
		}

		TLSB_PUT_NODEREG(node, KFT_ICCNSE,
		    TLSB_GET_NODEREG(node, KFT_ICCNSE));
		TLSB_PUT_NODEREG(node, KFT_IDPNSE0,
		    TLSB_GET_NODEREG(node, KFT_IDPNSE0));
		TLSB_PUT_NODEREG(node, KFT_IDPNSE1,
		    TLSB_GET_NODEREG(node, KFT_IDPNSE1));
		if (TLDEV_DTYPE(tldev) == TLDEV_DTYPE_KFTHA) {
			TLSB_PUT_NODEREG(node, KFT_IDPNSE2,
			    TLSB_GET_NODEREG(node, KFT_IDPNSE2));
			TLSB_PUT_NODEREG(node, KFT_IDPNSE3,
			    TLSB_GET_NODEREG(node, KFT_IDPNSE3));
		}
		/*
		 * Digital Unix cleares the Mailbox Transaction Register
		 * here. I don't think we should because we aren't using
		 * mailboxes yet, and the tech manual makes dire warnings
		 * about *not* rewriting this register.
		 */
	}
}

/*
 * System Corrected Errors.
 */
static const char *fmt1 = "        %-25s = 0x%l016x\n";

void
kn8ae_harderr(mces, type, logout, framep)
	unsigned long mces;
	unsigned long type;
	unsigned long logout;
	struct trapframe *framep;
{
	int whami, cpuwerr, dof_cnt;
	mc_hdr_ev5 *hdr;
	mc_cc_ev5 *mptr;
	struct tlsb_mchk_fatal *ptr;

	hdr = (mc_hdr_ev5 *) logout;
	mptr = (mc_cc_ev5 *) (logout + sizeof (*hdr));
	ptr = (struct tlsb_mchk_fatal *)
		(logout + sizeof (*hdr) + sizeof (*mptr));
	whami = alpha_pal_whami();

	printf("kn8ae: CPU ID %d system correctable error\n", whami);

	printf("    Machine Check Code 0x%lx\n", hdr->mcheck_code);
	printf(fmt1, "EI Status", mptr->ei_stat);
	printf(fmt1, "EI Address", mptr->ei_addr);
	printf(fmt1, "Fill Syndrome", mptr->fill_syndrome);
	printf(fmt1, "Interrupt Status Reg.", mptr->isr);
	printf("\n");
	dof_cnt = (ptr->rsvdheader & 0xffffffff00000000) >> 32; 
	cpuwerr = ptr->rsvdheader & 0xffff;
	
	printf(fmt1, "CPU W/Error.", cpuwerr);
	printf(fmt1, "DOF Count.", dof_cnt);
	printf(fmt1, "TLDEV", ptr->tldev);
	printf(fmt1, "TLSB Bus Error", ptr->tlber);
	printf(fmt1, "TLSB CNR", ptr->tlcnr);
	printf(fmt1, "TLSB VID", ptr->tlvid);
	printf(fmt1, "TLSB Error Syndrome 0", ptr->tlesr0);
	printf(fmt1, "TLSB Error Syndrome 1", ptr->tlesr1);
	printf(fmt1, "TLSB Error Syndrome 2", ptr->tlesr2);
	printf(fmt1, "TLSB Error Syndrome 3", ptr->tlesr3);
	printf(fmt1, "TLSB LEP_AERR", ptr->tlepaerr);
	printf(fmt1, "TLSB MODCONF", ptr->tlmodconfig);
	printf(fmt1, "TLSB LEP_MERR", ptr->tlepmerr);
	printf(fmt1, "TLSB LEP_DERR", ptr->tlepderr);
	printf(fmt1, "TLSB INTRMASK0", ptr->tlintrmask0);
	printf(fmt1, "TLSB INTRMASK1", ptr->tlintrmask1);
	printf(fmt1, "TLSB INTRSUM0", ptr->tlintrsum0);
	printf(fmt1, "TLSB INTRSUM1", ptr->tlintrsum1);
	printf(fmt1, "TLSB VMG", ptr->tlep_vmg);

	/* CLEAN UP */
	/*
	 * Here's what Digital Unix says to do-
	 *
	 * 1. Log the ECC error that got us here
	 *
	 * 2. Turn off error reporting
	 *
	 * 3. Attempt to have CPU read bad memory location (specified by the
	 *    tlfadr reg of the TIOP or TMEM (depending on type of error,
	 *    see upcoming code branches) and write data back to location.
	 *
         * 4. When the CPU attempts to read the location, another 620 interrupt
         *    should occur for the CPU at which instant PAL will scrub the
         *    location. Then the o.s. scrub routine finishes. If the PAL scrubs
	 *    the location then the scrubbed flag should be 0 (this is what we
	 *    expect).
	 *
         *    If it's a 1 then the alpha_scrub_long routine did the scrub.
	 *
         * 5. We renable correctable error logging and continue
	 */
	printf("WARNING THIS IS NOT DONE YET YOU MAY GET DATA CORRUPTION");
	clear_tlsb_ebits(0);
	/*
	 * Clear error by rewriting register.
	 */
        alpha_pal_wrmces(mces);
}

/*
 *  Processor Corrected Errors- BCACHE ECC errors.
 */

static void
kn8ae_softerr(mces, type, logout, framep)
	unsigned long mces;
	unsigned long type;
	unsigned long logout;
	struct trapframe *framep;
{
	int whami, cpuwerr, dof_cnt;
	mc_hdr_ev5 *hdr;
	mc_cc_ev5 *mptr;
	struct tlsb_mchk_soft *ptr;

	hdr = (mc_hdr_ev5 *) logout;
	mptr = (mc_cc_ev5 *) (logout + sizeof (*hdr));
	ptr = (struct tlsb_mchk_soft *)
		(logout + sizeof (*hdr) + sizeof (*mptr));
	whami = alpha_pal_whami();

	printf("kn8ae: CPU ID %d processor correctable error\n", whami);
	printf("    Machine Check Code 0x%lx\n", hdr->mcheck_code);
	printf(fmt1, "EI Status", mptr->ei_stat);
	printf(fmt1, "EI Address", mptr->ei_addr);
	printf(fmt1, "Fill Syndrome", mptr->fill_syndrome);
	printf(fmt1, "Interrupt Status Reg.", mptr->isr);
	printf("\n");
	dof_cnt = (ptr->rsvdheader & 0xffffffff00000000) >> 32; 
	cpuwerr = ptr->rsvdheader & 0xffff;
	
	printf(fmt1, "CPU W/Error.", cpuwerr);
	printf(fmt1, "DOF Count.", dof_cnt);
	printf(fmt1, "TLDEV", ptr->tldev);
	printf(fmt1, "TLSB Bus Error", ptr->tlber);
	printf(fmt1, "TLSB Error Syndrome 0", ptr->tlesr0);
	printf(fmt1, "TLSB Error Syndrome 1", ptr->tlesr1);
	printf(fmt1, "TLSB Error Syndrome 2", ptr->tlesr2);
	printf(fmt1, "TLSB Error Syndrome 3", ptr->tlesr3);

	/*
	 * Clear TLSB bits on all CPU TLSB nodes.
	 */
	clear_tlsb_ebits(1);

	/*
	 * Clear error by rewriting register.
	 */
        alpha_pal_wrmces(mces);
}

/*
 * KN8AE specific machine check handler
 */

void
kn8ae_mcheck(mces, type, logout, framep)
	unsigned long mces;
	unsigned long type;
	unsigned long logout;
	struct trapframe *framep;
{
	struct mchkinfo *mcp;
	int get_dwlpx_regs;
	struct tlsb_mchk_fatal mcs[TLSB_NODE_MAX+1], *ptr;
	mc_hdr_ev5 *hdr;
	mc_uc_ev5 *mptr;

	/*
	 * If we expected a machine check, just go handle it in common code.
	 */
	mcp = &curcpu()->ci_mcinfo;
	if (mcp->mc_expected) {
		machine_check(mces, framep, type, logout);
		return;
	}

	get_dwlpx_regs = 0;
	ptr = NULL;
	memset(mcs, 0, sizeof (mcs));

	hdr = (mc_hdr_ev5 *) logout;
	mptr = (mc_uc_ev5 *) (logout + sizeof (*hdr));

	/*
	 * If detected by the system, we print out some TLASER registers.
	 */
	if (type == ALPHA_SYS_MCHECK) {
#if	0
		int get_lsb_regs = 0;
		int get_dwlpx_regs = 0;
#endif

		ptr = (struct tlsb_mchk_fatal *)
		    (logout + sizeof (*hdr) + sizeof (*mptr));

#if	0
		if (ptr->tlepaerr & TLEPAERR_WSPC_RD) {
			get_dwlpx_regs++;
		}
		if ((ptr->tlepaerr & TLEPAERR_IBOX_TMO) &&
		    (mptr->ic_perr_stat & EV5_IC_PERR_IBOXTMO) &&
		    (ptr->tlepderr & TLEPDERR_GBTMO)) {
			get_dwlpx_regs++;
		}
#endif
	} else {
		/*
		 * We have a processor machine check- which doesn't
		 * have information with it about any TLSB related
		 * failures.
		 */
	}

	/*
	 * Now we can finally print some stuff...
	 */
	ev5_logout_print(hdr, mptr);
	if (type == ALPHA_SYS_MCHECK) {
		if (ptr->tlepaerr & TLEPAERR_WSPC_RD) {
			printf("\tWSPC READ error\n");
		}
		if ((ptr->tlepaerr & TLEPAERR_IBOX_TMO) &&
		    (mptr->ic_perr_stat & EV5_IC_PERR_IBOXTMO) &&
		    (ptr->tlepderr & TLEPDERR_GBTMO)) {
			printf ("\tWSPC IBOX timeout detected\n");
		}
#ifdef	DIAGNOSTIC
		printf(fmt1, "TLDEV", ptr->tldev);
		printf(fmt1, "TLSB Bus Error", ptr->tlber);
		printf(fmt1, "TLSB CNR", ptr->tlcnr);
		printf(fmt1, "TLSB VID", ptr->tlvid);
		printf(fmt1, "TLSB Error Syndrome 0", ptr->tlesr0);
		printf(fmt1, "TLSB Error Syndrome 1", ptr->tlesr1);
		printf(fmt1, "TLSB Error Syndrome 2", ptr->tlesr2);
		printf(fmt1, "TLSB Error Syndrome 3", ptr->tlesr3);
		printf(fmt1, "TLSB LEP_AERR", ptr->tlepaerr);
		printf(fmt1, "TLSB MODCONF", ptr->tlmodconfig);
		printf(fmt1, "TLSB LEP_MERR", ptr->tlepmerr);
		printf(fmt1, "TLSB LEP_DERR", ptr->tlepderr);
		printf(fmt1, "TLSB INTRMASK0", ptr->tlintrmask0);
		printf(fmt1, "TLSB INTRMASK1", ptr->tlintrmask1);
		printf(fmt1, "TLSB INTRSUM0", ptr->tlintrsum0);
		printf(fmt1, "TLSB INTRSUM1", ptr->tlintrsum1);
		printf(fmt1, "TLSB VMG", ptr->tlep_vmg);
#endif
	} else {
	}

	/*
	 * Now that we've printed all sorts of useful information
	 * and have decided that we really can't do any more to
	 * respond to the error, go on to the common code for
	 * final disposition. Usually this means that we die.
	 */
	clear_tlsb_ebits(0);

	machine_check(mces, framep, type, logout);
}

static void
dec_kn8ae_mcheck_handler(mces, framep, vector, param)
	unsigned long mces;
	struct trapframe *framep;
	unsigned long vector;
	unsigned long param;
{
	switch (vector) {
	case ALPHA_SYS_ERROR:
		kn8ae_harderr(mces, vector, param, framep);
		break;

	case ALPHA_PROC_ERROR:
		kn8ae_softerr(mces, vector, param, framep);
		break;

	case ALPHA_SYS_MCHECK:
	case ALPHA_PROC_MCHECK:
		kn8ae_mcheck(mces, vector, param, framep);
		break;
	default:
		printf("KN8AE_MCHECK: unknown check vector 0x%lx\n", vector);
		machine_check(mces, framep, vector, param);
		break;
	}
}
