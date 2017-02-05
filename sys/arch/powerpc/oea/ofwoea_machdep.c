/* $NetBSD: ofwoea_machdep.c,v 1.38.6.1 2017/02/05 13:40:18 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ofwoea_machdep.c,v 1.38.6.1 2017/02/05 13:40:18 skrll Exp $");

#include "opt_ppcarch.h"
#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_modular.h"

#include "wsdisplay.h"

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
#include <powerpc/spr.h>
#include <powerpc/pic/picvar.h>

#include "opt_oea.h"

#include "ksyms.h"

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#include "opt_ofwoea.h"

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

struct ofw_translations {
	vaddr_t va;
	int len;
#if defined (PMAC_G5)
	register64_t pa;
#else
	register_t pa;
#endif
	int mode;
}__attribute__((packed));

struct pmap ofw_pmap;
struct ofw_translations ofmap[32];
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

extern int ofwmsr;
extern int chosen;
extern uint32_t ticks_per_sec;
extern uint32_t ns_per_tick;
extern uint32_t ticks_per_intr;

static int save_ofmap(struct ofw_translations *, int);
static void restore_ofmap(struct ofw_translations *, int);
static void set_timebase(void);

extern void cpu_spinstart(u_int);
extern volatile u_int cpu_spinstart_ack;

void
ofwoea_initppc(u_int startkernel, u_int endkernel, char *args)
{
	int ofmaplen, node, l;
	register_t scratch;

#if defined(MULTIPROCESSOR) && defined(ofppc)
	char cpupath[32];
	int i;
#endif

	/* initialze bats */
	if ((oeacpufeat & OEACPU_NOBAT) == 0)
		ofwoea_batinit();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* get info of kernel symbol table from bootloader */
	memcpy(&startsym, args + strlen(args) + 1, sizeof(startsym));
	memcpy(&endsym, args + strlen(args) + 1 + sizeof(startsym),
	    sizeof(endsym));
	if (startsym == NULL || endsym == NULL)
	    startsym = endsym = NULL;
#endif

	/* get model name and perform model-specific actions */
	memset(model_name, 0, sizeof(model_name));
	node = OF_finddevice("/");
	if (node != -1) {
		l = OF_getprop(node, "model", model_name, sizeof(model_name));
		if (l == -1)
			OF_getprop(node, "name", model_name,
			    sizeof(model_name));
		model_init();
	}

	if (strcmp(model_name, "PowerMac11,2") == 0 ||
	    strcmp(model_name, "PowerMac11,1") == 0)
		OF_quiesce();

	/* Initialize bus_space */
	ofwoea_bus_space_init();

	ofwoea_consinit();

#if defined(MULTIPROCESSOR) && defined(ofppc)
	for (i=1; i < CPU_MAXNUM; i++) {
		snprintf(cpupath, sizeof(cpupath), "/cpus/@%x", i);
		node = OF_finddevice(cpupath);
		if (node <= 0)
			continue;
		aprint_verbose("Starting up CPU %d %s\n", i, cpupath);
		OF_start_cpu(node, (u_int)cpu_spinstart, i);
		for (l=0; l < 100000000; l++) {
			if (cpu_spinstart_ack == i) {
				aprint_verbose("CPU %d spun up.\n", i);
				break;
			}
			__asm volatile ("sync");
		}
	}
#endif

	oea_init(pic_ext_intr);

	ofmaplen = save_ofmap(NULL, 0);
	if (ofmaplen > 0)
		save_ofmap(ofmap, ofmaplen);

/*
 * XXX
 * we need to do this here instead of earlier on in ofwinit() for some reason
 * At least some versions of Apple OF 2.0.1 hang if we do this earlier
 */ 
	ofwmsr &= ~PSL_IP;

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
	}

	uvm_md_init();

	pmap_bootstrap(startkernel, endkernel);

