/*	$OpenBSD: loongson2_machdep.c,v 1.11 2011/03/31 20:37:44 miod Exp $	*/

/*
 * Copyright (c) 2009, 2010 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/kcore.h>

#include <uvm/uvm_extern.h>

#include <evbmips/loongson/autoconf.h>
#include <evbmips/loongson/loongson_intr.h>
#include <machine/kcore.h>
#include <machine/cpu.h>
#include <mips/pmon/pmon.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

boolean_t is_memory_range(paddr_t, psize_t, psize_t);
static void loongson2f_setup_window(uint, uint, uint64_t, uint64_t, uint64_t, uint);

/*
 * Canonical crossbow assignments on Loongson 2F based designs.
 * Might need to move to a per-design header file in the future.
 */

#define	MASTER_CPU		0
#define	MASTER_PCI		1

#define	WINDOW_CPU_LOW		0
#define	WINDOW_CPU_PCILO	1
#define	WINDOW_CPU_PCIHI	2
#define	WINDOW_CPU_DDR		3

#define	WINDOW_PCI_DDR		0

#define	DDR_PHYSICAL_BASE	0x0000000000000000UL	/* memory starts at 0 */
#define	DDR_PHYSICAL_SIZE	0x0000000080000000UL	/* up to 2GB */
#define	DDR_WINDOW_BASE		0x0000000080000000UL	/* mapped at 2GB */

#define	PCI_RESOURCE_BASE	0x0000000000000000UL
#define	PCI_RESOURCE_SIZE	0x0000000080000000UL

#define	PCI_DDR_BASE		0x0000000080000000UL	/* PCI->DDR at 2GB */

/* bonito interrupt mappings */
const struct bonito_irqmap loongson2e_irqmap[BONITO_NDIRECT] = {
	{ "mbox0",	BONITO_INTR_MBOX + 0,	IRQ_F_INT0 },
	{ "mbox1",	BONITO_INTR_MBOX + 1,	IRQ_F_INT0 },
	{ "mbox2",	BONITO_INTR_MBOX + 2,	IRQ_F_INT0 },
	{ "mbox3",	BONITO_INTR_MBOX + 3,	IRQ_F_INT0 },
	{ "dmardy",	BONITO_INTR_MBOX + 4,	IRQ_F_INT0 },
	{ "dmaempty",	BONITO_INTR_MBOX + 5,	IRQ_F_INT0 },
	{ "copyrdy",	BONITO_INTR_MBOX + 6,	IRQ_F_INT0 },
	{ "copyempty",	BONITO_INTR_MBOX + 7,	IRQ_F_INT0 },
	{ "coperr",	BONITO_INTR_MBOX + 8,	IRQ_F_INT1 },
	{ "pciirq",	BONITO_INTR_MBOX + 9,	IRQ_F_INT0 },
	{ "mastererr",	BONITO_INTR_MBOX + 10,	IRQ_F_INT1 },
	{ "systemerr",	BONITO_INTR_MBOX + 11,	IRQ_F_INT1 },
	{ "dramerr",	BONITO_INTR_MBOX + 12,	IRQ_F_INT1 },
	{ "retryerr",	BONITO_INTR_MBOX + 13,	IRQ_F_INT1 },
	{ NULL,		BONITO_INTR_MBOX + 14,	0 },
	{ NULL,		BONITO_INTR_MBOX + 15,	0 },
	{ "gpio0",	BONITO_INTR_GPIO + 0,	IRQ_F_INT0 },
	{ "gpio1",	BONITO_INTR_GPIO + 1,	IRQ_F_INT0 },
	{ "gpio2",	BONITO_INTR_GPIO + 2,	IRQ_F_INT0 },
	{ "gpio3",	BONITO_INTR_GPIO + 3,	IRQ_F_INT0 },
	{ "gpio4",	BONITO_INTR_GPIO + 4,	IRQ_F_INT0 },
	{ "gpio5",	BONITO_INTR_GPIO + 5,	IRQ_F_INT0 },
	{ "gpio6",	BONITO_INTR_GPIO + 6,	IRQ_F_INT0 },
	{ "gpio7",	BONITO_INTR_GPIO + 7,	IRQ_F_INT0 },
	{ "gpio8",	BONITO_INTR_GPIO + 8,	IRQ_F_INT0 },
	{ "gpin0",	BONITO_INTR_GPIN + 0,	IRQ_F_INT0 },
	{ "gpin1",	BONITO_INTR_GPIN + 1,	IRQ_F_INT0 },
	{ "gpin2",	BONITO_INTR_GPIN + 2,	IRQ_F_INT0 },
	{ "gpin3",	BONITO_INTR_GPIN + 3,	IRQ_F_INT0 },
	{ "gpin4",	BONITO_INTR_GPIN + 4,	IRQ_F_INT0 },
	{ "gpin5",	BONITO_INTR_GPIN + 5,	IRQ_F_INT0 },
	{ NULL,		BONITO_INTR_GPIN + 6,	0 },
};

