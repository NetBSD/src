/*	$NetBSD: pci_machdep.c,v 1.4.2.3 2002/04/17 00:04:44 nathanw Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Machine-specific functions for PCI autoconfiguration.
 *
 * On PCs, there are two methods of generating PCI configuration cycles.
 * We try to detect the appropriate mechanism for this machine and set
 * up a few function pointers to access the correct method directly.
 *
 * The configuration method can be hard-coded in the config file by
 * using `options PCI_CONF_MODE=N', where `N' is the configuration mode
 * as defined section 3.6.4.1, `Generating Configuration Cycles'.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#define _GALAXY_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#include <machine/walnut.h>
#include <powerpc/ibm4xx/ibm405gp.h>

static bus_space_tag_t pci_iot;
static bus_space_handle_t pci_ioh;

void pci_machdep_init(void)
{

	if (pci_ioh == 0) {
		pci_iot = 0;
		if (bus_space_map(pci_iot, PCIC0_BASE, 8, 0, &pci_ioh)){
			panic("Cannot map PCI registers\n");
		}
	}
}

void
pci_attach_hook(struct device *parent, struct device *self,
		struct pcibus_attach_args *pba)
{

#ifdef PCI_CONFIGURE_VERBOSE
	printf("pci_attach_hook\n");
	galaxy_show_pci_map();
#endif
	galaxy_setup_pci();
#ifdef PCI_CONFIGURE_VERBOSE
	galaxy_show_pci_map();
#endif
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	/*
	 * Bus number is irrelevant.  Configuration Mechanism 1 is in
	 * use, can have devices 0-32 (i.e. the `normal' range).
	 */
	return 5;
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	/* XXX magic number */
	tag = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);

	return tag;
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	pcireg_t data;

	/* 405GT BIOS disables interrupts here. Should we? --Art */
	bus_space_write_4(pci_iot, pci_ioh, PCIC0_CFGADDR, tag | reg);
	data = bus_space_read_4(pci_iot, pci_ioh, PCIC0_CFGDATA);
	bus_space_write_4(pci_iot, pci_ioh, PCIC0_CFGADDR, 0); /* 405GP pass2 errata #6 */
	return data;
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	bus_space_write_4(pci_iot, pci_ioh, PCIC0_CFGADDR, tag | reg);
	bus_space_write_4(pci_iot, pci_ioh, PCIC0_CFGDATA, data);
	bus_space_write_4(pci_iot, pci_ioh, PCIC0_CFGADDR, 0); /* 405GP pass2 errata #6 */
}


int
pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin;
	int dev = pa->pa_device;

	if (pin == 0) {
		/* No IRQ used. */
		goto bad;
	}

	if (pin > 4) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	/*
	 * We need to map the interrupt pin to the interrupt bit in the UIC
	 * associated with it.  This is highly machine-dependent.
	 */
	switch(dev) {
	case 1:
	case 2:
	case 3:
	case 4:
		*ihp = 27 + dev;
		break;
	default:
		printf("Hmm.. PCI device %d should not exist on this board\n",
			dev);
		goto bad;
	}
	return 0;

bad:
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char irqstr[8];		/* 4 + 2 + NUL + sanity */

	if (ih == 0 || ih >= ICU_LEN)
		panic("pci_intr_string: bogus handle 0x%x\n", ih);

	sprintf(irqstr, "irq %d", ih);
	return (irqstr);
	
}

const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
		   int (*func)(void *), void *arg)
{

	if (ih == 0 || ih >= ICU_LEN)
		panic("pci_intr_establish: bogus handle 0x%x\n", ih);

	return intr_establish(ih, IST_LEVEL, level, func, arg);
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	intr_disestablish(cookie);
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin,
		   int swiz, int *iline)
{

	if (bus == 0) {
		switch(dev) {
		case 1:
		case 2:
		case 3:
		case 4:
			*iline = 31 - dev;
		}
	} else {
		*iline = 20 + ((swiz + dev + 1) & 3);
	}
}

/* Avoid overconfiguration */
int
pci_conf_hook(pci_chipset_tag_t pc, int bus, int dev, int func, pcireg_t id)
{

	if ((PCI_VENDOR(id) == PCI_VENDOR_IBM && PCI_PRODUCT(id) == PCI_PRODUCT_IBM_405GP) ||
	    (PCI_VENDOR(id) == PCI_VENDOR_INTEL && PCI_PRODUCT(id) == PCI_PRODUCT_INTEL_80960_RP)) {
		/* Don't configure the bridge and PCI probe. */
		return 0;
	}
	return PCI_CONF_ALL & ~PCI_CONF_MAP_ROM;
}
