/*	$NetBSD: pciconf.c,v 1.55 2022/09/25 17:52:25 thorpej Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Derived in part from code from PMON/2000 (http://pmon.groupbsd.org/).
 */

/*
 * To do:
 *    - Perform all data structure allocation dynamically, don't have
 *	statically-sized arrays ("oops, you lose because you have too
 *	many slots filled!")
 *    - Do this in 2 passes, with an MD hook to control the behavior:
 *		(1) Configure the bus (possibly including expansion
 *		    ROMs.
 *		(2) Another pass to disable expansion ROMs if they're
 *		    mapped (since you're not supposed to leave them
 *		    mapped when you're not using them).
 *	This would facilitate MD code executing the expansion ROMs
 *	if necessary (possibly with an x86 emulator) to configure
 *	devices (e.g. VGA cards).
 *    - Deal with "anything can be hot-plugged" -- i.e., carry configuration
 *	information around & be able to reconfigure on the fly
 *    - Deal with segments (See IA64 System Abstraction Layer)
 *    - Deal with subtractive bridges (& non-spec positive/subtractive decode)
 *    - Deal with ISA/VGA/VGA palette snooping
 *    - Deal with device capabilities on bridges
 *    - Worry about changing a bridge to/from transparency
 * From thorpej (05/25/01)
 *    - Try to handle devices that are already configured (perhaps using that
 *      as a hint to where we put other devices)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciconf.c,v 1.55 2022/09/25 17:52:25 thorpej Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pccbbreg.h>

int pci_conf_debug = 0;

#if !defined(MIN)
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

/* per-bus constants. */
#define MAX_CONF_DEV	32			/* Arbitrary */
#define MAX_CONF_MEM	(3 * MAX_CONF_DEV)	/* Avg. 3 per device -- Arb. */
#define MAX_CONF_IO	(3 * MAX_CONF_DEV)	/* Avg. 1 per device -- Arb. */

struct _s_pciconf_bus_t;			/* Forward declaration */

struct pciconf_resource {
	vmem_t		*arena;
	bus_addr_t	min_addr;
	bus_addr_t	max_addr;
	bus_size_t	total_size;
};

#define	PCICONF_RESOURCE_NTYPES	3
CTASSERT(PCICONF_RESOURCE_IO < PCICONF_RESOURCE_NTYPES);
CTASSERT(PCICONF_RESOURCE_MEM < PCICONF_RESOURCE_NTYPES);
CTASSERT(PCICONF_RESOURCE_PREFETCHABLE_MEM < PCICONF_RESOURCE_NTYPES);

static const char *pciconf_resource_names[] = {
	[PCICONF_RESOURCE_IO]			=	"pci-io",
	[PCICONF_RESOURCE_MEM]			=	"pci-mem",
	[PCICONF_RESOURCE_PREFETCHABLE_MEM]	=	"pci-pmem",
};

struct pciconf_resources {
	struct pciconf_resource resources[PCICONF_RESOURCE_NTYPES];
};

struct pciconf_resource_rsvd {
	int		type;
	uint64_t	start;
	bus_size_t	size;
	void		(*callback)(void *, uint64_t);
	void		*callback_arg;
	LIST_ENTRY(pciconf_resource_rsvd) next;
};

static LIST_HEAD(, pciconf_resource_rsvd) pciconf_resource_reservations =
    LIST_HEAD_INITIALIZER(pciconf_resource_reservations);

typedef struct _s_pciconf_dev_t {
	int		ipin;
	int		iline;
	int		min_gnt;
	int		max_lat;
	int		enable;
	pcitag_t	tag;
	pci_chipset_tag_t	pc;
	struct _s_pciconf_bus_t	*ppb;		/* I am really a bridge */
	pcireg_t	ea_cap_ptr;
} pciconf_dev_t;

typedef struct _s_pciconf_win_t {
	pciconf_dev_t	*dev;
	int		reg;			/* 0 for busses */
	int		align;
	int		prefetch;
	uint64_t	size;
	uint64_t	address;
} pciconf_win_t;

typedef struct _s_pciconf_bus_t {
	int		busno;
	int		next_busno;
	int		last_busno;
	int		max_mingnt;
	int		min_maxlat;
	int		cacheline_size;
	int		prefetch;
	int		fast_b2b;
	int		freq_66;
	int		def_ltim;
	int		max_ltim;
	int		bandwidth_used;
	int		swiz;
	int		io_32bit;
	int		pmem_64bit;
	int		mem_64bit;
	int		io_align;
	int		mem_align;
	int		pmem_align;

	int		ndevs;
	pciconf_dev_t	device[MAX_CONF_DEV];

	/* These should be sorted in order of decreasing size */
	int		nmemwin;
	pciconf_win_t	pcimemwin[MAX_CONF_MEM];
	int		niowin;
	pciconf_win_t	pciiowin[MAX_CONF_IO];

	bus_size_t	io_total;
	bus_size_t	mem_total;
	bus_size_t	pmem_total;

	struct pciconf_resource io_res;
	struct pciconf_resource mem_res;
	struct pciconf_resource pmem_res;

	pci_chipset_tag_t	pc;
	struct _s_pciconf_bus_t *parent_bus;
} pciconf_bus_t;

static int	probe_bus(pciconf_bus_t *);
static void	alloc_busno(pciconf_bus_t *, pciconf_bus_t *);
static void	set_busreg(pci_chipset_tag_t, pcitag_t, int, int, int);
static int	pci_do_device_query(pciconf_bus_t *, pcitag_t, int, int, int);
static int	setup_iowins(pciconf_bus_t *);
static int	setup_memwins(pciconf_bus_t *);
static int	configure_bridge(pciconf_dev_t *);
static int	configure_bus(pciconf_bus_t *);
static uint64_t	pci_allocate_range(struct pciconf_resource *, uint64_t, int,
		    bool);
static pciconf_win_t	*get_io_desc(pciconf_bus_t *, bus_size_t);
static pciconf_win_t	*get_mem_desc(pciconf_bus_t *, bus_size_t);
static pciconf_bus_t	*query_bus(pciconf_bus_t *, pciconf_dev_t *, int);

static void	print_tag(pci_chipset_tag_t, pcitag_t);

static vmem_t *
create_vmem_arena(const char *name, bus_addr_t start, bus_size_t size,
    int flags)
{
	KASSERT(start < VMEM_ADDR_MAX);
	KASSERT(size == 0 ||
		(VMEM_ADDR_MAX - start) >= (size - 1));

	return vmem_create(name, start, size,
			   1,		/*quantum*/
			   NULL,	/*importfn*/
			   NULL,	/*releasefn*/
			   NULL,	/*source*/
			   0,		/*qcache_max*/
			   flags,
			   IPL_NONE);
}

static int
init_range_resource(struct pciconf_resource *r, const char *name,
    bus_addr_t start, bus_addr_t size)
{
	r->arena = create_vmem_arena(name, start, size, VM_NOSLEEP);
	if (r->arena == NULL)
		return ENOMEM;

	r->min_addr = start;
	r->max_addr = start + (size - 1);
	r->total_size = size;

	return 0;
}

static void
fini_range_resource(struct pciconf_resource *r)
{
	if (r->arena) {
		vmem_xfreeall(r->arena);
		vmem_destroy(r->arena);
	}
	memset(r, 0, sizeof(*r));
}

