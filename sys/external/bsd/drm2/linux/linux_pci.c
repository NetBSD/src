/*	$NetBSD: linux_pci.c,v 1.25.2.1 2024/10/04 11:40:50 martin Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifdef _KERNEL_OPT
#include "acpica.h"
#include "opt_pci.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_pci.c,v 1.25.2.1 2024/10/04 11:40:50 martin Exp $");

#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>
#endif

#include <linux/pci.h>

#include <drm/drm_agp_netbsd.h>

device_t
pci_dev_dev(struct pci_dev *pdev)
{

	return pdev->pd_dev;
}

void
pci_set_drvdata(struct pci_dev *pdev, void *drvdata)
{
	pdev->pd_drvdata = drvdata;
}

void *
pci_get_drvdata(struct pci_dev *pdev)
{
	return pdev->pd_drvdata;
}

const char *
pci_name(struct pci_dev *pdev)
{

	/* XXX not sure this has the right format */
	return device_xname(pci_dev_dev(pdev));
}

/*
 * Setup enough of a parent that we can access config space.
 * This is gross and grovels pci(4) and ppb(4) internals.
 */
static struct pci_dev *
alloc_fake_parent_device(device_t parent, const struct pci_attach_args *pa)
{

	if (parent == NULL || !device_is_a(parent, "pci"))
		return NULL;

	device_t pparent = device_parent(parent);
	if (pparent == NULL || !device_is_a(pparent, "ppb"))
		return NULL;

	struct pci_softc *pcisc = device_private(parent);
	struct ppb_softc *ppbsc = device_private(pparent);

	struct pci_dev *parentdev = kmem_zalloc(sizeof(*parentdev), KM_SLEEP);

	/* Copy this device's pci_attach_args{} as a base-line. */
	struct pci_attach_args *npa = &parentdev->pd_pa;
	*npa = *pa;

	/* Now update with stuff found in parent. */
	npa->pa_iot = pcisc->sc_iot;
	npa->pa_memt = pcisc->sc_memt;
	npa->pa_dmat = pcisc->sc_dmat;
	npa->pa_dmat64 = pcisc->sc_dmat64;
	npa->pa_pc = pcisc->sc_pc;
	npa->pa_flags = 0;	/* XXX? */

	/* Copy the parent tag, and read some info about it. */
	npa->pa_tag = ppbsc->sc_tag;
	pcireg_t id = pci_conf_read(npa->pa_pc, npa->pa_tag, PCI_ID_REG);
	pcireg_t subid = pci_conf_read(npa->pa_pc, npa->pa_tag,
	    PCI_SUBSYS_ID_REG);
	pcireg_t class = pci_conf_read(npa->pa_pc, npa->pa_tag, PCI_CLASS_REG);

	/*
	 * Fill in as much of pci_attach_args and pci_dev as reasonably possible.
	 * Most of this is not used currently.
	 */
	int bus, device, function;
	pci_decompose_tag(npa->pa_pc, npa->pa_tag, &bus, &device, &function);
	npa->pa_device = device;
	npa->pa_function = function;
	npa->pa_bus = bus;
	npa->pa_id = id;
	npa->pa_class = class;
	npa->pa_intrswiz = pcisc->sc_intrswiz;
	npa->pa_intrtag = pcisc->sc_intrtag;
	npa->pa_intrpin = PCI_INTERRUPT_PIN_NONE;

	parentdev->pd_dev = parent;

	parentdev->bus = NULL;
	parentdev->devfn = device << 3 | function;
	parentdev->vendor = PCI_VENDOR(id);
	parentdev->device = PCI_PRODUCT(id);
	parentdev->subsystem_vendor = PCI_SUBSYS_VENDOR(subid);
	parentdev->subsystem_device = PCI_SUBSYS_ID(subid);
	parentdev->revision = PCI_REVISION(class);
	parentdev->class = __SHIFTOUT(class, 0xffffff00UL); /* ? */

	return parentdev;
}

void
linux_pci_dev_init(struct pci_dev *pdev, device_t dev, device_t parent,
    const struct pci_attach_args *pa, int kludges)
{
	const uint32_t subsystem_id = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    PCI_SUBSYS_ID_REG);
	unsigned i;

	memset(pdev, 0, sizeof(*pdev)); /* paranoia */

	pdev->pd_pa = *pa;
	pdev->pd_kludges = kludges;
	pdev->pd_rom_vaddr = NULL;
	pdev->pd_dev = dev;
#if (NACPICA > 0)
	const int seg = pci_get_segment(pa->pa_pc);
	pdev->pd_ad = acpi_pcidev_find(seg, pa->pa_bus,
	    pa->pa_device, pa->pa_function);
