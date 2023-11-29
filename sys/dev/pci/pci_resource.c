/* $NetBSD: pci_resource.c,v 1.3.2.1 2023/11/29 12:34:46 martin Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * pci_resource.c --
 *
 * Scan current PCI resource allocations and attempt to assign resources
 * to devices that are not configured WITHOUT changing any configuration
 * performed by system firmware.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_resource.c,v 1.3.2.1 2023/11/29 12:34:46 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pci_resource.h>

#define	DPRINT		aprint_debug

#if defined(PCI_RESOURCE_TEST_VENDOR_ID) && \
    defined(PCI_RESOURCE_TEST_PRODUCT_ID)
#define IS_TEST_DEVICE(_pd)						      \
	(PCI_VENDOR(pd->pd_id) == PCI_RESOURCE_TEST_VENDOR_ID &&	      \
	 PCI_PRODUCT(pd->pd_id) == PCI_RESOURCE_TEST_PRODUCT_ID)
#else
#define IS_TEST_DEVICE(_pd)	0
#endif

#define	PCI_MAX_DEVICE	32
#define	PCI_MAX_FUNC	8

#define	PCI_MAX_IORES	6

#define	PCI_RANGE_FOREACH(_type)					      \
	for (u_int _type = PCI_RANGE_BUS; _type < NUM_PCI_RANGES; _type++)

static const char *pci_range_typenames[NUM_PCI_RANGES] = {
	[PCI_RANGE_BUS]  = "bus",
	[PCI_RANGE_IO]   = "io",
	[PCI_RANGE_MEM]  = "mem",
	[PCI_RANGE_PMEM] = "pmem",
};

struct pci_bus;

struct pci_iores {
	uint64_t	pi_base;	/* Base address */
	uint64_t	pi_size;	/* Resource size */
	uint8_t		pi_type;	/* PCI_MAPREG_TYPE_* */
	u_int		pi_bar;		/* PCI bar number */
	union {
		struct {
			uint8_t		memtype;
			bool		prefetch;
		} pi_mem;
	};
};

struct pci_device {
	bool		pd_present;	/* Device is present */
	bool		pd_configured;	/* Device is configured */
	struct pci_bus *pd_bus;	/* Parent bus */
	uint8_t		pd_devno;	/* Device number */
	uint8_t		pd_funcno;	/* Function number */
	pcitag_t	pd_tag;		/* PCI tag */

	pcireg_t	pd_id;		/* Vendor ID, Device ID */
	pcireg_t	pd_class;	/* Revision ID, Class Code */
	pcireg_t	pd_bhlc;	/* BIST, Header Type, Primary Latency
					 * Timer, Cache Line Size */

	struct pci_iores pd_iores[PCI_MAX_IORES];
	u_int		pd_niores;

	bool		pd_ppb;		/* PCI-PCI bridge */
	union {
		struct {
			pcireg_t	bridge_bus;
			struct pci_resource_range ranges[NUM_PCI_RANGES];
		} pd_bridge;
	};
};

struct pci_bus {
	uint8_t		pb_busno;	/* Bus number */
	struct pci_device *pb_bridge; /* Parent bridge, or NULL */

	struct pci_device pb_device[PCI_MAX_DEVICE * PCI_MAX_FUNC];
					/* Devices on bus */
	u_int		pb_lastdevno;	/* Last device found */

	struct pci_resource_range pb_ranges[NUM_PCI_RANGES];
	vmem_t		*pb_res[NUM_PCI_RANGES];
};

struct pci_resources {
	struct pci_bus **pr_bus;	/* Bus list */
	pci_chipset_tag_t pr_pc;	/* Chipset tag */
	uint8_t		pr_startbus;	/* First bus number */
	uint8_t		pr_endbus;	/* Last bus number */

	struct pci_resource_range pr_ranges[NUM_PCI_RANGES];
	vmem_t		*pr_res[NUM_PCI_RANGES];
};

static void	pci_resource_scan_bus(struct pci_resources *,
		    struct pci_device *, uint8_t);

#define	PCI_SBDF_FMT			"%04x:%02x:%02x.%u"
#define	PCI_SBDF_FMT_ARGS(_pr, _pd)	\
	pci_get_segment((_pr)->pr_pc),	\
	(_pd)->pd_bus->pb_busno,	\
	(_pd)->pd_devno,		\
	(_pd)->pd_funcno

#define	PCICONF_RES_BUS(_pr, _busno)				\
	((_pr)->pr_bus[(_busno) - (_pr)->pr_startbus])
#define	PCICONF_BUS_DEVICE(_pb, _devno, _funcno)		\
	(&(_pb)->pb_device[(_devno) * PCI_MAX_FUNC + (_funcno)])

/*
 * pci_create_vmem --
 *
 *   Create a vmem arena covering the specified range, used for tracking
 *   PCI resources.
 */
