/*	$NetBSD: pci_machdep.c,v 1.18 2006/05/14 21:56:33 elad Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.18 2006/05/14 21:56:33 elad Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _SGIMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI doesn't have any special needs; just use
 * the generic versions of these functions.
 */
struct sgimips_bus_dma_tag pci_bus_dma_tag = {
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync_mips3,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

void
pci_attach_hook(struct device *parent, struct device *self, struct pcibus_attach_args *pba)
{
	/* XXX */

	return;
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	if (busno == 0)
		return 5;	/* 2 on-board SCSI chips, slots 0, 1 and 2 */
	else
		return 32;	/* XXX */
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{

	return (bus << 16) | (device << 11) | (function << 8);
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

	return (*pc->pc_conf_read)(pc, tag, reg);
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	(*pc->pc_conf_write)(pc, tag, reg, data);
}

int
pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int pin = pa->pa_intrpin;
	int bus, dev, func, start;

	pci_decompose_tag(pc, intrtag, &bus, &dev, &func);

	if (dev < 3 && pin != PCI_INTERRUPT_PIN_A)
		panic("SCSI0 and SCSI1 must be hardwired!");

	switch (pin) {
	default:
	case PCI_INTERRUPT_PIN_NONE:
		return -1;

	case PCI_INTERRUPT_PIN_A:
		/*
		 * Each of SCSI{0,1}, & slots 0 - 2 has dedicated interrupt
		 * for pin A?
		 */
		*ihp = dev + 7;
		return 0;

	case PCI_INTERRUPT_PIN_B:
		start = 0;
		break;
	case PCI_INTERRUPT_PIN_C:
		start = 1;
		break;
	case PCI_INTERRUPT_PIN_D:
		start = 2;
		break;
	}

	/* Pins B,C,D are mapped to PCI_SHARED0 - PCI_SHARED2 interrupts */
	*ihp = 13 /* PCI_SHARED0 */ + (start + dev - 3) % 3;
	return 0;
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char irqstr[32];

	sprintf(irqstr, "crime interrupt %d", ih);
	return irqstr;
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

	return (void *)(pc->intr_establish)(ih, 0, func, arg);
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	(pc->intr_disestablish)(cookie);
}

#ifdef PCI_NETBSD_CONFIGURE
void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin, int swiz,
    int *iline)
{

	return;
}
#endif
