/*	$NetBSD: pci_ranges.c,v 1.4.14.1 2015/09/22 12:05:54 skrll Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Young <dyoung@NetBSD.org>.
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
__KERNEL_RCSID(0, "$NetBSD: pci_ranges.c,v 1.4.14.1 2015/09/22 12:05:54 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <prop/proplib.h>
#include <ppath/ppath.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pccbbreg.h>

#include <machine/autoconf.h>

typedef enum pci_alloc_regtype {
	  PCI_ALLOC_REGTYPE_NONE = 0
	, PCI_ALLOC_REGTYPE_BAR = 1
	, PCI_ALLOC_REGTYPE_WIN = 2
	, PCI_ALLOC_REGTYPE_CBWIN = 3
	, PCI_ALLOC_REGTYPE_VGA_EN = 4
} pci_alloc_regtype_t;

typedef enum pci_alloc_space {
	  PCI_ALLOC_SPACE_IO = 0
	, PCI_ALLOC_SPACE_MEM = 1
} pci_alloc_space_t;

typedef enum pci_alloc_flags {
	  PCI_ALLOC_F_PREFETCHABLE = 0x1
} pci_alloc_flags_t;

typedef struct pci_alloc {
	TAILQ_ENTRY(pci_alloc)		pal_link;
	pcitag_t			pal_tag;
	uint64_t			pal_addr;
	uint64_t			pal_size;
	pci_alloc_regtype_t		pal_type;
	struct pci_alloc_reg {
		int			r_ofs;
		pcireg_t		r_val;
		pcireg_t		r_mask;
	}				pal_reg[3];
	pci_alloc_space_t		pal_space;
	pci_alloc_flags_t		pal_flags;
} pci_alloc_t;

typedef struct pci_alloc_reg pci_alloc_reg_t;

TAILQ_HEAD(pci_alloc_list, pci_alloc);

typedef struct pci_alloc_list pci_alloc_list_t;

static pci_alloc_t *
pci_alloc_dup(const pci_alloc_t *pal)
{
	pci_alloc_t *npal;

	if ((npal = kmem_alloc(sizeof(*npal), KM_SLEEP)) == NULL)
		return NULL;

	*npal = *pal;

	return npal;
}

static bool
pci_alloc_linkdup(pci_alloc_list_t *pals, const pci_alloc_t *pal)
{
	pci_alloc_t *npal;

	if ((npal = pci_alloc_dup(pal)) == NULL)
		return false;
	
	TAILQ_INSERT_TAIL(pals, npal, pal_link);

	return true;
}
 
struct range_infer_ctx {
	pci_chipset_tag_t	ric_pc;
	pci_alloc_list_t	ric_pals;
	bus_addr_t		ric_mmio_bottom;
	bus_addr_t		ric_mmio_top;
	bus_addr_t		ric_io_bottom;
	bus_addr_t		ric_io_top;
};

static bool
io_range_extend(struct range_infer_ctx *ric, const pci_alloc_t *pal)
{
	if (ric->ric_io_bottom > pal->pal_addr)
		ric->ric_io_bottom = pal->pal_addr;
	if (ric->ric_io_top < pal->pal_addr + pal->pal_size)
		ric->ric_io_top = pal->pal_addr + pal->pal_size;

	return pci_alloc_linkdup(&ric->ric_pals, pal);
}

static bool
io_range_extend_by_bar(struct range_infer_ctx *ric, int bus, int dev, int fun,
    int ofs, pcireg_t curbar, pcireg_t sizebar)
{
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_BAR
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r->r_ofs = ofs;
	r->r_val = curbar;

	pal.pal_addr = PCI_MAPREG_IO_ADDR(curbar);
	pal.pal_size = PCI_MAPREG_IO_SIZE(sizebar);

	aprint_debug("%s: %d.%d.%d base at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return (pal.pal_size == 0) || io_range_extend(ric, &pal);
}

static bool
io_range_extend_by_vga_enable(struct range_infer_ctx *ric,
    int bus, int dev, int fun, pcireg_t csr, pcireg_t bcr)
{
	pci_alloc_reg_t *r;
	pci_alloc_t tpal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_VGA_EN
		, .pal_reg = {{
			  .r_ofs = PCI_COMMAND_STATUS_REG
			, .r_mask = PCI_COMMAND_IO_ENABLE
		  }, {
			  .r_ofs = PCI_BRIDGE_CONTROL_REG
			, .r_mask =
			    PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT
		  }}
	}, pal[2];

	aprint_debug("%s: %d.%d.%d enter\n", __func__, bus, dev, fun);

	if ((csr & PCI_COMMAND_IO_ENABLE) == 0 ||
	    (bcr & (PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT)) == 0) {
		aprint_debug("%s: %d.%d.%d I/O or VGA disabled\n",
		    __func__, bus, dev, fun);
		return true;
	}

	r = &tpal.pal_reg[0];
	tpal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_val = csr;
	r[1].r_val = bcr;

	pal[0] = pal[1] = tpal;

	pal[0].pal_addr = 0x3b0;
	pal[0].pal_size = 0x3bb - 0x3b0 + 1;

	pal[1].pal_addr = 0x3c0;
	pal[1].pal_size = 0x3df - 0x3c0 + 1;

	/* XXX add aliases for pal[0..1] */

	return io_range_extend(ric, &pal[0]) && io_range_extend(ric, &pal[1]);
}

