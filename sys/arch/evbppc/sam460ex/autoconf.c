/*	$NetBSD: autoconf.c,v 1.5 2026/09/28 20:41:00 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Copyright 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima for The NetBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * Autoconfiguration for the ACube Sam460ex (AMCC 460EX).
 * Modeled on evbppc/obs405/obs600_autoconf.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.5 2026/09/28 20:41:00 rkujawa Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <machine/sam460ex.h>

#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/pci_machdep.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/* PCIe INTA of a root port. */
int
pciex_inta_irq(int port)
{
	return SAM460EX_IRQ_PCIE_INTA(port);
}

#define	UIC_IRQ_BIT(irq)	__BIT(31 - ((irq) % SAM460EX_UIC_NIRQ))
#define	UIC_IRQ_BITS(lo, hi)	__BITS(31 - ((hi) % SAM460EX_UIC_NIRQ),	\
				       31 - ((lo) % SAM460EX_UIC_NIRQ))
#define	UIC3_PCIE_INTX		UIC_IRQ_BITS(SAM460EX_IRQ_PCIE_INTX,	\
				    SAM460EX_IRQ_PCIE_INTX + SAM460EX_NPCIE_INTX - 1)
#define	UIC3_SM502		UIC_IRQ_BIT(SAM460EX_IRQ_SM502)

/* when console is on a PCI card, see device_register() */
static device_t sam460ex_pci_console_dev;
static int sam460ex_pci_console_bdf[3];
static pcireg_t sam460ex_pci_console_id;

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{

	/* UIC1 */
	mtdcr(DCR_UIC1_BASE + DCR_UIC_PR,
	    mfdcr(DCR_UIC1_BASE + DCR_UIC_PR) & ~UIC_IRQ_BIT(SAM460EX_IRQ_PCI_INTX));
	mtdcr(DCR_UIC1_BASE + DCR_UIC_TR,
	    mfdcr(DCR_UIC1_BASE + DCR_UIC_TR) & ~UIC_IRQ_BIT(SAM460EX_IRQ_PCI_INTX));
	mtdcr(DCR_UIC1_BASE + DCR_UIC_SR, UIC_IRQ_BIT(SAM460EX_IRQ_PCI_INTX));
	/* UIC3 */
	mtdcr(DCR_UIC3_BASE + DCR_UIC_PR,
	    (mfdcr(DCR_UIC3_BASE + DCR_UIC_PR) | UIC3_PCIE_INTX) & ~UIC3_SM502);
	mtdcr(DCR_UIC3_BASE + DCR_UIC_TR,
	    mfdcr(DCR_UIC3_BASE + DCR_UIC_TR) & ~(UIC3_PCIE_INTX | UIC3_SM502));
	mtdcr(DCR_UIC3_BASE + DCR_UIC_SR, UIC3_PCIE_INTX | UIC3_SM502);

	/* Cascaded UICs: irq = 32 * n + input. */
	intr_init();
	pic_add(&pic_uic1);
	pic_add(&pic_uic2);
	pic_add(&pic_uic3);

	if (config_rootfound("plb", NULL) == NULL)
		panic("configure: mainbus not configured");

	if (sam460ex_pci_console_dev != NULL)
		aprint_normal("sam460ex: console=pci: %s at %d:%d:%d "
		    "(vendor 0x%04x product 0x%04x)\n",
		    device_xname(sam460ex_pci_console_dev),
		    sam460ex_pci_console_bdf[0], sam460ex_pci_console_bdf[1],
		    sam460ex_pci_console_bdf[2],
		    PCI_VENDOR(sam460ex_pci_console_id),
		    PCI_PRODUCT(sam460ex_pci_console_id));
	else if (sam460ex_console == SAM460EX_CONS_PCI)
		printf("sam460ex: console=pci: no suitable PCI display found, "
		    "using serial\n");

	pic_finish_setup();

	genppc_cpu_configure();
}

void
device_register(device_t dev, void *aux)
{

	ibm4xx_device_register(dev, aux, sam460ex_com_freq());

	/* console=sm502: the on-board SM502 voyagerfb becomes the console */
	if (sam460ex_console == SAM460EX_CONS_SM502 &&
	    device_is_a(dev, "voyagerfb"))
		prop_dictionary_set_bool(device_properties(dev),
		    "is_console", true);

	/* console=pci: find first/matching non-SM502 PCI display */
	if (sam460ex_console == SAM460EX_CONS_PCI &&
	    sam460ex_pci_console_dev == NULL && device_parent(dev) != NULL &&
	    device_is_a(device_parent(dev), "pci")) {
		struct pci_attach_args *pa = aux;
		const int *bdf = sam460ex_console_pci_bdf;

		if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
		    !(PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SILMOTION &&
		      PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SILMOTION_SM502) &&
		    (bdf[0] == -1 || bdf[0] == pa->pa_bus) &&
		    (bdf[1] == -1 || bdf[1] == pa->pa_device) &&
		    (bdf[2] == -1 || bdf[2] == pa->pa_function)) {
			/* announced later; a print here splits the "at" line */
			sam460ex_pci_console_dev = dev;
			sam460ex_pci_console_bdf[0] = pa->pa_bus;
			sam460ex_pci_console_bdf[1] = pa->pa_device;
			sam460ex_pci_console_bdf[2] = pa->pa_function;
			sam460ex_pci_console_id = pa->pa_id;
			prop_dictionary_set_bool(device_properties(dev),
			    "is_console", true);
		}
	}

	/* Match a root= from the bootargs */
	if (bootspec != NULL) {
		size_t len = strlen(bootspec);
		int part = 0;

		if (len > 1 && bootspec[len - 1] >= 'a' &&
		    bootspec[len - 1] < 'a' + MAXPARTITIONS &&
		    bootspec[len - 2] >= '0' && bootspec[len - 2] <= '9') {
			part = bootspec[len - 1] - 'a';
			len--;
		}
		if (strncmp(device_xname(dev), bootspec, len) == 0 &&
		    device_xname(dev)[len] == '\0') {
			booted_device = dev;
			booted_partition = part;
		}
	}
}

