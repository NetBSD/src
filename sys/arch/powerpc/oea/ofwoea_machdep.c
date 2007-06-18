/* $NetBSD: ofwoea_machdep.c,v 1.1.2.4 2007/06/18 03:31:33 macallan Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ofwoea_machdep.c,v 1.1.2.4 2007/06/18 03:31:33 macallan Exp $");


#include "opt_compat_netbsd.h"
#include "opt_ddb.h" 
#include "opt_kgdb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/extent.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <powerpc/bus.h>
#include <powerpc/oea/bat.h>
#include <powerpc/ofw_bus.h>
#include <powerpc/ofw_cons.h>
#include <powerpc/spr.h>
#include <arch/powerpc/pic/picvar.h>

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
#if NKSYMS || defined(DDB) || defined(LKM)
void *startsym, *endsym;
#endif
#ifdef TIMEBASE_FREQ
u_int timebase_freq = TIMEBASE_FREQ;
#else
u_int timebase_freq = 0;
#endif

extern int ofmsr;
extern int chosen;
extern uint32_t ticks_per_sec;
extern uint32_t ns_per_tick;
extern uint32_t ticks_per_intr;

static int save_ofmap(struct ofw_translations *, int);
static void restore_ofmap(struct ofw_translations *, int);
static void set_timebase(void);

void
ofwoea_initppc(u_int startkernel, u_int endkernel, char *args)
{
	int ofmaplen;
#if defined (PPC_OEA64_BRIDGE)
	register_t scratch;
#endif

#if defined (PPC_OEA)
	/* initialze bats */
	ofwoea_batinit();
#elif defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#endif /* PPC_OEA */

#if NKSYMS || defined(DDB) || defined(LKM)
	/* get info of kernel symbol table from bootloader */
	memcpy(&startsym, args + strlen(args) + 1, sizeof(startsym));
	memcpy(&endsym, args + strlen(args) + 1 + sizeof(startsym),
	    sizeof(endsym));
	if (startsym == NULL || endsym == NULL)
	    startsym = endsym = NULL;
#endif

	ofwoea_bus_space_init();

	ofwoea_consinit();

	oea_init(pic_ext_intr);

	ofmaplen = save_ofmap(NULL, 0);
	if (ofmaplen > 0)
		save_ofmap(ofmap, ofmaplen);

	ofmsr &= ~PSL_IP;

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

	uvm_setpagesize();
	pmap_bootstrap(startkernel, endkernel);

#if defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#if defined (PMAC_G5)
	/* Mapin 1st 256MB segment 1:1, also map in mem needed to access OFW*/
	pmap_setup_segment0_map(0, 0xff800000, 0x3fc00000, 0x400000, 0x0);
#elif defined (MAMBO)
	/* Mapin 1st 256MB segment 1:1, also map in mem needed to access OFW*/
	pmap_setup_segment0_map(0, 0xf4000000, 0xf4000000, 0x1000, 0x0);
#endif /* PMAC_G5 */

	/* Now enable translation (and machine checks/recoverable interrupts) */
	__asm __volatile ("sync; mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch)
	    : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));
#endif /* PPC_OEA64 || PPC_OEA64_BRIDGE */

	restore_ofmap(ofmap, ofmaplen);

	/* CPU clock stuff */
	set_timebase();
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