static bool
io_range_extend_by_win(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, int ofshigh,
    pcireg_t io, pcireg_t iohigh)
{
	const int fourkb = 4 * 1024;
	pcireg_t baser, limitr;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_WIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = io;

	baser = ((io >> PCI_BRIDGE_STATIO_IOBASE_SHIFT) &
	    PCI_BRIDGE_STATIO_IOBASE_MASK) >> 4;
	limitr = ((io >> PCI_BRIDGE_STATIO_IOLIMIT_SHIFT) &
	    PCI_BRIDGE_STATIO_IOLIMIT_MASK) >> 4;

	if (PCI_BRIDGE_IO_32BITS(io)) {
		pcireg_t baseh, limith;

		r[1].r_mask = ~(pcireg_t)0;
		r[1].r_ofs = ofshigh;
		r[1].r_val = iohigh;

		baseh = (iohigh >> PCI_BRIDGE_IOHIGH_BASE_SHIFT)
		    & PCI_BRIDGE_IOHIGH_BASE_MASK;
		limith = (iohigh >> PCI_BRIDGE_IOHIGH_LIMIT_SHIFT)
		    & PCI_BRIDGE_IOHIGH_LIMIT_MASK;

		baser |= baseh << 4;
		limitr |= limith << 4;
	}

	/* XXX check with the PCI standard */
	if (baser > limitr)
		return true;

	pal.pal_addr = baser * fourkb;
	pal.pal_size = (limitr - baser + 1) * fourkb;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return io_range_extend(ric, &pal);
}

static bool
io_range_extend_by_cbwin(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t base0, pcireg_t limit0)
{
	pcireg_t base, limit;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_CBWIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }, {
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = base0;
	r[1].r_ofs = ofs + 4;
	r[1].r_val = limit0;

	base = base0 & __BITS(31, 2);
	limit = limit0 & __BITS(31, 2);

	if (base > limit)
		return true;

	pal.pal_addr = base;
	pal.pal_size = limit - base + 4;	/* XXX */

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return io_range_extend(ric, &pal);
}

