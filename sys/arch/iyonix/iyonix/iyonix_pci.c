/*	$NetBSD: iyonix_pci.c,v 1.3.32.1 2007/05/22 17:27:02 matt Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Based on code written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 * Iyonix PCI interrupt support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iyonix_pci.c,v 1.3.32.1 2007/05/22 17:27:02 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <iyonix/iyonix/iyonixreg.h>
#include <iyonix/iyonix/iyonixvar.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

#include <sys/extent.h>
#include <dev/pci/pciconf.h>

int	iyonix_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *iyonix_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *iyonix_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*iyonix_pci_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	iyonix_pci_intr_disestablish(void *, void *);
void pci_conf_write_byte(pci_chipset_tag_t, pcitag_t, int, int);
int pci_conf_read_byte(pci_chipset_tag_t, pcitag_t, int);

void
iyonix_pci_init(pci_chipset_tag_t pc, void *cookie)
{

	pc->pc_intr_v = cookie;		/* the i80321 softc */
	pc->pc_intr_map = iyonix_pci_intr_map;
	pc->pc_intr_string = iyonix_pci_intr_string;
	pc->pc_intr_evcnt = iyonix_pci_intr_evcnt;
	pc->pc_intr_establish = iyonix_pci_intr_establish;
	pc->pc_intr_disestablish = iyonix_pci_intr_disestablish;
}

int
iyonix_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct i80321_softc *sc = pa->pa_pc->pc_intr_v;
	int b, d, f;
	uint32_t busno;

	busno = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, ATU_PCIXSR);
	busno = PCIXSR_BUSNO(busno);
	if (busno == 0xff)
		busno = 0;

	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag, &b, &d, &f);

	/* No mappings for devices not on our bus. */
	if (b != busno)
		goto no_mapping;

	/*
	 *  XXX We currently deal only with the southbridge and with
	 *      regular PCI. IOC devices may need further attention.
	 */

	/* Devices on the southbridge are all routed through xint 1 */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_ALI) {
		switch (PCI_PRODUCT(pa->pa_id)) {
			case PCI_PRODUCT_ALI_M1543: /* Southbridge */
			case PCI_PRODUCT_ALI_M5229: /* ATA */
			case PCI_PRODUCT_ALI_M5237: /* ohci */
			case PCI_PRODUCT_ALI_M5257: /* Modem */
			case PCI_PRODUCT_ALI_M5451: /* AC97 */
			case PCI_PRODUCT_ALI_M7101: /* PMC */
				*ihp = ICU_INT_XINT(1);
				return (0);
		}
	}

	/* Route other interrupts with default swizzling rule */
	*ihp = ICU_INT_XINT((d + pa->pa_intrpin - 1) % 4);
	return 0;

 no_mapping:
	printf("iyonix_pci_intr_map: no mapping for %d/%d/%d\n",
	    pa->pa_bus, pa->pa_device, pa->pa_function);
	return (1);
}

const char *
iyonix_pci_intr_string(void *v, pci_intr_handle_t ih)
{

	return (i80321_irqnames[ih]);
}

const struct evcnt *
iyonix_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX For now. */
	return (NULL);
}

void *
iyonix_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{

	return (i80321_intr_establish(ih, ipl, func, arg));
}

void
iyonix_pci_intr_disestablish(void *v, void *cookie)
{

	i80321_intr_disestablish(cookie);
}

void
pci_conf_write_byte(pci_chipset_tag_t pc, pcitag_t tag, int addr, int value)
{
	int temp;
	temp = pci_conf_read(pc, tag, addr&~3);
	temp = temp & ~(0xff << ((addr%4) * 8));
	temp = temp | (value << ((addr%4) * 8));
	pci_conf_write(pc, tag, addr&~3, temp);
}

int
pci_conf_read_byte(pci_chipset_tag_t pc, pcitag_t tag, int addr)
{
	int temp;
	temp = pci_conf_read(pc, tag, addr&~3);
	temp = temp >> ((addr%4) * 8);
	temp = temp & 0xff;
	return temp;
}

int
pci_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{

	/*
	 * We need to disable devices in the Southbridge, and as
	 * we have all the tags we need at this point, this is
	 * where we do it.
	 */
	if (PCI_VENDOR(id) == PCI_VENDOR_ALI &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_ALI_M1543)
	{
		pcitag_t tag;
		int status;
		pci_chipset_tag_t pc = (pci_chipset_tag_t) v;

		tag = pci_make_tag(pc, bus, dev, func);

		/* Undocumented magic */

		/* Disable USB */
		pci_conf_write_byte(pc, tag, 0x53, 0x40);
		pci_conf_write_byte(pc, tag, 0x52, 0x00);

		status = pci_conf_read_byte(pc, tag, 0x7e);
		pci_conf_write_byte(pc, tag, 0x7e, status & ~0x80);

		/* Disable modem */
		pci_conf_write_byte(pc, tag, 0x77, 1 << 6);

		/* Disable SCI */
		pci_conf_write_byte(pc, tag, 0x78, 1 << 7);
	}

	return (PCI_CONF_DEFAULT);
}
