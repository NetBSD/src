/* $NetBSD: pci_alphabook1.c,v 1.20 2021/06/25 13:38:21 thorpej Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Jeffrey Hsu and Chris G. Demetriou
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

__KERNEL_RCSID(0, "$NetBSD: pci_alphabook1.c,v 1.20 2021/06/25 13:38:21 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/rpb.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/lcavar.h>

#include <alpha/pci/siovar.h>
#include <alpha/pci/sioreg.h>

#include "sio.h"

static int	dec_alphabook1_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

static void
pci_alphabook1_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_alphabook1_intr_map;
	pc->pc_intr_string = sio_pci_intr_string;
	pc->pc_intr_evcnt = sio_pci_intr_evcnt;
	pc->pc_intr_establish = sio_pci_intr_establish;
	pc->pc_intr_disestablish = sio_pci_intr_disestablish;

	/* Not supported on AlphaBook. */
	pc->pc_pciide_compat_intr_establish = NULL;

#if NSIO
	sio_intr_setup(pc, iot);
#else
	panic("pci_alphabook1_pickintr: no I/O interrupt handler (no sio)");
#endif
}
ALPHA_PCI_INTR_INIT(ST_ALPHABOOK1, pci_alphabook1_pickintr)

int
dec_alphabook1_intr_map(const struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("dec_alphabook1_intr_map: bad interrupt pin %d\n",
		    buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	/*
	 * There is only one interrupting PCI device on the AlphaBook: an
	 * NCR SCSI at device 6.  Devices 7 and 8 are the SIO and a
	 * Cirrus PD6729 PCMCIA controller.  There are no option slots
	 * available.
	 *
	 * NOTE!  Apparently, there was a later AlphaBook which uses
	 * a different interrupt scheme, and has a built-in Tulip Ethernet
	 * interface!  We do not handle that here!
	 */

	switch (device) {
	case 6:					/* NCR SCSI */
		irq = 14;
		break;

	default:
	        printf("dec_alphabook1_intr_map: weird device number %d\n",
		    device);
	        return 1;
	}

	alpha_pci_intr_handle_init(ihp, irq, 0);
	return (0);
}