static void
io_range_infer(pci_chipset_tag_t pc, pcitag_t tag, void *ctx)
{
	struct range_infer_ctx *ric = ctx;
	pcireg_t bhlcr, limit, io;
	int bar, bus, dev, fun, hdrtype, nbar;
	bool ok = true;

	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);

	hdrtype = PCI_HDRTYPE_TYPE(bhlcr);

	pci_decompose_tag(pc, tag, &bus, &dev, &fun);

	switch (hdrtype) {
	case PCI_HDRTYPE_PPB:
		nbar = 2;
		/* Extract I/O windows */
		ok = ok && io_range_extend_by_win(ric, bus, dev, fun,
		    PCI_BRIDGE_STATIO_REG,
		    PCI_BRIDGE_IOHIGH_REG,
		    pci_conf_read(pc, tag, PCI_BRIDGE_STATIO_REG),
		    pci_conf_read(pc, tag, PCI_BRIDGE_IOHIGH_REG));
		ok = ok && io_range_extend_by_vga_enable(ric, bus, dev, fun,
		    pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG),
		    pci_conf_read(pc, tag, PCI_BRIDGE_CONTROL_REG));
		break;
	case PCI_HDRTYPE_PCB:
		/* Extract I/O windows */
		io = pci_conf_read(pc, tag, PCI_CB_IOBASE0);
		limit = pci_conf_read(pc, tag, PCI_CB_IOLIMIT0);
		ok = ok && io_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_IOBASE0, io, limit);
		io = pci_conf_read(pc, tag, PCI_CB_IOBASE1);
		limit = pci_conf_read(pc, tag, PCI_CB_IOLIMIT1);
		ok = ok && io_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_IOBASE1, io, limit);
		nbar = 1;
		break;
	case PCI_HDRTYPE_DEVICE:
		nbar = 6;
		break;
	default:
		aprint_debug("%s: unknown header type %d at %d.%d.%d\n",
		    __func__, hdrtype, bus, dev, fun);
		return;
	}

	for (bar = 0; bar < nbar; bar++) {
		pcireg_t basebar, sizebar;

		basebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), 0xffffffff);
		sizebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), basebar);

		if (sizebar == 0)
			continue;
		if (PCI_MAPREG_TYPE(sizebar) != PCI_MAPREG_TYPE_IO)
			continue;

		ok = ok && io_range_extend_by_bar(ric, bus, dev, fun,
		    PCI_BAR(bar), basebar, sizebar);
	}
	if (!ok) {
		aprint_verbose("I/O range inference failed at PCI %d.%d.%d\n",
		    bus, dev, fun);
	}
}

static bool
mmio_range_extend(struct range_infer_ctx *ric, const pci_alloc_t *pal)
{
	if (ric->ric_mmio_bottom > pal->pal_addr)
		ric->ric_mmio_bottom = pal->pal_addr;
	if (ric->ric_mmio_top < pal->pal_addr + pal->pal_size)
		ric->ric_mmio_top = pal->pal_addr + pal->pal_size;

	return pci_alloc_linkdup(&ric->ric_pals, pal);
}

