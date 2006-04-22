/*      $NetBSD: pci_machdep.c,v 1.7.4.1 2006/04/22 11:38:11 simonb Exp $      */

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/bus_private.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "locators.h"
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

/*
 * We can't use the generic pci_enumerate_bus, because the hypervisor
 * may hide the function 0 from us, while other functions are
 * available
 */
int
xen_pci_enumerate_bus(struct pci_softc *sc, const int *locators,
    int (*match)(struct pci_attach_args *), struct pci_attach_args *pap)
{
	pci_chipset_tag_t pc = sc->sc_pc;
	int device, function, nfunctions, ret;
	const struct pci_quirkdata *qd;
	pcireg_t id, bhlcr;
	pcitag_t tag;

	for (device = 0; device < sc->sc_maxndevs; device++)
	{
		if ((locators[PCICF_DEV] != PCICF_DEV_DEFAULT) &&
		    (locators[PCICF_DEV] != device))
			continue;

		tag = pci_make_tag(pc, sc->sc_bus, device, 0);

		bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
		id = pci_conf_read(pc, tag, PCI_ID_REG);
		qd = NULL;

		if (PCI_VENDOR(id) != PCI_VENDOR_INVALID) {
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(id) == 0)
				continue;
			
			if (PCI_HDRTYPE_TYPE(bhlcr) > 2)
				continue;

			qd = pci_lookup_quirkdata(PCI_VENDOR(id),
			    PCI_PRODUCT(id));

			if (qd != NULL &&
			      (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0)
				nfunctions = 8;
			else if (qd != NULL &&
			      (qd->quirks & PCI_QUIRK_MONOFUNCTION) != 0)
				nfunctions = 1;
			else
				nfunctions = PCI_HDRTYPE_MULTIFN(bhlcr) ? 8 : 1;
		} else {
			/*
			 * Vendor ID invalid. This may be because there's no
			 * device, or because the hypervisor is hidding
			 * function 0 from us. Try to probe other functions
			 * anyway.
			 */
			nfunctions = 8;
		}

		for (function = 0; function < nfunctions; function++) {
			if ((locators[PCICF_FUNCTION] != PCICF_FUNCTION_DEFAULT)
			    && (locators[PCICF_FUNCTION] != function))
				continue;

			if (qd != NULL &&
			    (qd->quirks & PCI_QUIRK_SKIP_FUNC(function)) != 0)
				continue;
			tag = pci_make_tag(pc, sc->sc_bus, device, function);
			ret = pci_probe_device(sc, tag, match, pap);
			if (match != NULL && ret != 0)
				return (ret);
		}
	}
	return (0);
}