static void
print_tag(pci_chipset_tag_t pc, pcitag_t tag)
{
	int	bus, dev, func;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	printf("PCI: bus %d, device %d, function %d: ", bus, dev, func);
}

#ifdef _LP64
#define	__used_only_lp64	__unused
#else
#define	__used_only_lp64	/* nothing */
#endif /* _LP64 */

/************************************************************************/
/************************************************************************/
/***********************   Bus probing routines   ***********************/
/************************************************************************/
/************************************************************************/
static pciconf_win_t *
get_io_desc(pciconf_bus_t *pb, bus_size_t size)
{
	int	i, n;

	n = pb->niowin;
	for (i = n; i > 0 && size > pb->pciiowin[i-1].size; i--)
		pb->pciiowin[i] = pb->pciiowin[i-1]; /* struct copy */
	return &pb->pciiowin[i];
}

static pciconf_win_t *
get_mem_desc(pciconf_bus_t *pb, bus_size_t size)
{
	int	i, n;

	n = pb->nmemwin;
	for (i = n; i > 0 && size > pb->pcimemwin[i-1].size; i--)
		pb->pcimemwin[i] = pb->pcimemwin[i-1]; /* struct copy */
	return &pb->pcimemwin[i];
}

/*
 * Set up bus common stuff, then loop over devices & functions.
 * If we find something, call pci_do_device_query()).
 */
static int
probe_bus(pciconf_bus_t *pb)
{
	int device;
	uint8_t devs[32];
	int i, n;

	pb->ndevs = 0;
	pb->niowin = 0;
	pb->nmemwin = 0;
	pb->freq_66 = 1;
#ifdef PCICONF_NO_FAST_B2B
	pb->fast_b2b = 0;
#else
	pb->fast_b2b = 1;
#endif
	pb->prefetch = 1;
	pb->max_mingnt = 0;	/* we are looking for the maximum */
	pb->min_maxlat = 0x100;	/* we are looking for the minimum */
	pb->bandwidth_used = 0;

	n = pci_bus_devorder(pb->pc, pb->busno, devs, __arraycount(devs));
	for (i = 0; i < n; i++) {
		pcitag_t tag;
		pcireg_t id, bhlcr;
		int function, nfunction;
		int confmode;

		device = devs[i];

		tag = pci_make_tag(pb->pc, pb->busno, device, 0);
		if (pci_conf_debug) {
			print_tag(pb->pc, tag);
		}
		id = pci_conf_read(pb->pc, tag, PCI_ID_REG);

		if (pci_conf_debug) {
			printf("id=%x: Vendor=%x, Product=%x\n",
			    id, PCI_VENDOR(id), PCI_PRODUCT(id));
		}
		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;

		bhlcr = pci_conf_read(pb->pc, tag, PCI_BHLC_REG);
		nfunction = PCI_HDRTYPE_MULTIFN(bhlcr) ? 8 : 1;
		for (function = 0; function < nfunction; function++) {
			tag = pci_make_tag(pb->pc, pb->busno, device, function);
			id = pci_conf_read(pb->pc, tag, PCI_ID_REG);
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
				continue;
			if (pb->ndevs + 1 < MAX_CONF_DEV) {
				if (pci_conf_debug) {
					print_tag(pb->pc, tag);
					printf("Found dev 0x%04x 0x%04x -- "
					    "really probing.\n",
					PCI_VENDOR(id), PCI_PRODUCT(id));
				}
#ifdef __HAVE_PCI_CONF_HOOK
				confmode = pci_conf_hook(pb->pc, pb->busno,
				    device, function, id);
				if (confmode == 0)
					continue;
#else
				/*
				 * Don't enable expansion ROMS -- some cards
				 * share address decoders between the EXPROM
				 * and PCI memory space, and enabling the ROM
				 * when not needed will cause all sorts of
				 * lossage.
				 */
				confmode = PCI_CONF_DEFAULT;
#endif
				if (pci_do_device_query(pb, tag, device,
				    function, confmode))
					return -1;
				pb->ndevs++;
			}
		}
	}
	return 0;
}

static void
alloc_busno(pciconf_bus_t *parent, pciconf_bus_t *pb)
{
	pb->busno = parent->next_busno;
	pb->next_busno = pb->busno + 1;
}

static void
set_busreg(pci_chipset_tag_t pc, pcitag_t tag, int prim, int sec, int sub)
{
	pcireg_t	busreg;

	busreg  = __SHIFTIN(prim, PCI_BRIDGE_BUS_PRIMARY);
	busreg |= __SHIFTIN(sec,  PCI_BRIDGE_BUS_SECONDARY);
	busreg |= __SHIFTIN(sub,  PCI_BRIDGE_BUS_SUBORDINATE);
	pci_conf_write(pc, tag, PCI_BRIDGE_BUS_REG, busreg);
}

static pciconf_bus_t *
query_bus(pciconf_bus_t *parent, pciconf_dev_t *pd, int dev)
{
	pciconf_bus_t	*pb;
	pcireg_t	io, pmem;
	pciconf_win_t	*pi, *pm;

	pb = kmem_zalloc(sizeof (pciconf_bus_t), KM_SLEEP);
	pb->cacheline_size = parent->cacheline_size;
	pb->parent_bus = parent;
	alloc_busno(parent, pb);

	pb->mem_align = 0x100000;	/* 1M alignment */
	pb->pmem_align = 0x100000;	/* 1M alignment */
	pb->io_align = 0x1000;		/* 4K alignment */

	set_busreg(parent->pc, pd->tag, parent->busno, pb->busno, 0xff);

	pb->swiz = parent->swiz + dev;

	memset(&pb->io_res, 0, sizeof(pb->io_res));
	memset(&pb->mem_res, 0, sizeof(pb->mem_res));
	memset(&pb->pmem_res, 0, sizeof(pb->pmem_res));

	pb->pc = parent->pc;
	pb->io_total = pb->mem_total = pb->pmem_total = 0;

	pb->io_32bit = 0;
	if (parent->io_32bit) {
		io = pci_conf_read(parent->pc, pd->tag, PCI_BRIDGE_STATIO_REG);
		if (PCI_BRIDGE_IO_32BITS(io))
			pb->io_32bit = 1;
	}

	pb->pmem_64bit = 0;
	if (parent->pmem_64bit) {
		pmem = pci_conf_read(parent->pc, pd->tag,
		    PCI_BRIDGE_PREFETCHMEM_REG);
		if (PCI_BRIDGE_PREFETCHMEM_64BITS(pmem))
			pb->pmem_64bit = 1;
	}

	/* Bridges only forward a 32-bit range of non-prefetcable memory. */
	pb->mem_64bit = 0;

	if (probe_bus(pb)) {
		printf("Failed to probe bus %d\n", pb->busno);
		goto err;
	}

	/* We have found all subordinate busses now, reprogram busreg. */
	pb->last_busno = pb->next_busno - 1;
	parent->next_busno = pb->next_busno;
	set_busreg(parent->pc, pd->tag, parent->busno, pb->busno,
		   pb->last_busno);
	if (pci_conf_debug)
		printf("PCI bus bridge (parent %d) covers busses %d-%d\n",
			parent->busno, pb->busno, pb->last_busno);

	if (pb->io_total > 0) {
		if (parent->niowin >= MAX_CONF_IO) {
			printf("pciconf: too many (%d) I/O windows\n",
			    parent->niowin);
			goto err;
		}
		pb->io_total |= pb->io_align - 1; /* Round up */
		pi = get_io_desc(parent, pb->io_total);
		pi->dev = pd;
		pi->reg = 0;
		pi->size = pb->io_total;
		pi->align = pb->io_align;	/* 4K min alignment */
		if (parent->io_align < pb->io_align)
			parent->io_align = pb->io_align;
		pi->prefetch = 0;
		parent->niowin++;
		parent->io_total += pb->io_total;
	}

	if (pb->mem_total > 0) {
		if (parent->nmemwin >= MAX_CONF_MEM) {
			printf("pciconf: too many (%d) MEM windows\n",
			     parent->nmemwin);
			goto err;
		}
		pb->mem_total |= pb->mem_align - 1; /* Round up */
		pm = get_mem_desc(parent, pb->mem_total);
		pm->dev = pd;
		pm->reg = 0;
		pm->size = pb->mem_total;
		pm->align = pb->mem_align;	/* 1M min alignment */
		if (parent->mem_align < pb->mem_align)
			parent->mem_align = pb->mem_align;
		pm->prefetch = 0;
		parent->nmemwin++;
		parent->mem_total += pb->mem_total;
	}

	if (pb->pmem_total > 0) {
		if (parent->nmemwin >= MAX_CONF_MEM) {
			printf("pciconf: too many MEM windows\n");
			goto err;
		}
		pb->pmem_total |= pb->pmem_align - 1; /* Round up */
		pm = get_mem_desc(parent, pb->pmem_total);
		pm->dev = pd;
		pm->reg = 0;
		pm->size = pb->pmem_total;
		pm->align = pb->pmem_align;	/* 1M alignment */
		if (parent->pmem_align < pb->pmem_align)
			parent->pmem_align = pb->pmem_align;
		pm->prefetch = 1;
		parent->nmemwin++;
		parent->pmem_total += pb->pmem_total;
	}

	return pb;
err:
	kmem_free(pb, sizeof(*pb));
	return NULL;
}

