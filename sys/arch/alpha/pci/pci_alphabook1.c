/* $NetBSD: pci_alphabook1.c,v 1.5 2000/06/29 08:58:48 mrg Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

__KERNEL_RCSID(0, "$NetBSD: pci_alphabook1.c,v 1.5 2000/06/29 08:58:48 mrg Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/lcavar.h>

#include <alpha/pci/pci_alphabook1.h>
#include <alpha/pci/siovar.h>
#include <alpha/pci/sioreg.h>

#include "sio.h"

int     dec_alphabook1_intr_map __P((void *, pcitag_t, int, int,
	    pci_intr_handle_t *));
const char *dec_alphabook1_intr_string __P((void *, pci_intr_handle_t));
const struct evcnt *dec_alphabook1_intr_evcnt __P((void *, pci_intr_handle_t));
void    *dec_alphabook1_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void    dec_alphabook1_intr_disestablish __P((void *, void *));

#define	LCA_SIO_DEVICE	7	/* XXX */

void
pci_alphabook1_pickintr(lcp)
	struct lca_config *lcp;
{
	bus_space_tag_t iot = &lcp->lc_iot;
	pci_chipset_tag_t pc = &lcp->lc_pc;
	pcireg_t sioclass;
	int sioII;

	/* XXX MAGIC NUMBER */
	sioclass = pci_conf_read(pc, pci_make_tag(pc, 0, LCA_SIO_DEVICE, 0),
	    PCI_CLASS_REG);
        sioII = (sioclass & 0xff) >= 3;

	if (!sioII)
		printf("WARNING: SIO NOT SIO II... NO BETS...\n");

	pc->pc_intr_v = lcp;
	pc->pc_intr_map = dec_alphabook1_intr_map;
	pc->pc_intr_string = dec_alphabook1_intr_string;
	pc->pc_intr_evcnt = dec_alphabook1_intr_evcnt;
	pc->pc_intr_establish = dec_alphabook1_intr_establish;
	pc->pc_intr_disestablish = dec_alphabook1_intr_disestablish;

	/* Not supported on AlphaBook. */
	pc->pc_pciide_compat_intr_establish = NULL;

#if NSIO
	sio_intr_setup(pc, iot);
	set_iointr(&sio_iointr);
#else
	panic("pci_alphabook1_pickintr: no I/O interrupt handler (no sio)");
#endif
}

int
dec_alphabook1_intr_map(lcv, bustag, buspin, line, ihp)
	void *lcv;
	pcitag_t bustag;
	int buspin, line;
	pci_intr_handle_t *ihp;
{
	struct lca_config *lcp = lcv;
	pci_chipset_tag_t pc = &lcp->lc_pc;
	int device, irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("dec_alphabook1_intr_map: bad interrupt pin %d\n",
		    buspin);
		return 1;
	}

	alpha_pci_decompose_tag(pc, bustag, NULL, &device, NULL);

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

	*ihp = irq;
	return (0);
}

const char *
dec_alphabook1_intr_string(lcv, ih)
	void *lcv;
	pci_intr_handle_t ih;
{
#if 0
	struct lca_config *lcp = lcv;
#endif

	return sio_intr_string(NULL /*XXX*/, ih);
}

const struct evcnt *
dec_alphabook1_intr_evcnt(lcv, ih)
	void *lcv;
	pci_intr_handle_t ih;
{
#if 0
	struct lca_config *lcp = lcv;
#endif

	return sio_intr_evcnt(NULL /*XXX*/, ih);
}

void *
dec_alphabook1_intr_establish(lcv, ih, level, func, arg)
	void *lcv, *arg;
	pci_intr_handle_t ih;
	int level;
	int (*func) __P((void *));
{
#if 0
	struct lca_config *lcp = lcv;
#endif

	return sio_intr_establish(NULL /*XXX*/, ih, IST_LEVEL, level, func,
	    arg);
}

void
dec_alphabook1_intr_disestablish(lcv, cookie)
	void *lcv, *cookie;
{
#if 0
	struct lca_config *lcp = lcv;
#endif

	sio_intr_disestablish(NULL /*XXX*/, cookie);
}
