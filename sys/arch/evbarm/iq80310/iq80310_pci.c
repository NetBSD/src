/*	$NetBSD: iq80310_pci.c,v 1.11.12.1 2014/08/20 00:02:55 tls Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * IQ80310 PCI interrupt support, using he i80312 Companion I/O chip.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iq80310_pci.c,v 1.11.12.1 2014/08/20 00:02:55 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <evbarm/iq80310/iq80310reg.h>
#include <evbarm/iq80310/iq80310var.h>

#include <arm/xscale/i80312reg.h>
#include <arm/xscale/i80312var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

int	iq80310_pci_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *iq80310_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *iq80310_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*iq80310_pci_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	iq80310_pci_intr_disestablish(void *, void *);

void
iq80310_pci_init(pci_chipset_tag_t pc, void *cookie)
{

	pc->pc_intr_v = cookie;		/* the i80312 softc */
	pc->pc_intr_map = iq80310_pci_intr_map;
	pc->pc_intr_string = iq80310_pci_intr_string;
	pc->pc_intr_evcnt = iq80310_pci_intr_evcnt;
	pc->pc_intr_establish = iq80310_pci_intr_establish;
	pc->pc_intr_disestablish = iq80310_pci_intr_disestablish;
}

#if defined(IOP310_TEAMASA_NPWR)
int
iq80310_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct i80312_softc *sc = pa->pa_pc->pc_intr_v;
	pcireg_t reg;
	int sbus;

	/*
	 * The Npwr routes #INTA of the on-board PCI devices directly
	 * through the CPLD.  There is no PCI-PCI bridge and no PCI
	 * slots on the Npwr.
	 *
	 * We also expect the devices to be on the Secondary side of
	 * the i80312.
	 */

	reg = bus_space_read_4(sc->sc_st, sc->sc_ppb_sh, PPB_REG_BUSINFO);
	sbus = PPB_BUSINFO_SECONDARY(reg);

	if (pa->pa_bus != sbus) {
		printf("iq80310_pci_intr_map: %d/%d/%d not on Secondary bus\n",
		    pa->pa_bus, pa->pa_device, pa->pa_function);
		return (1);
	}

	switch (pa->pa_device) {
	case 5:		/* LSI 53c1010 SCSI */
		*ihp = XINT3_IRQ(2);
		break;
	case 6:		/* Intel i82544GC Gig-E #1 */
		*ihp = XINT3_IRQ(1);
		break;
	case 7:		/* Intel i82544GC Gig-E #2 */
		*ihp = XINT3_IRQ(4);
		break;
	default:
		printf("iq80310_pci_intr_map: no mapping for %d/%d/%d\n",
		    pa->pa_bus, pa->pa_device, pa->pa_function);
		return (1);
	}

	return (0);
}
#else /* Default to stock IQ80310 */
int
iq80310_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct i80312_softc *sc = pa->pa_pc->pc_intr_v;
	pcitag_t tag;
	pcireg_t reg;
	int sbus, pbus;

	/*
	 * Mapping of PCI interrupts on the IQ80310 is pretty easy; there
	 * is a single interrupt line for all PCI devices on pre-F boards,
	 * and an interrupt line for each INTx# signal on F and later boards.
	 *
	 * The only exception is the on-board Ethernet; this devices has
	 * its own dedicated interrupt line.  The location of this device
	 * looks like this:
	 *
	 *	80312 Secondary -> PPB at dev #7 -> i82559 at dev #0
	 *
	 * In order to determine if we're mapping the interrupt for the
	 * on-board Ethernet, we must read the Secondary Bus # of the
	 * i80312, then use that to read the Secondary Bus # of the
	 * 21154 PPB.  At that point, we know that b/d/f of the i82559,
	 * and can determine if we're looking at that device.
	 */

	reg = bus_space_read_4(sc->sc_st, sc->sc_ppb_sh, PPB_REG_BUSINFO);
	pbus = PPB_BUSINFO_PRIMARY(reg);
	sbus = PPB_BUSINFO_SECONDARY(reg);

	/*
	 * XXX We don't know how to map interrupts on the Primary
	 * XXX PCI bus right now.
	 */
	if (pa->pa_bus == pbus) {
		printf("iq80310_pci_intr_map: can't map interrupts on "
		    "Primary bus\n");
		return (1);
	}

	tag = pci_make_tag(pa->pa_pc, sbus, 7, 0);

	/* Make sure the PPB is there. */
	reg = pci_conf_read(pa->pa_pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(reg) == PCI_VENDOR_INVALID ||
	    PCI_VENDOR(reg) == 0) {
		/*
		 * That's odd... no PPB there?  Oh well, issue a warning
		 * and continue on.
		 */
		printf("iq80310_pci_intr_map: PPB not found at %d/%d/%d ??\n",
		    sbus, 7, 0);
		goto pinmap;
	}
	
	/* Make sure the device that's there is a PPB. */
	reg = pci_conf_read(pa->pa_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(reg) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(reg) != PCI_SUBCLASS_BRIDGE_PCI) {
		/*
		 * That's odd... the device that's there isn't a PPB.
		 * Oh well, issue a warning and continue on.
		 */
		printf("iq80310_pci_intr_map: %d/%d/%d isn't a PPB ??\n",
		    sbus, 7, 0);
		goto pinmap;
	}

	/* Now read the PPB's secondary bus number. */
	reg = pci_conf_read(pa->pa_pc, tag, PPB_REG_BUSINFO);
	sbus = PPB_BUSINFO_SECONDARY(reg);

	if (pa->pa_bus == sbus && pa->pa_device == 0 &&
	    pa->pa_function == 0) {
		/* On-board i82559 Ethernet! */
		*ihp = XINT3_IRQ(XINT3_ETHERNET);
		return (0);
	}

 pinmap:
	if (pa->pa_intrpin == 0) {
		/* No IRQ used. */
		return (1);
	}
	if (pa->pa_intrpin > 4) {
		printf("iq80310_pci_intr_map: bad interrupt pin %d\n",
		    pa->pa_intrpin);
		return (1);
	}

	/* INTD# is always in XINT3. */
	if (pa->pa_intrpin == 4) {
		*ihp = XINT3_IRQ(XINT3_SINTD);
		return (0);
	}

	/* On pre-F boards, ALL of them are on XINT3. */
	if (/*pre-F*/0)
		*ihp = XINT3_IRQ(XINT3_SINTD);
	else
		*ihp = XINT0_IRQ(pa->pa_intrpin - 1);

	return (0);
}
#endif /* list of IQ80310-based designs */

const char *
iq80310_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	snprintf(buf, len, "iq80310 irq %ld", ih);
	return buf;
}

const struct evcnt *
iq80310_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX For now. */
	return (NULL);
}

void *
iq80310_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{

	return (iq80310_intr_establish(ih, ipl, func, arg));
}

void
iq80310_pci_intr_disestablish(void *v, void *cookie)
{

	iq80310_intr_disestablish(cookie);
}