#else
	pdev->pd_ad = NULL;
#endif
	pdev->pd_saved_state = NULL;
	pdev->pd_intr_handles = NULL;
	pdev->pd_drvdata = NULL;
	pdev->bus = kmem_zalloc(sizeof(*pdev->bus), KM_NOSLEEP);
	pdev->bus->pb_pc = pa->pa_pc;
	pdev->bus->pb_dev = parent;
	pdev->bus->number = pa->pa_bus;
	/*
	 * NetBSD doesn't have an easy "am I PCIe" or "give me PCIe speed
	 * from capability" function, but we already emulate the Linux
	 * versions that do.
	 */
	if (pci_is_pcie(pdev)) {
		pdev->bus->max_bus_speed = pcie_get_speed_cap(pdev);
	} else {
		/* XXX: Do AGP/PCI-X, etc.? */
		pdev->bus->max_bus_speed = PCI_SPEED_UNKNOWN;
	}
	pdev->bus->self = alloc_fake_parent_device(parent, pa);
	pdev->devfn = PCI_DEVFN(pa->pa_device, pa->pa_function);
	pdev->vendor = PCI_VENDOR(pa->pa_id);
	pdev->device = PCI_PRODUCT(pa->pa_id);
	pdev->subsystem_vendor = PCI_SUBSYS_VENDOR(subsystem_id);
	pdev->subsystem_device = PCI_SUBSYS_ID(subsystem_id);
	pdev->revision = PCI_REVISION(pa->pa_class);
	pdev->class = __SHIFTOUT(pa->pa_class, 0xffffff00UL); /* ? */

	CTASSERT(__arraycount(pdev->pd_resources) == PCI_NUM_RESOURCES);
	for (i = 0; i < PCI_NUM_RESOURCES; i++) {
		const int reg = PCI_BAR(i);

		pdev->pd_resources[i].type = pci_mapreg_type(pa->pa_pc,
		    pa->pa_tag, reg);
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, reg,
			pdev->pd_resources[i].type,
			&pdev->pd_resources[i].addr,
			&pdev->pd_resources[i].size,
			&pdev->pd_resources[i].flags)) {
			pdev->pd_resources[i].addr = 0;
			pdev->pd_resources[i].size = 0;
			pdev->pd_resources[i].flags = 0;
		}
		pdev->pd_resources[i].kva = NULL;
		pdev->pd_resources[i].mapped = false;
	}
}

int
pci_find_capability(struct pci_dev *pdev, int cap)
{

	return pci_get_capability(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, cap,
	    NULL, NULL);
}

int
pci_read_config_dword(struct pci_dev *pdev, int reg, uint32_t *valuep)
{

	KASSERT(!ISSET(reg, 3));
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg);
	return 0;
}

int
pci_read_config_word(struct pci_dev *pdev, int reg, uint16_t *valuep)
{

	KASSERT(!ISSET(reg, 1));
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    (reg &~ 2)) >> (8 * (reg & 2));
	return 0;
}

int
pci_read_config_byte(struct pci_dev *pdev, int reg, uint8_t *valuep)
{

	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    (reg &~ 3)) >> (8 * (reg & 3));
	return 0;
}

int
pci_write_config_dword(struct pci_dev *pdev, int reg, uint32_t value)
{

	KASSERT(!ISSET(reg, 3));
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg, value);
	return 0;
}

int
pci_bus_read_config_dword(struct pci_bus *bus, unsigned devfn, int reg,
    uint32_t *valuep)
{
	pcitag_t tag = pci_make_tag(bus->pb_pc, bus->number, PCI_SLOT(devfn),
	    PCI_FUNC(devfn));

	KASSERT(!ISSET(reg, 1));
	*valuep = pci_conf_read(bus->pb_pc, tag, reg & ~3) >> (8 * (reg & 3));
	return 0;
}

int
pci_bus_read_config_word(struct pci_bus *bus, unsigned devfn, int reg,
    uint16_t *valuep)
{
	pcitag_t tag = pci_make_tag(bus->pb_pc, bus->number, PCI_SLOT(devfn),
	    PCI_FUNC(devfn));

	KASSERT(!ISSET(reg, 1));
	*valuep = pci_conf_read(bus->pb_pc, tag, reg &~ 2) >> (8 * (reg & 2));
	return 0;
}

int
pci_bus_read_config_byte(struct pci_bus *bus, unsigned devfn, int reg,
    uint8_t *valuep)
{
	pcitag_t tag = pci_make_tag(bus->pb_pc, bus->number, PCI_SLOT(devfn),
	    PCI_FUNC(devfn));

	*valuep = pci_conf_read(bus->pb_pc, tag, reg &~ 3) >> (8 * (reg & 3));
	return 0;
}

