/* $NetBSD: ofwoea_machdep.c,v 1.62 2021/12/05 07:13:48 msaitoh Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: ofwoea_machdep.c,v 1.62 2021/12/05 07:13:48 msaitoh Exp $");

#include "ksyms.h"
#include "wsdisplay.h"

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_oea.h"
#include "opt_ofwoea.h"
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/vmparam.h>
#include <machine/autoconf.h>
#include <sys/bus.h>
#include <powerpc/oea/bat.h>
#include <powerpc/oea/ofw_rasconsvar.h>
#include <powerpc/oea/cpufeat.h>
#include <powerpc/include/oea/spr.h>
#include <powerpc/ofw_cons.h>
#include <powerpc/ofw_machdep.h>
#include <powerpc/spr.h>
#include <powerpc/pic/picvar.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#ifdef ofppc
extern struct model_data modeldata;
#endif

#ifdef OFWOEA_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

typedef struct _rangemap {
	u_int32_t addr;
	u_int32_t size;
	int type;
} rangemap_t;

struct OF_translation ofw_translations[OFW_MAX_TRANSLATIONS];

/*
 * Data structures holding OpenFirmware's translations when running
 * in virtual-mode.
 *
 * When we call into OpenFirmware, we point the calling CPU's
 * cpu_info::ci_battable at ofw_battable[].  For now, this table
 * is empty, which will ensure that any DSI exceptions that occur
 * during the firmware call will not erroneously load kernel BAT
 * mappings that could clobber the firmware's translations.
 */
struct pmap ofw_pmap;
struct bat ofw_battable[BAT_VA2IDX(0xffffffff)+1];

char bootpath[256];
char model_name[64];
#if NKSYMS || defined(DDB) || defined(MODULAR)
void *startsym, *endsym;
#endif

#if PPC_OEA601
#define TIMEBASE_FREQ (1000000000)  /* RTC register */
#endif

#ifdef TIMEBASE_FREQ
u_int timebase_freq = TIMEBASE_FREQ;
#else
u_int timebase_freq = 0;
#endif

int ofw_quiesce;

extern int ofwmsr;
extern uint32_t ticks_per_sec;
extern uint32_t ns_per_tick;
extern uint32_t ticks_per_intr;

static void get_timebase_frequency(void);
static void init_decrementer(void);

static void restore_ofmap(void);

void
ofwoea_initppc(u_int startkernel, u_int endkernel, char *args)
{
	register_t scratch;

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* get info of kernel symbol table from bootloader */
	memcpy(&startsym, args + strlen(args) + 1, sizeof(startsym));
	memcpy(&endsym, args + strlen(args) + 1 + sizeof(startsym),
	    sizeof(endsym));
	if (startsym == NULL || endsym == NULL)
	    startsym = endsym = NULL;
#endif

	/* Parse the args string */
	if (args) {
		strcpy(bootpath, args);
		args = bootpath;
		while (*++args && *args != ' ');
		if (*args) {
			*args++ = 0;
			while (*args)
				BOOT_FLAG(*args++, boothowto);
		}
	} else {
		int chs = OF_finddevice("/chosen");
		int len;

		len = OF_getprop(chs, "bootpath", bootpath, sizeof(bootpath) - 1);
		if (len > -1)
			bootpath[len] = 0;
	}

	/* Get the timebase frequency from the firmware. */
	get_timebase_frequency();

	/* Probe for the console device; it's initialized later. */
	ofwoea_cnprobe();

	if (ofw_quiesce)
		OF_quiesce();

	oea_init(pic_ext_intr);

	/*
	 * Now that we've installed our own exception vectors,
	 * ensure that exceptions that happen while running
	 * firmware code fall into ours.
	 */
	ofwmsr &= ~PSL_IP;

	/* Initialize bus_space */
	ofwoea_bus_space_init();

	/* Initialize the console device. */
	ofwoea_consinit();

	uvm_md_init();

	pmap_bootstrap(startkernel, endkernel);

/* as far as I can tell, the pmap_setup_seg0 stuff is horribly broken */
#if defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#if defined (PMAC_G5)
	/* Mapin 1st 256MB segment 1:1, also map in mem needed to access OFW*/
	if (oeacpufeat & OEACPU_64_BRIDGE) {
		vaddr_t va;
		paddr_t pa;
		vsize_t size;
		int i;

		pmap_setup_segment0_map(0, msgbuf_paddr, msgbuf_paddr,
		    round_page(MSGBUFSIZE), 0x0);

		/* Map OFW code+data */

		for (i = 0; i < __arraycount(ofw_translations); i++) {
			va = ofw_translations[i].virt;
			size = ofw_translations[i].size;
			pa = ofw_translations[i].phys;
			/* XXX mode */

			if (size == 0) {
				/* No more, all done! */
				break;
			}

			if (va < 0xff800000)
				continue;


			for (; va < (ofw_translations[i].virt + size);
			    va += PAGE_SIZE, pa += PAGE_SIZE) {
				pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL,
				    VM_PROT_ALL | PMAP_WIRED);
			}
		}

#if NWSDISPLAY > 0
		/* Map video frame buffer */

		struct rasops_info *ri = &rascons_console_screen.scr_ri;

		if (ri->ri_bits != NULL) {
			for (va = (vaddr_t) ri->ri_bits;
			    va < round_page((vaddr_t) ri->ri_bits +
				ri->ri_height * ri->ri_stride);
			    va += PAGE_SIZE) {
				pmap_enter(pmap_kernel(), va, va,
				    VM_PROT_READ | VM_PROT_WRITE,
				    PMAP_NOCACHE | PMAP_WIRED);
			}
		}
#endif
	}
