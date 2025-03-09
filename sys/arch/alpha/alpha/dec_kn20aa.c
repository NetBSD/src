/* $NetBSD: dec_kn20aa.c,v 1.69 2025/03/09 01:06:42 thorpej Exp $ */

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
/*
 * Additional Copyright (c) 1997 by Matthew Jacob for NASA/Ames Research Center
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_kn20aa.c,v 1.69 2025/03/09 01:06:42 thorpej Exp $");

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

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

void dec_kn20aa_init(void);
static void dec_kn20aa_cons_init(void);
static void dec_kn20aa_device_register(device_t, void *);

static void dec_kn20aa_mcheck_handler
(unsigned long, struct trapframe *, unsigned long, unsigned long);

static void dec_kn20aa_mcheck(unsigned long, unsigned long,
				     unsigned long, struct trapframe *);

const struct alpha_variation_table dec_kn20aa_variations[] = {
	{ 0, "AlphaStation 500 or 600 (KN20AA)" },
	{ 0, NULL },
};

void
dec_kn20aa_init(void)
{
	uint64_t variation;

	platform.family = "AlphaStation 500 or 600 (KN20AA)";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_kn20aa_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "cia";
	platform.cons_init = dec_kn20aa_cons_init;
	platform.device_register = dec_kn20aa_device_register;
	platform.mcheck_handler = dec_kn20aa_mcheck_handler;
}

static void
dec_kn20aa_cons_init(void)
{
	struct cia_config *ccp;
	extern struct cia_config cia_configuration;

	ccp = &cia_configuration;
	cia_init(ccp);

	pci_consinit(&ccp->cc_pc, &ccp->cc_iot, &ccp->cc_memt,
	    &ccp->cc_iot, &ccp->cc_memt);
}

static void
dec_kn20aa_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}

static void
dec_kn20aa_mcheck(unsigned long mces, unsigned long type, unsigned long logout, struct trapframe *framep)
{
	struct mchkinfo *mcp;
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

	hdr = (mc_hdr_ev5 *) logout;
	mptr = (mc_uc_ev5 *) (logout + sizeof (*hdr));

	/*
	 * Now we can finally print some stuff...
	 */
	ev5_logout_print(hdr, mptr);

	machine_check(mces, framep, type, logout);
}

static void
dec_kn20aa_mcheck_handler(unsigned long mces, struct trapframe *framep, unsigned long vector, unsigned long param)
{

	switch (vector) {
	case ALPHA_SYS_MCHECK:
	case ALPHA_PROC_MCHECK:
		dec_kn20aa_mcheck(mces, vector, param, framep);
		break;
	default:
		printf("KN20AA_MCHECK: unknown check vector 0x%lx\n", vector);
		machine_check(mces, framep, vector, param);
		break;
	}
}
