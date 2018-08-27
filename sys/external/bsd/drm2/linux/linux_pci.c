/*	$NetBSD: linux_pci.c,v 1.4 2018/08/27 14:19:59 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_pci.c,v 1.4 2018/08/27 14:19:59 riastradh Exp $");

#include <linux/pci.h>

device_t
pci_dev_dev(struct pci_dev *pdev)
{

	return pdev->pd_dev;
}

/* XXX Nouveau kludge!  */
struct drm_device *
pci_get_drvdata(struct pci_dev *pdev)
{

	return pdev->pd_drm_dev;
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
	pdev->pd_ad = acpi_pcidev_find(0 /*XXX segment*/, pa->pa_bus,
	    pa->pa_device, pa->pa_function);
#else
	pdev->pd_ad = NULL;
#endif
	pdev->pd_saved_state = NULL;
	pdev->pd_intr_handles = NULL;
	pdev->bus = kmem_zalloc(sizeof(*pdev->bus), KM_NOSLEEP);
	pdev->bus->pb_pc = pa->pa_pc;
	pdev->bus->pb_dev = parent;
	pdev->bus->number = pa->pa_bus;
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
#ifdef notyet
	const struct pci_attach_args *const pa = &pdev->pd_pa;

	if (pci_msi_alloc_exact(pa, &pdev->pd_intr_handles, 1))
		return -EINVAL;

	pdev->msi_enabled = 1;
	return 0;
#else
	return -ENOSYS;
#endif
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

	resource->size = size;
	return 0;
}

/*
 * XXX Mega-kludgerific!  pci_get_bus_and_slot and pci_get_class are
 * defined only for their single purposes in i915drm, in
 * i915_get_bridge_dev and intel_detect_pch.  We can't define them more
 * generally without adapting pci_find_device (and pci_enumerate_bus
 * internally) to pass a cookie through.
 */

static int
pci_kludgey_match_bus0_dev0_func0(const struct pci_attach_args *pa)
{

	if (pa->pa_bus != 0)
		return 0;
	if (pa->pa_device != 0)
		return 0;
	if (pa->pa_function != 0)
		return 0;

	return 1;
}

struct pci_dev *
pci_get_bus_and_slot(int bus, int slot)
{
	struct pci_attach_args pa;

	KASSERT(bus == 0);
	KASSERT(slot == PCI_DEVFN(0, 0));

	if (!pci_find_device(&pa, &pci_kludgey_match_bus0_dev0_func0))
		return NULL;

	struct pci_dev *const pdev = kmem_zalloc(sizeof(*pdev), KM_SLEEP);
	linux_pci_dev_init(pdev, NULL, NULL, &pa, NBPCI_KLUDGE_GET_MUMBLE);

	return pdev;
}

static int
pci_kludgey_match_isa_bridge(const struct pci_attach_args *pa)
{

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE)
		return 0;
	if (PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	return 1;
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

struct pci_dev *		/* XXX i915 kludge */
pci_get_class(uint32_t class_subclass_shifted __unused, struct pci_dev *from)
{
	struct pci_attach_args pa;

	KASSERT(class_subclass_shifted == (PCI_CLASS_BRIDGE_ISA << 8));

	if (from != NULL) {
		pci_dev_put(from);
		return NULL;
	}

	if (!pci_find_device(&pa, &pci_kludgey_match_isa_bridge))
		return NULL;

	struct pci_dev *const pdev = kmem_zalloc(sizeof(*pdev), KM_SLEEP);
	linux_pci_dev_init(pdev, NULL, NULL, &pa, NBPCI_KLUDGE_GET_MUMBLE);

	return pdev;
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

	KASSERT(i < PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].addr;
}

bus_size_t
pci_resource_len(struct pci_dev *pdev, unsigned i)
{

	KASSERT(i < PCI_NUM_RESOURCES);
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

	KASSERT(i < PCI_NUM_RESOURCES);
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
	if (error) {
		/* Horrible hack: try asking the fake AGP device.  */
		if (!agp_i810_borrow(pdev->pd_resources[i].addr, size,
			&pdev->pd_resources[i].bsh))
			return NULL;
	}
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
pci_is_root_bus(struct pci_bus *bus)
{

	/* XXX Cop-out. */
	return false;
}

int
pci_domain_nr(struct pci_bus *bus)
{

	return device_unit(bus->pb_dev);
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
