/*	$NetBSD: pci_machdep.c,v 1.10.12.1 2017/12/03 11:36:36 jdolecek Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.10.12.1 2017/12/03 11:36:36 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/pci_machdep.h>
#include <powerpc/ibm4xx/dev/pcicreg.h>

static struct powerpc_bus_space pci_iot = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
	IBM405GP_PCIC0_BASE,		/* extent base */
	IBM405GP_PCIC0_BASE + 8,	/* extent limit */
};

static bus_space_handle_t pci_ioh;

void
ibm4xx_pci_machdep_init(void)
{

	if (pci_ioh == 0 &&
	   (bus_space_init(&pci_iot, "pcicfg", NULL, 0) ||
	    bus_space_map(&pci_iot, IBM405GP_PCIC0_BASE, 8, 0, &pci_ioh)))
		panic("Cannot map PCI registers");
}

void
ibm4xx_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{

#ifdef PCI_CONFIGURE_VERBOSE
	printf("pci_attach_hook\n");
	ibm4xx_show_pci_map();
#endif
	ibm4xx_setup_pci();
#ifdef PCI_CONFIGURE_VERBOSE
	ibm4xx_show_pci_map();
#endif
}

pcitag_t
ibm4xx_pci_make_tag(void *v, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	/* XXX magic number */
	tag = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);

	return tag;
}

void
ibm4xx_pci_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
ibm4xx_pci_conf_read(void *v, pcitag_t tag, int reg)
{
	pcireg_t data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	/* 405GT BIOS disables interrupts here. Should we? --Art */
	bus_space_write_4(&pci_iot, pci_ioh, PCIC_CFGADDR, tag | reg);
	data = bus_space_read_4(&pci_iot, pci_ioh, PCIC_CFGDATA);
	/* 405GP pass2 errata #6 */
	bus_space_write_4(&pci_iot, pci_ioh, PCIC_CFGADDR, 0);
	return data;
}

void
ibm4xx_pci_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	bus_space_write_4(&pci_iot, pci_ioh, PCIC_CFGADDR, tag | reg);
	bus_space_write_4(&pci_iot, pci_ioh, PCIC_CFGDATA, data);
	/* 405GP pass2 errata #6 */
	bus_space_write_4(&pci_iot, pci_ioh, PCIC_CFGADDR, 0);
}

int
ibm4xx_pci_intr_setattr(void *v, pci_intr_handle_t *ihp, int attr,
    uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		return 0;
	default:
		return ENODEV;
	}
}

/* Avoid overconfiguration */
int
ibm4xx_pci_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{

	if ((PCI_VENDOR(id) == PCI_VENDOR_IBM && PCI_PRODUCT(id) == PCI_PRODUCT_IBM_405GP) ||
	    (PCI_VENDOR(id) == PCI_VENDOR_INTEL && PCI_PRODUCT(id) == PCI_PRODUCT_INTEL_80960_RP)) {
		/* Don't configure the bridge and PCI probe. */
		return 0;
	}
	return PCI_CONF_DEFAULT;
}
