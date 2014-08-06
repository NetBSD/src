/*	$NetBSD: pci.h,v 1.7 2014/08/06 13:52:06 riastradh Exp $	*/

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

#ifndef _LINUX_PCI_H_
#define _LINUX_PCI_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <machine/limits.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>

#include <linux/dma-mapping.h>
#include <linux/ioport.h>

struct pci_bus;

struct pci_device_id {
	uint32_t	vendor;
	uint32_t	device;
	uint32_t	subvendor;
	uint32_t	subdevice;
	uint32_t	class;
	uint32_t	class_mask;
	unsigned long	driver_data;
};

#define	PCI_ANY_ID		((pcireg_t)-1)

#define	PCI_BASE_CLASS_DISPLAY	PCI_CLASS_DISPLAY

#define	PCI_CLASS_BRIDGE_ISA						\
	((PCI_CLASS_BRIDGE << 8) | PCI_SUBCLASS_BRIDGE_ISA)
CTASSERT(PCI_CLASS_BRIDGE_ISA == 0x0601);

/* XXX This is getting silly...  */
#define	PCI_VENDOR_ID_ASUSTEK	PCI_VENDOR_ASUSTEK
#define	PCI_VENDOR_ID_ATI	PCI_VENDOR_ATI
#define	PCI_VENDOR_ID_DELL	PCI_VENDOR_DELL
#define	PCI_VENDOR_ID_IBM	PCI_VENDOR_IBM
#define	PCI_VENDOR_ID_HP	PCI_VENDOR_HP
#define	PCI_VENDOR_ID_INTEL	PCI_VENDOR_INTEL
#define	PCI_VENDOR_ID_NVIDIA	PCI_VENDOR_NVIDIA
#define	PCI_VENDOR_ID_SONY	PCI_VENDOR_SONY
#define	PCI_VENDOR_ID_VIA	PCI_VENDOR_VIATECH

#define	PCI_DEVICE_ID_ATI_RADEON_QY	PCI_PRODUCT_ATI_RADEON_RV100_QY

#define	PCI_DEVFN(DEV, FN)						\
	(__SHIFTIN((DEV), __BITS(3, 7)) | __SHIFTIN((FN), __BITS(0, 2)))
#define	PCI_SLOT(DEVFN)		__SHIFTOUT((DEVFN), __BITS(3, 7))
#define	PCI_FUNC(DEVFN)		__SHIFTOUT((DEVFN), __BITS(0, 2))

#define	PCI_NUM_RESOURCES	((PCI_MAPREG_END - PCI_MAPREG_START) / 4)
#define	DEVICE_COUNT_RESOURCE	PCI_NUM_RESOURCES

#define	PCI_CAP_ID_AGP	PCI_CAP_AGP

typedef int pci_power_t;

#define	PCI_D0		0
#define	PCI_D1		1
#define	PCI_D2		2
#define	PCI_D3hot	3
#define	PCI_D3cold	4

#define	__pci_iomem

struct pci_dev {
	struct pci_attach_args	pd_pa;
	int			pd_kludges;	/* Gotta lose 'em...  */
#define	NBPCI_KLUDGE_GET_MUMBLE	0x01
#define	NBPCI_KLUDGE_MAP_ROM	0x02
	bus_space_tag_t		pd_rom_bst;
	bus_space_handle_t	pd_rom_bsh;
	bus_size_t		pd_rom_size;
	void			*pd_rom_vaddr;
	device_t		pd_dev;
	struct {
		pcireg_t		type;
		bus_addr_t		addr;
		bus_size_t		size;
		int			flags;
		bus_space_tag_t		bst;
		bus_space_handle_t	bsh;
		void __pci_iomem	*kva;
	}			pd_resources[PCI_NUM_RESOURCES];
	struct pci_conf_state	*pd_saved_state;
	struct device		dev;		/* XXX Don't believe me!  */
	struct pci_bus		*bus;
	uint32_t		devfn;
	uint16_t		vendor;
	uint16_t		device;
	uint16_t		subsystem_vendor;
	uint16_t		subsystem_device;
	uint8_t			revision;
	uint32_t		class;
	bool			msi_enabled;
};

static inline device_t
pci_dev_dev(struct pci_dev *pdev)
{
	return pdev->pd_dev;
}