static vmem_t *
pci_create_vmem(const char *name, bus_addr_t start, bus_addr_t end)
{
	vmem_t *arena;
	int error __diagused;

	arena = vmem_create(name, 0, 0, 1, NULL, NULL, NULL, 0, VM_SLEEP,
	    IPL_NONE);
	error = vmem_add(arena, start, end - start + 1, VM_SLEEP);
	KASSERTMSG(error == 0, "error=%d", error);

	return arena;
}

/*
 * pci_new_bus --
 *
 *   Create a new PCI bus and initialize its resource ranges.
 */
static struct pci_bus *
pci_new_bus(struct pci_resources *pr, uint8_t busno, struct pci_device *bridge)
{
	struct pci_bus *pb;
	struct pci_resource_range *ranges;

	pb = kmem_zalloc(sizeof(*pb), KM_SLEEP);
	pb->pb_busno = busno;
	pb->pb_bridge = bridge;
	if (bridge == NULL) {
		/*
		 * No additional constraints on resource allocations for
		 * the root bus.
		 */
		ranges = pr->pr_ranges;
	} else {
		/*
		 * Resource allocations for this bus are constrained by the
		 * bridge forwarding settings.
		 */
		ranges = bridge->pd_bridge.ranges;
	}
	memcpy(pb->pb_ranges, ranges, sizeof(pb->pb_ranges));

	return pb;
}

/*
 * pci_resource_device_functions --
 *
 *   Returns the number of PCI functions for a a given bus and device.
 */
static uint8_t
pci_resource_device_functions(struct pci_resources *pr,
    uint8_t busno, uint8_t devno)
{
	struct pci_bus *pb;
	struct pci_device *pd;

	pb = PCICONF_RES_BUS(pr, busno);
	pd = PCICONF_BUS_DEVICE(pb, devno, 0);
	if (!pd->pd_present) {
		return 0;
	}

	return PCI_HDRTYPE_MULTIFN(pd->pd_bhlc) ? 8 : 1;
}

/*
 * pci_resource_device_print --
 *
 *   Log details about a device.
 */
static void
pci_resource_device_print(struct pci_resources *pr,
    struct pci_device *pd)
{
	struct pci_iores *pi;
	u_int res;

	DPRINT("PCI: " PCI_SBDF_FMT " %04x:%04x %02x 0x%06x",
	       PCI_SBDF_FMT_ARGS(pr, pd),
	       PCI_VENDOR(pd->pd_id), PCI_PRODUCT(pd->pd_id),
	       PCI_REVISION(pd->pd_class), (pd->pd_class >> 8) & 0xffffff);

	switch (PCI_HDRTYPE_TYPE(pd->pd_bhlc)) {
	case PCI_HDRTYPE_DEVICE:
		DPRINT(" (device)\n");
		break;
	case PCI_HDRTYPE_PPB:
		DPRINT(" (bridge %u -> %u-%u)\n",
		    PCI_BRIDGE_BUS_NUM_PRIMARY(pd->pd_bridge.bridge_bus),
		    PCI_BRIDGE_BUS_NUM_SECONDARY(pd->pd_bridge.bridge_bus),
		    PCI_BRIDGE_BUS_NUM_SUBORDINATE(pd->pd_bridge.bridge_bus));

		if (pd->pd_bridge.ranges[PCI_RANGE_IO].end) {
			DPRINT("PCI: " PCI_SBDF_FMT
			       " [bridge] window io  %#" PRIx64 "-%#" PRIx64
			       "\n",
			       PCI_SBDF_FMT_ARGS(pr, pd),
			       pd->pd_bridge.ranges[PCI_RANGE_IO].start,
			       pd->pd_bridge.ranges[PCI_RANGE_IO].end);
		}
		if (pd->pd_bridge.ranges[PCI_RANGE_MEM].end) {
			DPRINT("PCI: " PCI_SBDF_FMT
			       " [bridge] window mem %#" PRIx64 "-%#" PRIx64
			       " (non-prefetchable)\n",
			       PCI_SBDF_FMT_ARGS(pr, pd),
			       pd->pd_bridge.ranges[PCI_RANGE_MEM].start,
			       pd->pd_bridge.ranges[PCI_RANGE_MEM].end);
		}
		if (pd->pd_bridge.ranges[PCI_RANGE_PMEM].end) {
			DPRINT("PCI: " PCI_SBDF_FMT
			       " [bridge] window mem %#" PRIx64 "-%#" PRIx64
			       " (prefetchable)\n",
			       PCI_SBDF_FMT_ARGS(pr, pd),
			       pd->pd_bridge.ranges[PCI_RANGE_PMEM].start,
			       pd->pd_bridge.ranges[PCI_RANGE_PMEM].end);
		}

		break;
	default:
		DPRINT(" (0x%02x)\n", PCI_HDRTYPE_TYPE(pd->pd_bhlc));
	}

