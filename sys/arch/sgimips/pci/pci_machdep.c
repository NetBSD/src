/*	$NetBSD: pci_machdep.c,v 1.7 2003/07/15 03:35:54 lukem Exp $	*/

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
 *          NetBSD Project.  See http://www.netbsd.org/ for
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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.7 2003/07/15 03:35:54 lukem Exp $");

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
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

void
pci_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
	/* XXX */

	return;
}

int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{
	return 5;	/* 2 on-board SCSI chips, slots 0, 1 and 2 */
}

pcitag_t
pci_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	return (bus << 16) | (device << 11) | (function << 8);
}

void
pci_decompose_tag(pc, tag, bp, dp, fp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int *bp, *dp, *fp;
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	return (*pc->pc_conf_read)(pc, tag, reg);
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	(*pc->pc_conf_write)(pc, tag, reg, data);
}

int
pci_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int pin = pa->pa_intrpin;
	int bus, dev, func, start;

	pci_decompose_tag(pc, intrtag, &bus, &dev, &func);

	if (dev < 3 && pin != PCI_INTERRUPT_PIN_A)
		panic("SCSI0 and SCSI1 must be hardwired!");
		
	switch (pin) {
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
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char irqstr[32];

	sprintf(irqstr, "crime interrupt %d", ih);
	return irqstr;
}

const struct evcnt *
pci_intr_evcnt(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

extern void *	crime_intr_establish(int, int, int, int (*)(void *), void *);

void *
pci_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level, (*func)(void *);
	void *arg;
{
	return crime_intr_establish(ih, 0, 0, func, arg);
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	panic("pci_intr_disestablish: not implemented");

	return;
}
