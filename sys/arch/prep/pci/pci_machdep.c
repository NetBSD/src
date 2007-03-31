/*	$NetBSD: pci_machdep.c,v 1.28.6.2 2007/03/31 15:45:40 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.28.6.2 2007/03/31 15:45:40 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/platform.h>
#include <machine/pnp.h>

#include <dev/isa/isavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

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

/* 0 == direct 1 == indirect */
int prep_pci_config_mode = 1;
extern struct prep_pci_chipset *prep_pct;

void
prep_pci_get_chipset_tag(pci_chipset_tag_t pc)
{
	int i;

	i = pci_chipset_tag_type();

	if (i == PCIBridgeIndirect || i == PCIBridgeRS6K) {
		prep_pci_config_mode = 1;
		prep_pci_get_chipset_tag_indirect(pc);
	} else if (i == PCIBridgeDirect) {
		prep_pci_get_chipset_tag_direct(pc);
		prep_pci_config_mode = 0;
	} else
		panic("Unknown PCI chipset tag configuration method");
}

int
prep_pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	struct prep_pci_chipset_businfo *pbi;
	prop_object_t busmax;

	pbi = SIMPLEQ_FIRST(&prep_pct->pc_pbi);
	while (busno--)
		pbi = SIMPLEQ_NEXT(pbi, next);
	if (pbi == NULL)
		return 32;

	busmax = prop_dictionary_get(pbi->pbi_properties,
	    "prep-pcibus-maxdevices");
	if (busmax == NULL)
		return 32;
	else
		return prop_number_integer_value(busmax);

	return 32;
}

int
prep_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct prep_pci_chipset_businfo *pbi;
	prop_dictionary_t dict, devsub;
	prop_object_t pinsub;
	prop_number_t pbus;
	int busno, bus, pin, line, swiz, dev, i;
	char key[20];

	pin = pa->pa_intrpin;
	line = pa->pa_intrline;
	bus = busno = pa->pa_bus;
	swiz = pa->pa_intrswiz;
	dev = pa->pa_device;
	i = 0;

	pbi = SIMPLEQ_FIRST(&prep_pct->pc_pbi);
	while (busno--)
		pbi = SIMPLEQ_NEXT(pbi, next);
	KASSERT(pbi != NULL);

	dict = prop_dictionary_get(pbi->pbi_properties, "prep-pci-intrmap");
	if (dict != NULL)
		i = prop_dictionary_count(dict);

	if (dict == NULL || i == 0) {
		/* We have a non-PReP bus.  now it gets hard */
		pbus = prop_dictionary_get(pbi->pbi_properties,
		    "prep-pcibus-parent");
		if (pbus == NULL)
			goto bad;
		busno = prop_number_integer_value(pbus);
		pbus = prop_dictionary_get(pbi->pbi_properties,
		    "prep-pcibus-rawdevnum");
		dev = prop_number_integer_value(pbus);

		/* now that we know the parent bus, we need to find it's pbi */
		pbi = SIMPLEQ_FIRST(&prep_pct->pc_pbi);
		while (busno--)
			pbi = SIMPLEQ_NEXT(pbi, next);
		KASSERT(pbi != NULL);

		/* set the pin to the rawpin */
		pin = pa->pa_rawintrpin;
		/* now we have the pbi, ask for dict again */
		dict = prop_dictionary_get(pbi->pbi_properties,
		    "prep-pci-intrmap");
		if (dict == NULL)
			goto bad;
	}

	/* No IRQ used. */
	if (pin == 0)
		goto bad;
	if (pin > 4) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	sprintf(key, "devfunc-%d", dev);
	devsub = prop_dictionary_get(dict, key);
	if (devsub == NULL)
		goto bad;
	sprintf(key, "pin-%c", 'A' + (pin-1));
	pinsub = prop_dictionary_get(devsub, key);
	if (pinsub == NULL)
		goto bad;
	line = prop_number_integer_value(pinsub);
	
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
prep_pci_intr_string(void *v, pci_intr_handle_t ih)
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == 0 || ih >= ICU_LEN || ih == IRQ_SLAVE)
		panic("pci_intr_string: bogus handle 0x%x", ih);

	sprintf(irqstr, "irq %d", ih);
	return (irqstr);
	
}