#elif defined (MAMBO)
	/* Mapin 1st 256MB segment 1:1, also map in mem needed to access OFW*/
	if (oeacpufeat & OEACPU_64_BRIDGE)
		pmap_setup_segment0_map(0, 0xf4000000, 0xf4000000, 0x1000, 0x0);
#endif /* PMAC_G5 */
#endif /* PPC_OEA64 || PPC_OEA64_BRIDGE */

	/* Now enable translation (and machine checks/recoverable interrupts) */
	__asm __volatile ("sync; mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch)
	    : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	restore_ofmap();

	rascons_finalize();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((uintptr_t)endsym - (uintptr_t)startsym), startsym, endsym);
#endif

	/* Kick off the clock. */
	init_decrementer();

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

static void
get_timebase_frequency(void)
{
	int qhandle, phandle, node;
	char type[32];

	if (timebase_freq != 0) {
		ticks_per_sec = timebase_freq;
		return;
	}

	node = OF_finddevice("/cpus/@0");
	if (node != -1 &&
	    OF_getprop(node, "timebase-frequency", &ticks_per_sec,
		       sizeof ticks_per_sec) > 0) {
		return;
	}

	node = OF_finddevice("/");
	for (qhandle = node; qhandle; qhandle = phandle) {
		if (OF_getprop(qhandle, "device_type", type, sizeof type) > 0
		    && strcmp(type, "cpu") == 0
		    && OF_getprop(qhandle, "timebase-frequency",
			&ticks_per_sec, sizeof ticks_per_sec) > 0) {
			return;
		}
		if ((phandle = OF_child(qhandle)))
			continue;
		while (qhandle) {
			if ((phandle = OF_peer(qhandle)))
				break;
			qhandle = OF_parent(qhandle);
		}
	}
	panic("no cpu node");
}

static void
init_decrementer(void)
{
	int scratch, msr;

	KASSERT(ticks_per_sec != 0);

	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		: "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	ns_per_tick = 1000000000 / ticks_per_sec;
	ticks_per_intr = ticks_per_sec / hz;
	cpu_timebase = ticks_per_sec;

#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601)
		curcpu()->ci_lasttb = rtc_nanosecs();
	else
#endif
		curcpu()->ci_lasttb = mftbl();

	mtspr(SPR_DEC, ticks_per_intr);
	mtmsr(msr);
}