static bool
mmio_range_extend_by_bar(struct range_infer_ctx *ric, int bus, int dev,
    int fun, int ofs, pcireg_t curbar, pcireg_t sizebar)
{
	int type;
	bool prefetchable;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_BAR
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r->r_ofs = ofs;
	r->r_val = curbar;

	pal.pal_addr = PCI_MAPREG_MEM_ADDR(curbar);

	type = PCI_MAPREG_MEM_TYPE(curbar);
	prefetchable = PCI_MAPREG_MEM_PREFETCHABLE(curbar);

	if (prefetchable)
		pal.pal_flags |= PCI_ALLOC_F_PREFETCHABLE;

	switch (type) {
	case PCI_MAPREG_MEM_TYPE_32BIT:
		pal.pal_size = PCI_MAPREG_MEM_SIZE(sizebar);
		break;
	case PCI_MAPREG_MEM_TYPE_64BIT:
		pal.pal_size = PCI_MAPREG_MEM64_SIZE(sizebar);
		break;
	case PCI_MAPREG_MEM_TYPE_32BIT_1M:
	default:
		aprint_debug("%s: ignored memory type %d at %d.%d.%d\n",
		    __func__, type, bus, dev, fun);
		return false;
	}

	aprint_debug("%s: %d.%d.%d base at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return (pal.pal_size == 0) || mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_vga_enable(struct range_infer_ctx *ric,
    int bus, int dev, int fun, pcireg_t csr, pcireg_t bcr)
{
	pci_alloc_reg_t *r;
	pci_alloc_t tpal = {
		  .pal_flags = PCI_ALLOC_F_PREFETCHABLE	/* XXX a guess */
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_VGA_EN
		, .pal_reg = {{
			  .r_ofs = PCI_COMMAND_STATUS_REG
			, .r_mask = PCI_COMMAND_MEM_ENABLE
		  }, {
			  .r_ofs = PCI_BRIDGE_CONTROL_REG
			, .r_mask =
			    PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT
		  }}
	}, pal;

	aprint_debug("%s: %d.%d.%d enter\n", __func__, bus, dev, fun);

	if ((csr & PCI_COMMAND_MEM_ENABLE) == 0 ||
	    (bcr & (PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT)) == 0) {
		aprint_debug("%s: %d.%d.%d memory or VGA disabled\n",
		    __func__, bus, dev, fun);
		return true;
	}

	r = &tpal.pal_reg[0];
	tpal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_val = csr;
	r[1].r_val = bcr;

	pal = tpal;

	pal.pal_addr = 0xa0000;
	pal.pal_size = 0xbffff - 0xa0000 + 1;

	return mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_win(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t mem)
{
	const int onemeg = 1024 * 1024;
	pcireg_t baser, limitr;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_WIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r->r_ofs = ofs;
	r->r_val = mem;

	baser = (mem >> PCI_BRIDGE_MEMORY_BASE_SHIFT) &
	    PCI_BRIDGE_MEMORY_BASE_MASK;
	limitr = (mem >> PCI_BRIDGE_MEMORY_LIMIT_SHIFT) &
	    PCI_BRIDGE_MEMORY_LIMIT_MASK;

	/* XXX check with the PCI standard */
	if (baser > limitr || limitr == 0)
		return true;

	pal.pal_addr = baser * onemeg;
	pal.pal_size = (limitr - baser + 1) * onemeg;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_prememwin(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t mem,
    int hibaseofs, pcireg_t hibase,
    int hilimitofs, pcireg_t hilimit)
{
	const int onemeg = 1024 * 1024;
	uint64_t baser, limitr;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = PCI_ALLOC_F_PREFETCHABLE
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_WIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = mem;

	baser = (mem >> PCI_BRIDGE_PREFETCHMEM_BASE_SHIFT) &
	    PCI_BRIDGE_PREFETCHMEM_BASE_MASK;
	limitr = (mem >> PCI_BRIDGE_PREFETCHMEM_LIMIT_SHIFT) &
	    PCI_BRIDGE_PREFETCHMEM_LIMIT_MASK;

	if (PCI_BRIDGE_PREFETCHMEM_64BITS(mem)) {
		r[1].r_mask = r[2].r_mask = ~(pcireg_t)0;
		r[1].r_ofs = hibaseofs;
		r[1].r_val = hibase;
		r[2].r_ofs = hilimitofs;
		r[2].r_val = hilimit;

		baser |= hibase << 12;
		limitr |= hibase << 12;
	}

	/* XXX check with the PCI standard */
	if (baser > limitr || limitr == 0)
		return true;

	pal.pal_addr = baser * onemeg;
	pal.pal_size = (limitr - baser + 1) * onemeg;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_cbwin(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t base, pcireg_t limit,
    bool prefetchable)
{
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_CBWIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }, {
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	if (prefetchable)
		pal.pal_flags |= PCI_ALLOC_F_PREFETCHABLE;

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = base;
	r[1].r_ofs = ofs + 4;
	r[1].r_val = limit;

	if (base > limit)
		return true;

	if (limit == 0)
		return true;

	pal.pal_addr = base;
	pal.pal_size = limit - base + 4096;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return mmio_range_extend(ric, &pal);
}

static void
mmio_range_infer(pci_chipset_tag_t pc, pcitag_t tag, void *ctx)
{
	struct range_infer_ctx *ric = ctx;
	pcireg_t bcr, bhlcr, limit, mem, premem, hiprebase, hiprelimit;
	int bar, bus, dev, fun, hdrtype, nbar;
	bool ok = true;

	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);

	hdrtype = PCI_HDRTYPE_TYPE(bhlcr);

	pci_decompose_tag(pc, tag, &bus, &dev, &fun);

	switch (hdrtype) {
	case PCI_HDRTYPE_PPB:
		nbar = 2;
		/* Extract memory windows */
		ok = ok && mmio_range_extend_by_win(ric, bus, dev, fun,
		    PCI_BRIDGE_MEMORY_REG,
		    pci_conf_read(pc, tag, PCI_BRIDGE_MEMORY_REG));
		premem = pci_conf_read(pc, tag, PCI_BRIDGE_PREFETCHMEM_REG);
		if (PCI_BRIDGE_PREFETCHMEM_64BITS(premem)) {
			aprint_debug("%s: 64-bit prefetchable memory window "
			    "at %d.%d.%d\n", __func__, bus, dev, fun);
			hiprebase = pci_conf_read(pc, tag,
			    PCI_BRIDGE_PREFETCHBASE32_REG);
			hiprelimit = pci_conf_read(pc, tag,
			    PCI_BRIDGE_PREFETCHLIMIT32_REG);
		} else
			hiprebase = hiprelimit = 0;
		ok = ok &&
		    mmio_range_extend_by_prememwin(ric, bus, dev, fun,
		        PCI_BRIDGE_PREFETCHMEM_REG, premem,
		        PCI_BRIDGE_PREFETCHBASE32_REG, hiprebase,
		        PCI_BRIDGE_PREFETCHLIMIT32_REG, hiprelimit) &&
		    mmio_range_extend_by_vga_enable(ric, bus, dev, fun,
		        pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG),
		        pci_conf_read(pc, tag, PCI_BRIDGE_CONTROL_REG));
		break;
	case PCI_HDRTYPE_PCB:
		/* Extract memory windows */
		bcr = pci_conf_read(pc, tag, PCI_BRIDGE_CONTROL_REG);
		mem = pci_conf_read(pc, tag, PCI_CB_MEMBASE0);
		limit = pci_conf_read(pc, tag, PCI_CB_MEMLIMIT0);
		ok = ok && mmio_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_MEMBASE0, mem, limit,
		    (bcr & CB_BCR_PREFETCH_MEMWIN0) != 0);
		mem = pci_conf_read(pc, tag, PCI_CB_MEMBASE1);
		limit = pci_conf_read(pc, tag, PCI_CB_MEMLIMIT1);
		ok = ok && mmio_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_MEMBASE1, mem, limit,
		    (bcr & CB_BCR_PREFETCH_MEMWIN1) != 0);
		nbar = 1;
		break;
	case PCI_HDRTYPE_DEVICE:
		nbar = 6;
		break;
	default:
		aprint_debug("%s: unknown header type %d at %d.%d.%d\n",
		    __func__, hdrtype, bus, dev, fun);
		return;
	}

	for (bar = 0; bar < nbar; bar++) {
		pcireg_t basebar, sizebar;

		basebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), 0xffffffff);
		sizebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), basebar);

		if (sizebar == 0)
			continue;
		if (PCI_MAPREG_TYPE(sizebar) != PCI_MAPREG_TYPE_MEM)
			continue;

		ok = ok && mmio_range_extend_by_bar(ric, bus, dev, fun,
		    PCI_BAR(bar), basebar, sizebar);
	}
	if (!ok) {
		aprint_verbose("MMIO range inference failed at PCI %d.%d.%d\n",
		    bus, dev, fun);
	}
}

