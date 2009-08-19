/* $NetBSD: machdep.c,v 1.32.44.2 2009/08/19 18:45:56 yamt Exp $ */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *      This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.32.44.2 2009/08/19 18:45:56 yamt Exp $");

#include "opt_ddb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/powerpc.h>
#include <machine/stdarg.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pic/picvar.h>

#include <dev/cons.h>

#include <amiga/amiga/cc.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/isr.h>
#include <amiga/amiga/memlist.h>
#include <amigappc/amigappc/p5reg.h>

#include "fd.h"
#include "ser.h"

/* prototypes */
void show_me_regs(void);

extern void setup_amiga_intr(void);
#if NSER > 0
extern void ser_outintr(void);
extern void serintr(void);
#endif
#if NFD > 0
extern void fdintr(int);
#endif

/* PIC interrupt handler type */
typedef int (*ih_t)(void *);

/*
 * patched by some devices at attach time (currently, only the coms)
 */
int amiga_serialspl = 4;

/*
 * current open serial device speed;  used by some SCSI drivers to reduce
 * DMA transfer lengths.
 */
int ser_open_speed;

#define AMIGAMEMREGIONS 8
static struct mem_region physmemr[AMIGAMEMREGIONS], availmemr[AMIGAMEMREGIONS];
char cpu_model[80];

/* interrupt handler chains for level2 and level6 interrupt */
struct isr *isr_ports;
struct isr *isr_exter;

extern void *startsym, *endsym;

void
add_isr(struct isr *isr)
{
	struct isr **p, *q;

	p = isr->isr_ipl == 2 ? &isr_ports : &isr_exter;

	while ((q = *p) != NULL)
		p = &q->isr_forw;
	isr->isr_forw = NULL;
	*p = isr;
#if 0 /* XXX always enabled */
	/* enable interrupt */
	custom.intena = isr->isr_ipl == 2 ?
	    INTF_SETCLR | INTF_PORTS :
	    INTF_SETCLR | INTF_EXTER;
#endif
}

void
remove_isr(struct isr *isr)
{
	struct isr **p, *q;

	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;

	while ((q = *p) != NULL && q != isr)
		p = &q->isr_forw;
	if (q)
		*p = q->isr_forw;
	else
		panic("remove_isr: handler not registered");
#if 0 /* XXX always enabled, why disable? */
	/* disable interrupt if no more handlers */
	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;
	if (*p == NULL) {
		custom.intena = isr->isr_ipl == 6 ?
		    INTF_EXTER : INTF_PORTS;
	}
#endif
}

static void
ports_intr(struct isr **p)
{
	struct isr *q;

	while ((q = *p) != NULL) {
		if ((q->isr_intr)(q->isr_arg))
			break;
		p = &q->isr_forw;
	}
	if (q == NULL)
		ciaa_intr();  /* ciaa handles keyboard and parallel port */
}

static void
exter_intr(struct isr **p)
{
	struct isr *q;

	while ((q = *p) != NULL) {
		if ((q->isr_intr)(q->isr_arg))
			break;
		p = &q->isr_forw;
	}
	/*
	 * XXX ciab_intr() is not needed, neither the timers nor the
	 * floppy disk FGL interrupt
	 */
}