int
pci_bus_write_config_dword(struct pci_bus *bus, unsigned devfn, int reg,
    uint32_t value)
{
	pcitag_t tag = pci_make_tag(bus->pb_pc, bus->number, PCI_SLOT(devfn),
	    PCI_FUNC(devfn));

	KASSERT(!ISSET(reg, 3));
	pci_conf_write(bus->pb_pc, tag, reg, value);
	return 0;
}

static void
pci_rmw_config(pci_chipset_tag_t pc, pcitag_t tag, int reg, unsigned int bytes,
    uint32_t value)
{
	const uint32_t mask = ~((~0UL) << (8 * bytes));
	const int reg32 = (reg &~ 3);
	const unsigned int shift = (8 * (reg & 3));
	uint32_t value32;

	KASSERT(bytes <= 4);
	KASSERT(!ISSET(value, ~mask));
	value32 = pci_conf_read(pc, tag, reg32);
	value32 &=~ (mask << shift);
	value32 |= (value << shift);
	pci_conf_write(pc, tag, reg32, value32);
}

int
pci_write_config_word(struct pci_dev *pdev, int reg, uint16_t value)
{

	KASSERT(!ISSET(reg, 1));
	pci_rmw_config(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg, 2, value);
	return 0;
}

int
pci_write_config_byte(struct pci_dev *pdev, int reg, uint8_t value)
{

	pci_rmw_config(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg, 1, value);
	return 0;
}

int
pci_bus_write_config_word(struct pci_bus *bus, unsigned devfn, int reg,
    uint16_t value)
{
	pcitag_t tag = pci_make_tag(bus->pb_pc, bus->number, PCI_SLOT(devfn),
	    PCI_FUNC(devfn));

	KASSERT(!ISSET(reg, 1));
	pci_rmw_config(bus->pb_pc, tag, reg, 2, value);
	return 0;
}

int
pci_bus_write_config_byte(struct pci_bus *bus, unsigned devfn, int reg,
    uint8_t value)
{
	pcitag_t tag = pci_make_tag(bus->pb_pc, bus->number, PCI_SLOT(devfn),
	    PCI_FUNC(devfn));

	pci_rmw_config(bus->pb_pc, tag, reg, 1, value);
	return 0;
}

int
pci_enable_msi(struct pci_dev *pdev)
{
	const struct pci_attach_args *const pa = &pdev->pd_pa;

	if (pci_msi_alloc_exact(pa, &pdev->pd_intr_handles, 1))
		return -EINVAL;

	pdev->msi_enabled = 1;
	return 0;
}

void
pci_disable_msi(struct pci_dev *pdev __unused)
{
	const struct pci_attach_args *const pa = &pdev->pd_pa;

	if (pdev->pd_intr_handles != NULL) {
		pci_intr_release(pa->pa_pc, pdev->pd_intr_handles, 1);
		pdev->pd_intr_handles = NULL;
	}
	pdev->msi_enabled = 0;
}

void
pci_set_master(struct pci_dev *pdev)
{
	pcireg_t csr;

	csr = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG, csr);
}

void
pci_clear_master(struct pci_dev *pdev)
{
	pcireg_t csr;

	csr = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG);
	csr &= ~(pcireg_t)PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG, csr);
}

int
pcie_capability_read_dword(struct pci_dev *pdev, int reg, uint32_t *valuep)
{
	pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	pcitag_t tag = pdev->pd_pa.pa_tag;
	int off;

	*valuep = 0;

	/* Must have capabilities. */
	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return 1;

	*valuep = pci_conf_read(pc, tag, off + reg);

	return 0;
}

int
pcie_capability_read_word(struct pci_dev *pdev, int reg, uint16_t *valuep)
{
	pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	pcitag_t tag = pdev->pd_pa.pa_tag;
	int off;

	*valuep = 0;

	/* Must have capabilities. */
	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return 1;

	*valuep = pci_conf_read(pc, tag, off + (reg &~ 2)) >> (8 * (reg & 2));

	return 0;
}

int
pcie_capability_write_dword(struct pci_dev *pdev, int reg, uint32_t value)
{
	pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	pcitag_t tag = pdev->pd_pa.pa_tag;
	int off;

	/* Must have capabilities. */
	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return 1;

	pci_conf_write(pc, tag, off + reg, value);

	return 0;
}

