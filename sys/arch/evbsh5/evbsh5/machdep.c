/*	$NetBSD: machdep.c,v 1.14 2003/04/02 03:54:27 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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

#include "opt_sh5_debug.h"
#include "opt_sh5_cpu.h"
#include "dtfcons.h"
#include "sysfpga.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/param.h>
#include <machine/memregion.h>
#include <machine/bootparams.h>
#include <machine/cacheops.h>

#include <uvm/uvm_extern.h>

#include <sh5/pmap.h>

#include <sh5/sh5/stb1var.h>
#include <sh5/sh5/dtf_comms.h>

#include <sh5/dev/superhywayvar.h>
#include <sh5/dev/pbridgereg.h>
#include <sh5/dev/rtcreg.h>
#include <sh5/dev/rtcvar.h>

#include <evbsh5/dev/sysfpgavar.h>

#include <evbsh5/evbsh5/machdep.h>

#ifndef SH5_CPU_SPEED
static void compute_ctc_tick_per_us(void);
#endif

/*
 * A reasonable default set of boot parameters, in case we were booted
 * via jtag, or the simulator.
 */
struct boot_params bootparams = {
	/* bp_magic */
	BP_MAGIC,

	/* bp_version */
	BP_VERSION,

	/* bp_flags */
	RB_SINGLE | RB_KDB,

	/* bp_kseg0_phys */
	0xffffffff80000000ll,

	/* bp_machine */
	"Unknown machine (using \"Cayman\" defaults)",

	/* bp_cpu[1] */
	{
		{
			SH5_CPUID_STB1,		/* cpuid */
			0,			/* version */
			0x0d,			/* pport */
#ifdef SH5_CPU_SPEED
			SH5_CPU_SPEED*1000*1000,/* speed */
#else
			256000000,
#endif
		}
	},

	/* bp_mem[4] */
	{
		{
			BP_MEM_TYPE_SDRAM,	/* type */
			0xff,			/* pport */
			0xfffffff80000000ll,	/* physstart */
			0x00800000ll		/* physsize */
		},
		{ BP_MEM_TYPE_UNUSED },
		{ BP_MEM_TYPE_UNUSED },
		{ BP_MEM_TYPE_UNUSED }
	},

	/* bp_bootdev */
	{
		"/mainbus0/superhyway0,0x8/femi0,0x4000000/sysfpga0,0x1000/sm0",
		"netbsd",
		0
	},

	/* bp_consdev */
	{
		"/mainbus0/superhyway0,0x9/pbridge0,0x1030000/scif0",
		38400
	}
};

struct vm_map *exec_map;
struct vm_map *mb_map;
struct vm_map *phys_map;

char machine[] = MACHINE;

char cpu_model[128];

/*
 * Physical addresses of important devices on the Cayman board
 * which need to be mapped early on during bootstrap to gain
 * access to configuration information.
 */
#define EVBSH5_SYSFPGA_PHYS_ADDR	0x04000000
#define EVBSH5_SYSFPGA_LEN		0x00004000
#define EVBSH5_PBRIDGE_PHYS_ADDR	\
	    (SUPERHYWAY_PPORT_TO_BUSADDR(0x09) + PBRIDGE_OFFSET_BASE)
#define EVBSH5_PBRIDGE_LEN		PBRIDGE_REG_SZ

/*
 * Handles through which the above device regions can be accessed.
 */
bus_space_handle_t _evbsh5_bh_pbridge;
bus_space_handle_t _evbsh5_bh_sysfpga;

static struct mem_region mr[BP_N_MEMBLOCKS + 1];

void	*symbol_table;
long	symbol_table_size;