void
restore_ofmap(void)
{
	vaddr_t va, size;
	paddr_t pa;
	int i;

	pmap_pinit(&ofw_pmap);

#ifndef _LP64
	ofw_pmap.pm_sr[0] = KERNELN_SEGMENT(0)|SR_PRKEY;
	ofw_pmap.pm_sr[KERNEL_SR] = KERNEL_SEGMENT|SR_SUKEY|SR_PRKEY;

#ifdef KERNEL2_SR
	ofw_pmap.pm_sr[KERNEL2_SR] = KERNEL2_SEGMENT|SR_SUKEY|SR_PRKEY;
#endif
#endif

	for (i = 0; i < __arraycount(ofw_translations); i++) {
		va = ofw_translations[i].virt;
		size = ofw_translations[i].size;
		pa = ofw_translations[i].phys;
		/* XXX mode */

		if (size == 0) {
			/* No more, all done! */
			break;
		}

		if (va < 0xf0000000)	/* XXX */
			continue;

		/*
		 * XXX macallan@
		 * My beige G3 throws a DSI trap if we try to map the last page
		 * of the 32bit address space. On old world macs the firmware
		 * ROM occupies 4MB at 0xffc00000, triggering it when we 
		 * restore OF translations. This just works around a bug
		 * elsewhere in pmap and should go away once fixed there.
		 */
		if (pa == 0xffc00000 && size == 0x400000)
			size = 0x3ff000;

		while (size > 0) {
			pmap_enter(&ofw_pmap, va, pa, VM_PROT_ALL,
			    VM_PROT_ALL|PMAP_WIRED);
			pa += PAGE_SIZE;
			va += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	}
	pmap_update(&ofw_pmap);
}

/* we define these partially, as we will fill the rest in later */
struct powerpc_bus_space genppc_isa_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_base = 0x00000000,
};

struct powerpc_bus_space genppc_isa_mem_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_base = 0x00000000,
};

/* This gives us a maximum of 6 PCI busses, assuming both io/mem on each.
 * Increase if necc.
 */
static char ex_storage[EXSTORAGE_MAX][EXTENT_FIXED_STORAGE_SIZE(EXTMAP_RANGES)]
	__attribute__((aligned(8)));


static void
find_ranges(int base, rangemap_t *regions, int *cur, int type)
{
	int node, i, len, reclen;
	u_int32_t parent_acells, acells, scells, map[160];
	char tmp[32];

	node = base;
	if (OF_getprop(node, "device_type", tmp, sizeof(tmp)) == -1)
		goto rec;
	if ((type == RANGE_TYPE_PCI || type == RANGE_TYPE_FIRSTPCI) &&
	    strcmp("pci", tmp) != 0)
		goto rec;
	if (type == RANGE_TYPE_ISA && strcmp("isa", tmp) != 0)
		goto rec;
	if (type == RANGE_TYPE_MACIO && strcmp("memory-controller", tmp) == 0) {
		len = OF_getprop(node, "reg", map, sizeof(map));
		acells = 1;
		scells = 1;
	} else {
		len = OF_getprop(node, "ranges", map, sizeof(map));
	}
	if (len == -1)
		goto rec;
	if (OF_getprop(OF_parent(node), "#address-cells", &parent_acells,
	    sizeof(parent_acells)) != sizeof(parent_acells))
		parent_acells = 1;
	if (OF_getprop(node, "#address-cells", &acells,
	    sizeof(acells)) != sizeof(acells))
		acells = 3;
	if (OF_getprop(node, "#size-cells", &scells,
	    sizeof(scells)) != sizeof(scells))
		scells = 2;
#ifdef ofppc
	if (modeldata.ranges_offset == 0)
		scells -= 1;
#endif
	if (type == RANGE_TYPE_ISA)
		reclen = 6;
	else
		reclen = parent_acells + acells + scells;
	/*
	 * There exist ISA buses with empty ranges properties.  This is
	 * known to occur on the Pegasos II machine, and likely others.
	 * According to them, that means that the isa bus is a fake bus, and
	 * the real maps are the PCI maps of the preceding bus.  To deal
	 * with this, we will set cur to -1 and return.
	 */
	if (type == RANGE_TYPE_ISA && strcmp("isa", tmp) == 0 && len == 0) {
		*cur = -1;
		DPRINTF("Found empty range in isa bus\n");
		return;
	}

	DPRINTF("found a map reclen=%d cur=%d len=%d\n", reclen, *cur, len);
	switch (type) {
		case RANGE_TYPE_PCI:
		case RANGE_TYPE_FIRSTPCI:
			for (i=0; i < len/(4*reclen); i++) {
				DPRINTF("FOUND PCI RANGE\n");
				regions[*cur].size =
				    map[i*reclen + parent_acells + acells + scells - 1];
				/* skip ranges of size==0 */
				if (regions[*cur].size == 0)
					continue;
				regions[*cur].type = (map[i*reclen] >> 24) & 0x3;
				regions[*cur].addr = map[i*reclen + parent_acells + acells - 1];
				(*cur)++;
			}
			break;
		case RANGE_TYPE_ISA:
			for (i=0; i < len/(4*reclen); i++) {
				if (map[i*reclen] == 1)
					regions[*cur].type = RANGE_IO;
				else
					regions[*cur].type = RANGE_MEM;
				DPRINTF("FOUND ISA RANGE TYPE=%d\n",
					regions[*cur].type);
				regions[*cur].size =
				    map[i*reclen + acells + scells];
				(*cur)++;
			}
			break;
		case RANGE_TYPE_MACIO:
			regions[*cur].type = RANGE_MEM;
			if (len == 8) {
				regions[*cur].size = map[1];
				regions[*cur].addr = map[0];
			} else {
				regions[*cur].size = map[2];
				regions[*cur].addr = map[1];
			}				
			(*cur)++;		
			break;
	}
	DPRINTF("returning with CUR=%d\n", *cur);
	return;
rec:
	for (node = OF_child(base); node; node = OF_peer(node)) {
		DPRINTF("RECURSE 1 STEP\n");
		find_ranges(node, regions, cur, type);
		if (*cur == -1)
			return;
	}
}