	for (res = 0; res < pd->pd_niores; res++) {
		pi = &pd->pd_iores[res];

		DPRINT("PCI: " PCI_SBDF_FMT
		       " [device] resource BAR%u: %s @ %#" PRIx64 " size %#"
		       PRIx64,
		       PCI_SBDF_FMT_ARGS(pr, pd), pi->pi_bar,
		       pi->pi_type == PCI_MAPREG_TYPE_MEM ? "mem" : "io ",
		       pi->pi_base, pi->pi_size);

		if (pi->pi_type == PCI_MAPREG_TYPE_MEM) {
			switch (pi->pi_mem.memtype) {
			case PCI_MAPREG_MEM_TYPE_32BIT:
				DPRINT(", 32-bit");
				break;
			case PCI_MAPREG_MEM_TYPE_32BIT_1M:
				DPRINT(", 32-bit (1M)");
				break;
			case PCI_MAPREG_MEM_TYPE_64BIT:
				DPRINT(", 64-bit");
				break;
			}
			DPRINT(" %sprefetchable",
			    pi->pi_mem.prefetch ? "" : "non-");
		}
		DPRINT("\n");
	}
}

/*
 * pci_resource_scan_bar --
 *
 *   Determine the current BAR configuration for a given device.
 */
static void
pci_resource_scan_bar(struct pci_resources *pr,
    struct pci_device *pd, pcireg_t mapreg_start, pcireg_t mapreg_end,
    bool is_ppb)
{
	pci_chipset_tag_t pc = pr->pr_pc;
	pcitag_t tag = pd->pd_tag;
	pcireg_t mapreg = mapreg_start;
	pcireg_t ocmd, cmd, bar[2], mask[2];
	uint64_t addr, size;
	struct pci_iores *pi;

	if (!is_ppb) {
		ocmd = cmd = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		cmd &= ~(PCI_COMMAND_MASTER_ENABLE |
			 PCI_COMMAND_MEM_ENABLE |
			 PCI_COMMAND_IO_ENABLE);
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, cmd);
	}

	while (mapreg < mapreg_end) {
		u_int width = 4;

		bar[0] = pci_conf_read(pc, tag, mapreg);
		pci_conf_write(pc, tag, mapreg, 0xffffffff);
		mask[0] = pci_conf_read(pc, tag, mapreg);
		pci_conf_write(pc, tag, mapreg, bar[0]);

		switch (PCI_MAPREG_TYPE(mask[0])) {
		case PCI_MAPREG_TYPE_MEM:
			switch (PCI_MAPREG_MEM_TYPE(mask[0])) {
			case PCI_MAPREG_MEM_TYPE_32BIT:
			case PCI_MAPREG_MEM_TYPE_32BIT_1M:
				size = PCI_MAPREG_MEM_SIZE(mask[0]);
				addr = PCI_MAPREG_MEM_ADDR(bar[0]);
				break;
			case PCI_MAPREG_MEM_TYPE_64BIT:
				bar[1] = pci_conf_read(pc, tag, mapreg + 4);
				pci_conf_write(pc, tag, mapreg + 4, 0xffffffff);
				mask[1] = pci_conf_read(pc, tag, mapreg + 4);
				pci_conf_write(pc, tag, mapreg + 4, bar[1]);

				size = PCI_MAPREG_MEM64_SIZE(
				    ((uint64_t)mask[1] << 32) | mask[0]);
				addr = PCI_MAPREG_MEM64_ADDR(
				    ((uint64_t)bar[1] << 32) | bar[0]);
				width = 8;
				break;
			default:
				size = 0;
			}
			if (size > 0) {
				pi = &pd->pd_iores[pd->pd_niores++];
				pi->pi_type = PCI_MAPREG_TYPE_MEM;
				pi->pi_base = addr;
				pi->pi_size = size;
				pi->pi_bar = (mapreg - mapreg_start) / 4;
				pi->pi_mem.memtype =
				    PCI_MAPREG_MEM_TYPE(mask[0]);
				pi->pi_mem.prefetch =
				    PCI_MAPREG_MEM_PREFETCHABLE(mask[0]);
			}
			break;
		case PCI_MAPREG_TYPE_IO:
			size = PCI_MAPREG_IO_SIZE(mask[0] | 0xffff0000);
			addr = PCI_MAPREG_IO_ADDR(bar[0]);
			if (size > 0) {
				pi = &pd->pd_iores[pd->pd_niores++];
				pi->pi_type = PCI_MAPREG_TYPE_IO;
				pi->pi_base = addr;
				pi->pi_size = size;
				pi->pi_bar = (mapreg - mapreg_start) / 4;
			}
			break;
		}

		KASSERT(pd->pd_niores <= PCI_MAX_IORES);

		mapreg += width;
	}

	if (!is_ppb) {
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, ocmd);
	}
}

/*
 * pci_resource_scan_bridge --
 *
 *   Determine the current configuration of a PCI-PCI bridge.
 */
static void
pci_resource_scan_bridge(struct pci_resources *pr,
    struct pci_device *pd)
{
	pci_chipset_tag_t pc = pr->pr_pc;
	pcitag_t tag = pd->pd_tag;
	pcireg_t res, reshigh;

