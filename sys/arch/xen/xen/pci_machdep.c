/*      $NetBSD: pci_machdep.c,v 1.1.2.1 2005/01/18 15:09:04 bouyer Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
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
 *
 */

#include <sys/null.h>
#include <sys/types.h>
#include <sys/systm.h>

#define _X86_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <machine/evtchn.h>

#include "opt_ddb.h"

/* mask of already-known busses */
u_int32_t pci_bus_attached[256 / 32];

struct x86_bus_dma_tag pci_bus_dma_tag = {
	0,
	0,
	0,
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

void
pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args * pba)
{
	int bus = pba->pba_bus;

	pci_bus_attached[bus / 32] |= 1 << (bus & 0x1f);
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return 32;
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pcitag, int bus, int dev, int func)
{
	pcitag_t tag;

	tag.bus = bus;
	tag.device = dev;
	tag.function = func;

	return tag;
}

void
pci_decompose_tag(pci_chipset_tag_t pcitag, pcitag_t tag,
    int *busp, int *devp, int *funcp)
{
	*busp = tag.bus;
	*devp = tag.device;
	*funcp = tag.function;
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pcitag, pcitag_t dev, int reg)
{
	physdev_op_t physdev_op;

	physdev_op.cmd = PHYSDEVOP_PCI_CFGREG_READ;
	pci_decompose_tag(pcitag, dev,
	    &physdev_op.u.pci_cfgreg_read.bus,
	    &physdev_op.u.pci_cfgreg_read.dev,
	    &physdev_op.u.pci_cfgreg_read.func);
	physdev_op.u.pci_cfgreg_read.reg = reg;
	physdev_op.u.pci_cfgreg_read.len = 4;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0) {
		/*
		 * this can be a valid condition, if we're reading a
		 * nonexistent device. I guess the real hardware
		 * return 0xffffffff in this case.
		 */
		return 0xffffffff;
	}
	return physdev_op.u.pci_cfgreg_read.value;
}

void
pci_conf_write(pci_chipset_tag_t pcitag, pcitag_t dev, int reg, pcireg_t val)
{
	physdev_op_t physdev_op;

	physdev_op.cmd = PHYSDEVOP_PCI_CFGREG_WRITE;
	pci_decompose_tag(pcitag, dev,
	    &physdev_op.u.pci_cfgreg_write.bus,
	    &physdev_op.u.pci_cfgreg_write.dev,
	    &physdev_op.u.pci_cfgreg_write.func);
	physdev_op.u.pci_cfgreg_write.reg = reg;
	physdev_op.u.pci_cfgreg_write.len = 4;
	physdev_op.u.pci_cfgreg_write.value = val;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0) {
		printf("HYPERVISOR_physdev_op(PCI_CFGREG_WRITE) failed\n");
#ifdef DDB
		Debugger();
#endif
	}
}

int
pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	physdev_op_t physdev_op;
	pcireg_t intr;
	int pin;
	int line;

	/* initialise device, to get the real IRQ */
	physdev_op.cmd = PHYSDEVOP_PCI_INITIALISE_DEVICE;
	physdev_op.u.pci_initialise_device.bus = pa->pa_bus;
	physdev_op.u.pci_initialise_device.dev = pa->pa_device;
	physdev_op.u.pci_initialise_device.func = pa->pa_function;
	if (HYPERVISOR_physdev_op(&physdev_op) < 0)
		panic("HYPERVISOR_physdev_op(PHYSDEVOP_PCI_INITIALISE_DEVICE)");

	intr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG);
	pin = pa->pa_intrpin;
	pa->pa_intrline = line = PCI_INTERRUPT_LINE(intr);
	printf("pci_intr_map: pin %d line %d\n", pin, line);
#if 0 /* XXXX why is it always 0 ? */
	if (pin == 0) {
		/* No IRQ used */
		goto bad;
	}
#endif
	if (pin > PCI_INTERRUPT_PIN_MAX) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	if (line == 0 || line == X86_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		printf("pci_intr_map: no mapping for pin %c (line=%02x)\n",
		    '@' + pin, line);
		goto bad;
	}

	ihp->pirq = line;
	ihp->irq = bind_pirq_to_irq(ihp->pirq);
	printf("pci_intr_map: irq %d for pirq %d\n", ihp->irq, ihp->pirq);
	if (ihp->irq == -1)
		goto bad;

	return 0;

bad:
	ihp->pirq = -1;
	ihp->irq = -1;
	return 1;
}
const char
*pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char buf[64];
	snprintf(buf, 64, "physical irq %d, irq %d", ih.pirq, ih.irq);
	return buf;
	
}

const struct evcnt*
pci_intr_evcnt(pci_chipset_tag_t pcitag, pci_intr_handle_t intrh)
{
	return NULL;
}

void *
pci_intr_establish(pci_chipset_tag_t pcitag, pci_intr_handle_t intrh,
    int level, int (*func)(void *), void *arg)
{
	return (void *)pirq_establish(intrh.pirq, intrh.irq, func, arg, level);
}

void
pci_intr_disestablish(pci_chipset_tag_t pcitag, void *cookie)
{
}