static int
find_lowest_range(rangemap_t *ranges, int nrof, int type)
{
	int i, low = 0;
	u_int32_t addr = 0xffffffff;

	for (i=0; i < nrof; i++) {
		if (ranges[i].type == type && ranges[i].addr != 0 &&
		    ranges[i].addr < addr) {
			low = i;
			addr = ranges[i].addr;
		}
	}
	if (addr == 0xffffffff)
		return -1;
	return low;
}

/*
 * Find a region of memory, and create a bus_space_tag for it.
 * Notes:
 * For ISA node is ignored.
 * node is the starting node.  if -1, we start at / and map everything.
 */

int
ofwoea_map_space(int rangetype, int iomem, int node,
    struct powerpc_bus_space *tag, const char *name)
{
	int i, cur, range, nrofholes, error;
	static int exmap=0;
	rangemap_t region, holes[32], list[32];

	memset(list, 0, sizeof(list));
	memset(&region, 0, sizeof(region));
	cur = 0;
	if (rangetype == RANGE_TYPE_ISA || node == -1)
		node = OF_finddevice("/");
	if (rangetype == RANGE_TYPE_ISA) {
		u_int32_t size = 0;
		rangemap_t regions[32];

		DPRINTF("LOOKING FOR FIRSTPCI\n");
		find_ranges(node, list, &cur, RANGE_TYPE_FIRSTPCI);
		range = 0;
		DPRINTF("LOOKING FOR ISA\n");
		find_ranges(node, regions, &range, RANGE_TYPE_ISA);
		if (range == 0 || cur == 0)
			return -1; /* no isa stuff found */
		/*
		 * This may be confusing to some.  The ISA ranges property
		 * is supposed to be a set of IO ranges for the ISA bus, but
		 * generally, it's just a set of pci devfunc lists that tell
		 * you to go look at the parent PCI device for the actual
		 * ranges.
		 */
		if (range == -1) {
			/* we found a rangeless isa bus */
			if (iomem == RANGE_IO)
				size = 0x10000;
			else
				size = 0x1000000;
		}
		DPRINTF("found isa stuff\n");
		for (i=0; i < range; i++)
			if (regions[i].type == iomem)
				size = regions[i].size;
		if (iomem == RANGE_IO) {
			/* the first io range is the one */
			for (i=0; i < cur; i++)
				if (list[i].type == RANGE_IO && size) {
					DPRINTF("found IO\n");
					tag->pbs_offset = list[i].addr;
					tag->pbs_limit = size;
					error = bus_space_init(tag, name,
					    ex_storage[exmap],
					    sizeof(ex_storage[exmap]));
					exmap++;
					return error;
				}
		} else {
			for (i=0; i < cur; i++)
				if (list[i].type == RANGE_MEM &&
				    list[i].size == size) {
					DPRINTF("found mem\n");
					tag->pbs_offset = list[i].addr;
					tag->pbs_limit = size;
					error = bus_space_init(tag, name,
					    ex_storage[exmap],
					    sizeof(ex_storage[exmap]));
					exmap++;
					return error;
				}
		}
		return -1; /* NO ISA FOUND */
	}
	find_ranges(node, list, &cur, rangetype);