	pd->pd_ppb = true;

	res = pci_conf_read(pc, tag, PCI_BRIDGE_BUS_REG);
	pd->pd_bridge.bridge_bus = res;
	pd->pd_bridge.ranges[PCI_RANGE_BUS].start =
	    PCI_BRIDGE_BUS_NUM_SECONDARY(res);
	pd->pd_bridge.ranges[PCI_RANGE_BUS].end =
	    PCI_BRIDGE_BUS_NUM_SUBORDINATE(res);

	res = pci_conf_read(pc, tag, PCI_BRIDGE_STATIO_REG);
	pd->pd_bridge.ranges[PCI_RANGE_IO].start =
	    PCI_BRIDGE_STATIO_IOBASE_ADDR(res);
	pd->pd_bridge.ranges[PCI_RANGE_IO].end =
	    PCI_BRIDGE_STATIO_IOLIMIT_ADDR(res);
	if (PCI_BRIDGE_IO_32BITS(res)) {
		reshigh = pci_conf_read(pc, tag, PCI_BRIDGE_IOHIGH_REG);
		pd->pd_bridge.ranges[PCI_RANGE_IO].start |=
		    __SHIFTOUT(reshigh, PCI_BRIDGE_IOHIGH_BASE) << 16;
		pd->pd_bridge.ranges[PCI_RANGE_IO].end |=
		    __SHIFTOUT(reshigh, PCI_BRIDGE_IOHIGH_LIMIT) << 16;
	}
	if (pd->pd_bridge.ranges[PCI_RANGE_IO].start >=
	    pd->pd_bridge.ranges[PCI_RANGE_IO].end) {
		pd->pd_bridge.ranges[PCI_RANGE_IO].start = 0;
		pd->pd_bridge.ranges[PCI_RANGE_IO].end = 0;
	}

	res = pci_conf_read(pc, tag, PCI_BRIDGE_MEMORY_REG);
	pd->pd_bridge.ranges[PCI_RANGE_MEM].start =
	    PCI_BRIDGE_MEMORY_BASE_ADDR(res);
	pd->pd_bridge.ranges[PCI_RANGE_MEM].end =
	    PCI_BRIDGE_MEMORY_LIMIT_ADDR(res);
	if (pd->pd_bridge.ranges[PCI_RANGE_MEM].start >=
	    pd->pd_bridge.ranges[PCI_RANGE_MEM].end) {
		pd->pd_bridge.ranges[PCI_RANGE_MEM].start = 0;
		pd->pd_bridge.ranges[PCI_RANGE_MEM].end = 0;
	}

	res = pci_conf_read(pc, tag, PCI_BRIDGE_PREFETCHMEM_REG);
	pd->pd_bridge.ranges[PCI_RANGE_PMEM].start =
	    PCI_BRIDGE_PREFETCHMEM_BASE_ADDR(res);
	pd->pd_bridge.ranges[PCI_RANGE_PMEM].end =
	    PCI_BRIDGE_PREFETCHMEM_LIMIT_ADDR(res);
	if (PCI_BRIDGE_PREFETCHMEM_64BITS(res)) {
		reshigh = pci_conf_read(pc, tag,
		    PCI_BRIDGE_PREFETCHBASEUP32_REG);
		pd->pd_bridge.ranges[PCI_RANGE_PMEM].start |=
		    (uint64_t)reshigh << 32;
		reshigh = pci_conf_read(pc, tag,
		    PCI_BRIDGE_PREFETCHLIMITUP32_REG);
		pd->pd_bridge.ranges[PCI_RANGE_PMEM].end |=
		    (uint64_t)reshigh << 32;
	}
	if (pd->pd_bridge.ranges[PCI_RANGE_PMEM].start >=
	    pd->pd_bridge.ranges[PCI_RANGE_PMEM].end) {
		pd->pd_bridge.ranges[PCI_RANGE_PMEM].start = 0;
		pd->pd_bridge.ranges[PCI_RANGE_PMEM].end = 0;
	}
}

/*
 * pci_resource_scan_device --
 *
 *   Determine the current configuration of a PCI device.
 */
static bool
pci_resource_scan_device(struct pci_resources *pr,
    struct pci_bus *parent_bus, uint8_t devno, uint8_t funcno)
{
	struct pci_device *pd;
	pcitag_t tag;
	pcireg_t id, bridge_bus;
	uint8_t sec_bus;

	tag = pci_make_tag(pr->pr_pc, parent_bus->pb_busno, devno, funcno);
	id = pci_conf_read(pr->pr_pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(id) == PCI_VENDOR_INVALID) {
		return false;
	}