static const char *
pci_alloc_regtype_string(const pci_alloc_regtype_t t)
{
	switch (t) {
	case PCI_ALLOC_REGTYPE_BAR:
		return "bar";
	case PCI_ALLOC_REGTYPE_WIN:
	case PCI_ALLOC_REGTYPE_CBWIN:
		return "window";
	case PCI_ALLOC_REGTYPE_VGA_EN:
		return "vga-enable";
	default:
		return "<unknown>";
	}
}

static void
pci_alloc_print(pci_chipset_tag_t pc, const pci_alloc_t *pal)
{
	int bus, dev, fun;
	const pci_alloc_reg_t *r;

	pci_decompose_tag(pc, pal->pal_tag, &bus, &dev, &fun);
	r = &pal->pal_reg[0];

	aprint_normal("%s range [0x%08" PRIx64 ", 0x%08" PRIx64 ")"
	    " at %d.%d.%d %s%s 0x%02x\n",
	    (pal->pal_space == PCI_ALLOC_SPACE_IO) ? "IO" : "MMIO",
	    pal->pal_addr, pal->pal_addr + pal->pal_size,
	    bus, dev, fun,
	    (pal->pal_flags & PCI_ALLOC_F_PREFETCHABLE) ? "prefetchable " : "",
	    pci_alloc_regtype_string(pal->pal_type),
	    r->r_ofs);
}

