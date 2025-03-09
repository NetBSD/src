/* $NetBSD: dec_1000a.c,v 1.36 2025/03/09 01:06:41 thorpej Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is based on dec_kn20aa.c, written by Chris G. Demetriou at
 * Carnegie-Mellon University. Platform support for Noritake, Pintake, and
 * Corelle by Ross Harvey with copyright assignment by permission of Avalon
 * Computer Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

__KERNEL_RCSID(0, "$NetBSD: dec_1000a.c,v 1.36 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>
#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

void _dec_1000a_init(void);
static void dec_1000a_cons_init(void);
static void dec_1000a_device_register(device_t, void *);

static const struct alpha_variation_table dec_1000_variations[] = {
	{ 0, "AlphaServer 1000" },
	{ 0, NULL },
};

static const struct alpha_variation_table dec_1000a_variations[] = {
	{ 0, "AlphaServer 1000A" },
	{ 0, NULL },
};

void
_dec_1000a_init(void)
{
	uint64_t variation;

	platform.family = "AlphaServer 1000/1000A";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    cputype == ST_DEC_1000 ? dec_1000_variations
					   : dec_1000a_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	switch(PCS_CPU_MAJORTYPE(LOCATE_PCS(hwrpb, 0))) {
	    case PCS_PROC_EV4:
	    case PCS_PROC_EV45:
		platform.iobus = "apecs";
		break;
	    default:
		platform.iobus = "cia";
		break;
	}
	platform.cons_init = dec_1000a_cons_init;
	platform.device_register = dec_1000a_device_register;
}

static void
dec_1000a_cons_init(void)
{
	struct ctb *ctb;
	struct cia_config *ccp;
	struct apecs_config *acp;
	extern struct cia_config cia_configuration;
	extern struct apecs_config apecs_configuration;
	bus_space_tag_t iot, memt;
	struct alpha_pci_chipset *pcichipset;
	uint64_t saveslot;

	if (strcmp(platform.iobus, "cia") == 0) {
		ccp = &cia_configuration;
		cia_init(ccp);
		iot = &ccp->cc_iot;
		memt = &ccp->cc_memt;
		pcichipset = &ccp->cc_pc;
	} else {
		acp = &apecs_configuration;
		apecs_init(acp);
		iot = &acp->ac_iot;
		memt = &acp->ac_memt;
		pcichipset = &acp->ac_pc;
	}

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);

	saveslot = ctb->ctb_turboslot;

	if (ctb->ctb_term_type == CTB_GRAPHICS &&
	    CTB_TURBOSLOT_TYPE(ctb->ctb_turboslot) != CTB_TURBOSLOT_TYPE_PCI) {
		/*
		 * AlphaServer 1000s have a firmware bug whereby the
		 * built-in ISA VGA is reported incorrectly -- ctb_turboslot
		 * is mostly 0.  Patch it up to what the common code expects.
		 * We'll restore it at the end in case the firmware is picky.
		 */
		ctb->ctb_turboslot = CTB_TURBOSLOT_TYPE_ISA << 16;
	}

	pci_consinit(pcichipset, iot, memt, iot, memt);

	ctb->ctb_turboslot = saveslot;
}

static void
dec_1000a_device_register(device_t dev, void *aux)
{
	pci_find_bootdev(NULL, dev, aux);
}