	pd = PCICONF_BUS_DEVICE(parent_bus, devno, funcno);
	pd->pd_present = true;
	pd->pd_bus = parent_bus;
	pd->pd_tag = tag;
	pd->pd_devno = devno;
	pd->pd_funcno = funcno;
	pd->pd_id = id;
	pd->pd_class = pci_conf_read(pr->pr_pc, tag, PCI_CLASS_REG);
	pd->pd_bhlc = pci_conf_read(pr->pr_pc, tag, PCI_BHLC_REG);

	switch (PCI_HDRTYPE_TYPE(pd->pd_bhlc)) {
	case PCI_HDRTYPE_DEVICE:
		pci_resource_scan_bar(pr, pd, PCI_MAPREG_START,
		    PCI_MAPREG_END, false);
		break;
	case PCI_HDRTYPE_PPB:
		pci_resource_scan_bar(pr, pd, PCI_MAPREG_START,
		    PCI_MAPREG_PPB_END, true);
		pci_resource_scan_bridge(pr, pd);
		break;
	}

	pci_resource_device_print(pr, pd);

	if (PCI_HDRTYPE_TYPE(pd->pd_bhlc) == PCI_HDRTYPE_PPB &&
	    PCI_CLASS(pd->pd_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pd->pd_class) == PCI_SUBCLASS_BRIDGE_PCI) {
		bridge_bus = pci_conf_read(pr->pr_pc, tag, PCI_BRIDGE_BUS_REG);
		sec_bus = PCI_BRIDGE_BUS_NUM_SECONDARY(bridge_bus);
		if (sec_bus <= pr->pr_endbus) {
			pci_resource_scan_bus(pr, pd, sec_bus);
		}
	}

	return true;
}

/*
 * pci_resource_scan_bus --
 *
 *   Enumerate devices on a bus, recursively.
 */
static void
pci_resource_scan_bus(struct pci_resources *pr,
    struct pci_device *bridge_dev, uint8_t busno)
{
	struct pci_bus *pb;
	uint8_t devno, funcno;
	uint8_t nfunc;

	KASSERT(busno >= pr->pr_startbus);
	KASSERT(busno <= pr->pr_endbus);

	if (PCICONF_RES_BUS(pr, busno) != NULL) {
		/*
		 * Firmware has configured more than one bridge with the
		 * same secondary bus number.
		 */
		panic("Bus %u already scanned (firmware bug!)", busno);
		return;
	}

	pb = pci_new_bus(pr, busno, bridge_dev);
	PCICONF_RES_BUS(pr, busno) = pb;

	for (devno = 0; devno < PCI_MAX_DEVICE; devno++) {
		if (!pci_resource_scan_device(pr, pb, devno, 0)) {
			continue;
		}
		pb->pb_lastdevno = devno;

		nfunc = pci_resource_device_functions(pr, busno, devno);
		for (funcno = 1; funcno < nfunc; funcno++) {
			pci_resource_scan_device(pr, pb, devno, funcno);
		}
	}
}

/*
 * pci_resource_claim --
 *
 *   Claim a resource from a vmem arena. This is called to inform the
 *   resource manager about resources already configured by system firmware.
 */
static int
pci_resource_claim(vmem_t *arena, vmem_addr_t start, vmem_addr_t end)
{
	KASSERT(end >= start);

	return vmem_xalloc(arena, end - start + 1, 0, 0, 0, start, end,
	    VM_BESTFIT | VM_NOSLEEP, NULL);
}

/*
 * pci_resource_alloc --
 *
 *   Allocate a resource from a vmem arena. This is called when configuring
 *   devices that were not already configured by system firmware.
 */
static int
pci_resource_alloc(vmem_t *arena, vmem_size_t size, vmem_size_t align,
    uint64_t *base)
{
	vmem_addr_t addr;
	int error;

	KASSERT(size != 0);

	error = vmem_xalloc(arena, size, align, 0, 0, VMEM_ADDR_MIN,
	    VMEM_ADDR_MAX, VM_BESTFIT | VM_NOSLEEP, &addr);
	if (error == 0) {
		*base = (uint64_t)addr;
	}

	return error;
}

/*
 * pci_resource_init_device --
 *
 *   Discover resources assigned by system firmware, notify the resource
 *   manager of these ranges, and determine if the device has additional
 *   resources that need to be allocated.
 */
static void
pci_resource_init_device(struct pci_resources *pr,
    struct pci_device *pd)
{
	struct pci_iores *pi;
	struct pci_bus *pb = pd->pd_bus;
	vmem_t *res_io = pb->pb_res[PCI_RANGE_IO];
	vmem_t *res_mem = pb->pb_res[PCI_RANGE_MEM];
	vmem_t *res_pmem = pb->pb_res[PCI_RANGE_PMEM];
	pcireg_t cmd;
	u_int enabled, required;
	u_int iores;
	int error;

	KASSERT(pd->pd_present);

	if (IS_TEST_DEVICE(pd)) {
		cmd = pci_conf_read(pr->pr_pc, pd->pd_tag,
		    PCI_COMMAND_STATUS_REG);
		cmd &= ~(PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_IO_ENABLE|
			 PCI_COMMAND_MASTER_ENABLE);
		pci_conf_write(pr->pr_pc, pd->pd_tag, PCI_COMMAND_STATUS_REG,
		    cmd);
	}

