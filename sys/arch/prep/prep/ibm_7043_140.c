/*	$NetBSD: ibm_7043_140.c,v 1.2 2003/07/15 02:54:52 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibm_7043_140.c,v 1.2 2003/07/15 02:54:52 lukem Exp $");

#include "opt_openpic.h"
#if !defined(OPENPIC)
#error RS/6000 43P 7043-140 require "options OPENPIC" in kernel config file!
#endif

#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/intr.h>
#include <machine/platform.h>

#include <powerpc/openpic.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

void pci_intr_fixup_ibm_7043_140(int, int, int *);
void init_intr_mpic(void);

struct platform platform_ibm_7043_140 = {
	"IBM Model 7042/7043 (ED)",		/* model */
	platform_generic_match,			/* match */
	prep_pci_get_chipset_tag_indirect,	/* pci_get_chipset_tag */
	pci_intr_fixup_ibm_7043_140,		/* pci_intr_fixup */
	init_intr_mpic,				/* init_intr */
	cpu_setup_unknown,			/* cpu_setup */
	reset_prep_generic,			/* reset */
	obiodevs_nodev,				/* obiodevs */
};

void
pci_intr_fixup_ibm_7043_140(int bus, int dev, int *line)
{

	if (*line >= 1 && *line < OPENPIC_INTR_NUM - 3)
		*line += I8259_INTR_NUM;
}

void
init_intr_mpic(void)
{
	unsigned char *baseaddr = (unsigned char *)0xC0006800;	/* XXX */
#if NPCI > 0
	struct prep_pci_chipset pc;
	pcitag_t tag;
	pcireg_t id, address;

	(*platform->pci_get_chipset_tag)(&pc);

	tag = pci_make_tag(&pc, 0, 13, 0);
	id = pci_conf_read(&pc, tag, PCI_ID_REG);

	if (PCI_VENDOR(id) == PCI_VENDOR_IBM
	    && PCI_PRODUCT(id) == PCI_PRODUCT_IBM_MPIC) {
		address = pci_conf_read(&pc, tag, 0x10);
		if ((address & PCI_MAPREG_TYPE_MASK) == PCI_MAPREG_TYPE_MEM) {
			address &= PCI_MAPREG_MEM_ADDR_MASK;
			baseaddr = (unsigned char *)(PREP_BUS_SPACE_MEM | address);
		}
	}
#endif

	openpic_init(baseaddr);
}
