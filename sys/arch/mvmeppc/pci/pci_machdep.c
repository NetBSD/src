/*	$NetBSD: pci_machdep.c,v 1.3 2003/03/18 16:40:21 matt Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/platform.h>

#include <powerpc/pio.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#define	PCI_MODE1_ENABLE	0x80000000UL
#define	PCI_MODE1_ADDRESS_REG	(MVMEPPC_KVA_BASE_IO + 0xcf8)
#define	PCI_MODE1_DATA_REG	(MVMEPPC_KVA_BASE_IO + 0xcfc)

#define	o2i(off)	((off)/sizeof(pcireg_t))

void pci_intr_fixup(int, int, int *);

#ifdef DEBUG
#define	DPRF(x)	printf x
#else
#define	DPRF(x)
#endif

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
struct powerpc_bus_dma_tag pci_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,			/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

void
pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{

	/* Nothing to do. */
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	/*
	 * Bus number is irrelevant.  Configuration Mechanism 1 is in
	 * use, can have devices 0-32 (i.e. the `normal' range).
	 */
	return (32);
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag = PCI_MODE1_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
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
		*fp = (tag >> 8) & 0x7;
	return;
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	pcireg_t data;

	out32rb(PCI_MODE1_ADDRESS_REG, tag | reg);
	data = in32rb(PCI_MODE1_DATA_REG);
	out32rb(PCI_MODE1_ADDRESS_REG, 0);
	return data;
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	out32rb(PCI_MODE1_ADDRESS_REG, tag | reg);
	out32rb(PCI_MODE1_DATA_REG, data);
	out32rb(PCI_MODE1_ADDRESS_REG, 0);
}

int
pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin;
	int line = pa->pa_intrline;

	if (pin == 0) {
		/* No IRQ used. */
		goto bad;
	}

	if (pin > 4) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	/*
	* Section 6.2.4, `Miscellaneous Functions', says that 255 means
	* `unknown' or `no connection' on a PC.  We assume that a device with
	* `no connection' either doesn't have an interrupt (in which case the
	* pin number should be 0, and would have been noticed above), or
	* wasn't configured by the BIOS (in which case we punt, since there's
	* no real way we can know how the interrupt lines are mapped in the
	* hardware).
	*
	* XXX
	* Since IRQ 0 is only used by the clock, and we can't actually be sure
	* that the BIOS did its job, we also recognize that as meaning that
	* the BIOS has not configured the device.
	*/
	if (line == 0 || line == 255) {
		printf("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		goto bad;
	} else {
		if (line >= ICU_LEN) {
			printf("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
		if (line == IRQ_SLAVE) {
			printf("pci_intr_map: changed line 2 to line 9\n");
			line = 9;
		}
	}

	*ihp = line;
	return 0;

bad:
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == 0 || ih >= ICU_LEN || ih == IRQ_SLAVE)
		panic("pci_intr_string: bogus handle 0x%x", ih);

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

	if (ih == 0 || ih >= ICU_LEN || ih == IRQ_SLAVE)
		panic("pci_intr_establish: bogus handle 0x%x", ih);

	return isa_intr_establish(NULL, ih, IST_LEVEL, level, func, arg);
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	isa_intr_disestablish(NULL, cookie);
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin,
    int swiz, int *iline)
{

	(*platform->pci_intr_fixup)(bus, dev, iline);
}