int
pcie_capability_write_word(struct pci_dev *pdev, int reg, uint16_t value)
{
	pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	pcitag_t tag = pdev->pd_pa.pa_tag;
	int off;

	/* Must have capabilities. */
	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return 1;

	pci_rmw_config(pc, tag, off + reg, 2, value);

	return 0;
}

/* From PCIe 5.0 7.5.3.4 "Device Control Register" */
static const unsigned readrqmax[] = {
	128,
	256,
	512,
	1024,
	2048,
	4096,
};

int
pcie_get_readrq(struct pci_dev *pdev)
{
	pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	pcitag_t tag = pdev->pd_pa.pa_tag;
	unsigned val;
	int off;

	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return -EINVAL; /* XXX NetBSD->Linux */

	val = __SHIFTOUT(pci_conf_read(pc, tag, off + PCIE_DCSR),
	    PCIE_DCSR_MAX_READ_REQ);

	if (val >= __arraycount(readrqmax))
		val = 0;
	return readrqmax[val];
}

int
pcie_set_readrq(struct pci_dev *pdev, int val)
{
	pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	pcitag_t tag = pdev->pd_pa.pa_tag;
	pcireg_t reg, newval = 0;
	unsigned i;
	int off;

	for (i = 0; i < __arraycount(readrqmax); i++) {
		if (readrqmax[i] == val) {
			newval = i;
			break;
		}
	}

	if (i == __arraycount(readrqmax))
		return -EINVAL;

	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return -EINVAL; /* XXX NetBSD->Linux */

	reg = pci_conf_read(pc, tag, off + PCIE_DCSR);
	reg &= ~PCIE_DCSR_MAX_READ_REQ | (newval << 12);
	pci_conf_write(pc, tag, off + PCIE_DCSR, reg);

	return 0;
}

bus_addr_t
pcibios_align_resource(void *p, const struct resource *resource,
    bus_addr_t addr, bus_size_t size)
{
	panic("pcibios_align_resource has accessed unaligned neurons!");
}

int
pci_bus_alloc_resource(struct pci_bus *bus, struct resource *resource,
    bus_size_t size, bus_size_t align, bus_addr_t start, int type __unused,
    bus_addr_t (*align_fn)(void *, const struct resource *, bus_addr_t,
	bus_size_t) __unused,
    struct pci_dev *pdev)
{
	const struct pci_attach_args *const pa = &pdev->pd_pa;
	bus_space_tag_t bst;
	int error;

	switch (resource->flags) {
	case IORESOURCE_MEM:
		bst = pa->pa_memt;
		break;

	case IORESOURCE_IO:
		bst = pa->pa_iot;
		break;

	default:
		panic("I don't know what kind of resource you want!");
	}

	resource->r_bst = bst;
	error = bus_space_alloc(bst, start, __type_max(bus_addr_t),
	    size, align, 0, 0, &resource->start, &resource->r_bsh);
	if (error)
		return error;

	resource->end = start + (size - 1);
	return 0;
}

struct pci_domain_bus_and_slot {
	int domain, bus, slot;
};

static int
pci_match_domain_bus_and_slot(void *cookie, const struct pci_attach_args *pa)
{
	const struct pci_domain_bus_and_slot *C = cookie;

	if (pci_get_segment(pa->pa_pc) != C->domain)
		return 0;
	if (pa->pa_bus != C->bus)
		return 0;
	if (PCI_DEVFN(pa->pa_device, pa->pa_function) != C->slot)
		return 0;

	return 1;
}

struct pci_dev *
pci_get_domain_bus_and_slot(int domain, int bus, int slot)
{
	struct pci_attach_args pa;
	struct pci_domain_bus_and_slot context = {domain, bus, slot},
	    *C = &context;

	if (!pci_find_device1(&pa, &pci_match_domain_bus_and_slot, C))
		return NULL;

	struct pci_dev *const pdev = kmem_zalloc(sizeof(*pdev), KM_SLEEP);
	linux_pci_dev_init(pdev, NULL, NULL, &pa, NBPCI_KLUDGE_GET_MUMBLE);

	return pdev;
}

void
pci_dev_put(struct pci_dev *pdev)
{

	if (pdev == NULL)
		return;

	KASSERT(ISSET(pdev->pd_kludges, NBPCI_KLUDGE_GET_MUMBLE));
	kmem_free(pdev->bus, sizeof(*pdev->bus));
	kmem_free(pdev, sizeof(*pdev));
}

struct pci_get_class_state {
	uint32_t		class_subclass_interface;
	const struct pci_dev	*from;
};

