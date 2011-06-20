/*	$NetBSD: machdep.c,v 1.36 2011/06/20 17:44:33 matt Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.36 2011/06/20 17:44:33 matt Exp $");

#include "opt_explora.h"
#include "opt_modular.h"
#include "ksyms.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <machine/explora.h>
#include <machine/powerpc.h>
#include <machine/tlb.h>
#include <machine/pcb.h>
#include <machine/trap.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr403cgx.h>

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#define MEMREGIONS	2
#define TLB_PG_SIZE	(16*1024*1024)

char cpu_model[80];
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

static const unsigned int cpuspeed = 66000000;

prop_dictionary_t board_properties;
struct vm_map *phys_map = NULL;
char msgbuf[MSGBUFSIZE];
paddr_t msgbuf_paddr;

static struct mem_region phys_mem[MEMREGIONS];
static struct mem_region avail_mem[MEMREGIONS];

void		initppc(vaddr_t, vaddr_t);

void
initppc(vaddr_t startkernel, vaddr_t endkernel)
{
	u_int i, j, t, br[4];
	u_int maddr, msize, size;

	br[0] = mfdcr(DCR_BR4);
	br[1] = mfdcr(DCR_BR5);
	br[2] = mfdcr(DCR_BR6);
	br[3] = mfdcr(DCR_BR7);

	for (i = 0; i < 4; i++)
		for (j = i+1; j < 4; j++)
			if (br[j] < br[i])
				t = br[j], br[j] = br[i], br[i] = t;

	for (i = 0, size = 0; i < 4; i++) {
		if (((br[i] >> 19) & 3) != 3)
			continue;
		maddr = ((br[i] >> 24) & 0xff) << 20;
		msize = 1 << (20 + ((br[i] >> 21) & 7));
		if (maddr+msize > size)
			size = maddr+msize;
	}

	phys_mem[0].start = 0;
	phys_mem[0].size = size & ~PGOFSET;
	avail_mem[0].start = startkernel;
	avail_mem[0].size = size-startkernel;

	__asm volatile(
	    "	mtpid %0	\n"
	    "	sync		\n"
	    : : "r" (KERNEL_PID) );

	/*
	 * Setup initial tlbs.
	 * Kernel memory and console device are
	 * mapped into the first (reserved) tlbs.
	 */

	for (maddr = 0; maddr < endkernel; maddr += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(maddr, maddr, TLB_PG_SIZE, TLB_EX);

	/* Map PCKBC, PCKBC2, COM, LPT. This is far beyond physmem. */
	ppc4xx_tlb_reserve(BASE_ISA, BASE_ISA, TLB_PG_SIZE, TLB_I | TLB_G);

#ifndef COM_IS_CONSOLE
	ppc4xx_tlb_reserve(BASE_FB,  BASE_FB,  TLB_PG_SIZE, TLB_I | TLB_G);
	ppc4xx_tlb_reserve(BASE_FB2, BASE_FB2, TLB_PG_SIZE, TLB_I | TLB_G);
#endif

	/* Disable all external interrupts */
	mtdcr(DCR_EXIER, 0);

	/* Disable all timer interrupts */
	mtspr(SPR_TCR, 0);

	ibm4xx_init(startkernel, endkernel, pic_ext_intr);
}

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	prop_number_t pn;
	char pbuf[9];

	/*
	 * Initialize error message buffer (before start of kernel)
	 */
	initmsgbuf((void *)msgbuf, round_page(MSGBUFSIZE));

	printf("%s%s", copyright, version);
	printf("NCD Explora451\n");

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Set up the board properties database.
	 */
	board_properties = prop_dictionary_create();
	KASSERT(board_properties != NULL);

	pn = prop_number_create_integer(ctob(physmem));
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

	pn = prop_number_create_integer(cpuspeed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
				pn) == false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	intr_init();
	
	/*
	 * Look for the ibm4xx modules in the right place.
	 */
	module_machine = module_machine_ibm4xx;
	fake_mapiodev = 0;
}

void
cpu_reboot(int howto, char *what)
{
	static int syncing = 0;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();
		resettodr();
	}

	splhigh();

	if (!cold && (howto & RB_DUMP))
		/*XXX dumpsys()*/;

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if (howto & RB_HALT) {
		printf("halted\n\n");

		while (1)
			;
	}

	printf("rebooting\n\n");

	/* flush cache for msgbuf */
	__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

	ppc4xx_reset();

#ifdef DDB
	while (1)
		Debugger();
#else
	while (1)
		;
#endif
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{
	*mem = phys_mem;
	*avail = avail_mem;
}
