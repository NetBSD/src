/*	$NetBSD: pci_machdep.c,v 1.21.12.1 2017/12/03 11:35:59 jdolecek Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.21.12.1 2017/12/03 11:35:59 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/intr.h>

#include <powerpc/pio.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <machine/pci_machdep.h>

extern struct genppc_pci_chipset *genppc_pct;

#define	PCI_MODE1_ADDRESS_REG	(PREP_BUS_SPACE_IO + 0xcf8)
#define	PCI_MODE1_DATA_REG	(PREP_BUS_SPACE_IO + 0xcfc)

void
bebox_pci_get_chipset_tag(pci_chipset_tag_t pc)
{
	pc->pc_conf_v = (void *)pc;

	pc->pc_attach_hook = genppc_pci_indirect_attach_hook;
	pc->pc_bus_maxdevs = genppc_pci_bus_maxdevs;
	pc->pc_make_tag = genppc_pci_indirect_make_tag;
	pc->pc_conf_read = genppc_pci_indirect_conf_read;
	pc->pc_conf_write = genppc_pci_indirect_conf_write;

	pc->pc_intr_v = (void *)pc;

	pc->pc_intr_map = genppc_pci_intr_map;
	pc->pc_intr_string = genppc_pci_intr_string;
	pc->pc_intr_evcnt = genppc_pci_intr_evcnt;
	pc->pc_intr_establish = genppc_pci_intr_establish;
	pc->pc_intr_disestablish = genppc_pci_intr_disestablish;
	pc->pc_intr_setattr = genppc_pci_intr_setattr;
	pc->pc_intr_type = genppc_pci_intr_type;
	pc->pc_intr_alloc = genppc_pci_intr_alloc;
	pc->pc_intr_release = genppc_pci_intr_release;
	pc->pc_intx_alloc = genppc_pci_intx_alloc;

	pc->pc_msi_v = (void *)pc;
	genppc_pci_chipset_msi_init(pc);

	pc->pc_msix_v = (void *)pc;
	genppc_pci_chipset_msix_init(pc);

	pc->pc_conf_interrupt = bebox_pci_conf_interrupt;
	pc->pc_decompose_tag = genppc_pci_indirect_decompose_tag;
	pc->pc_conf_hook = genppc_pci_conf_hook;

	pc->pc_addr = mapiodev(PCI_MODE1_ADDRESS_REG, 4, false);
	pc->pc_data = mapiodev(PCI_MODE1_DATA_REG, 4, false);
	pc->pc_bus = 0;
	pc->pc_node = 0;
	pc->pc_memt = 0;
	pc->pc_iot = 0;
}

void
bebox_pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
    int *iline)
{

	if (bus == 0) {
		switch (dev) {
		case 12: /*       SCSI is bit 10, mapped to IRQ 26 */
		case 13: /* PCI slot 1 is bit 11, mapped to IRQ 27 */
		case 14: /* PCI slot 2 is bit 12, mapped to IRQ 28 */
		case 15: /* PCI slot 3 is bit 13, mapped to IRQ 29 */
#define  BEBOX_PCIBUS0_DEV2LINE(d)	((d) + 14)
			*iline = BEBOX_PCIBUS0_DEV2LINE(dev);
		}
	} else
#define  BEBOX_PCIBUS_SWIZ2LINE(s)	((s) + 14)
		*iline = BEBOX_PCIBUS_SWIZ2LINE(swiz);
}