	enabled = required = 0;
	cmd = pci_conf_read(pr->pr_pc, pd->pd_tag, PCI_COMMAND_STATUS_REG);
	if ((cmd & PCI_COMMAND_MEM_ENABLE) != 0) {
		enabled |= __BIT(PCI_MAPREG_TYPE_MEM);
	}
	if ((cmd & PCI_COMMAND_IO_ENABLE) != 0) {
		enabled |= __BIT(PCI_MAPREG_TYPE_IO);
	}

	for (iores = 0; iores < pd->pd_niores; iores++) {
		pi = &pd->pd_iores[iores];

		required |= __BIT(pi->pi_type);

		if (IS_TEST_DEVICE(pd)) {
			pci_conf_write(pr->pr_pc, pd->pd_tag,
			    PCI_BAR(pi->pi_bar), 0);
			continue;
		}
		if ((enabled & __BIT(pi->pi_type)) == 0) {
			continue;
		}

		if (pi->pi_type == PCI_MAPREG_TYPE_IO) {
			error = res_io == NULL ? ERANGE :
			    pci_resource_claim(res_io, pi->pi_base,
				pi->pi_base + pi->pi_size - 1);
			if (error) {
				DPRINT("PCI: " PCI_SBDF_FMT " [device] io "
				       " %#" PRIx64 "-%#" PRIx64
				       " invalid (%d)\n",
				       PCI_SBDF_FMT_ARGS(pr, pd),
				       pi->pi_base,
				       pi->pi_base + pi->pi_size - 1,
				       error);
			}
			continue;
		}

		KASSERT(pi->pi_type == PCI_MAPREG_TYPE_MEM);
		error = ERANGE;
		if (pi->pi_mem.prefetch) {
			/*
			 * Prefetchable memory must be allocated from the
			 * bridge's prefetchable region.
			 */
			if (res_pmem != NULL) {
				error = pci_resource_claim(res_pmem, pi->pi_base,
				    pi->pi_base + pi->pi_size - 1);
			}
		} else if (pi->pi_mem.memtype == PCI_MAPREG_MEM_TYPE_64BIT) {
			/*
			 * Non-prefetchable 64-bit memory can be allocated from
			 * any range. Prefer allocations from the prefetchable
			 * region to save 32-bit only resources for 32-bit BARs.
			 */
			if (res_pmem != NULL) {
				error = pci_resource_claim(res_pmem, pi->pi_base,
				    pi->pi_base + pi->pi_size - 1);
			}
			if (error && res_mem != NULL) {
				error = pci_resource_claim(res_mem, pi->pi_base,
				    pi->pi_base + pi->pi_size - 1);
			}
		} else {
			/*
			 * Non-prefetchable 32-bit memory can be allocated from
			 * any range, provided that the range is below 4GB. Try
			 * the non-prefetchable range first, and if that fails,
			 * make one last attempt at allocating from the
			 * prefetchable range in case the platform provides
			 * memory below 4GB.
			 */
			if (res_mem != NULL) {
				error = pci_resource_claim(res_mem, pi->pi_base,
				    pi->pi_base + pi->pi_size - 1);
			}
			if (error && res_pmem != NULL) {
				error = pci_resource_claim(res_pmem, pi->pi_base,
				    pi->pi_base + pi->pi_size - 1);
			}
		}
		if (error) {
			DPRINT("PCI: " PCI_SBDF_FMT " [device] mem"
			       " (%sprefetchable)"
			       " %#" PRIx64 "-%#" PRIx64
			       " invalid (%d)\n",
			       PCI_SBDF_FMT_ARGS(pr, pd),
			       pi->pi_mem.prefetch ? "" : "non-",
			       pi->pi_base,
			       pi->pi_base + pi->pi_size - 1,
			       error);
		}
	}

	pd->pd_configured = (enabled & required) == required;

	if (!pd->pd_configured) {
		DPRINT("PCI: " PCI_SBDF_FMT " [device] "
		       "not configured by firmware\n",
		       PCI_SBDF_FMT_ARGS(pr, pd));
	}
}

/*
 * pci_resource_init_bus --
 *
 *   Discover resources in use on a given bus, recursively.
 */
