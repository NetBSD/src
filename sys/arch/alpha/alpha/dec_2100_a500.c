/* $NetBSD: dec_2100_a500.c,v 1.27 2025/03/09 01:06:41 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_2100_a500.c,v 1.27 2025/03/09 01:06:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/lwp.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/alpha.h>

#include <dev/pci/pcivar.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>

#define	DR_VERBOSE(f)	while (0)

void _dec_2100_a500_init(void);
static void dec_2100_a500_cons_init(void);
static void dec_2100_a500_device_register(device_t, void *);
static void dec_2100_a500_machine_check(unsigned long, struct trapframe *,
	unsigned long, unsigned long);

void
_dec_2100_a500_init(void)
{

	switch (cputype) {
	case ST_DEC_2100_A500:
		if (alpha_implver() == ALPHA_IMPLVER_EV5)
			platform.family = "AlphaServer 2100 (\"Sable-Gamma\")";
		else
			platform.family = "AlphaServer 2100 (\"Sable\")";
		break;

	case ST_DEC_2100A_A500:
		platform.family = "AlphaServer 2100A (\"Lynx\")";
		break;

	default:
		panic("dec_2100_a500_init: Not a Sable, Sable-Gamma, or Lynx?");
	}

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		/* XXX don't know variations yet */
		platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "ttwoga";
	platform.cons_init = dec_2100_a500_cons_init;
	platform.device_register = dec_2100_a500_device_register;
	platform.mcheck_handler = dec_2100_a500_machine_check;
}

static void
dec_2100_a500_cons_init(void)
{
	struct ctb *ctb;
	uint64_t ctbslot;
	struct ttwoga_config *tcp;
	bus_space_tag_t isa_iot, isa_memt;

	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);
	ctbslot = ctb->ctb_turboslot;

	tcp = ttwoga_init(0);

	isa_iot = &tcp->tc_iot;
	isa_memt = &tcp->tc_memt;

	switch (ctb->ctb_term_type) {
	case CTB_PRINTERPORT:
		/* serial console ... */
		assert(CTB_TURBOSLOT_HOSE(ctbslot) == 0);
		break;

	case CTB_GRAPHICS:
		/* PCI display might be on a different hose. */
		if (CTB_TURBOSLOT_TYPE(ctbslot) == CTB_TURBOSLOT_TYPE_PCI) {
			tcp = ttwoga_init(CTB_TURBOSLOT_HOSE(ctbslot));
		}
		break;

	default:
		/* Let pci_consinit() handle it. */
		break;
	}

	pci_consinit(&tcp->tc_pc, &tcp->tc_iot, &tcp->tc_memt,
	    isa_iot, isa_memt);
}

static void
dec_2100_a500_device_register(device_t dev, void *aux)
{
	static device_t primarydev;
	struct bootdev_data *b = bootdev_data;

	if (booted_device != NULL || b == NULL) {
		return;
	}

	if (primarydev == NULL) {
		if (device_is_a(dev, "ttwopci")) {
			struct pcibus_attach_args *pba = aux;

			if (b->bus == pba->pba_bus) {
				primarydev = dev;
				DR_VERBOSE(printf("\nprimarydev = %s\n",
				    device_xname(dev)));
			}
		}
		return;
	}

	pci_find_bootdev(primarydev, dev, aux);
}

/*
 * Sable, Sable-Gamma, and Lynx machine check handlers.
 */

static void
dec_2100_a500_machine_check(unsigned long mces, struct trapframe *framep,
    unsigned long vector, unsigned long param)
{
	struct mchkinfo *mcp = &curcpu()->ci_mcinfo;

	/*
	 * This is a work-around for a T2 core logic bug.  See
	 * alpha/pci/ttwoga_pci.c.
	 */

	if (ttwoga_conf_cpu == cpu_number())
		mcp->mc_expected = 1;

	machine_check(mces, framep, vector, param);
}