static struct pciconf_resource_rsvd *
pci_resource_is_reserved(int type, uint64_t addr, uint64_t size)
{
	struct pciconf_resource_rsvd *rsvd;

	LIST_FOREACH(rsvd, &pciconf_resource_reservations, next) {
		if (rsvd->type != type)
			continue;
		if (rsvd->start <= addr + size && rsvd->start + rsvd->size >= addr)
			return rsvd;
	}

	return NULL;
}

static struct pciconf_resource_rsvd *
pci_bar_is_reserved(pciconf_bus_t *pb, pciconf_dev_t *pd, int br)
{
	pcireg_t base, base64, mask, mask64;
	pcitag_t tag;
	uint64_t addr, size;

	/*
	 * Resource reservation does not apply to bridges
	 */
	if (pd->ppb)
		return NULL;

	tag = pd->tag;

	/*
	 * Look to see if this device is enabled and one of the resources
	 * is already in use (eg. firmware configured console device).
	 */
	base = pci_conf_read(pb->pc, tag, br);
	pci_conf_write(pb->pc, tag, br, 0xffffffff);
	mask = pci_conf_read(pb->pc, tag, br);
	pci_conf_write(pb->pc, tag, br, base);

	switch (PCI_MAPREG_TYPE(base)) {
	case PCI_MAPREG_TYPE_IO:
		addr = PCI_MAPREG_IO_ADDR(base);
		size = PCI_MAPREG_IO_SIZE(mask);
		return pci_resource_is_reserved(PCI_CONF_MAP_IO, addr, size);

	case PCI_MAPREG_TYPE_MEM:
		if (PCI_MAPREG_MEM_TYPE(base) == PCI_MAPREG_MEM_TYPE_64BIT) {
			base64 = pci_conf_read(pb->pc, tag, br + 4);
			pci_conf_write(pb->pc, tag, br + 4, 0xffffffff);
			mask64 = pci_conf_read(pb->pc, tag, br + 4);
			pci_conf_write(pb->pc, tag, br + 4, base64);
			addr = (uint64_t)PCI_MAPREG_MEM64_ADDR(
			      (((uint64_t)base64) << 32) | base);
			size = (uint64_t)PCI_MAPREG_MEM64_SIZE(
			      (((uint64_t)mask64) << 32) | mask);
		} else {
			addr = PCI_MAPREG_MEM_ADDR(base);
			size = PCI_MAPREG_MEM_SIZE(mask);
		}
		return pci_resource_is_reserved(PCI_CONF_MAP_MEM, addr, size);

	default:
		return NULL;
	}
}