prop_dictionary_t pci_rsrc_dict = NULL;

static bool
pci_range_record(pci_chipset_tag_t pc, prop_array_t rsvns,
    pci_alloc_list_t *pals, pci_alloc_space_t space)
{
	int bus, dev, fun, i;
	prop_array_t regs;
	prop_dictionary_t reg;
	const pci_alloc_t *pal;
	const pci_alloc_reg_t *r;
	prop_dictionary_t rsvn;

	TAILQ_FOREACH(pal, pals, pal_link) {
		bool ok = true;

		r = &pal->pal_reg[0];

		if (pal->pal_space != space)
			continue;

		if ((rsvn = prop_dictionary_create()) == NULL)
			return false;

		if ((regs = prop_array_create()) == NULL) {
			prop_object_release(rsvn);
			return false;
		}

		if (!prop_dictionary_set(rsvn, "regs", regs)) {
			prop_object_release(rsvn);
			prop_object_release(regs);
			return false;
		}

		for (i = 0; i < __arraycount(pal->pal_reg); i++) {
			r = &pal->pal_reg[i];

			if (r->r_mask == 0)
				break;

			ok = (reg = prop_dictionary_create()) != NULL;
			if (!ok)
				break;

			ok = prop_dictionary_set_uint16(reg, "offset",
			        r->r_ofs) &&
			    prop_dictionary_set_uint32(reg, "val", r->r_val) &&
			    prop_dictionary_set_uint32(reg, "mask",
			        r->r_mask) && prop_array_add(regs, reg);
			if (!ok) {
				prop_object_release(reg);
				break;
			}
		}

		pci_decompose_tag(pc, pal->pal_tag, &bus, &dev, &fun);

		ok = ok &&
		    prop_dictionary_set_cstring_nocopy(rsvn, "type",
		        pci_alloc_regtype_string(pal->pal_type)) &&
		    prop_dictionary_set_uint64(rsvn, "address",
		        pal->pal_addr) &&
		    prop_dictionary_set_uint64(rsvn, "size", pal->pal_size) &&
		    prop_dictionary_set_uint8(rsvn, "bus", bus) &&
		    prop_dictionary_set_uint8(rsvn, "device", dev) &&
		    prop_dictionary_set_uint8(rsvn, "function", fun) &&
		    prop_array_add(rsvns, rsvn);
		prop_object_release(rsvn);
		if (!ok)
			return false;
	}
	return true;
}