const struct bonito_irqmap loongson2f_irqmap[BONITO_NDIRECT] = {
	{ "gpio0",	LOONGSON_INTR_GPIO0,	IRQ_F_INT0 },
	{ "gpio1",	LOONGSON_INTR_GPIO1,	IRQ_F_INT0 },
	{ "gpio2",	LOONGSON_INTR_GPIO2,	IRQ_F_INT0 },
	{ "gpio3",	LOONGSON_INTR_GPIO3,	IRQ_F_INT0 },

	{ "pci inta",	LOONGSON_INTR_PCIA,	IRQ_F_INT0 },
	{ "pci intb",	LOONGSON_INTR_PCIB,	IRQ_F_INT0 },
	{ "pci intc",	LOONGSON_INTR_PCIC,	IRQ_F_INT0 },
	{ "pci intd",	LOONGSON_INTR_PCID,	IRQ_F_INT0 },

	{ "pci perr",	LOONGSON_INTR_PCI_PARERR, IRQ_F_EDGE|IRQ_F_INT1 },
	{ "pci serr",	LOONGSON_INTR_PCI_SYSERR, IRQ_F_EDGE|IRQ_F_INT1 },

	{ "denali",	LOONGSON_INTR_DRAM_PARERR, IRQ_F_INT1 },

	{ "mips int0",	LOONGSON_INTR_INT0,	IRQ_F_INT0 },
	{ "mips int1",	LOONGSON_INTR_INT1,	IRQ_F_INT1 },
	{ "mips int2",	LOONGSON_INTR_INT2,	IRQ_F_INT2 },
	{ "mips int3",	LOONGSON_INTR_INT3,	IRQ_F_INT3 },
};

/*
 * Setup memory mappings for Loongson 2E processors.
 */

void
loongson2e_setup(paddr_t memlo, paddr_t memhi,
    vaddr_t vkernstart, vaddr_t vkernend, bus_dma_tag_t t)
{
	if (memhi > ((DDR_PHYSICAL_SIZE - BONITO_PCIHI_BASE) >> 20)) {
		pmon_printf("WARNING! %d MB of memory will not be used",
		    memhi - ((DDR_PHYSICAL_SIZE - BONITO_PCIHI_BASE) >> 20));
		memhi = (DDR_PHYSICAL_SIZE - BONITO_PCIHI_BASE) >> 20;
	}

	physmem = btoc(memlo + memhi);

	/* do NOT stomp on exception area */
	/* mips_page_physload() will skip the kernel */
	mem_clusters[0].start = DDR_PHYSICAL_BASE;
	mem_clusters[0].size = memlo;
	mem_cluster_cnt = 1;

	if (memhi != 0) {
		mem_clusters[1].start = BONITO_PCIHI_BASE;
		mem_clusters[1].size = memhi;
		mem_cluster_cnt = 2;
	}

	t->_wbase = PCI_DDR_BASE;
	t->_bounce_alloc_lo = DDR_PHYSICAL_BASE;
	t->_bounce_alloc_hi = DDR_PHYSICAL_BASE + DDR_PHYSICAL_SIZE;
}