void
restore_ofmap(struct ofw_translations *map, int len)
{
	int n = len / sizeof(struct ofw_translations);
	int i;

	pmap_pinit(&ofw_pmap);

#if defined(PPC_OEA64_BRIDGE)
	ofw_pmap.pm_sr[0x0] = KERNELN_SEGMENT(0);
#endif
	ofw_pmap.pm_sr[KERNEL_SR] = KERNEL_SEGMENT;

#ifdef KERNEL2_SR
	ofw_pmap.pm_sr[KERNEL2_SR] = KERNEL2_SEGMENT;
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
 * Scan the device tree for ranges, and batmap them.
 */

static u_int16_t
ranges_bitmap(int node, u_int16_t bitmap)
{
	int child, mlen, len, acells, scells, reclen, i, addr, j;
	u_int32_t map[160];

	for (child = OF_child(node); child; child = OF_peer(child)) {
		mlen = OF_getprop(child, "ranges", map, sizeof(map));
		if (mlen == -1)
			goto noranges;
		len = OF_getprop(child, "#address-cells", &acells,
		    sizeof(acells));
		if (len == -1)
			goto noranges;
		len = OF_getprop(child, "#size-cells", &scells,
		    sizeof(scells));
		if (len == -1)
			goto noranges;

		reclen = acells + 1 + scells;

		for (i=0; i < reclen/(mlen/4); i++) {
			addr = map[reclen * i + acells + 1];
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

void
ofwoea_batinit(void)
{
#if 0
	/* this is what macppc used to do */
        oea_batinit(0x80000000, BAT_BL_256M, 0xf0000000, BAT_BL_256M,
                    0x90000000, BAT_BL_256M, 0xa0000000, BAT_BL_256M,
                                0xb0000000, BAT_BL_256M, 0);
#else
        u_int16_t bitmap;
	int node, i;

	node = OF_finddevice("/");
	bitmap = ranges_bitmap(node, 0);
	oea_batinit(0);

	/* XXX this is a macppc-specific hack */
	bitmap = 0x8f00;
	for (i=1; i < 0x10; i++)
		if (bitmap & (1 << i))
			oea_iobat_add(0x10000000 * i, BAT_BL_256M);
#endif
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
static char ex_storage[EXSTORAGE_MAX][EXTENT_FIXED_STORAGE_SIZE(8)]
	__attribute__((aligned(8)));


static void
find_ranges(int base, rangemap_t *regions, int *cur, int type)
{
	int node, i, len, reclen;
	u_int32_t acells, scells, map[160];
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
	if (OF_getprop(node, "#address-cells", &acells,
	    sizeof(acells)) != sizeof(acells))
		acells = 1;
	if (OF_getprop(node, "#size-cells", &scells,
	    sizeof(scells)) != sizeof(scells))
		scells = 1;
	if (type == RANGE_TYPE_ISA)
		reclen = 6;
	else
		reclen = acells + scells + 1;
	printf("found a map reclen=%d cur=%d\n", reclen, *cur);
	switch (type) {
		case RANGE_TYPE_PCI:
		case RANGE_TYPE_FIRSTPCI:
			for (i=0; i < len/(4*reclen); i++) {
				regions[*cur].type = map[i*reclen] >> 24;
				regions[*cur].addr = map[i*reclen + acells];
				regions[*cur].size =
				    map[i*reclen + acells + scells];
				(*cur)++;
			}
			break;
		case RANGE_TYPE_ISA:
			for (i=0; i < len/(4*reclen); i++) {
				if (map[i*reclen] == 1)
					regions[*cur].type = RANGE_IO;
				else
					regions[*cur].type = RANGE_MEM;
				regions[*cur].size =
				    map[i*reclen + acells + scells];
				(*cur)++;
			}
			break;
	}
	return;
rec:
	for (node = OF_child(base); node; node = OF_peer(node))
		find_ranges(node, regions, cur, type);
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
	u_int32_t addr;
	rangemap_t region, holes[32], list[32];

	memset(list, 0, sizeof(list));
	cur = 0;
	if (rangetype == RANGE_TYPE_ISA || node == -1)
		node = OF_finddevice("/");
	if (rangetype == RANGE_TYPE_ISA) {
		u_int32_t size = 0;
		rangemap_t regions[32];

		find_ranges(node, list, &cur, RANGE_TYPE_FIRSTPCI);
		range = 0;
		find_ranges(node, regions, &range, RANGE_TYPE_ISA);
		if (range == 0 || cur == 0)
			return -1; /* no isa stuff found */
		for (i=0; i < range; i++)
			if (regions[i].type == iomem)
				size = regions[i].size;
		if (iomem == RANGE_IO) {
			/* the first io range is the one */
			for (i=0; i < cur; i++)
				if (list[i].type == RANGE_IO && size) {
					tag->pbs_offset = list[i].addr;
					tag->pbs_limit = size;
					error = bus_space_init(tag, name, NULL, 0);
					return error;
				}
		} else {
			for (i=0; i < cur; i++)
				if (list[i].type == RANGE_MEM &&
				    list[i].size == size) {
					tag->pbs_offset = list[i].addr;
					tag->pbs_limit = size;
					error = bus_space_init(tag, name, NULL, 0);
					return error;
				}
		}
		return -1; /* NO ISA FOUND */
	}
	find_ranges(node, list, &cur, rangetype);

	printf("cur == %d\n", cur);
	/* now list should contain a list of memory regions */
	for (i=0; i < cur; i++)
		printf("addr=0x%x size=0x%x type=%d\n", list[i].addr,
		    list[i].size, list[i].type);

	addr=0;
	range = find_lowest_range(list, cur, iomem);
	i = 0;
	nrofholes = 0;
	while (range != -1) {
		printf("range==%d\n", range);
		printf("i==%d\n", i);
		if (i == 0) {
			memcpy(&region, &list[range], sizeof(rangemap_t));
			list[range].addr = 0;
			i++;
			range = find_lowest_range(list, cur, iomem);
			continue;
		}
		if (region.addr + region.size < list[range].addr) {
			/* allocate a hole */
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
	printf("RANGE iomem=%d FOUND\n", iomem);
	printf("addr=0x%x size=0x%x type=%d\n", region.addr,
		    region.size, region.type);
	printf("HOLES FOUND\n");
	for (i=0; i < nrofholes; i++)
		printf("addr=0x%x size=0x%x type=%d\n", holes[i].addr,
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
			error =
			    extent_alloc_region(tag->pbs_extent,
				holes[i].addr, holes[i].size, EX_NOWAIT);
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