static int
pci_get_class_match(void *cookie, const struct pci_attach_args *pa)
{
	struct pci_get_class_state *C = cookie;

	if (C->from) {
		if ((pci_get_segment(C->from->pd_pa.pa_pc) ==
			pci_get_segment(pa->pa_pc)) &&
		    C->from->pd_pa.pa_bus == pa->pa_bus &&
		    C->from->pd_pa.pa_device == pa->pa_device &&
		    C->from->pd_pa.pa_function == pa->pa_function)
			C->from = NULL;
		return 0;
	}
	if (C->class_subclass_interface !=
	    (PCI_CLASS(pa->pa_class) << 16 |
		PCI_SUBCLASS(pa->pa_class) << 8 |
		PCI_INTERFACE(pa->pa_class)))
		return 0;

	return 1;
}

struct pci_dev *
pci_get_class(uint32_t class_subclass_interface, struct pci_dev *from)
{
	struct pci_get_class_state context = {class_subclass_interface, from},
	    *C = &context;
	struct pci_attach_args pa;
	struct pci_dev *pdev = NULL;

	if (!pci_find_device1(&pa, &pci_get_class_match, C))
		goto out;
	pdev = kmem_zalloc(sizeof(*pdev), KM_SLEEP);
	linux_pci_dev_init(pdev, NULL, NULL, &pa, NBPCI_KLUDGE_GET_MUMBLE);

out:	if (from)
		pci_dev_put(from);
	return pdev;
}

int
pci_dev_present(const struct pci_device_id *ids)
{

	/* XXX implement me -- pci_find_device doesn't pass a cookie */
	return 0;
}

void
pci_unmap_rom(struct pci_dev *pdev, void __pci_rom_iomem *vaddr __unused)
{

	/* XXX Disable the ROM address decoder.  */
	KASSERT(ISSET(pdev->pd_kludges, NBPCI_KLUDGE_MAP_ROM));
	KASSERT(vaddr == pdev->pd_rom_vaddr);
	bus_space_unmap(pdev->pd_rom_bst, pdev->pd_rom_bsh, pdev->pd_rom_size);
	pdev->pd_kludges &= ~NBPCI_KLUDGE_MAP_ROM;
	pdev->pd_rom_vaddr = NULL;
}

/* XXX Whattakludge!  Should move this in sys/arch/.  */
static int
pci_map_rom_md(struct pci_dev *pdev)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)
	const bus_addr_t rom_base = 0xc0000;
	const bus_size_t rom_size = 0x20000;
	bus_space_handle_t rom_bsh;
	int error;

	if (PCI_CLASS(pdev->pd_pa.pa_class) != PCI_CLASS_DISPLAY)
		return ENXIO;
	if (PCI_SUBCLASS(pdev->pd_pa.pa_class) != PCI_SUBCLASS_DISPLAY_VGA)
		return ENXIO;
	/* XXX Check whether this is the primary VGA card?  */
	error = bus_space_map(pdev->pd_pa.pa_memt, rom_base, rom_size,
	    (BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE), &rom_bsh);
	if (error)
		return ENXIO;

	pdev->pd_rom_bst = pdev->pd_pa.pa_memt;
	pdev->pd_rom_bsh = rom_bsh;
	pdev->pd_rom_size = rom_size;
	pdev->pd_kludges |= NBPCI_KLUDGE_MAP_ROM;

	return 0;
#else
	return ENXIO;
#endif
}

void __pci_rom_iomem *
pci_map_rom(struct pci_dev *pdev, size_t *sizep)
{

	KASSERT(!ISSET(pdev->pd_kludges, NBPCI_KLUDGE_MAP_ROM));

	if (pci_mapreg_map(&pdev->pd_pa, PCI_MAPREG_ROM, PCI_MAPREG_TYPE_ROM,
		(BUS_SPACE_MAP_PREFETCHABLE | BUS_SPACE_MAP_LINEAR),
		&pdev->pd_rom_bst, &pdev->pd_rom_bsh, NULL, &pdev->pd_rom_size)
	    != 0)
		goto fail_mi;
	pdev->pd_kludges |= NBPCI_KLUDGE_MAP_ROM;

	/* XXX This type is obviously wrong in general...  */
	if (pci_find_rom(&pdev->pd_pa, pdev->pd_rom_bst, pdev->pd_rom_bsh,
		pdev->pd_rom_size, PCI_ROM_CODE_TYPE_X86,
		&pdev->pd_rom_found_bsh, &pdev->pd_rom_found_size)) {
		pci_unmap_rom(pdev, NULL);
		goto fail_mi;
	}
	goto success;

fail_mi:
	if (pci_map_rom_md(pdev) != 0)
		goto fail_md;

	/* XXX This type is obviously wrong in general...  */
	if (pci_find_rom(&pdev->pd_pa, pdev->pd_rom_bst, pdev->pd_rom_bsh,
		pdev->pd_rom_size, PCI_ROM_CODE_TYPE_X86,
		&pdev->pd_rom_found_bsh, &pdev->pd_rom_found_size)) {
		pci_unmap_rom(pdev, NULL);
		goto fail_md;
	}

success:
	KASSERT(pdev->pd_rom_found_size <= SIZE_T_MAX);
	*sizep = pdev->pd_rom_found_size;
	pdev->pd_rom_vaddr = bus_space_vaddr(pdev->pd_rom_bst,
	    pdev->pd_rom_found_bsh);
	return pdev->pd_rom_vaddr;

fail_md:
	return NULL;
}