static int
pci_do_device_query(pciconf_bus_t *pb, pcitag_t tag, int dev, int func,
    int mode)
{
	pciconf_dev_t	*pd;
	pciconf_win_t	*pi, *pm;
	pcireg_t	classreg, cmd, icr, bhlc, bar, mask, bar64, mask64,
	    busreg;
	uint64_t	size;
	int		br, width, reg_start, reg_end;

	pd = &pb->device[pb->ndevs];
	pd->pc = pb->pc;
	pd->tag = tag;
	pd->ppb = NULL;
	pd->enable = mode;
	pd->ea_cap_ptr = 0;

	classreg = pci_conf_read(pb->pc, tag, PCI_CLASS_REG);

	cmd = pci_conf_read(pb->pc, tag, PCI_COMMAND_STATUS_REG);
	bhlc = pci_conf_read(pb->pc, tag, PCI_BHLC_REG);

	if (pci_get_capability(pb->pc, tag, PCI_CAP_EA, &pd->ea_cap_ptr,
	    NULL)) {
		/* XXX Skip devices with EA for now. */
		print_tag(pb->pc, tag);
		printf("skipping devices with Enhanced Allocations\n");
		return 0;
	}

	if (PCI_CLASS(classreg) != PCI_CLASS_BRIDGE
	    && PCI_HDRTYPE_TYPE(bhlc) != PCI_HDRTYPE_PPB) {
		cmd &= ~(PCI_COMMAND_MASTER_ENABLE |
		    PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE);
		pci_conf_write(pb->pc, tag, PCI_COMMAND_STATUS_REG, cmd);
	} else if (pci_conf_debug) {
		print_tag(pb->pc, tag);
		printf("device is a bridge; not clearing enables\n");
	}

	if ((cmd & PCI_STATUS_BACKTOBACK_SUPPORT) == 0)
		pb->fast_b2b = 0;

	if ((cmd & PCI_STATUS_66MHZ_SUPPORT) == 0)
		pb->freq_66 = 0;

	switch (PCI_HDRTYPE_TYPE(bhlc)) {
	case PCI_HDRTYPE_DEVICE:
		reg_start = PCI_MAPREG_START;
		reg_end = PCI_MAPREG_END;
		break;
	case PCI_HDRTYPE_PPB:
		pd->ppb = query_bus(pb, pd, dev);
		if (pd->ppb == NULL)
			return -1;
		return 0;
	case PCI_HDRTYPE_PCB:
		reg_start = PCI_MAPREG_START;
		reg_end = PCI_MAPREG_PCB_END;

		busreg = pci_conf_read(pb->pc, tag, PCI_BUSNUM);
		busreg = (busreg & 0xff000000) |
		    __SHIFTIN(pb->busno, PCI_BRIDGE_BUS_PRIMARY) |
		    __SHIFTIN(pb->next_busno, PCI_BRIDGE_BUS_SECONDARY) |
		    __SHIFTIN(pb->next_busno, PCI_BRIDGE_BUS_SUBORDINATE);
		pci_conf_write(pb->pc, tag, PCI_BUSNUM, busreg);

		pb->next_busno++;
		break;
	default:
		return -1;
	}

	icr = pci_conf_read(pb->pc, tag, PCI_INTERRUPT_REG);
	pd->ipin = PCI_INTERRUPT_PIN(icr);
	pd->iline = PCI_INTERRUPT_LINE(icr);
	pd->min_gnt = PCI_MIN_GNT(icr);
	pd->max_lat = PCI_MAX_LAT(icr);
	if (pd->iline || pd->ipin) {
		pci_conf_interrupt(pb->pc, pb->busno, dev, pd->ipin, pb->swiz,
		    &pd->iline);
		icr &= ~(PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
		icr |= (pd->iline << PCI_INTERRUPT_LINE_SHIFT);
		pci_conf_write(pb->pc, tag, PCI_INTERRUPT_REG, icr);
	}

	if (pd->min_gnt != 0 || pd->max_lat != 0) {
		if (pd->min_gnt != 0 && pd->min_gnt > pb->max_mingnt)
			pb->max_mingnt = pd->min_gnt;

		if (pd->max_lat != 0 && pd->max_lat < pb->min_maxlat)
			pb->min_maxlat = pd->max_lat;

		pb->bandwidth_used += pd->min_gnt * 4000000 /
				(pd->min_gnt + pd->max_lat);
	}

	width = 4;
	for (br = reg_start; br < reg_end; br += width) {
#if 0
/* XXX Should only ignore if IDE not in legacy mode? */
		if (PCI_CLASS(classreg) == PCI_CLASS_MASS_STORAGE &&
		    PCI_SUBCLASS(classreg) == PCI_SUBCLASS_MASS_STORAGE_IDE) {
			break;
		}
#endif
		bar = pci_conf_read(pb->pc, tag, br);
		pci_conf_write(pb->pc, tag, br, 0xffffffff);
		mask = pci_conf_read(pb->pc, tag, br);
		pci_conf_write(pb->pc, tag, br, bar);
		width = 4;

		if (   (mode & PCI_CONF_MAP_IO)
		    && (PCI_MAPREG_TYPE(mask) == PCI_MAPREG_TYPE_IO)) {
			/*
			 * Upper 16 bits must be one.  Devices may hardwire
			 * them to zero, though, per PCI 2.2, 6.2.5.1, p 203.
			 */
			mask |= 0xffff0000;

			size = PCI_MAPREG_IO_SIZE(mask);
			if (size == 0) {
				if (pci_conf_debug) {
					print_tag(pb->pc, tag);
					printf("I/O BAR 0x%x is void\n", br);
				}
				continue;
			}

			if (pb->niowin >= MAX_CONF_IO) {
				printf("pciconf: too many I/O windows\n");
				return -1;
			}

			pi = get_io_desc(pb, size);
			pi->dev = pd;
			pi->reg = br;
			pi->size = (uint64_t)size;
			pi->align = 4;
			if (pb->io_align < pi->size)
				pb->io_align = pi->size;
			pi->prefetch = 0;
			if (pci_conf_debug) {
				print_tag(pb->pc, tag);
				printf("Register 0x%x, I/O size %" PRIu64 "\n",
				    br, pi->size);
			}
			pb->niowin++;
			pb->io_total += size;
		} else if ((mode & PCI_CONF_MAP_MEM)
			   && (PCI_MAPREG_TYPE(mask) == PCI_MAPREG_TYPE_MEM)) {
			switch (PCI_MAPREG_MEM_TYPE(mask)) {
			case PCI_MAPREG_MEM_TYPE_32BIT:
			case PCI_MAPREG_MEM_TYPE_32BIT_1M:
				size = (uint64_t)PCI_MAPREG_MEM_SIZE(mask);
				break;
			case PCI_MAPREG_MEM_TYPE_64BIT:
				bar64 = pci_conf_read(pb->pc, tag, br + 4);
				pci_conf_write(pb->pc, tag, br + 4, 0xffffffff);
				mask64 = pci_conf_read(pb->pc, tag, br + 4);
				pci_conf_write(pb->pc, tag, br + 4, bar64);
				size = (uint64_t)PCI_MAPREG_MEM64_SIZE(
				      (((uint64_t)mask64) << 32) | mask);
				width = 8;
				break;
			default:
				print_tag(pb->pc, tag);
				printf("reserved mapping type 0x%x\n",
					PCI_MAPREG_MEM_TYPE(mask));
				continue;
			}

			if (size == 0) {
				if (pci_conf_debug) {
					print_tag(pb->pc, tag);
					printf("MEM%d BAR 0x%x is void\n",
					    PCI_MAPREG_MEM_TYPE(mask) ==
						PCI_MAPREG_MEM_TYPE_64BIT ?
						64 : 32, br);
				}
				continue;
			} else {
				if (pci_conf_debug) {
					print_tag(pb->pc, tag);
					printf("MEM%d BAR 0x%x has size %#lx\n",
					    PCI_MAPREG_MEM_TYPE(mask) ==
						PCI_MAPREG_MEM_TYPE_64BIT ?
						64 : 32,
					    br, (unsigned long)size);
				}
			}

			if (pb->nmemwin >= MAX_CONF_MEM) {
				printf("pciconf: too many memory windows\n");
				return -1;
			}

			pm = get_mem_desc(pb, size);
			pm->dev = pd;
			pm->reg = br;
			pm->size = size;
			pm->align = 4;
			pm->prefetch = PCI_MAPREG_MEM_PREFETCHABLE(mask);
			if (pci_conf_debug) {
				print_tag(pb->pc, tag);
				printf("Register 0x%x, memory size %"
				    PRIu64 "\n", br, pm->size);
			}
			pb->nmemwin++;
			if (pm->prefetch) {
				pb->pmem_total += size;
				if (pb->pmem_align < pm->size)
					pb->pmem_align = pm->size;
			} else {
				pb->mem_total += size;
				if (pb->mem_align < pm->size)
					pb->mem_align = pm->size;
			}
		}
	}

	if (mode & PCI_CONF_MAP_ROM) {
		bar = pci_conf_read(pb->pc, tag, PCI_MAPREG_ROM);
		pci_conf_write(pb->pc, tag, PCI_MAPREG_ROM, 0xfffffffe);
		mask = pci_conf_read(pb->pc, tag, PCI_MAPREG_ROM);
		pci_conf_write(pb->pc, tag, PCI_MAPREG_ROM, bar);

		if (mask != 0 && mask != 0xffffffff) {
			if (pb->nmemwin >= MAX_CONF_MEM) {
				printf("pciconf: too many memory windows\n");
				return -1;
			}
			size = (uint64_t)PCI_MAPREG_MEM_SIZE(mask);

			pm = get_mem_desc(pb, size);
			pm->dev = pd;
			pm->reg = PCI_MAPREG_ROM;
			pm->size = size;
			pm->align = 4;
			pm->prefetch = 0;
			if (pci_conf_debug) {
				print_tag(pb->pc, tag);
				printf("Expansion ROM memory size %"
				    PRIu64 "\n", pm->size);
			}
			pb->nmemwin++;
			if (pm->prefetch) {
				pb->pmem_total += size;
				if (pb->pmem_align < pm->size)
					pb->pmem_align = pm->size;
			} else {
				pb->mem_total += size;
				if (pb->mem_align < pm->size)
					pb->mem_align = pm->size;
			}
		}
	} else {
		/* Don't enable ROMs if we aren't going to map them. */
		mode &= ~PCI_CONF_ENABLE_ROM;
		pd->enable &= ~PCI_CONF_ENABLE_ROM;
	}

	if (!(mode & PCI_CONF_ENABLE_ROM)) {
		/* Ensure ROM is disabled */
		bar = pci_conf_read(pb->pc, tag, PCI_MAPREG_ROM);
		pci_conf_write(pb->pc, tag, PCI_MAPREG_ROM,
		    bar & ~PCI_MAPREG_ROM_ENABLE);
	}

	return 0;
}

/************************************************************************/
/************************************************************************/
/********************   Bus configuration routines   ********************/
/************************************************************************/
/************************************************************************/
static uint64_t
pci_allocate_range(struct pciconf_resource * const r, const uint64_t amt,
		   const int align, const bool ok64 __used_only_lp64)
{
	vmem_size_t const size = (vmem_size_t) amt;
	vmem_addr_t result;
	int error;

#ifdef _LP64
	/*
	 * If a 64-bit range IS OK, then we prefer allocating above 4GB.
	 *
	 * XXX We guard this with _LP64 because vmem uses uintptr_t
	 * internally.
	 */
	if (!ok64) {
		error = vmem_xalloc(r->arena, size, align, 0, 0,
				    VMEM_ADDR_MIN, 0xffffffffUL,
				    VM_BESTFIT | VM_NOSLEEP,
				    &result);
	} else {
		error = vmem_xalloc(r->arena, size, align, 0, 0,
				    (1UL << 32), VMEM_ADDR_MAX,
				    VM_BESTFIT | VM_NOSLEEP,
				    &result);
		if (error) {
			error = vmem_xalloc(r->arena, size, align, 0, 0,
					    VMEM_ADDR_MIN, VMEM_ADDR_MAX,
					    VM_BESTFIT | VM_NOSLEEP,
					    &result);
		}
	}
#else
	error = vmem_xalloc(r->arena, size, align, 0, 0,
			    VMEM_ADDR_MIN, 0xffffffffUL,
			    VM_BESTFIT | VM_NOSLEEP,
			    &result);
#endif /* _L64 */

	if (error)
		return ~0ULL;

	return result;
}

static int
setup_iowins(pciconf_bus_t *pb)
{
	pciconf_win_t	*pi;
	pciconf_dev_t	*pd;
	struct pciconf_resource_rsvd *rsvd;
	int		error;

	for (pi = pb->pciiowin; pi < &pb->pciiowin[pb->niowin]; pi++) {
		if (pi->size == 0)
			continue;

		pd = pi->dev;
		rsvd = pci_bar_is_reserved(pb, pd, pi->reg);

		if (pb->io_res.arena == NULL) {
			/* Bus has no IO ranges, disable IO BAR */
			pi->address = 0;
			pd->enable &= ~PCI_CONF_ENABLE_IO;
			goto write_ioaddr;
		}

		pi->address = pci_allocate_range(&pb->io_res, pi->size,
		    pi->align, false);
		if (~pi->address == 0) {
			print_tag(pd->pc, pd->tag);
			printf("Failed to allocate PCI I/O space (%"
			    PRIu64 " req)\n", pi->size);
			return -1;
		}
		if (pd->ppb && pi->reg == 0) {
			error = init_range_resource(&pd->ppb->io_res,
			    "ppb-io", pi->address, pi->size);
			if (error) {
				print_tag(pd->pc, pd->tag);
				printf("Failed to alloc I/O arena for bus %d\n",
				    pd->ppb->busno);
				return -1;
			}
			continue;
		}
		if (!pb->io_32bit && pi->address > 0xFFFF) {
			pi->address = 0;
			pd->enable &= ~PCI_CONF_ENABLE_IO;
		} else {
			pd->enable |= PCI_CONF_ENABLE_IO;
		}
write_ioaddr:
		if (pci_conf_debug) {
			print_tag(pd->pc, pd->tag);
			printf("Putting %" PRIu64 " I/O bytes @ %#" PRIx64
			    " (reg %x)\n", pi->size, pi->address, pi->reg);
		}
		pci_conf_write(pd->pc, pd->tag, pi->reg,
		    PCI_MAPREG_IO_ADDR(pi->address) | PCI_MAPREG_TYPE_IO);

		if (rsvd != NULL && rsvd->start != pi->address)
			rsvd->callback(rsvd->callback_arg, pi->address);
	}
	return 0;
}

static int
setup_memwins(pciconf_bus_t *pb)
{
	pciconf_win_t	*pm;
	pciconf_dev_t	*pd;
	pcireg_t	base;
	struct pciconf_resource *r;
	struct pciconf_resource_rsvd *rsvd;
	bool		ok64;
	int		error;

	for (pm = pb->pcimemwin; pm < &pb->pcimemwin[pb->nmemwin]; pm++) {
		if (pm->size == 0)
			continue;

		ok64 = false;
		pd = pm->dev;
		rsvd = pci_bar_is_reserved(pb, pd, pm->reg);

		if (pm->prefetch) {
			r = &pb->pmem_res;
			ok64 = pb->pmem_64bit;
		} else {
			r = &pb->mem_res;
			ok64 = pb->mem_64bit && pd->ppb == NULL;
		}

		/*
		 * We need to figure out if the memory BAR is 64-bit
		 * capable or not.  If it's not, then we need to constrain
		 * the address allocation.
		 */
		if (pm->reg == PCI_MAPREG_ROM) {
			ok64 = false;
		} else if (ok64) {
			base = pci_conf_read(pd->pc, pd->tag, pm->reg);
			ok64 = PCI_MAPREG_MEM_TYPE(base) ==
			    PCI_MAPREG_MEM_TYPE_64BIT;
		}

		pm->address = pci_allocate_range(r, pm->size, pm->align,
						 ok64);
		if (~pm->address == 0 && r == &pb->pmem_res) {
			r = &pb->mem_res;
			pm->address = pci_allocate_range(r, pm->size,
							 pm->align, ok64);
		}
		if (~pm->address == 0) {
			print_tag(pd->pc, pd->tag);
			printf(
			   "Failed to allocate PCI memory space (%" PRIu64
			   " req, prefetch=%d ok64=%d)\n", pm->size,
			   pm->prefetch, (int)ok64);
			return -1;
		}
		if (pd->ppb && pm->reg == 0) {
			const char *name = pm->prefetch ? "ppb-pmem"
							: "ppb-mem";
			r = pm->prefetch ? &pd->ppb->pmem_res
					 : &pd->ppb->mem_res;
			error = init_range_resource(r, name,
			    pm->address, pm->size);
			if (error) {
				print_tag(pd->pc, pd->tag);
				printf("Failed to alloc MEM arena for bus %d\n",
				    pd->ppb->busno);
				return -1;
			}
			continue;
		}
		if (!ok64 && pm->address > 0xFFFFFFFFULL) {
			pm->address = 0;
			pd->enable &= ~PCI_CONF_ENABLE_MEM;
		} else
			pd->enable |= PCI_CONF_ENABLE_MEM;

		if (pm->reg != PCI_MAPREG_ROM) {
			if (pci_conf_debug) {
				print_tag(pd->pc, pd->tag);
				printf(
				    "Putting %" PRIu64 " MEM bytes @ %#"
				    PRIx64 " (reg %x)\n", pm->size,
				    pm->address, pm->reg);
			}
			base = pci_conf_read(pd->pc, pd->tag, pm->reg);
			base = PCI_MAPREG_MEM_ADDR(pm->address) |
			    PCI_MAPREG_MEM_TYPE(base);
			pci_conf_write(pd->pc, pd->tag, pm->reg, base);
			if (PCI_MAPREG_MEM_TYPE(base) ==
			    PCI_MAPREG_MEM_TYPE_64BIT) {
				base = (pcireg_t)
				    (PCI_MAPREG_MEM64_ADDR(pm->address) >> 32);
				pci_conf_write(pd->pc, pd->tag, pm->reg + 4,
				    base);
			}
		}

		if (rsvd != NULL && rsvd->start != pm->address) {
			/*
			 * Resource allocation will never reuse a reserved
			 * address. Check to see if the BAR is still reserved
			 * to cover the case where the new resource was not
			 * applied. In this case, there is no need to notify
			 * the device callback of a change.
			 */
			if (!pci_bar_is_reserved(pb, pd, pm->reg)) {
				rsvd->callback(rsvd->callback_arg, pm->address);
			}
		}
	}
	for (pm = pb->pcimemwin; pm < &pb->pcimemwin[pb->nmemwin]; pm++) {
		if (pm->reg == PCI_MAPREG_ROM && pm->address != -1) {
			pd = pm->dev;
			if (!(pd->enable & PCI_CONF_MAP_ROM))
				continue;
			if (pci_conf_debug) {
				print_tag(pd->pc, pd->tag);
				printf(
				    "Putting %" PRIu64 " ROM bytes @ %#"
				    PRIx64 " (reg %x)\n", pm->size,
				    pm->address, pm->reg);
			}
			base = (pcireg_t) pm->address;
			if (pd->enable & PCI_CONF_ENABLE_ROM)
				base |= PCI_MAPREG_ROM_ENABLE;

			pci_conf_write(pd->pc, pd->tag, pm->reg, base);
		}
	}
	return 0;
}

static bool
constrain_bridge_mem_range(struct pciconf_resource * const r,
			   u_long * const base,
			   u_long * const limit,
			   const bool ok64 __used_only_lp64)
{

	*base = r->min_addr;
	*limit = r->max_addr;

#ifdef _LP64
	if (!ok64) {
		if (r->min_addr >= (1UL << 32)) {
			return true;
		}
		if (r->max_addr > 0xffffffffUL) {
			*limit = 0xffffffffUL;
		}
	}
#endif /* _LP64 */

	return false;
}

/*
 * Configure I/O, memory, and prefetcable memory spaces, then make
 * a call to configure_bus().
 */
static int
configure_bridge(pciconf_dev_t *pd)
{
	unsigned long	io_base, io_limit, mem_base, mem_limit;
	pciconf_bus_t	*pb;
	pcireg_t	io, iohigh, mem, cmd;
	int		rv;
	bool		isprefetchmem64;
	bool		bad_range;

	pb = pd->ppb;
	/* Configure I/O base & limit*/
	if (pb->io_res.arena) {
		io_base = pb->io_res.min_addr;
		io_limit = pb->io_res.max_addr;
	} else {
		io_base  = 0x1000;	/* 4K */
		io_limit = 0x0000;
	}
	if (pb->io_32bit) {
		iohigh = __SHIFTIN(io_base >> 16, PCI_BRIDGE_IOHIGH_BASE) |
		    __SHIFTIN(io_limit >> 16, PCI_BRIDGE_IOHIGH_LIMIT);
	} else {
		if (io_limit > 0xFFFF) {
			printf("Bus %d bridge does not support 32-bit I/O.  ",
			    pb->busno);
			printf("Disabling I/O accesses\n");
			io_base  = 0x1000;	/* 4K */
			io_limit = 0x0000;
		}
		iohigh = 0;
	}
	io = pci_conf_read(pb->pc, pd->tag, PCI_BRIDGE_STATIO_REG) &
	    PCI_BRIDGE_STATIO_STATUS;
	io |= __SHIFTIN((io_base >> 8) & PCI_BRIDGE_STATIO_IOADDR,
	    PCI_BRIDGE_STATIO_IOBASE);
	io |= __SHIFTIN((io_limit >> 8) & PCI_BRIDGE_STATIO_IOADDR,
	    PCI_BRIDGE_STATIO_IOLIMIT);
	pci_conf_write(pb->pc, pd->tag, PCI_BRIDGE_STATIO_REG, io);
	pci_conf_write(pb->pc, pd->tag, PCI_BRIDGE_IOHIGH_REG, iohigh);

	/* Configure mem base & limit */
	bad_range = false;
	if (pb->mem_res.arena) {
		bad_range = constrain_bridge_mem_range(&pb->mem_res,
						       &mem_base,
						       &mem_limit,
						       false);
	} else {
		mem_base  = 0x100000;	/* 1M */
		mem_limit = 0x000000;
	}
	if (bad_range) {
		printf("Bus %d bridge MEM range out of range.  ", pb->busno);
		printf("Disabling MEM accesses\n");
		mem_base  = 0x100000;	/* 1M */
		mem_limit = 0x000000;
	}
	mem = __SHIFTIN((mem_base >> 16) & PCI_BRIDGE_MEMORY_ADDR,
	    PCI_BRIDGE_MEMORY_BASE);
	mem |= __SHIFTIN((mem_limit >> 16) & PCI_BRIDGE_MEMORY_ADDR,
	    PCI_BRIDGE_MEMORY_LIMIT);
	pci_conf_write(pb->pc, pd->tag, PCI_BRIDGE_MEMORY_REG, mem);

	/* Configure prefetchable mem base & limit */
	mem = pci_conf_read(pb->pc, pd->tag, PCI_BRIDGE_PREFETCHMEM_REG);
	isprefetchmem64 = PCI_BRIDGE_PREFETCHMEM_64BITS(mem);
	bad_range = false;
	if (pb->pmem_res.arena) {
		bad_range = constrain_bridge_mem_range(&pb->pmem_res,
						       &mem_base,
						       &mem_limit,
						       isprefetchmem64);
	} else {
		mem_base  = 0x100000;	/* 1M */
		mem_limit = 0x000000;
	}
	if (bad_range) {
		printf("Bus %d bridge does not support 64-bit PMEM.  ",
		    pb->busno);
		printf("Disabling prefetchable-MEM accesses\n");
		mem_base  = 0x100000;	/* 1M */
		mem_limit = 0x000000;
	}
	mem = __SHIFTIN((mem_base >> 16) & PCI_BRIDGE_PREFETCHMEM_ADDR,
	    PCI_BRIDGE_PREFETCHMEM_BASE);
	mem |= __SHIFTIN((mem_limit >> 16) & PCI_BRIDGE_PREFETCHMEM_ADDR,
	    PCI_BRIDGE_PREFETCHMEM_LIMIT);
	pci_conf_write(pb->pc, pd->tag, PCI_BRIDGE_PREFETCHMEM_REG, mem);
	/*
	 * XXX -- 64-bit systems need a lot more than just this...
	 */
	if (isprefetchmem64) {
		mem_base  = (uint64_t)mem_base  >> 32;
		mem_limit = (uint64_t)mem_limit >> 32;
		pci_conf_write(pb->pc, pd->tag,
		    PCI_BRIDGE_PREFETCHBASEUP32_REG, mem_base & 0xffffffff);
		pci_conf_write(pb->pc, pd->tag,
		    PCI_BRIDGE_PREFETCHLIMITUP32_REG, mem_limit & 0xffffffff);
	}

	rv = configure_bus(pb);

	fini_range_resource(&pb->io_res);
	fini_range_resource(&pb->mem_res);
	fini_range_resource(&pb->pmem_res);

	if (rv == 0) {
		cmd = pci_conf_read(pd->pc, pd->tag, PCI_BRIDGE_CONTROL_REG);
		cmd &= ~PCI_BRIDGE_CONTROL; /* Clear control bit first */
		cmd |= PCI_BRIDGE_CONTROL_PERE | PCI_BRIDGE_CONTROL_SERR;
		if (pb->fast_b2b)
			cmd |= PCI_BRIDGE_CONTROL_SECFASTB2B;

		pci_conf_write(pd->pc, pd->tag, PCI_BRIDGE_CONTROL_REG, cmd);
		cmd = pci_conf_read(pd->pc, pd->tag, PCI_COMMAND_STATUS_REG);
		cmd |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE;
		pci_conf_write(pd->pc, pd->tag, PCI_COMMAND_STATUS_REG, cmd);
	}

	return rv;
}

/*
 * Calculate latency values, allocate I/O and MEM segments, then set them
 * up.  If a PCI-PCI bridge is found, configure the bridge separately,
 * which will cause a recursive call back here.
 */
static int
configure_bus(pciconf_bus_t *pb)
{
	pciconf_dev_t	*pd;
	int		def_ltim, max_ltim, band, bus_mhz;

	if (pb->ndevs == 0) {
		if (pci_conf_debug)
			printf("PCI bus %d - no devices\n", pb->busno);
		return 1;
	}
	bus_mhz = pb->freq_66 ? 66 : 33;
	max_ltim = pb->max_mingnt * bus_mhz / 4;	/* cvt to cycle count */
	band = 4000000;					/* 0.25us cycles/sec */
	if (band < pb->bandwidth_used) {
		printf("PCI bus %d: Warning: Total bandwidth exceeded!? (%d)\n",
		    pb->busno, pb->bandwidth_used);
		def_ltim = -1;
	} else {
		def_ltim = (band - pb->bandwidth_used) / pb->ndevs;
		if (def_ltim > pb->min_maxlat)
			def_ltim = pb->min_maxlat;
		def_ltim = def_ltim * bus_mhz / 4;
	}
	def_ltim = (def_ltim + 7) & ~7;
	max_ltim = (max_ltim + 7) & ~7;

	pb->def_ltim = MIN(def_ltim, 255);
	pb->max_ltim = MIN(MAX(max_ltim, def_ltim), 255);

	/*
	 * Now we have what we need to initialize the devices.
	 * It would probably be better if we could allocate all of these
	 * for all busses at once, but "not right now".  First, get a list
	 * of free memory ranges from the m.d. system.
	 */
	if (setup_iowins(pb) || setup_memwins(pb)) {
		printf("PCI bus configuration failed: "
		"unable to assign all I/O and memory ranges.\n");
		return -1;
	}

	/*
	 * Configure the latency for the devices, and enable them.
	 */
	for (pd = pb->device; pd < &pb->device[pb->ndevs]; pd++) {
		pcireg_t cmd, classreg, misc;
		int	ltim;

		if (pci_conf_debug) {
			print_tag(pd->pc, pd->tag);
			printf("Configuring device.\n");
		}
		classreg = pci_conf_read(pd->pc, pd->tag, PCI_CLASS_REG);
		misc = pci_conf_read(pd->pc, pd->tag, PCI_BHLC_REG);
		cmd = pci_conf_read(pd->pc, pd->tag, PCI_COMMAND_STATUS_REG);
		if (pd->enable & PCI_CONF_ENABLE_PARITY)
			cmd |= PCI_COMMAND_PARITY_ENABLE;
		if (pd->enable & PCI_CONF_ENABLE_SERR)
			cmd |= PCI_COMMAND_SERR_ENABLE;
		if (pb->fast_b2b)
			cmd |= PCI_COMMAND_BACKTOBACK_ENABLE;
		if (PCI_CLASS(classreg) != PCI_CLASS_BRIDGE ||
		    PCI_SUBCLASS(classreg) != PCI_SUBCLASS_BRIDGE_PCI) {
			if (pd->enable & PCI_CONF_ENABLE_IO)
				cmd |= PCI_COMMAND_IO_ENABLE;
			if (pd->enable & PCI_CONF_ENABLE_MEM)
				cmd |= PCI_COMMAND_MEM_ENABLE;
			if (pd->enable & PCI_CONF_ENABLE_BM)
				cmd |= PCI_COMMAND_MASTER_ENABLE;
			ltim = pd->min_gnt * bus_mhz / 4;
			ltim = MIN (MAX (pb->def_ltim, ltim), pb->max_ltim);
		} else {
			cmd |= PCI_COMMAND_MASTER_ENABLE;
			ltim = MIN (pb->def_ltim, pb->max_ltim);
		}
		if ((pd->enable &
		    (PCI_CONF_ENABLE_MEM | PCI_CONF_ENABLE_IO)) == 0) {
			print_tag(pd->pc, pd->tag);
			printf("Disabled due to lack of resources.\n");
			cmd &= ~(PCI_COMMAND_MASTER_ENABLE |
			    PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE);
		}
		pci_conf_write(pd->pc, pd->tag, PCI_COMMAND_STATUS_REG, cmd);

		misc &= ~((PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT) |
		    (PCI_CACHELINE_MASK << PCI_CACHELINE_SHIFT));
		misc |= (ltim & PCI_LATTIMER_MASK) << PCI_LATTIMER_SHIFT;
		misc |= ((pb->cacheline_size >> 2) & PCI_CACHELINE_MASK) <<
		    PCI_CACHELINE_SHIFT;
		pci_conf_write(pd->pc, pd->tag, PCI_BHLC_REG, misc);

		if (pd->ppb) {
			if (configure_bridge(pd) < 0)
				return -1;
			continue;
		}
	}

	if (pci_conf_debug)
		printf("PCI bus %d configured\n", pb->busno);

	return 0;
}

static bool
mem_region_ok64(struct pciconf_resource * const r __used_only_lp64)
{
	bool rv = false;

#ifdef _LP64
	/*
	 * XXX We need to guard this with _LP64 because vmem uses
	 * uintptr_t internally.
	 */
	vmem_size_t result;
	if (vmem_xalloc(r->arena, 1/*size*/, 1/*align*/, 0/*phase*/,
			0/*nocross*/, (1UL << 32), VMEM_ADDR_MAX,
			VM_INSTANTFIT | VM_NOSLEEP, &result) == 0) {
		vmem_free(r->arena, result, 1);
		rv = true;
	}
#endif /* _LP64 */

	return rv;
}

/*
 * pciconf_resource_init:
 *
 *	Allocate and initilize a pci configuration resources container.
 */
struct pciconf_resources *
pciconf_resource_init(void)
{
	struct pciconf_resources *rs;

	rs = kmem_zalloc(sizeof(*rs), KM_SLEEP);

	return (rs);
}

/*
 * pciconf_resource_fini:
 *
 *	Dispose of a pci configuration resources container.
 */
void
pciconf_resource_fini(struct pciconf_resources *rs)
{
	int i;

	for (i = 0; i < PCICONF_RESOURCE_NTYPES; i++) {
		fini_range_resource(&rs->resources[i]);
	}

	kmem_free(rs, sizeof(*rs));
}

/*
 * pciconf_resource_add:
 *
 *	Add a pci configuration resource to a container.
 */
int
pciconf_resource_add(struct pciconf_resources *rs, int type,
    bus_addr_t start, bus_size_t size)
{
	bus_addr_t end = start + (size - 1);
	struct pciconf_resource *r;
	struct pciconf_resource_rsvd *rsvd;
	int error, rsvd_type, align;
	vmem_addr_t result;
	bool first;

	if (size == 0 || end <= start)
		return EINVAL;

	if (type < 0 || type >= PCICONF_RESOURCE_NTYPES)
		return EINVAL;

	r = &rs->resources[type];

	first = r->arena == NULL;
	if (first) {
		r->arena = create_vmem_arena(pciconf_resource_names[type],
		    0, 0, VM_SLEEP);
		r->min_addr = VMEM_ADDR_MAX;
		r->max_addr = VMEM_ADDR_MIN;
	}

	error = vmem_add(r->arena, start, size, VM_SLEEP);
	if (error == 0) {
		if (start < r->min_addr)
			r->min_addr = start;
		if (end > r->max_addr)
			r->max_addr = end;
	}

	r->total_size += size;

	switch (type) {
	case PCICONF_RESOURCE_IO:
		rsvd_type = PCI_CONF_MAP_IO;
		align = 0x1000;
		break;
	case PCICONF_RESOURCE_MEM:
	case PCICONF_RESOURCE_PREFETCHABLE_MEM:
		rsvd_type = PCI_CONF_MAP_MEM;
		align = 0x100000;
		break;
	default:
		rsvd_type = 0;
		align = 0;
		break;
	}

	/*
	 * Exclude reserved ranges from available resources
	 */
	LIST_FOREACH(rsvd, &pciconf_resource_reservations, next) {
		if (rsvd->type != rsvd_type)
			continue;
		/*
		 * The reserved range may not be within our resource window.
		 * That's fine, so ignore the error.
		 */
		(void)vmem_xalloc(r->arena, rsvd->size, align, 0, 0,
				  rsvd->start, rsvd->start + rsvd->size,
				  VM_BESTFIT | VM_NOSLEEP,
				  &result);
	}

	return 0;
}

/*
 * pciconf_resource_reserve:
 *
 *	Mark a pci configuration resource as in-use. Devices
 *	already configured to use these resources are notified
 *	during resource assignment if their resources are changed.
 */
void
pciconf_resource_reserve(int type, bus_addr_t start, bus_size_t size,
    void (*callback)(void *, uint64_t), void *callback_arg)
{
	struct pciconf_resource_rsvd *rsvd;

	rsvd = kmem_zalloc(sizeof(*rsvd), KM_SLEEP);
	rsvd->type = type;
	rsvd->start = start;
	rsvd->size = size;
	rsvd->callback = callback;
	rsvd->callback_arg = callback_arg;
	LIST_INSERT_HEAD(&pciconf_resource_reservations, rsvd, next);
}

/*
 * Let's configure the PCI bus.
 * This consists of basically scanning for all existing devices,
 * identifying their needs, and then making another pass over them
 * to set:
 *	1. I/O addresses
 *	2. Memory addresses (Prefetchable and not)
 *	3. PCI command register
 *	4. The latency part of the PCI BHLC (BIST (Built-In Self Test),
 *	    Header type, Latency timer, Cache line size) register
 *
 * The command register is set to enable fast back-to-back transactions
 * if the host bridge says it can handle it.  We also configure
 * Master Enable, SERR enable, parity enable, and (if this is not a
 * PCI-PCI bridge) the I/O and Memory spaces.  Apparently some devices
 * will not report some I/O space.
 *
 * The latency is computed to be a "fair share" of the bus bandwidth.
 * The bus bandwidth variable is initialized to the number of PCI cycles
 * in one second.  The number of cycles taken for one transaction by each
 * device (MAX_LAT + MIN_GNT) is then subtracted from the bandwidth.
 * Care is taken to ensure that the latency timer won't be set such that
 * it would exceed the critical time for any device.
 *
 * This is complicated somewhat due to the presence of bridges.  PCI-PCI
 * bridges are probed and configured recursively.
 */
int
pci_configure_bus(pci_chipset_tag_t pc, struct pciconf_resources *rs,
    int firstbus, int cacheline_size)
{
	pciconf_bus_t	*pb;
	int		rv;

	pb = kmem_zalloc(sizeof (pciconf_bus_t), KM_SLEEP);
	pb->busno = firstbus;
	pb->next_busno = pb->busno + 1;
	pb->last_busno = 255;
	pb->cacheline_size = cacheline_size;
	pb->parent_bus = NULL;
	pb->swiz = 0;
	pb->io_32bit = 1;
	pb->io_res = rs->resources[PCICONF_RESOURCE_IO];

	pb->mem_res = rs->resources[PCICONF_RESOURCE_MEM];
	if (pb->mem_res.arena == NULL)
		pb->mem_res = rs->resources[PCICONF_RESOURCE_PREFETCHABLE_MEM];

	pb->pmem_res = rs->resources[PCICONF_RESOURCE_PREFETCHABLE_MEM];
	if (pb->pmem_res.arena == NULL)
		pb->pmem_res = rs->resources[PCICONF_RESOURCE_MEM];

	/*
	 * Probe the memory region arenas to see if allocation of
	 * 64-bit addresses is possible.
	 */
	pb->mem_64bit = mem_region_ok64(&pb->mem_res);
	pb->pmem_64bit = mem_region_ok64(&pb->pmem_res);

	pb->pc = pc;
	pb->io_total = pb->mem_total = pb->pmem_total = 0;

	rv = probe_bus(pb);
	pb->last_busno = pb->next_busno - 1;
	if (rv == 0)
		rv = configure_bus(pb);

	/*
	 * All done!
	 */
	kmem_free(pb, sizeof(*pb));
	return rv;
}