/* as far as I can tell, the pmap_setup_seg0 stuff is horribly broken */
#if defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#if defined (PMAC_G5)
	/* Mapin 1st 256MB segment 1:1, also map in mem needed to access OFW*/
	if (oeacpufeat & OEACPU_64_BRIDGE) {
		vaddr_t va;
		paddr_t pa;
		int i;

		pmap_setup_segment0_map(0, msgbuf_paddr, msgbuf_paddr,
		    round_page(MSGBUFSIZE), 0x0);

		/* Map OFW code+data */

		for (i = 0; i < ofmaplen / sizeof(struct ofw_translations); i++) {
			if (ofmap[i].va < 0xff800000)
				continue;

			for (va = ofmap[i].va, pa = ofmap[i].pa;
			    va < ofmap[i].va + ofmap[i].len;
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

	restore_ofmap(ofmap, ofmaplen);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((uintptr_t)endsym - (uintptr_t)startsym), startsym, endsym);
#endif

	/* CPU clock stuff */
	set_timebase();

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
set_timebase(void)
{
	int qhandle, phandle, msr, scratch;
	char type[32];

	if (timebase_freq != 0) {
		ticks_per_sec = timebase_freq;
		goto found;
	}

	for (qhandle = OF_peer(0); qhandle; qhandle = phandle) {
		if (OF_getprop(qhandle, "device_type", type, sizeof type) > 0
		    && strcmp(type, "cpu") == 0
		    && OF_getprop(qhandle, "timebase-frequency",
			&ticks_per_sec, sizeof ticks_per_sec) > 0) {
			goto found;
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

found:
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

static int
save_ofmap(struct ofw_translations *map, int maxlen)
{
	int mmui, mmu, len;

	OF_getprop(chosen, "mmu", &mmui, sizeof mmui);
	mmu = OF_instance_to_package(mmui);

	if (map) {
		memset(map, 0, maxlen); /* to be safe */
		len = OF_getprop(mmu, "translations", map, maxlen);
	} else
		len = OF_getproplen(mmu, "translations");

	if (len < 0)
		len = 0;
	return len;
}


/* The PMAC_G5 code here needs to be replaced by code that looks for the
   size_cells and does the right thing automatically.
*/
void
restore_ofmap(struct ofw_translations *map, int len)
{
	int n = len / sizeof(struct ofw_translations);
	int i;

	pmap_pinit(&ofw_pmap);

#ifndef _LP64
	ofw_pmap.pm_sr[0] = KERNELN_SEGMENT(0)|SR_PRKEY;
	ofw_pmap.pm_sr[KERNEL_SR] = KERNEL_SEGMENT|SR_SUKEY|SR_PRKEY;

#ifdef KERNEL2_SR
	ofw_pmap.pm_sr[KERNEL2_SR] = KERNEL2_SEGMENT|SR_SUKEY|SR_PRKEY;
#endif
#endif

	for (i = 0; i < n; i++) {
#if defined (PMAC_G5)
		register64_t pa = map[i].pa;
#else
		register_t pa = map[i].pa;
#endif
		vaddr_t va = map[i].va;
		size_t length = map[i].len;

		if (va < 0xf0000000) /* XXX */
			continue;

		while (length > 0) {
			pmap_enter(&ofw_pmap, va, (paddr_t)pa, VM_PROT_ALL,
			    VM_PROT_ALL|PMAP_WIRED);
			pa += PAGE_SIZE;
			va += PAGE_SIZE;
			length -= PAGE_SIZE;
		}
	}
	pmap_update(&ofw_pmap);
}



/*
 * Scan the device tree for ranges, and return them as bitmap 0..15
 */
#if !defined(macppc) && defined(PPC_OEA)
static u_int16_t
ranges_bitmap(int node, u_int16_t bitmap)
{
	int child, mlen, acells, scells, reclen, i, j;
	u_int32_t addr, len, map[160];

	for (child = OF_child(node); child; child = OF_peer(child)) {
		mlen = OF_getprop(child, "ranges", map, sizeof(map));
		if (mlen == -1)
			goto noranges;

		j = OF_getprop(child, "#address-cells", &acells,
		    sizeof(acells));
		if (j == -1)
			goto noranges;

		j = OF_getprop(child, "#size-cells", &scells,
		    sizeof(scells));
		if (j == -1)
			goto noranges;

#ifdef ofppc
		reclen = acells + modeldata.ranges_offset + scells;
#else
		reclen = acells + 1 + scells;
#endif

		for (i=0; i < (mlen/4)/reclen; i++) {
			addr = map[reclen * i + acells];
			len = map[reclen * i + reclen - 1];
			for (j = 0; j < len / 0x10000000; j++)
				bitmap |= 1 << ((addr+j*0x10000000) >>28);
			bitmap |= 1 << (addr >> 28);
		}
noranges:
		bitmap |= ranges_bitmap(child, bitmap);
		continue;
	}
	return bitmap;
}
#endif /* !macppc && PPC_OEA */

void
ofwoea_batinit(void)
{
#if defined (PPC_OEA)

#ifdef macppc
	/*
	 * cover PCI and register space but not the firmware ROM
	 */
#ifdef PPC_OEA601

        /*
	 * use segment registers for the 601
	 */
	if ((mfpvr() >> 16 ) == MPC601)
	    oea_batinit(
		0x80000000, BAT_BL_256M,
		0x90000000, BAT_BL_256M,
		0xa0000000, BAT_BL_256M,
		0xb0000000, BAT_BL_256M,
		0xf0000000, BAT_BL_256M,
		0);
	else
#endif
	/*
	 * map to bats
	 */
	oea_batinit(0x80000000, BAT_BL_1G,
		    0xf0000000, BAT_BL_128M,
		    0xf8000000, BAT_BL_64M,
		    0xfe000000, BAT_BL_8M,	/* Grackle IO */
		    0);
#else
	uint16_t bitmap;
	int node, i;

	node = OF_finddevice("/");

	bitmap = ranges_bitmap(node, 0);
	oea_batinit(0);

	for (i=1; i < 0x10; i++) {
		/* skip the three vital SR regions */
		if (i == USER_SR || i == KERNEL_SR || i == KERNEL2_SR)
			continue;
		if (bitmap & (1 << i)) {
			oea_iobat_add(0x10000000 * i, BAT_BL_256M);
			DPRINTF("Batmapped 256M at 0x%x\n", 0x10000000 * i);
		}
	}
#endif
#endif /* OEA */
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
	len = OF_getprop(node, "ranges", map, sizeof(map));
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
	 * the real maps are the PCI maps of the preceeding bus.  To deal
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

	if (rangetype == RANGE_TYPE_PCI) {
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