void __pci_rom_iomem *
pci_platform_rom(struct pci_dev *pdev __unused, size_t *sizep)
{

	*sizep = 0;
	return NULL;
}

int
pci_enable_rom(struct pci_dev *pdev)
{
	const pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	const pcitag_t tag = pdev->pd_pa.pa_tag;
	pcireg_t addr;
	int s;

	/* XXX Don't do anything if the ROM isn't there.  */

	s = splhigh();
	addr = pci_conf_read(pc, tag, PCI_MAPREG_ROM);
	addr |= PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(pc, tag, PCI_MAPREG_ROM, addr);
	splx(s);

	return 0;
}

void
pci_disable_rom(struct pci_dev *pdev)
{
	const pci_chipset_tag_t pc = pdev->pd_pa.pa_pc;
	const pcitag_t tag = pdev->pd_pa.pa_tag;
	pcireg_t addr;
	int s;

	s = splhigh();
	addr = pci_conf_read(pc, tag, PCI_MAPREG_ROM);
	addr &= ~(pcireg_t)PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(pc, tag, PCI_MAPREG_ROM, addr);
	splx(s);
}

bus_addr_t
pci_resource_start(struct pci_dev *pdev, unsigned i)
{

	if (i >= PCI_NUM_RESOURCES)
		panic("resource %d >= max %d", i, PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].addr;
}

bus_size_t
pci_resource_len(struct pci_dev *pdev, unsigned i)
{

	if (i >= PCI_NUM_RESOURCES)
		panic("resource %d >= max %d", i, PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].size;
}

bus_addr_t
pci_resource_end(struct pci_dev *pdev, unsigned i)
{

	return pci_resource_start(pdev, i) + (pci_resource_len(pdev, i) - 1);
}

int
pci_resource_flags(struct pci_dev *pdev, unsigned i)
{

	if (i >= PCI_NUM_RESOURCES)
		panic("resource %d >= max %d", i, PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].flags;
}

void __pci_iomem *
pci_iomap(struct pci_dev *pdev, unsigned i, bus_size_t size)
{
	int error;

	KASSERT(i < PCI_NUM_RESOURCES);
	KASSERT(pdev->pd_resources[i].kva == NULL);

	if (PCI_MAPREG_TYPE(pdev->pd_resources[i].type) != PCI_MAPREG_TYPE_MEM)
		return NULL;
	if (pdev->pd_resources[i].size < size)
		return NULL;
	error = bus_space_map(pdev->pd_pa.pa_memt, pdev->pd_resources[i].addr,
	    size, BUS_SPACE_MAP_LINEAR | pdev->pd_resources[i].flags,
	    &pdev->pd_resources[i].bsh);
	if (error)
		return NULL;
	pdev->pd_resources[i].bst = pdev->pd_pa.pa_memt;
	pdev->pd_resources[i].kva = bus_space_vaddr(pdev->pd_resources[i].bst,
	    pdev->pd_resources[i].bsh);
	pdev->pd_resources[i].mapped = true;

	return pdev->pd_resources[i].kva;
}

void
pci_iounmap(struct pci_dev *pdev, void __pci_iomem *kva)
{
	unsigned i;

	CTASSERT(__arraycount(pdev->pd_resources) == PCI_NUM_RESOURCES);
	for (i = 0; i < PCI_NUM_RESOURCES; i++) {
		if (pdev->pd_resources[i].kva == kva)
			break;
	}
	KASSERT(i < PCI_NUM_RESOURCES);

	pdev->pd_resources[i].kva = NULL;
	bus_space_unmap(pdev->pd_resources[i].bst, pdev->pd_resources[i].bsh,
	    pdev->pd_resources[i].size);
}

void
pci_save_state(struct pci_dev *pdev)
{

	KASSERT(pdev->pd_saved_state == NULL);
	pdev->pd_saved_state = kmem_alloc(sizeof(*pdev->pd_saved_state),
	    KM_SLEEP);
	pci_conf_capture(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    pdev->pd_saved_state);
}

