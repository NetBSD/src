/*	$NetBSD: machdep.c,v 1.26.4.1 2009/05/13 17:16:40 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.26.4.1 2009/05/13 17:16:40 jym Exp $");

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
#include <sys/user.h>
#include <sys/reboot.h>
#include <sys/ksyms.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <machine/explora.h>
#include <machine/bus.h>
#include <machine/powerpc.h>
#include <machine/tlb.h>
#include <machine/trap.h>

#include <powerpc/spr.h>
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

extern struct user *proc0paddr;

prop_dictionary_t board_properties;
struct vm_map *phys_map = NULL;
struct vm_map *mb_map = NULL;
char msgbuf[MSGBUFSIZE];
paddr_t msgbuf_paddr;

static struct mem_region phys_mem[MEMREGIONS];
static struct mem_region avail_mem[MEMREGIONS];

void		bootstrap(u_int, u_int);
static void	install_extint(void (*)(void));
int		lcsplx(int);

/*
 * Trap vectors
 */
extern int defaulttrap, defaultsize;
extern int sctrap, scsize;
extern int alitrap, alisize;
extern int dsitrap, dsisize;
extern int isitrap, isisize;
extern int mchktrap, mchksize;
extern int tlbimiss4xx, tlbim4size;
extern int tlbdmiss4xx, tlbdm4size;
extern int pitfitwdog, pitfitwdogsize;
extern int debugtrap, debugsize;
extern int errata51handler, errata51size;
#ifdef DDB
extern int ddblow, ddbsize;
#endif
static struct {
	int vector;
	void *addr;
	void *size;
} trap_table[] = {
	{ EXC_SC,	&sctrap,	&scsize },
	{ EXC_ALI,	&alitrap,	&alisize },
	{ EXC_DSI,	&dsitrap,	&dsisize },
	{ EXC_ISI,	&isitrap,	&isisize },
	{ EXC_MCHK,	&mchktrap,	&mchksize },
	{ EXC_ITMISS,	&tlbimiss4xx,	&tlbim4size },
	{ EXC_DTMISS,	&tlbdmiss4xx,	&tlbdm4size },
	{ EXC_PIT,	&pitfitwdog,	&pitfitwdogsize },
	{ EXC_DEBUG,	&debugtrap,	&debugsize },
	{ (EXC_DTMISS|EXC_ALI), &errata51handler, &errata51size },
#if defined(DDB)
	{ EXC_PGM,	&ddblow,	&ddbsize },
#endif /* DDB */
};

/*
 * Install a trap vector. We cannot use memcpy because the
 * destination may be zero.
 */
static void
trap_copy(void *src, int dest, size_t len)
{
	uint32_t *src_p = src;
	uint32_t *dest_p = (void *)dest;

	while (len > 0) {
		*dest_p++ = *src_p++;
		len -= sizeof(uint32_t);
	}
}

void
bootstrap(u_int startkernel, u_int endkernel)
{
	u_int i, j, t, br[4];
	u_int maddr, msize, size;
	struct cpu_info * const ci = &cpu_info[0];

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

	consinit();

	/* Disable all external interrupts */
	mtdcr(DCR_EXIER, 0);

	/* Disable all timer interrupts */
	mtspr(SPR_TCR, 0);

	/* Initialize cache info for memcpy, etc. */
	cpu_probe_cache();

	/*
	 * Initialize lwp0 and current pcb and pmap pointers.
	 */
	lwp0.l_cpu = ci;
	lwp0.l_addr = proc0paddr;
	memset(lwp0.l_addr, 0, sizeof *lwp0.l_addr);

	curpcb = &proc0paddr->u_pcb;
	curpcb->pcb_pm = pmap_kernel();

	/*
	 * Install trap vectors.
	 */

	for (i = EXC_RSVD; i <= EXC_LAST; i += 0x100)
		trap_copy(&defaulttrap, i, (size_t)&defaultsize);

	for (i = 0; i < sizeof(trap_table)/sizeof(trap_table[0]); i++) {
		trap_copy(trap_table[i].addr, trap_table[i].vector,
		    (size_t)trap_table[i].size);
	}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

	/*
	 * Set Exception vector base.
	 * Handle trap instruction as PGM exception.
	 */

	mtspr(SPR_EVPR, 0);

	t = mfspr(SPR_DBCR0);
	t &= ~DBCR0_TDE;
	mtspr(SPR_DBCR0, t);

	/*
	 * External interrupt handler install.
	 */

	install_extint(ext_intr);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	__asm volatile (
	    "	mfmsr %0	\n"
	    "	ori %0,%0,%1	\n"
	    "	mtmsr %0	\n"
	    "	sync		\n"
	    : : "r" (0), "K" (PSL_IR|PSL_DR|PSL_ME) );

	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);
	fake_mapiodev = 0;
}

static void
install_extint(void (*handler)(void))
{
	extern int extint, extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int omsr, msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif
	__asm volatile (
	    "	mfmsr %0	\n"
	    "	andi. %1,%0,%2	\n"
	    "	mtmsr %1	\n"
	    : "=r" (omsr), "=r" (msr) : "K" ((u_short)~PSL_EE) );
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	__asm volatile (
	    "	mtmsr %0	\n"
	    : : "r" (omsr) );
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
}

int
lcsplx(int ipl)
{
	return spllower(ipl);	/*XXX*/
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