const struct evcnt *
prep_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
prep_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih == 0 || ih >= ICU_LEN || ih == IRQ_SLAVE)
		panic("pci_intr_establish: bogus handle 0x%x", ih);

	return isa_intr_establish(NULL, ih, IST_LEVEL, level, func, arg);
}

void
prep_pci_intr_disestablish(void *v, void *cookie)
{

	isa_intr_disestablish(NULL, cookie);
}

void
prep_pci_conf_interrupt(void *v, int bus, int dev, int pin,
    int swiz, int *iline)
{
	/* do nothing */
}

extern pcitag_t prep_pci_direct_make_tag(void *, int, int, int);
extern pcitag_t prep_pci_indirect_make_tag(void *, int, int, int);
extern pcireg_t prep_pci_direct_conf_read(void *, pcitag_t, int);
extern pcireg_t prep_pci_indirect_conf_read(void *, pcitag_t, int);

int
prep_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, int func,
	pcireg_t id)
{
	struct prep_pci_chipset_businfo *pbi;
	prop_number_t bmax, pbus;
	pcitag_t tag;
	pcireg_t class;

	/*
	 * The P9100 board found in some IBM machines cannot be
	 * over-configured.
	 */
	if (PCI_VENDOR(id) == PCI_VENDOR_WEITEK &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_WEITEK_P9100)
		return 0;

	/* We have already mapped the MPIC2 if we have one, so leave it
	   alone */
	if (PCI_VENDOR(id) == PCI_VENDOR_IBM &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_IBM_MPIC2)
		return 0;

	if (PCI_VENDOR(id) == PCI_VENDOR_IBM &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_IBM_MPIC)
		return 0;

	if (PCI_VENDOR(id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_INTEL_PCEB)
		return 0;

	if (PCI_VENDOR(id) == PCI_VENDOR_MOT &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_MOT_RAVEN)
		return (PCI_CONF_ALL & ~PCI_CONF_MAP_MEM);

	/* NOTE, all device specific stuff must be above this line */
	/* don't do this on the primary host bridge */
	if (bus == 0 && dev == 0 && func == 0)
		return PCI_CONF_DEFAULT;

	if (prep_pci_config_mode) {
		tag = prep_pci_indirect_make_tag(pct, bus, dev, func);
		class = prep_pci_indirect_conf_read(pct, tag,
		    PCI_CLASS_REG);
	} else {
		tag = prep_pci_direct_make_tag(pct, bus, dev, func);
		class = prep_pci_direct_conf_read(pct, tag,
		    PCI_CLASS_REG);
	}

	/*
	 * PCI bridges have special needs.  We need to discover where they
	 * came from, and wire them appropriately.
	 */
	if (PCI_CLASS(class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_PCI) {
		pbi = malloc(sizeof(struct prep_pci_chipset_businfo), M_DEVBUF,
		    M_NOWAIT);
		KASSERT(pbi != NULL);
		pbi->pbi_properties = prop_dictionary_create();
		KASSERT(pbi->pbi_properties != NULL);
		setup_pciintr_map(pbi, bus, dev, func);

		/* record the parent bus, and the parent device number */
		pbus = prop_number_create_integer(bus);
		prop_dictionary_set(pbi->pbi_properties, "prep-pcibus-parent",
		    pbus);
		prop_object_release(pbus);
		pbus = prop_number_create_integer(dev);
		prop_dictionary_set(pbi->pbi_properties,
		    "prep-pcibus-rawdevnum", pbus);
		prop_object_release(pbus);

		/* now look for bus quirks */

		if (PCI_VENDOR(id) == PCI_VENDOR_DEC &&
		    PCI_PRODUCT(id) == PCI_PRODUCT_DEC_21154) {
			bmax = prop_number_create_integer(8);
			KASSERT(bmax != NULL);
			prop_dictionary_set(pbi->pbi_properties,
			    "prep-pcibus-maxdevices", bmax);
			prop_object_release(bmax);
		}

		SIMPLEQ_INSERT_TAIL(&prep_pct->pc_pbi, pbi, next);
	}

	return (PCI_CONF_DEFAULT);
}