prop_dictionary_t
pci_rsrc_filter(prop_dictionary_t rsrcs0,
    bool (*predicate)(void *, prop_dictionary_t), void *arg)
{
	int i, space;
	prop_dictionary_t rsrcs;
	prop_array_t rsvns;
	ppath_t *op, *p;

	if ((rsrcs = prop_dictionary_copy(rsrcs0)) == NULL)
		return NULL;

	for (space = 0; space < 2; space++) {
		op = p = ppath_create();
		p = ppath_push_key(p, (space == 0) ? "memory" : "io");
		p = ppath_push_key(p, "bios-reservations");
		if (p == NULL) {
			ppath_release(op);
			return NULL;
		}
		if ((rsvns = ppath_lookup(rsrcs0, p)) == NULL) {
			printf("%s: reservations not found\n", __func__);
			ppath_release(p);
			return NULL;
		}
		for (i = prop_array_count(rsvns); --i >= 0; ) {
			prop_dictionary_t rsvn;

			if ((p = ppath_push_idx(p, i)) == NULL) {
				printf("%s: ppath_push_idx\n", __func__);
				ppath_release(op);
				prop_object_release(rsrcs);
				return NULL;
			}

			rsvn = ppath_lookup(rsrcs0, p);

			KASSERT(rsvn != NULL);

			if (!(*predicate)(arg, rsvn)) {
				ppath_copydel_object((prop_object_t)rsrcs0,
				    (prop_object_t *)&rsrcs, p);
			}

			if ((p = ppath_pop(p, NULL)) == NULL) {
				printf("%s: ppath_pop\n", __func__);
				ppath_release(p);
				prop_object_release(rsrcs);
				return NULL;
			}
		}
		ppath_release(op);
	}
	return rsrcs;
}
 
void
pci_ranges_infer(pci_chipset_tag_t pc, int minbus, int maxbus,
    bus_addr_t *iobasep, bus_size_t *iosizep,
    bus_addr_t *membasep, bus_size_t *memsizep)
{
	prop_dictionary_t iodict = NULL, memdict = NULL;
	prop_array_t iorsvns, memrsvns;
	struct range_infer_ctx ric = {
		  .ric_io_bottom = ~((bus_addr_t)0)
		, .ric_io_top = 0
		, .ric_mmio_bottom = ~((bus_addr_t)0)
		, .ric_mmio_top = 0
		, .ric_pals = TAILQ_HEAD_INITIALIZER(ric.ric_pals)
	};
	const pci_alloc_t *pal;

	ric.ric_pc = pc;
	pci_device_foreach_min(pc, minbus, maxbus, mmio_range_infer, &ric);
	pci_device_foreach_min(pc, minbus, maxbus, io_range_infer, &ric);
	if (membasep != NULL)
		*membasep = ric.ric_mmio_bottom;
	if (memsizep != NULL)
		*memsizep = ric.ric_mmio_top - ric.ric_mmio_bottom;
	if (iobasep != NULL)
		*iobasep = ric.ric_io_bottom;
	if (iosizep != NULL)
		*iosizep = ric.ric_io_top - ric.ric_io_bottom;
	aprint_verbose("%s: inferred %" PRIuMAX
	    " bytes of memory-mapped PCI space at 0x%" PRIxMAX "\n", __func__,
	    (uintmax_t)(ric.ric_mmio_top - ric.ric_mmio_bottom),
	    (uintmax_t)ric.ric_mmio_bottom); 
	aprint_verbose("%s: inferred %" PRIuMAX
	    " bytes of PCI I/O space at 0x%" PRIxMAX "\n", __func__,
	    (uintmax_t)(ric.ric_io_top - ric.ric_io_bottom),
	    (uintmax_t)ric.ric_io_bottom); 
	TAILQ_FOREACH(pal, &ric.ric_pals, pal_link)
		pci_alloc_print(pc, pal);

	if ((memdict = prop_dictionary_create()) == NULL) {
		aprint_error("%s: could not create PCI MMIO "
		    "resources dictionary\n", __func__);
	} else if ((memrsvns = prop_array_create()) == NULL) {
		aprint_error("%s: could not create PCI BIOS memory "
		    "reservations array\n", __func__);
	} else if (!prop_dictionary_set(memdict, "bios-reservations",
	    memrsvns)) {
		aprint_error("%s: could not record PCI BIOS memory "
		    "reservations array\n", __func__);
	} else if (!pci_range_record(pc, memrsvns, &ric.ric_pals,
	    PCI_ALLOC_SPACE_MEM)) {
		aprint_error("%s: could not record PCI BIOS memory "
		    "reservations\n", __func__);
	} else if (!prop_dictionary_set_uint64(memdict,
	    "start", ric.ric_mmio_bottom) ||
	    !prop_dictionary_set_uint64(memdict, "size",
	     ric.ric_mmio_top - ric.ric_mmio_bottom)) {
		aprint_error("%s: could not record PCI memory min & max\n",
		    __func__);
	} else if ((iodict = prop_dictionary_create()) == NULL) {
		aprint_error("%s: could not create PCI I/O "
		    "resources dictionary\n", __func__);
	} else if ((iorsvns = prop_array_create()) == NULL) {
		aprint_error("%s: could not create PCI BIOS I/O "
		    "reservations array\n", __func__);
	} else if (!prop_dictionary_set(iodict, "bios-reservations",
	    iorsvns)) {
		aprint_error("%s: could not record PCI BIOS I/O "
		    "reservations array\n", __func__);
	} else if (!pci_range_record(pc, iorsvns, &ric.ric_pals,
	    PCI_ALLOC_SPACE_IO)) {
		aprint_error("%s: could not record PCI BIOS I/O "
		    "reservations\n", __func__);
	} else if (!prop_dictionary_set_uint64(iodict,
	    "start", ric.ric_io_bottom) ||
	    !prop_dictionary_set_uint64(iodict, "size",
	     ric.ric_io_top - ric.ric_io_bottom)) {
		aprint_error("%s: could not record PCI I/O min & max\n",
		    __func__);
	} else if ((pci_rsrc_dict = prop_dictionary_create()) == NULL) {
		aprint_error("%s: could not create PCI resources dictionary\n",
		    __func__);
	} else if (!prop_dictionary_set(pci_rsrc_dict, "memory", memdict) ||
	           !prop_dictionary_set(pci_rsrc_dict, "io", iodict)) {
		aprint_error("%s: could not record PCI memory- or I/O-"
		    "resources dictionary\n", __func__);
		prop_object_release(pci_rsrc_dict);
		pci_rsrc_dict = NULL;
	}

	if (iodict != NULL)
		prop_object_release(iodict);
	if (memdict != NULL)
		prop_object_release(memdict);
	/* XXX release iorsvns, memrsvns */
}