static void
pci_resource_init_bus(struct pci_resources *pr, uint8_t busno)
{
	struct pci_bus *pb, *parent_bus;
	struct pci_device *pd, *bridge;
	uint8_t devno, funcno;
	uint8_t nfunc;
	int error;

	KASSERT(busno >= pr->pr_startbus);
	KASSERT(busno <= pr->pr_endbus);

	pb = PCICONF_RES_BUS(pr, busno);
	bridge = pb->pb_bridge;

	KASSERT(pb != NULL);
	KASSERT((busno == pr->pr_startbus) == (bridge == NULL));

	if (bridge == NULL) {
		/* Use resources provided by firmware. */
		PCI_RANGE_FOREACH(prtype) {
			pb->pb_res[prtype] = pr->pr_res[prtype];
			pr->pr_res[prtype] = NULL;
		}
	} else {
		/*
		 * Using the resources configured in to the bridge by
		 * firmware, claim the resources on the parent bus and
		 * create a new vmem arena for the secondary bus.
		 */
		KASSERT(bridge->pd_bus != NULL);
		parent_bus = bridge->pd_bus;
		PCI_RANGE_FOREACH(prtype) {
			if (parent_bus->pb_res[prtype] == NULL ||
			    !bridge->pd_bridge.ranges[prtype].end) {
				continue;
			}
			error = pci_resource_claim(
			    parent_bus->pb_res[prtype],
			    bridge->pd_bridge.ranges[prtype].start,
			    bridge->pd_bridge.ranges[prtype].end);
			if (error == 0) {
				pb->pb_res[prtype] = pci_create_vmem(
				    pci_resource_typename(prtype),
				    bridge->pd_bridge.ranges[prtype].start,
				    bridge->pd_bridge.ranges[prtype].end);
				KASSERT(pb->pb_res[prtype] != NULL);
			} else {
				DPRINT("PCI: " PCI_SBDF_FMT " bridge (bus %u)"
				       " %-4s %#" PRIx64 "-%#" PRIx64
				       " invalid\n",
				       PCI_SBDF_FMT_ARGS(pr, bridge), busno,
				       pci_resource_typename(prtype),
				       bridge->pd_bridge.ranges[prtype].start,
				       bridge->pd_bridge.ranges[prtype].end);
			}
		}
	}

	for (devno = 0; devno <= pb->pb_lastdevno; devno++) {
		KASSERT(devno < PCI_MAX_DEVICE);
		nfunc = pci_resource_device_functions(pr, busno, devno);
		for (funcno = 0; funcno < nfunc; funcno++) {
			pd = PCICONF_BUS_DEVICE(pb, devno, funcno);
			if (!pd->pd_present) {
				continue;
			}
			if (pd->pd_ppb) {
				uint8_t sec_bus = PCI_BRIDGE_BUS_NUM_SECONDARY(
				    pd->pd_bridge.bridge_bus);
				pci_resource_init_bus(pr, sec_bus);
			}
			pci_resource_init_device(pr, pd);
		}
	}
}

/*
 * pci_resource_probe --
 *
 *   Scan for PCI devices and initialize the resource manager.
 */
static void
pci_resource_probe(struct pci_resources *pr,
    const struct pci_resource_info *info)
{
	uint8_t startbus = (uint8_t)info->ranges[PCI_RANGE_BUS].start;
	uint8_t endbus = (uint8_t)info->ranges[PCI_RANGE_BUS].end;
	u_int nbus;

	KASSERT(startbus <= endbus);
	KASSERT(pr->pr_bus == NULL);

	nbus = endbus - startbus + 1;

	pr->pr_pc = info->pc;
	pr->pr_startbus = startbus;
	pr->pr_endbus = endbus;
	pr->pr_bus = kmem_zalloc(nbus * sizeof(struct pci_bus *), KM_SLEEP);
	memcpy(pr->pr_ranges, info->ranges, sizeof(pr->pr_ranges));
	PCI_RANGE_FOREACH(prtype) {
		if (prtype == PCI_RANGE_BUS || info->ranges[prtype].end) {
			pr->pr_res[prtype] = pci_create_vmem(
			    pci_resource_typename(prtype),
			    info->ranges[prtype].start,
			    info->ranges[prtype].end);
			KASSERT(pr->pr_res[prtype] != NULL);
		}
	}

	/* Scan devices */
	pci_resource_scan_bus(pr, NULL, pr->pr_startbus);

	/*
	 * Create per-bus resource pools and remove ranges that are already
	 * in use by devices and downstream bridges.
	 */
	pci_resource_init_bus(pr, pr->pr_startbus);
}

/*
 * pci_resource_alloc_device --
 *
 *   Attempt to allocate resources for a given device.
 */