void
pci_restore_state(struct pci_dev *pdev)
{

	KASSERT(pdev->pd_saved_state != NULL);
	pci_conf_restore(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    pdev->pd_saved_state);
	kmem_free(pdev->pd_saved_state, sizeof(*pdev->pd_saved_state));
	pdev->pd_saved_state = NULL;
}

bool
pci_is_pcie(struct pci_dev *pdev)
{

	return (pci_find_capability(pdev, PCI_CAP_PCIEXPRESS) != 0);
}

bool
pci_dma_supported(struct pci_dev *pdev, uintmax_t mask)
{

	/* XXX Cop-out.  */
	if (mask > DMA_BIT_MASK(32))
		return pci_dma64_available(&pdev->pd_pa);
	else
		return true;
}

bool
pci_is_thunderbolt_attached(struct pci_dev *pdev)
{

	/* XXX Cop-out.  */
	return false;
}

bool
pci_is_root_bus(struct pci_bus *bus)
{

	return bus->number == 0;
}

int
pci_domain_nr(struct pci_bus *bus)
{

	return pci_get_segment(bus->pb_pc);
}

/*
 * We explicitly rename pci_enable/disable_device so that you have to
 * review each use of them, since NetBSD's PCI API does _not_ respect
 * our local enablecnt here, but there are different parts of NetBSD
 * that automatically enable/disable like PMF, so you have to decide
 * for each one whether to call it or not.
 */

int
linux_pci_enable_device(struct pci_dev *pdev)
{
	const struct pci_attach_args *pa = &pdev->pd_pa;
	pcireg_t csr;
	int s;

	if (pdev->pd_enablecnt++)
		return 0;

	s = splhigh();
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	/* If someone else (firmware) already enabled it, credit them.  */
	if (csr & (PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE))
		pdev->pd_enablecnt++;
	csr |= PCI_COMMAND_IO_ENABLE;
	csr |= PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);
	splx(s);

	return 0;
}

void
linux_pci_disable_device(struct pci_dev *pdev)
{
	const struct pci_attach_args *pa = &pdev->pd_pa;
	pcireg_t csr;
	int s;

	if (--pdev->pd_enablecnt)
		return;

	s = splhigh();
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	csr &= ~PCI_COMMAND_IO_ENABLE;
	csr &= ~PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);
	splx(s);
}

void
linux_pci_dev_destroy(struct pci_dev *pdev)
{
	unsigned i;

	if (pdev->bus->self != NULL) {
		kmem_free(pdev->bus->self, sizeof(*pdev->bus->self));
	}
	if (pdev->bus != NULL) {
		kmem_free(pdev->bus, sizeof(*pdev->bus));
		pdev->bus = NULL;
	}
	if (ISSET(pdev->pd_kludges, NBPCI_KLUDGE_MAP_ROM)) {
		pci_unmap_rom(pdev, pdev->pd_rom_vaddr);
		pdev->pd_rom_vaddr = 0;
	}
	for (i = 0; i < __arraycount(pdev->pd_resources); i++) {
		if (!pdev->pd_resources[i].mapped)
			continue;
		bus_space_unmap(pdev->pd_resources[i].bst,
		    pdev->pd_resources[i].bsh, pdev->pd_resources[i].size);
	}

	/* There is no way these should be still in use.  */
	KASSERT(pdev->pd_saved_state == NULL);
	KASSERT(pdev->pd_intr_handles == NULL);
}