/*
 * Setup memory mappings for Loongson 2F processors.
 */

void
loongson2f_setup(paddr_t memlo, paddr_t memhi,
    vaddr_t vkernstart, vaddr_t vkernend, bus_dma_tag_t t)
{
	/*
	 * Because we'll only set up a 2GB window for the PCI bus to
	 * access local memory, we'll limit ourselves to 2GB of usable
	 * memory as well.
	 *
	 * Note that this is a bad justification for this; it should be
	 * possible to setup a 1GB PCI space / 3GB memory access window,
	 * and use bounce buffers if physmem > 3GB; but at the moment
	 * there is no need to solve this problem until Loongson 2F-based
	 * hardware with more than 2GB of memory is commonly encountered.
	 *
	 * Also note that, despite the crossbar window registers being
	 * 64-bit wide, the upper 32-bit always read back as zeroes, so
	 * it is dubious whether it is possible to use more than a 4GB
	 * address space... and thus more than 2GB of physical memory.
	 */

	physmem = btoc(memlo + memhi);
	if (physmem > btoc(DDR_PHYSICAL_SIZE)) {
		pmon_printf("WARNING! %d MB of memory will not be used",
		    (physmem >> 20) - (DDR_PHYSICAL_SIZE >> 20));
		memhi = DDR_PHYSICAL_SIZE - btoc(256 << 20);
	}

	physmem = btoc(memlo + memhi);

	/*
	 * PMON configures the system with only the low 256MB of memory
	 * accessible.
	 *
	 * We need to reprogram the address windows in order to be able to
	 * access the whole memory, both by the local processor and by the
	 * PCI bus.
	 *
	 * To make our life easier, we'll setup the memory as a contiguous
	 * range starting at 2GB, and take into account the fact that the
	 * first 256MB are also aliased at address zero (which is where the
	 * kernel is loaded, really).
	 */

	if (memhi != 0 ) {
		/* do NOT stomp on exception area */
		/* also make sure to skip the kernel */
		const paddr_t kernend = MIPS_KSEG0_TO_PHYS(round_page(vkernend));
		mem_clusters[0].start = DDR_WINDOW_BASE + kernend;
		mem_clusters[0].size = (memlo + memhi - kernend);
		t->_wbase = PCI_DDR_BASE;
		t->_bounce_alloc_lo = DDR_WINDOW_BASE;
		t->_bounce_alloc_hi = DDR_WINDOW_BASE + DDR_PHYSICAL_SIZE;
		mem_cluster_cnt = 1;
	} else {
		/* do NOT stomp on exception area */
		/* mips_page_physload() will skip the kernel */
		mem_clusters[0].start = DDR_PHYSICAL_BASE + PAGE_SIZE;
		mem_clusters[0].size = (memlo + memhi - PAGE_SIZE - PAGE_SIZE);
		t->_wbase = PCI_DDR_BASE;
		t->_bounce_alloc_lo = DDR_PHYSICAL_BASE;
		t->_bounce_alloc_hi = DDR_PHYSICAL_BASE + DDR_PHYSICAL_SIZE;
		mem_cluster_cnt = 1;
	}

	/*
	 * Allow access to memory beyond 256MB, by programming the
	 * Loongson 2F address window registers.
	 * This also makes sure PCI->DDR accesses can use a contiguous
	 * area regardless of the actual memory size.
	 */

	/*
	 * Master #0 (cpu) window #0 allows access to the low 256MB
	 * of memory at address zero onwards.
	 * This window is inherited from PMON; we set it up just in case.
	 */
	loongson2f_setup_window(MASTER_CPU, WINDOW_CPU_LOW, DDR_PHYSICAL_BASE,
	    ~(0x0fffffffUL), DDR_PHYSICAL_BASE, MASTER_CPU);

	/*
	 * Master #0 (cpu) window #1 allows access to the ``low'' PCI
	 * space (from 0x10000000 to 0x1fffffff).
	 * This window is inherited from PMON; we set it up just in case.
	 */
	loongson2f_setup_window(MASTER_CPU, WINDOW_CPU_PCILO, BONITO_PCILO_BASE,
	    ~(0x0fffffffUL), BONITO_PCILO_BASE, MASTER_PCI);

	/*
	 * Master #1 (PCI) window #0 allows access to the memory space
	 * by PCI devices at addresses 0x80000000 onwards.
	 * This window is inherited from PMON, but its mask might be too
	 * restrictive (256MB) so we make sure it matches our needs.
	 */
	loongson2f_setup_window(MASTER_PCI, WINDOW_PCI_DDR, PCI_DDR_BASE,
	    ~(DDR_PHYSICAL_SIZE - 1), DDR_PHYSICAL_BASE, MASTER_CPU);

	/*
	 * Master #0 (CPU) window #2 allows access to a subset of the ``high''
	 * PCI space (from 0x40000000 to 0x7fffffff only).
	 */
	loongson2f_setup_window(MASTER_CPU, WINDOW_CPU_PCIHI, LS2F_PCIHI_BASE,
	    ~((uint64_t)LS2F_PCIHI_SIZE - 1), LS2F_PCIHI_BASE, MASTER_PCI);

	/*
	 * Master #0 (CPU) window #3 allows access to the whole memory space
	 * at addresses 0x80000000 onwards.
	 */
	loongson2f_setup_window(MASTER_CPU, WINDOW_CPU_DDR, DDR_WINDOW_BASE,
	    ~(DDR_PHYSICAL_SIZE - 1), DDR_PHYSICAL_BASE, MASTER_CPU);

}

