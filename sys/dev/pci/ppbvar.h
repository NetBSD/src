/*	$NetBSD: ppbvar.h,v 1.2.16.2 2017/12/03 11:37:28 jdolecek Exp $	*/

/*
 * Copyright (c) 1996, 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ppbvar.h,v 1.2.16.2 2017/12/03 11:37:28 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/evcnt.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>
#include <dev/pci/pcidevs.h>

struct ppb_softc {
	device_t sc_dev;		/* generic device glue */
	pci_chipset_tag_t sc_pc;	/* our PCI chipset... */
	pcitag_t sc_tag;		/* ...and tag. */
	pci_intr_handle_t *sc_pihp;
	void *sc_intrhand;
	int sc_pciecapoff;
	struct evcnt sc_ev_intr;	/* interrupts */
	struct evcnt sc_ev_abp;		/*  Attention Button Pressed */
	struct evcnt sc_ev_pfd;		/*  Power Fault Detected */
	struct evcnt sc_ev_msc;		/*  MRL Sensor Changed */
	struct evcnt sc_ev_pdc;		/*  Presence Detect Changed */
	struct evcnt sc_ev_cc;		/*  Command Completed */
	struct evcnt sc_ev_lacs;	/*  Data Link Layer State Changed */

	pcireg_t sc_pciconfext[48];
};