static void
amigappc_identify(void)
{
	extern u_long ticks_per_sec, ns_per_tick;
	static const char pll603[] = {10, 10, 10, 10, 20, 20, 25, 0,
			30, 0, 40, 0, 15,0, 35, 0};
	static const char pll604[] = {10, 10, 70, 10, 20, 65, 25, 45,
			30, 55, 40, 50, 15, 60, 35, 0};
	const char *mach, *pup, *cpuname;
	const char *p5type_p = (const char *)0xf00010;
	register int pvr, hid1;
	int cpu = 604;
	int cpuclock, busclock;

	/*
	 * PVR holds the CPU-type and version while
         * HID1 holds the PLL configuration
	 */
	__asm ("mfpvr %0; mfspr %1,1009" : "=r"(pvr), "=r"(hid1));

	/* Amiga types which can run a PPC board */
	if (is_a4000()) {
		mach = "Amiga 4000";
	}
	else if (is_a3000()) {
		mach = "Amiga 3000";
	}
	else {
		mach = "Amiga 1200";
	}

	/* find CPU type - BlizzardPPC has 603e, CyberstormPPC has 604e */
	switch (pvr >> 16) {
	case 3:
		cpuname = "603";
		cpu = 603;
		break;
	case 4:
		cpuname = "604";
		cpu = 604;
		break;
	case 6:
		cpuname = "603e";
		cpu = 603;
		break;
	case 7:
		cpuname = "603e+";
		cpu = 603;
		break;
	case 9:
	case 10:
		cpuname = "604e";
		cpu = 604;
		break;
	default:
		cpuname = "unknown";
		break;
	}

	switch (p5type_p[0]) {
	case 'D':
		pup = "[PowerUP]";
		break;
	case 'E':
		pup = "[CSPPC]";
		break;
	case 'F':
		pup = "[CS Mk.III]";
		break;
	case 'I':
		pup = "[BlizzardPPC]";
		break;
	default:
		pup = "";
		break;
	}

	/* XXX busclock can be measured with CIA-timers */
	switch (p5type_p[1]) {
	case 'A':
		busclock = 60000000;
		break;
	/* case B, C, D */
	default:
		busclock = 66666667;
		break;
	}

	/*
	 * compute cpuclock based on PLL configuration
	 */
	if (cpu == 603)
		cpuclock = busclock * pll603[hid1>>28 & 0xf] / 10;
	else /* 604 */
		cpuclock = busclock * pll604[hid1>>28 & 0xf] / 10;

	snprintf(cpu_model, sizeof(cpu_model),
	    "%s %s (%s v%d.%d %d MHz, busclk %d MHz)", mach, pup, cpuname,
	    pvr>>8 & 0xff, pvr & 0xff, cpuclock/1000000, busclock/1000000);

	/*
	 * set timebase
	 */
	ticks_per_sec = busclock / 4;
	ns_per_tick = 1000000000 / ticks_per_sec;
}

static void
amigappc_reboot(void)
{

	/*
	 * reboot CSPPC/BPPC
	 */
	if (!is_a1200()) {
		P5write(P5_REG_LOCK,0x60);
		P5write(P5_REG_LOCK,0x50);
		P5write(P5_REG_LOCK,0x30);
		P5write(P5_REG_SHADOW,P5_SET_CLEAR|P5_SHADOW);
		P5write(P5_REG_LOCK,0x00);
	}
	else
		P5write(P5_BPPC_MAGIC,0x00);

	P5write(P5_REG_LOCK,0x60);
	P5write(P5_REG_LOCK,0x50);
	P5write(P5_REG_LOCK,0x30);
	P5write(P5_REG_SHADOW,P5_SELF_RESET);
	P5write(P5_REG_RESET,P5_AMIGA_RESET);
}

static void
amigappc_install_handlers(void)
{

#if NSER > 0
	intr_establish(INTB_TBE, IST_LEVEL, IPL_SOFTCLOCK, (ih_t)ser_outintr, NULL);
	intr_establish(INTB_RBF, IST_LEVEL, IPL_SERIAL, (ih_t)serintr, NULL);
#endif

#if NFD > 0
	intr_establish(INTB_DSKBLK, IST_LEVEL, IPL_SOFTCLOCK, (ih_t)fdintr, 0);
#endif

	intr_establish(INTB_PORTS, IST_LEVEL, IPL_SOFTCLOCK, (ih_t)ports_intr,
	    &isr_ports);

	intr_establish(INTB_BLIT, IST_LEVEL, IPL_BIO, (ih_t)blitter_handler,
	    NULL);
	intr_establish(INTB_COPER, IST_LEVEL, IPL_BIO, (ih_t)copper_handler,
	    NULL);
	intr_establish(INTB_VERTB, IST_LEVEL, IPL_BIO, (ih_t)vbl_handler,
	    NULL);

	intr_establish(INTB_AUD0, IST_LEVEL, IPL_VM, (ih_t)audio_handler,
	    NULL);
	intr_establish(INTB_AUD1, IST_LEVEL, IPL_VM, (ih_t)audio_handler,
	    NULL);
	intr_establish(INTB_AUD2, IST_LEVEL, IPL_VM, (ih_t)audio_handler,
	    NULL);
	intr_establish(INTB_AUD3, IST_LEVEL, IPL_VM, (ih_t)audio_handler,
	    NULL);

	intr_establish(INTB_EXTER, IST_LEVEL, IPL_CLOCK, (ih_t)exter_intr,
	    &isr_exter);
}