static inline void
linux_pci_dev_init(struct pci_dev *pdev, device_t dev,
    const struct pci_attach_args *pa, int kludges)
{
	const uint32_t subsystem_id = pci_conf_read(pa->pa_pc, pa->pa_tag,
	    PCI_SUBSYS_ID_REG);
	unsigned i;

	pdev->pd_pa = *pa;
	pdev->pd_kludges = kludges;
	pdev->pd_rom_vaddr = NULL;
	pdev->pd_dev = dev;
	pdev->bus = NULL;	/* XXX struct pci_dev::bus */
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
	}
}

static inline int
pci_find_capability(struct pci_dev *pdev, int cap)
{
	return pci_get_capability(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, cap,
	    NULL, NULL);
}

static inline int
pci_read_config_dword(struct pci_dev *pdev, int reg, uint32_t *valuep)
{
	KASSERT(!ISSET(reg, 3));
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg);
	return 0;
}

static inline int
pci_read_config_word(struct pci_dev *pdev, int reg, uint16_t *valuep)
{
	KASSERT(!ISSET(reg, 1));
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    (reg &~ 3)) >> (8 * (reg & 3));
	return 0;
}

static inline int
pci_read_config_byte(struct pci_dev *pdev, int reg, uint8_t *valuep)
{
	*valuep = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    (reg &~ 1)) >> (8 * (reg & 1));
	return 0;
}

static inline int
pci_write_config_dword(struct pci_dev *pdev, int reg, uint32_t value)
{
	KASSERT(!ISSET(reg, 3));
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag, reg, value);
	return 0;
}

static inline void
pci_rmw_config(struct pci_dev *pdev, int reg, unsigned int bytes,
    uint32_t value)
{
	const uint32_t mask = ~((~0UL) << (8 * bytes));
	const int reg32 = (reg &~ 3);
	const unsigned int shift = (8 * (reg & 3));
	uint32_t value32;

	KASSERT(bytes <= 4);
	KASSERT(!ISSET(value, ~mask));
	pci_read_config_dword(pdev, reg32, &value32);
	value32 &=~ (mask << shift);
	value32 |= (value << shift);
	pci_write_config_dword(pdev, reg32, value32);
}

static inline int
pci_write_config_word(struct pci_dev *pdev, int reg, uint16_t value)
{
	KASSERT(!ISSET(reg, 1));
	pci_rmw_config(pdev, reg, 2, value);
	return 0;
}

static inline int
pci_write_config_byte(struct pci_dev *pdev, int reg, uint8_t value)
{
	pci_rmw_config(pdev, reg, 1, value);
	return 0;
}

/*
 * XXX pci msi
 */
static inline int
pci_enable_msi(struct pci_dev *pdev)
{
	return -ENOSYS;
}

static inline void
pci_disable_msi(struct pci_dev *pdev __unused)
{
	KASSERT(pdev->msi_enabled);
}

static inline void
pci_set_master(struct pci_dev *pdev)
{
	pcireg_t csr;

	csr = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG, csr);
}

static inline void
pci_clear_master(struct pci_dev *pdev)
{
	pcireg_t csr;

	csr = pci_conf_read(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG);
	csr &= ~(pcireg_t)PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    PCI_COMMAND_STATUS_REG, csr);
}

#define	PCIBIOS_MIN_MEM	0	/* XXX bogus x86 kludge bollocks */

static inline bus_addr_t
pcibios_align_resource(void *p, const struct resource *resource,
    bus_addr_t addr, bus_size_t size)
{
	panic("pcibios_align_resource has accessed unaligned neurons!");
}

static inline int
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

static inline int		/* XXX inline?  */
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

static inline struct pci_dev *
pci_get_bus_and_slot(int bus, int slot)
{
	struct pci_attach_args pa;

	KASSERT(bus == 0);
	KASSERT(slot == PCI_DEVFN(0, 0));

	if (!pci_find_device(&pa, &pci_kludgey_match_bus0_dev0_func0))
		return NULL;

	struct pci_dev *const pdev = kmem_zalloc(sizeof(*pdev), KM_SLEEP);
	linux_pci_dev_init(pdev, NULL, &pa, NBPCI_KLUDGE_GET_MUMBLE);

	return pdev;
}

static inline int		/* XXX inline?  */
pci_kludgey_match_isa_bridge(const struct pci_attach_args *pa)
{

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE)
		return 0;
	if (PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	return 1;
}