/*
 * Setup a window in the Loongson2F crossbar.
 */

static void
loongson2f_setup_window(uint master, uint window, uint64_t base, uint64_t mask,
    uint64_t mmap, uint slave)
{
	volatile uint64_t *awrreg;

	awrreg = (volatile uint64_t *)MIPS_PHYS_TO_XKPHYS(CCA_UNCACHED,
	    LOONGSON_AWR_BASE(master, window));
	*awrreg = base;
	(void)*awrreg;

	awrreg = (volatile uint64_t *)MIPS_PHYS_TO_XKPHYS(CCA_UNCACHED,
	    LOONGSON_AWR_SIZE(master, window));
	*awrreg = mask;
	(void)*awrreg;

	awrreg = (volatile uint64_t *)MIPS_PHYS_TO_XKPHYS(CCA_UNCACHED,
	    LOONGSON_AWR_MMAP(master, window));
	*awrreg = mmap | slave;
	(void)*awrreg;
}

/*
 * Return whether a given physical address points to managed memory.
 * (used by /dev/mem)
 */

boolean_t
is_memory_range(paddr_t pa, psize_t len, psize_t limit)
{
	uint64_t fp, lp;
	int i;

	fp = atop(pa);
	lp = atop(round_page(pa + len));

	if (limit != 0 && lp > atop(limit))
		return FALSE;

	/*
	 * Allow access to the low 256MB aliased region on 2F systems,
	 * if we are accessing memory at 2GB onwards.
	 */
	if (pa < 0x10000000 && loongson_ver >= 0x2f) {
		fp += btoc(mem_clusters[0].start);
		lp += btoc(mem_clusters[0].start);
	}

	for (i = 0; i < VM_PHYSSEG_MAX; i++)
		if (fp >= btoc(mem_clusters[i].start) &&
		    lp <= btoc(mem_clusters[i].start + mem_clusters[i].size))
			return TRUE;

	return FALSE;
}