static void
amigappc_bat_add(paddr_t pa, register_t len, register_t prot)
{
	static int ni = 0, nd = 0;
	const u_int i = pa >> 28;

	battable[i].batl = BATL(pa, prot, BAT_PP_RW);
	battable[i].batu = BATU(pa, len, BAT_Vs);

	/*
	 * Let's start loading the BAT registers.
	 */
	if (!(prot & (BAT_I|BAT_G))) {
		switch (ni) {
		case 0:
			__asm volatile ("isync");
			__asm volatile ("mtibatl 0,%0; mtibatu 0,%1;"
			    ::	"r"(battable[i].batl),
				"r"(battable[i].batu));
			__asm volatile ("isync");
			ni = 1;
			break;
		case 1:
			__asm volatile ("isync");
			__asm volatile ("mtibatl 1,%0; mtibatu 1,%1;"
			    ::	"r"(battable[i].batl),
				"r"(battable[i].batu));
			__asm volatile ("isync");
			ni = 2;
			break;
		case 2:
			__asm volatile ("isync");
			__asm volatile ("mtibatl 2,%0; mtibatu 2,%1;"
			    ::	"r"(battable[i].batl),
				"r"(battable[i].batu));
			__asm volatile ("isync");
			ni = 3;
			break;
		case 3:
			__asm volatile ("isync");
			__asm volatile ("mtibatl 3,%0; mtibatu 3,%1;"
			    ::	"r"(battable[i].batl),
				"r"(battable[i].batu));
			__asm volatile ("isync");
			ni = 4;
			break;
		default:
			break;
		}
	}
	switch (nd) {
	case 0:
		__asm volatile ("isync");
		__asm volatile ("mtdbatl 0,%0; mtdbatu 0,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		__asm volatile ("isync");
		nd = 1;
		break;
	case 1:
		__asm volatile ("isync");
		__asm volatile ("mtdbatl 1,%0; mtdbatu 1,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		__asm volatile ("isync");
		nd = 2;
		break;
	case 2:
		__asm volatile ("isync");
		__asm volatile ("mtdbatl 2,%0; mtdbatu 2,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		__asm volatile ("isync");
		nd = 3;
		break;
	case 3:
		__asm volatile ("isync");
		__asm volatile ("mtdbatl 3,%0; mtdbatu 3,%1;"
		    ::	"r"(battable[i].batl),
			"r"(battable[i].batu));
		__asm volatile ("isync");
		nd = 4;
		break;
	default:
		break;
	}
}

static void
amigappc_batinit(paddr_t pa, ...)
{
	va_list ap;
	register_t msr = mfmsr();

	/*
	 * Set BAT registers to unmapped to avoid overlapping mappings below.
	 */
	if ((msr & (PSL_IR|PSL_DR)) == 0) {
		__asm volatile ("isync");
		__asm volatile ("mtibatu 0,%0" :: "r"(0));
		__asm volatile ("mtibatu 1,%0" :: "r"(0));
		__asm volatile ("mtibatu 2,%0" :: "r"(0));
		__asm volatile ("mtibatu 3,%0" :: "r"(0));
		__asm volatile ("mtdbatu 0,%0" :: "r"(0));
		__asm volatile ("mtdbatu 1,%0" :: "r"(0));
		__asm volatile ("mtdbatu 2,%0" :: "r"(0));
		__asm volatile ("mtdbatu 3,%0" :: "r"(0));
		__asm volatile ("isync");
	}

	/*
	 * Setup BATs
	 */
	va_start(ap, pa);
	while (pa != ~0) {
		register_t len = va_arg(ap, register_t);
		register_t prot = va_arg(ap, register_t);

		amigappc_bat_add(pa, len, prot);
		pa = va_arg(ap, paddr_t);
	}
	va_end(ap);
}

#if 0
/*
 * customized oea_startup(), supports up to 64k msgbuf at 0xfff70000
 */
static void
amigappc_startup(const char *model)
{
	uintptr_t sz;
	void *v;
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	KASSERT(curcpu() != NULL);
	KASSERT(lwp0.l_cpu != NULL);
	KASSERT(curcpu()->ci_intstk != 0);
	KASSERT(curcpu()->ci_intrdepth == -1);

        sz = round_page(MSGBUFSIZE);
	v = (void *)0xfff70000;
	initmsgbuf(v, sz);

	printf("%s%s", copyright, version);
	if (model != NULL)
		printf("Model: %s\n", model);
	cpu_identify(NULL, 0);

	format_bytes(pbuf, sizeof(pbuf), ctob((u_int)physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate away the pages that map to 0xDEA[CDE]xxxx.  Do this after
	 * the bufpages are allocated in case they overlap since it's not
	 * fatal if we can't allocate these.
	 */
	if (KERNEL_SR == 13 || KERNEL2_SR == 14) {
		int error;
		minaddr = 0xDEAC0000;
		error = uvm_map(kernel_map, &minaddr, 0x30000,
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,  
				UVM_ADV_NORMAL, UVM_FLAG_FIXED));
		if (error != 0 || minaddr != 0xDEAC0000)
			printf("oea_startup: failed to allocate DEAD "
			    "ZONE: error=%d\n", error);
	}
 
	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

#ifndef PMAP_MAP_POOLPAGE
	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    mclbytes*nmbclusters, VM_MAP_INTRSAFE, FALSE, NULL);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}
#endif

void
initppc(u_int startkernel, u_int endkernel)
{
	struct boot_memseg *ms;
	u_int i, r;

	/*
	 * amigappc memory region set
	 */
	ms = &memlist->m_seg[0];
	for (i=0, r=0; i<memlist->m_nseg; i++, ms++) {
		if (ms->ms_attrib & MEMF_FAST) {
			/*
			 * XXX Only recognize the memory segment in which
			 * the kernel resides, for now
			 */
			if (ms->ms_start <= startkernel &&
			    ms->ms_start + ms->ms_size > endkernel) {
				physmemr[r].start = (paddr_t)ms->ms_start;
				physmemr[r].size = (psize_t)ms->ms_size & ~PGOFSET;
				availmemr[r].start = (endkernel + PGOFSET) & ~PGOFSET;
				availmemr[r].size  = (physmemr[0].start +
						      physmemr[0].size)
						     - availmemr[0].start;
			}
			r++;
		}
	}
	physmemr[r].start = 0;
	physmemr[r].size = 0;
	availmemr[r].start = 0;
	availmemr[r].size = 0;
	if (r == 0)
		panic("initppc: no suitable memory segment found");

	/*
	 * Initialize BAT tables.
	 */
	if (!is_a1200()) {
		/* A3000 or A4000 */
		amigappc_batinit(0x00000000, BAT_BL_16M, BAT_I|BAT_G,
		    0x08000000, BAT_BL_128M, 0,
		    0xfff00000, BAT_BL_512K, 0,
		    0x40000000, BAT_BL_256M, BAT_I|BAT_G,
		    ~0);
	}
	else {
		panic("A1200 BPPC batinit?");
	}

	/*
	 * Set up trap vectors and interrupt handler
	 */
	oea_init(NULL);

	/* XXX bus_space_init() not needed here */

	/*
	 * Set the page size
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module
	 */
	pmap_bootstrap(startkernel, endkernel);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif

	/*
	 * CPU model, bus clock, timebase
	 */
	amigappc_identify();

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * This is called during initppc, before the system is really initialized.
 * It shall provide the total and the available regions of RAM.
 * Both lists must have a zero-size entry as terminator.
 * The available regions need not take the kernel into account, but needs
 * to provide space for two additional entry beyond the terminating one.
 */
void
mem_regions(struct mem_region **memp, struct mem_region **availp)
{

	*memp = physmemr;
	*availp = availmemr;
}

/*
 * Machine dependent startup code
 */
void
cpu_startup(void)
{
	int msr;

	/*
	 * hello world
	 */
	oea_startup(cpu_model);

	/* Setup interrupts */
	pic_init();
	setup_amiga_intr();
	amigappc_install_handlers();

	oea_install_extint(pic_ext_intr);

	/*
	 * Now allow hardware interrupts.
	 */
	splhigh();
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
	    :	"=r"(msr)
	    :	"K"(PSL_EE));

#if 0 /* XXX Not in amiga bus.h - fix it? */
	/*
	 * Now that we have VM, malloc's are OK in bus_space.
	 */
	bus_space_mallocok();
#endif
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{

	custom_chips_init();
	/*
	** Initialize the console before we print anything out.
	*/
	cninit();
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;

	if (cold) {
		howto |= RB_HALT;
		goto halt_sys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && syncing == 0) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

	/* Disable intr */
	splhigh();

	/* Do dump if requested */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		oea_dumpsys();

halt_sys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n\n");
	delay(1000000);
	amigappc_reboot();

	for (;;);
	/* NOTREACHED */
}

/*
 * Try to emulate the functionality from m68k/m68k/sys_machdep.c
 * used by several amiga drivers.
 */
int
dma_cachectl(void *addr, int len)
{
#if 0 /* XXX */
	paddr_t pa, end;
	int inc = curcpu()->ci_ci.dcache_line_size;

	pa = kvtop(addr);
	for (end = pa + len; pa < end; pa += inc)
		__asm volatile("dcbf 0,%0" :: "r"(pa));
	__asm volatile("sync");

	pa = kvtop(addr);
	for (end = pa + len; pa < end; pa += inc)
		__asm volatile("icbi 0,%0" :: "r"(pa));
	__asm volatile("isync");

#endif
	return 0;
}

/* show PPC registers */
void show_me_regs(void)
{
	register u_long	scr0, scr1, scr2, scr3;

	__asm volatile ("mfmsr %0" : "=r"(scr0) :);
	printf("MSR %08lx\n", scr0);

	__asm volatile ("mfspr %0,1; mfspr %1,8; mfspr %2,9; mfspr %3,18"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("XER   %08lx\tLR    %08lx\tCTR   %08lx\tDSISR %08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,19; mfspr %1,22; mfspr %2,25; mfspr %3,26"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("DAR   %08lx\tDEC   %08lx\tSDR1  %08lx\tSRR0  %08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,27; mfspr %1,268; mfspr %2,269; mfspr %3,272"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("SRR1  %08lx\tTBL   %08lx\tTBU   %08lx\tSPRG0 %08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,273; mfspr %1,274; mfspr %2,275; mfspr %3,282"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("SPRG1 %08lx\tSPRG2 %08lx\tSPRG3 %08lx\tEAR   %08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,528; mfspr %1,529; mfspr %2,530; mfspr %3,531"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("IBAT0U%08lx\tIBAT0L%08lx\tIBAT1U%08lx\tIBAT1L%08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,532; mfspr %1,533; mfspr %2,534; mfspr %3,535" 
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("IBAT2U%08lx\tIBAT2L%08lx\tIBAT3U%08lx\tIBAT3L%08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,536; mfspr %1,537; mfspr %2,538; mfspr %3,539"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("DBAT0U%08lx\tDBAT0L%08lx\tDBAT1U%08lx\tDBAT1L%08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,540; mfspr %1,541; mfspr %2,542; mfspr %3,543"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("DBAT2U%08lx\tDBAT2L%08lx\tDBAT3U%08lx\tDBAT3L%08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,1008; mfspr %1,1009; mfspr %2,1010; mfspr %3,1013"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("HID0  %08lx\tHID1  %08lx\tIABR  %08lx\tDABR  %08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,953; mfspr %1,954; mfspr %2,957; mfspr %3,958"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("PCM1  %08lx\tPCM2  %08lx\tPCM3  %08lx\tPCM4  %08lx\n",
		scr0, scr1, scr2, scr3);

	__asm volatile ("mfspr %0,952; mfspr %1,956; mfspr %2,959; mfspr %3,955"
		: "=r"(scr0),"=r"(scr1),"=r"(scr2),"=r"(scr3) :);
	printf("MMCR0 %08lx\tMMCR1 %08lx\tSDA   %08lx\tSIA   %08lx\n",
		scr0, scr1, scr2, scr3);
}