static void
pci_resource_alloc_device(struct pci_resources *pr, struct pci_device *pd)
{
	struct pci_iores *pi;
	vmem_t *arena;
	pcireg_t cmd, ocmd, base;
	uint64_t addr;
	u_int enabled;
	u_int res;
	u_int align;
	int error;

	enabled = 0;
	ocmd = cmd = pci_conf_read(pr->pr_pc, pd->pd_tag,
	    PCI_COMMAND_STATUS_REG);
	if ((cmd & PCI_COMMAND_MEM_ENABLE) != 0) {
		enabled |= __BIT(PCI_MAPREG_TYPE_MEM);
	}
	if ((cmd & PCI_COMMAND_IO_ENABLE) != 0) {
		enabled |= __BIT(PCI_MAPREG_TYPE_IO);
	}

	for (res = 0; res < pd->pd_niores; res++) {
		pi = &pd->pd_iores[res];

		if ((enabled & __BIT(pi->pi_type)) != 0) {
			continue;
		}

		if (pi->pi_type == PCI_MAPREG_TYPE_IO) {
			arena = pd->pd_bus->pb_res[PCI_RANGE_IO];
			align = uimax(pi->pi_size, 4);
		} else {
			KASSERT(pi->pi_type == PCI_MAPREG_TYPE_MEM);
			arena = NULL;
			align = uimax(pi->pi_size, 16);
			if (pi->pi_mem.prefetch) {
				arena = pd->pd_bus->pb_res[PCI_RANGE_PMEM];
			}
			if (arena == NULL) {
				arena = pd->pd_bus->pb_res[PCI_RANGE_MEM];
			}
		}
		if (arena == NULL) {
			DPRINT("PCI: " PCI_SBDF_FMT " BAR%u failed to"
			       " allocate %#" PRIx64 " bytes (no arena)\n",
			       PCI_SBDF_FMT_ARGS(pr, pd),
			       pi->pi_bar, pi->pi_size);
			return;
		}
		error = pci_resource_alloc(arena, pi->pi_size, align, &addr);
		if (error != 0) {
			DPRINT("PCI: " PCI_SBDF_FMT " BAR%u failed to"
			       " allocate %#" PRIx64 " bytes (no space)\n",
			       PCI_SBDF_FMT_ARGS(pr, pd),
			       pi->pi_bar, pi->pi_size);
			return;
		}
		DPRINT("PCI: " PCI_SBDF_FMT " BAR%u assigned range"
		       " 0x%#" PRIx64 "-0x%#" PRIx64 "\n",
		       PCI_SBDF_FMT_ARGS(pr, pd),
		       pi->pi_bar, addr, addr + pi->pi_size - 1);

		if (pi->pi_type == PCI_MAPREG_TYPE_IO) {
			cmd |= PCI_COMMAND_IO_ENABLE;
			pci_conf_write(pr->pr_pc, pd->pd_tag,
			    PCI_BAR(pi->pi_bar),
			    PCI_MAPREG_IO_ADDR(addr) | PCI_MAPREG_TYPE_IO);
		} else {
			cmd |= PCI_COMMAND_MEM_ENABLE;
			base = pci_conf_read(pr->pr_pc, pd->pd_tag,
			    PCI_BAR(pi->pi_bar));
			base = PCI_MAPREG_MEM_ADDR(addr) |
			    PCI_MAPREG_MEM_TYPE(base);
			pci_conf_write(pr->pr_pc, pd->pd_tag,
			    PCI_BAR(pi->pi_bar), base);
			if (pi->pi_mem.memtype == PCI_MAPREG_MEM_TYPE_64BIT) {
				base = (pcireg_t)
				    (PCI_MAPREG_MEM64_ADDR(addr) >> 32);
				pci_conf_write(pr->pr_pc, pd->pd_tag,
				    PCI_BAR(pi->pi_bar + 1), base);
			}
		}
	}

	if (ocmd != cmd) {
		pci_conf_write(pr->pr_pc, pd->pd_tag,
		    PCI_COMMAND_STATUS_REG, cmd);
	}
}

/*
 * pci_resource_alloc_bus --
 *
 *   Attempt to assign resources to all devices on a given bus, recursively.
 */
static void
pci_resource_alloc_bus(struct pci_resources *pr, uint8_t busno)
{
	struct pci_bus *pb = PCICONF_RES_BUS(pr, busno);
	struct pci_device *pd;
	uint8_t devno, funcno;

	for (devno = 0; devno <= pb->pb_lastdevno; devno++) {
		for (funcno = 0; funcno < 8; funcno++) {
			pd = PCICONF_BUS_DEVICE(pb, devno, funcno);
			if (!pd->pd_present) {
				if (funcno == 0) {
					break;
				}
				continue;
			}
			if (!pd->pd_configured) {
				pci_resource_alloc_device(pr, pd);
			}
			if (pd->pd_ppb) {
				uint8_t sec_bus = PCI_BRIDGE_BUS_NUM_SECONDARY(
				    pd->pd_bridge.bridge_bus);
				pci_resource_alloc_bus(pr, sec_bus);
			}
		}
	}
}

/*
 * pci_resource_init --
 *
 *   Public interface to PCI resource manager. Scans for available devices
 *   and assigns resources.
 */
void
pci_resource_init(const struct pci_resource_info *info)
{
	struct pci_resources pr = {};

	pci_resource_probe(&pr, info);
	pci_resource_alloc_bus(&pr, pr.pr_startbus);
}

/*
 * pci_resource_typename --
 *
 *   Return a string description of a PCI range type.
 */
const char *
pci_resource_typename(enum pci_range_type prtype)
{
	KASSERT(prtype < NUM_PCI_RANGES);
	return pci_range_typenames[prtype];
}