static bool
pcibus_rsvn_predicate(void *arg, prop_dictionary_t rsvn)
{
	struct pcibus_attach_args *pba = arg;
	uint8_t bus;

	if (!prop_dictionary_get_uint8(rsvn, "bus", &bus))
		return false;

	return pba->pba_bus <= bus && bus <= pba->pba_sub;
}

static bool
pci_rsvn_predicate(void *arg, prop_dictionary_t rsvn)
{
	struct pci_attach_args *pa = arg;
	uint8_t bus, device, function;
	bool rc;

	rc = prop_dictionary_get_uint8(rsvn, "bus", &bus) &&
	    prop_dictionary_get_uint8(rsvn, "device", &device) &&
	    prop_dictionary_get_uint8(rsvn, "function", &function);

	if (!rc)
		return false;

	return pa->pa_bus == bus && pa->pa_device == device &&
	    pa->pa_function == function;
}

void
device_pci_props_register(device_t dev, void *aux)
{
	cfdata_t cf;
	prop_dictionary_t dict;

	cf = (device_parent(dev) != NULL) ? device_cfdata(dev) : NULL;
#if 0
	aprint_normal_dev(dev, "is%s a pci, parent %p, cf %p, ifattr %s\n",
	    device_is_a(dev, "pci") ? "" : " not",
	    device_parent(dev),
	    cf,
	    cf != NULL ? cfdata_ifattr(cf) : "");
#endif
	if (pci_rsrc_dict == NULL)
		return;

	if (!device_is_a(dev, "pci") &&
	    (cf == NULL || strcmp(cfdata_ifattr(cf), "pci") != 0))
		return;

	dict = pci_rsrc_filter(pci_rsrc_dict,
	    device_is_a(dev, "pci") ? &pcibus_rsvn_predicate
				    : &pci_rsvn_predicate, aux);
	if (dict == NULL)
		return;
	(void)prop_dictionary_set(device_properties(dev),
	    "pci-resources", dict);
}