enum pci_bus_speed
pcie_get_speed_cap(struct pci_dev *dev)
{
	pci_chipset_tag_t pc = dev->pd_pa.pa_pc;
	pcitag_t tag = dev->pd_pa.pa_tag;
	pcireg_t lcap, lcap2, xcap;
	int off;

	/* Must have capabilities. */
	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return PCI_SPEED_UNKNOWN;

	/* Only PCIe 3.x has LCAP2. */
	xcap = pci_conf_read(pc, tag, off + PCIE_XCAP);
	if (__SHIFTOUT(xcap, PCIE_XCAP_VER_MASK) >= 2) {
		lcap2 = pci_conf_read(pc, tag, off + PCIE_LCAP2);
		if (lcap2) {
			if ((lcap2 & PCIE_LCAP2_SUP_LNKS64) != 0) {
				return PCIE_SPEED_64_0GT;
			}
			if ((lcap2 & PCIE_LCAP2_SUP_LNKS32) != 0) {
				return PCIE_SPEED_32_0GT;
			}
			if ((lcap2 & PCIE_LCAP2_SUP_LNKS16) != 0) {
				return PCIE_SPEED_16_0GT;
			}
			if ((lcap2 & PCIE_LCAP2_SUP_LNKS8) != 0) {
				return PCIE_SPEED_8_0GT;
			}
			if ((lcap2 & PCIE_LCAP2_SUP_LNKS5) != 0) {
				return PCIE_SPEED_5_0GT;
			}
			if ((lcap2 & PCIE_LCAP2_SUP_LNKS2) != 0) {
				return PCIE_SPEED_2_5GT;
			}
		}
	}

	lcap = pci_conf_read(pc, tag, off + PCIE_LCAP);
	if ((lcap & PCIE_LCAP_MAX_SPEED) == PCIE_LCAP_MAX_SPEED_64) {
		return PCIE_SPEED_64_0GT;
	}
	if ((lcap & PCIE_LCAP_MAX_SPEED) == PCIE_LCAP_MAX_SPEED_32) {
		return PCIE_SPEED_32_0GT;
	}
	if ((lcap & PCIE_LCAP_MAX_SPEED) == PCIE_LCAP_MAX_SPEED_16) {
		return PCIE_SPEED_16_0GT;
	}
	if ((lcap & PCIE_LCAP_MAX_SPEED) == PCIE_LCAP_MAX_SPEED_8) {
		return PCIE_SPEED_8_0GT;
	}
	if ((lcap & PCIE_LCAP_MAX_SPEED) == PCIE_LCAP_MAX_SPEED_5) {
		return PCIE_SPEED_5_0GT;
	}
	if ((lcap & PCIE_LCAP_MAX_SPEED) == PCIE_LCAP_MAX_SPEED_2) {
		return PCIE_SPEED_2_5GT;
	}

	return PCI_SPEED_UNKNOWN;
}

/*
 * This should walk the tree, it only checks this device currently.
 * It also does not write to limiting_dev (the only caller in drm2
 * currently does not use it.)
 */
unsigned
pcie_bandwidth_available(struct pci_dev *dev,
    struct pci_dev **limiting_dev,
    enum pci_bus_speed *speed,
    enum pcie_link_width *width)
{
	pci_chipset_tag_t pc = dev->pd_pa.pa_pc;
	pcitag_t tag = dev->pd_pa.pa_tag;
	pcireg_t lcsr;
	unsigned per_line_speed, num_lanes;
	int off;

	/* Must have capabilities. */
	if (pci_get_capability(pc, tag, PCI_CAP_PCIEXPRESS, &off, NULL) == 0)
		return 0;

	if (speed)
		*speed = PCI_SPEED_UNKNOWN;
	if (width)
		*width = 0;

	lcsr = pci_conf_read(pc, tag, off + PCIE_LCSR);

	switch (lcsr & PCIE_LCSR_NLW) {
	case PCIE_LCSR_NLW_X1:
	case PCIE_LCSR_NLW_X2:
	case PCIE_LCSR_NLW_X4:
	case PCIE_LCSR_NLW_X8:
	case PCIE_LCSR_NLW_X12:
	case PCIE_LCSR_NLW_X16:
	case PCIE_LCSR_NLW_X32:
		num_lanes = __SHIFTOUT(lcsr, PCIE_LCSR_NLW);
		if (width)
			*width = num_lanes;
		break;
	default:
		num_lanes = 0;
		break;
	}

	switch (__SHIFTOUT(lcsr, PCIE_LCSR_LINKSPEED)) {
	case PCIE_LCSR_LINKSPEED_2:
		*speed = PCIE_SPEED_2_5GT;
		per_line_speed = 2500 * 8 / 10;
		break;
	case PCIE_LCSR_LINKSPEED_5:
		*speed = PCIE_SPEED_5_0GT;
		per_line_speed = 5000 * 8 / 10;
		break;
	case PCIE_LCSR_LINKSPEED_8:
		*speed = PCIE_SPEED_8_0GT;
		per_line_speed = 8000 * 128 / 130;
		break;
	case PCIE_LCSR_LINKSPEED_16:
		*speed = PCIE_SPEED_16_0GT;
		per_line_speed = 16000 * 128 / 130;
		break;
	case PCIE_LCSR_LINKSPEED_32:
		*speed = PCIE_SPEED_32_0GT;
		per_line_speed = 32000 * 128 / 130;
		break;
	case PCIE_LCSR_LINKSPEED_64:
		*speed = PCIE_SPEED_64_0GT;
		per_line_speed = 64000 * 128 / 130;
		break;
	default:
		per_line_speed = 0;
	}

	return num_lanes * per_line_speed;
}
