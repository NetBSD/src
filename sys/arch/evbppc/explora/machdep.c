/*	$NetBSD: machdep.c,v 1.3 2003/07/15 01:37:36 lukem Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.3 2003/07/15 01:37:36 lukem Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>
#include <sys/properties.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/explora.h>
#include <machine/bus.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/dcr403cgx.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#define MEMREGIONS	2
#define TLB_PG_SIZE	(16*1024*1024)

char cpu_model[80];
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

extern struct user *proc0paddr;

struct propdb *board_info = NULL;
struct vm_map *phys_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *exec_map = NULL;
char msgbuf[MSGBUFSIZE];
paddr_t msgbuf_paddr;
static unsigned cpuspeed = 66000000;
static unsigned memsize;

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

static void
set_tlb(int idx, u_int addr, u_int flags)
{
	u_int lo, hi;

	addr &= ~(TLB_PG_SIZE-1);

	lo = addr | TLB_EX | TLB_WR | flags;
#ifdef PPC_4XX_NOCACHE
	lo |= TLB_I;
#endif
	hi = addr | TLB_VALID | TLB_PG_16M;

	asm volatile(
	    "	tlbwe %1,%0,1	\n"
	    "	tlbwe %2,%0,0	\n"
	    "	sync		\n"
	    : : "r" (idx), "r" (lo), "r" (hi) );
}

void
bootstrap(u_int startkernel, u_int endkernel)
{
	u_int i, j, t, br[4];
	u_int maddr, msize, size;
	struct cpu_info * const ci = &cpu_info[0];

	consinit();

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

	asm volatile(
	    "	mtpid %0	\n"
	    "	sync		\n"
	    : : "r" (1) );

	/*
	 * Setup initial tlbs.
	 * Physical memory is mapped into the first (reserved) tlbs.
	 * Memory of potential console devices is mapped into the
	 * last tlbs. They are only needed until the devices are configured.
	 */

	t = 0;
	for (maddr = 0; maddr < phys_mem[0].size; maddr += TLB_PG_SIZE)
		set_tlb(t++, maddr, 0);

	t = NTLB-1;
	set_tlb(t--, BASE_FB, TLB_I | TLB_G);
	set_tlb(t--, BASE_FB2, TLB_I | TLB_G);
	set_tlb(t--, BASE_COM, TLB_I | TLB_G);

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
	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

	/*
	 * Install trap vectors.
	 */

	for (i = EXC_RSVD; i <= EXC_LAST; i += 0x100)
		memcpy((void *)i, &defaulttrap, (size_t)&defaultsize);

	for (i = 0; i < sizeof(trap_table)/sizeof(trap_table[0]); i++) {
		memcpy((void *)trap_table[i].vector, trap_table[i].addr,
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
	asm volatile (
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

#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(0, NULL, NULL);
#endif

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
	asm volatile (
	    "	mfmsr %0	\n"
	    "	andi. %1,%0,%2	\n"
	    "	mtmsr %1	\n"
	    : "=r" (omsr), "=r" (msr) : "K" ((u_short)~PSL_EE) );
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	asm volatile (
	    "	mtmsr %0	\n"
	    : : "r" (omsr) );
}

void
cpu_startup(void)
{
	caddr_t v;
	vaddr_t minaddr, maxaddr;
	u_int sz, i, base, residual;
	char pbuf[9];

	/*
	 * Initialize error message buffer (before start of kernel)
	 */
	initmsgbuf((caddr_t)msgbuf, round_page(MSGBUFSIZE));

	printf("%s", version);
	printf("NCD Explora451\n");

	memsize = ctob(physmem);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (u_int)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	sz = MAXBSIZE * nbuf;
	minaddr = 0;
	if (uvm_map(kernel_map, (vaddr_t *)&minaddr, round_page(sz),
		NULL, UVM_UNKNOWN_OFFSET, 0,
		UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
			    UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	buffers = (char *)minaddr;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE) {
		/* Don't want to alloc more physical mem than ever needed */
		base = MAXBSIZE;
		residual = 0;
	}
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = NBPG * (i < residual ? base + 1 : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

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
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %u buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up the buffers.
	 */
	bufinit();

	/*
	 * Set up the board properties database.
	 */
	if (!(board_info = propdb_create("board info")))
		panic("Cannot create board info database");

	if (board_info_set("processor-frequency", &cpuspeed, 
	    sizeof(&cpuspeed), PROP_CONST, 0))
		panic("setting processor-frequency");
	if (board_info_set("mem-size", &memsize, 
	    sizeof(&memsize), PROP_CONST, 0))
		panic("setting mem-size");
}

int
lcsplx(int ipl)
{
	return spllower(ipl);	/*XXX*/
}

void
softnet(void)
{
	int isr;

	isr = netisr;
	netisr = 0;

#define DONETISR(bit, fn)		\
	do {				\
		if (isr & (1 << bit))	\
		fn();			\
	} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

#include "com.h"
void
softserial(void)
{
#if NCOM > 0
	void comsoft(void);	/* XXX from dev/ic/com.c */

	comsoft();
#endif
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
inittodr(time_t base)
{
	if (base > 365*24*60*60 && time.tv_sec < 365*24*60*60)
		time.tv_sec = base;
}

void
resettodr(void)
{
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{
	*mem = phys_mem;
	*avail = avail_mem;
}