static inline void
pci_dev_put(struct pci_dev *pdev)
{

	if (pdev == NULL)
		return;

	KASSERT(ISSET(pdev->pd_kludges, NBPCI_KLUDGE_GET_MUMBLE));
	kmem_free(pdev, sizeof(*pdev));
}

static inline struct pci_dev *
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
	linux_pci_dev_init(pdev, NULL, &pa, NBPCI_KLUDGE_GET_MUMBLE);

	return pdev;
}

#define	__pci_rom_iomem

static inline void
pci_unmap_rom(struct pci_dev *pdev, void __pci_rom_iomem *vaddr __unused)
{

	KASSERT(ISSET(pdev->pd_kludges, NBPCI_KLUDGE_MAP_ROM));
	KASSERT(vaddr == pdev->pd_rom_vaddr);
	bus_space_unmap(pdev->pd_rom_bst, pdev->pd_rom_bsh, pdev->pd_rom_size);
	pdev->pd_kludges &= ~NBPCI_KLUDGE_MAP_ROM;
	pdev->pd_rom_vaddr = NULL;
}

static inline void __pci_rom_iomem *
pci_map_rom(struct pci_dev *pdev, size_t *sizep)
{
	bus_space_handle_t bsh;
	bus_size_t size;

	KASSERT(!ISSET(pdev->pd_kludges, NBPCI_KLUDGE_MAP_ROM));

	if (pci_mapreg_map(&pdev->pd_pa, PCI_MAPREG_ROM, PCI_MAPREG_TYPE_ROM,
		(BUS_SPACE_MAP_PREFETCHABLE | BUS_SPACE_MAP_LINEAR),
		&pdev->pd_rom_bst, &pdev->pd_rom_bsh, NULL, &pdev->pd_rom_size)
	    != 0)
		return NULL;
	pdev->pd_kludges |= NBPCI_KLUDGE_MAP_ROM;

	/* XXX This type is obviously wrong in general...  */
	if (pci_find_rom(&pdev->pd_pa, pdev->pd_rom_bst, pdev->pd_rom_bsh,
		PCI_ROM_CODE_TYPE_X86, &bsh, &size)) {
		pci_unmap_rom(pdev, NULL);
		return NULL;
	}

	KASSERT(size <= SIZE_T_MAX);
	*sizep = size;
	pdev->pd_rom_vaddr = bus_space_vaddr(pdev->pd_rom_bst, bsh);
	return pdev->pd_rom_vaddr;
}

static inline bus_addr_t
pci_resource_start(struct pci_dev *pdev, unsigned i)
{

	KASSERT(i < PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].addr;
}

static inline bus_size_t
pci_resource_len(struct pci_dev *pdev, unsigned i)
{

	KASSERT(i < PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].size;
}

static inline bus_addr_t
pci_resource_end(struct pci_dev *pdev, unsigned i)
{

	return pci_resource_start(pdev, i) + (pci_resource_len(pdev, i) - 1);
}

static inline int
pci_resource_flags(struct pci_dev *pdev, unsigned i)
{

	KASSERT(i < PCI_NUM_RESOURCES);
	return pdev->pd_resources[i].flags;
}

static inline void __pci_iomem *
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

	return pdev->pd_resources[i].kva;
}

static inline void
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

static inline void
pci_save_state(struct pci_dev *pdev)
{

	KASSERT(pdev->pd_saved_state == NULL);
	pdev->pd_saved_state = kmem_alloc(sizeof(*pdev->pd_saved_state),
	    KM_SLEEP);
	pci_conf_capture(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    pdev->pd_saved_state);
}

static inline void
pci_restore_state(struct pci_dev *pdev)
{

	KASSERT(pdev->pd_saved_state != NULL);
	pci_conf_restore(pdev->pd_pa.pa_pc, pdev->pd_pa.pa_tag,
	    pdev->pd_saved_state);
	kmem_free(pdev->pd_saved_state, sizeof(*pdev->pd_saved_state));
	pdev->pd_saved_state = NULL;
}

static inline bool
pci_is_pcie(struct pci_dev *pdev)
{

	return (pci_find_capability(pdev, PCI_CAP_PCIEXPRESS) != 0);
}

static inline bool
pci_dma_supported(struct pci_dev *pdev, uintmax_t mask)
{

	/* XXX Cop-out.  */
	if (mask > DMA_BIT_MASK(32))
		return pci_dma64_available(&pdev->pd_pa);
	else
		return true;
}

#endif  /* _LINUX_PCI_H_ */