void
evbsh5_init(void *symtab, vaddr_t endkernel)
{
#if NDTFCONS > 0
	extern char *_dtf_buffer;
	extern void _dtf_trap_frob(void);
	vaddr_t dtfbuf;
	paddr_t frob_p;
#endif
	struct boot_params *bp;
	u_long ksize;
	vsize_t size;
	caddr_t v;
	paddr_t kseg0_phys;
	int i, j;

	bp = (struct boot_params *)endkernel;
	if (bp->bp_magic == BP_MAGIC && bp->bp_version == BP_VERSION)
		bootparams = *bp;
	else
		symtab = (void *)endkernel;	/* Assume no symbol table */

	bp = &bootparams;
	kseg0_phys = (paddr_t)bp->bp_kseg0_phys;
	symbol_table = symtab;
	symbol_table_size = (long)endkernel - (long)symtab;

	cpu_identify();

	endkernel = sh5_round_page(endkernel);
	ksize = sh5_round_page(endkernel - SH5_KSEG0_BASE);

	for (i = j = 0; i < BP_N_MEMBLOCKS; i++) {
		paddr_t pa;
		psize_t ps;

		if (bp->bp_mem[i].type != BP_MEM_TYPE_SDRAM)
			continue;

		pa = (paddr_t) bp->bp_mem[i].physstart;
		ps = (psize_t) bp->bp_mem[i].physsize;

		if (pa == kseg0_phys) {
			pa += ksize;
			ps -= ksize;
		}

		mr[j].mr_start = pa;
		mr[j].mr_size = ps;

		if (pa >= kseg0_phys &&
		    (pa + ps) < (kseg0_phys + SH5_KSEG0_SIZE))
			mr[j].mr_kvastart = SH5_KSEG0_BASE + (pa - kseg0_phys);
		else
			mr[j].mr_kvastart = SH5_KSEG1_SIZE;
		j++;
	}

	mr[j].mr_start = 0;
	mr[j].mr_size = 0;

	pmap_bootstrap(endkernel, kseg0_phys, mr);

#if NDTFCONS > 0
	dtfbuf = (vaddr_t) &_dtf_buffer;
	frob_p = (paddr_t) (uintptr_t) _dtf_trap_frob;
	frob_p = kseg0_phys + (frob_p - SH5_KSEG0_BASE);

	dtf_init(0xc100018, frob_p,
	    (paddr_t)(kseg0_phys + (dtfbuf - SH5_KSEG0_BASE)), dtfbuf);
#endif

	/*
	 * Map the on-chip devices behind the peripheral bridge
	 */
	bus_space_map(&_sh5_bus_space_tag, EVBSH5_PBRIDGE_PHYS_ADDR,
	    EVBSH5_PBRIDGE_LEN, 0, &_evbsh5_bh_pbridge);

	/*
	 * Map the system FPGA/Super IO area
	 */
	bus_space_map(&_sh5_bus_space_tag, EVBSH5_SYSFPGA_PHYS_ADDR,
	    EVBSH5_SYSFPGA_LEN, 0, &_evbsh5_bh_sysfpga);

#ifndef SH5_CPU_SPEED
	compute_ctc_tick_per_us();
#else
	_sh5_ctc_ticks_per_us = SH5_CPU_SPEED;
#endif

	boothowto = bp->bp_flags;

	/*
	 * Call allocsys() now so we can steal pages from KSEG0.
	 */
	size = (vsize_t)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_pageboot_alloc(round_page(size))) == 0)
		panic("startup: no room for tables");
	if ((allocsys(v, NULL) - v) != size)
		panic("startup: table size inconsistency");
}

#ifndef SH5_CPU_SPEED
static void
compute_ctc_tick_per_us(void)
{
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	register_t ctcstart, ctcstop;
	u_int8_t r64cnt;

	bt = &_sh5_bus_space_tag;

	/*
	 * Map the RTC's registers
	 */
	bus_space_subregion(bt, _evbsh5_bh_pbridge, PBRIDGE_OFFSET_RTC,
	    RTC_REG_SIZE, &bh);

	bus_space_write_1(bt, bh, RTC_REG_RCR1, 0);
	bus_space_write_1(bt, bh, RTC_REG_RCR2, RTC_RCR2_START|RTC_RCR2_RTCEN);

	/*
	 * Set the cpu cycle counter to a reasonably high value such that
	 * it won't wrap around in the loop
	 */
	ctcstart = 0xffffffff;

	/*
	 * Fetch the current value of the 128Hz RTC counter and
	 * add 16 so we can time the loop to pretty much exactly 125mS
	 */
	r64cnt = rtc_read_r64cnt(bt, bh);
	while (r64cnt == rtc_read_r64cnt(bt, bh))
		;

	__asm __volatile("putcon %0, ctc" :: "r"(ctcstart));

	r64cnt = (r64cnt + 17) & RTC_R64CNT_MASK;

	/*
	 * Wait 125mS
	 */
	while (rtc_read_r64cnt(bt, bh) != r64cnt)
		;

	__asm __volatile("getcon ctc, %0" : "=r"(ctcstop));

	/*
	 * Compute the number of CTC ticks per micro-second, for use
	 * in the delay() loop.
	 */
	_sh5_ctc_ticks_per_us = (ctcstart - ctcstop) / 125000;

	bus_space_unmap(bt, bh, RTC_REG_SIZE);
}
#endif

void
cpu_startup(void)
{
	u_int i, base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[16];

	/*
	 * Now allocate buffers proper.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (void *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = PAGE_SIZE * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				      "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(pmap_kernel());

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

	strcpy(cpu_model, bootparams.bp_machine);
	printf("%s%s, %d-bit mode\n", version, cpu_model,
	    (sizeof(void *) == 8) ? 64 : 32);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * PAGE_SIZE);
	printf("using %u buffers containing %s bytes of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

int
cpu_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	return (EOPNOTSUPP);
}

void
cpu_reboot(int how, char *bootstr)
{

	sh5_reboot(how, bootstr);

#if NSYSFPGA > 0
	sysfpga_sreset();
#endif

	for (;;)
		;
}

void
sh5_nmi_clear(void)
{
#if NSYSFPGA > 0
	sysfpga_nmi_clear();
#endif
}