	DPRINTF("cur == %d\n", cur);
	/* now list should contain a list of memory regions */
	for (i=0; i < cur; i++)
		DPRINTF("addr=0x%x size=0x%x type=%d\n", list[i].addr,
		    list[i].size, list[i].type);

	range = find_lowest_range(list, cur, iomem);
	i = 0;
	nrofholes = 0;
	while (range != -1) {
		DPRINTF("range==%d\n", range);
		DPRINTF("i==%d\n", i);
		if (i == 0) {
			memcpy(&region, &list[range], sizeof(rangemap_t));
			list[range].addr = 0;
			i++;
			range = find_lowest_range(list, cur, iomem);
			continue;
		}
		if (region.addr + region.size < list[range].addr) {
			/* allocate a hole */
			holes[nrofholes].type = iomem;
			holes[nrofholes].addr = region.size + region.addr;
			holes[nrofholes].size = list[range].addr -
			    holes[nrofholes].addr - 1;
			nrofholes++;
		}
		region.size = list[range].size + list[range].addr -
		    region.addr;
		list[range].addr = 0;
		range = find_lowest_range(list, cur, iomem);
	}
	DPRINTF("RANGE iomem=%d FOUND\n", iomem);
	DPRINTF("addr=0x%x size=0x%x type=%d\n", region.addr,
		    region.size, region.type);
	DPRINTF("HOLES FOUND\n");
	for (i=0; i < nrofholes; i++)
		DPRINTF("addr=0x%x size=0x%x type=%d\n", holes[i].addr,
		    holes[i].size, holes[i].type);
	/* AT THIS POINT WE MAP IT */

	if ((rangetype == RANGE_TYPE_PCI) || (rangetype == RANGE_TYPE_MACIO)) {
		if (exmap == EXSTORAGE_MAX)
			panic("Not enough ex_storage space. "
			    "Increase EXSTORAGE_MAX");

		/* XXX doing this in here might be wrong */
		if (iomem == 1) {
			/* we map an IO region */
			tag->pbs_offset = region.addr;
			tag->pbs_base = 0;
			tag->pbs_limit = region.size;
		} else {
			/* ... or a memory region */
			tag->pbs_offset = 0;
			tag->pbs_base = region.addr;
			tag->pbs_limit = region.size + region.addr;
		}

		error = bus_space_init(tag, name, ex_storage[exmap],
		    sizeof(ex_storage[exmap]));
		exmap++;
		if (error)
			panic("ofwoea_bus_space_init: can't init tag %s", name);
		for (i=0; i < nrofholes; i++) {
			if (holes[i].type == RANGE_IO) {
				error = extent_alloc_region(tag->pbs_extent,
				    holes[i].addr - tag->pbs_offset,
				    holes[i].size, EX_NOWAIT);
			} else {
				error = extent_alloc_region(tag->pbs_extent,
				    holes[i].addr, holes[i].size, EX_NOWAIT);
			}
			if (error)
				panic("ofwoea_bus_space_init: can't block out"
				    " reserved space 0x%x-0x%x: error=%d",
				    holes[i].addr, holes[i].addr+holes[i].size,
				    error);
		}
		return error;
	}
	return -1;
}

void
ofwoea_bus_space_init(void)
{
	int error;

	error = ofwoea_map_space(RANGE_TYPE_ISA, RANGE_IO, -1,
	    &genppc_isa_io_space_tag, "isa-ioport");
	if (error > 0)
		panic("Could not map ISA IO");

	error = ofwoea_map_space(RANGE_TYPE_ISA, RANGE_MEM, -1,
	    &genppc_isa_mem_space_tag, "isa-iomem");
	if (error > 0)
		panic("Could not map ISA MEM");
}
