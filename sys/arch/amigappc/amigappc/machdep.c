/* $NetBSD: machdep.c,v 1.44 2010/11/01 19:00:08 phx Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.44 2010/11/01 19:00:08 phx Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/powerpc.h>
#include <machine/stdarg.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pic/picvar.h>

#include <amiga/amiga/cc.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/isr.h>
#include <amiga/amiga/memlist.h>
#include <amigappc/amigappc/p5reg.h>

#include "opt_ddb.h"
#include "opt_ipkdb.h"

#include "fd.h"
#include "ser.h"

extern void setup_amiga_intr(void);
#if NSER > 0
extern void ser_outintr(void);
extern void serintr(void);
#endif
#if NFD > 0
extern void fdintr(int);
#endif

#define AMIGAMEMREGIONS 8
static struct mem_region physmemr[AMIGAMEMREGIONS], availmemr[AMIGAMEMREGIONS];
static char model[80];

/*
 * patched by some devices at attach time (currently, only the coms)
 */
int amiga_serialspl = 4;

/*
 * current open serial device speed;  used by some SCSI drivers to reduce
 * DMA transfer lengths.
 */
int ser_open_speed;

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

	/* enable interrupt */
	custom.intena = isr->isr_ipl == 2 ?
	    INTF_SETCLR | INTF_PORTS :
	    INTF_SETCLR | INTF_EXTER;
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

	/* disable interrupt if no more handlers */
	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;
	if (*p == NULL) {
		custom.intena = isr->isr_ipl == 6 ?
		    INTF_EXTER : INTF_PORTS;
	}
}

static int
ports_intr(void *arg)
{
	struct isr **p;
	struct isr *q;

	p = (struct isr **)arg;
	while ((q = *p) != NULL) {
		if ((q->isr_intr)(q->isr_arg))
			break;
		p = &q->isr_forw;
	}
	if (q == NULL)
		ciaa_intr();  /* ciaa handles keyboard and parallel port */

	custom.intreq = INTF_PORTS;
	return 0;
}

static int
exter_intr(void *arg)
{
	struct isr **p;
	struct isr *q;

	p = (struct isr **)arg;
	while ((q = *p) != NULL) {
		if ((q->isr_intr)(q->isr_arg))
			break;
		p = &q->isr_forw;
	}
	if (q == NULL)
		ciab_intr();  /* clear ciab icr */

	custom.intreq = INTF_EXTER;
	return 0;
}

static int
lev1_intr(void *arg)
{
	unsigned short ireq;

	ireq = custom.intreqr;
	if (ireq & INTF_TBE) {
#if NSER > 0
		ser_outintr();
#else
		custom.intreq = INTF_TBE;
#endif
	}
	if (ireq & INTF_DSKBLK) {
#if NFD > 0
		fdintr(0);
#endif
		custom.intreq = INTF_DSKBLK;
	}
	if (ireq & INTF_SOFTINT) {
#ifdef DEBUG
		printf("intrhand: SOFTINT ignored\n");
#endif
		custom.intreq = INTF_SOFTINT;
	}
	return 0;
}

static int
lev3_intr(void *arg)
{
	unsigned short ireq;

	ireq = custom.intreqr;
	if (ireq & INTF_BLIT)
		blitter_handler();
	if (ireq & INTF_COPER)
		copper_handler();
	if (ireq & INTF_VERTB)
		vbl_handler();
	return 0;
}

static int
lev4_intr(void *arg)
{

	audio_handler();
	return 0;
}

static int
lev5_intr(void *arg)
{
	unsigned short ireq;

	ireq = custom.intreqr;
	if (ireq & INTF_RBF)
		serintr();
	if (ireq & INTF_DSKSYNC)
		custom.intreq = INTF_DSKSYNC;
	return 0;
}

static void
amigappc_install_handlers(void)
{

	/* handlers for all 6 Amiga interrupt levels */
	intr_establish(1, IST_LEVEL, IPL_BIO, lev1_intr, NULL);

	intr_establish(2, IST_LEVEL, IPL_BIO, ports_intr, &isr_ports);

	intr_establish(3, IST_LEVEL, IPL_TTY, lev3_intr, NULL);

	intr_establish(4, IST_LEVEL, IPL_AUDIO, lev4_intr, NULL);

	intr_establish(5, IST_LEVEL, IPL_SERIAL, lev5_intr, NULL);

	intr_establish(6, IST_LEVEL, IPL_SERIAL, exter_intr, &isr_exter);
}

static void
amigappc_identify(void)
{
	extern u_long ns_per_tick, ticks_per_sec;
	static const unsigned char pll603[] = {
		10, 10, 10, 10, 20, 20, 25, 00,
		30, 00, 40, 00, 15, 00, 35, 00
	};
	static const unsigned char pll604[] = {
		10, 10, 70, 10, 20, 65, 25, 45,
		30, 55, 40, 50, 15, 60, 35, 00
	};
	const char *cpuname, *mach, *p5type_p, *pup;
	u_long busclock, cpuclock;
	int cpu;
	register int pvr, hid1;

	/* PowerUp ROM id location */
	p5type_p = (const char *)0xf00010;

	/*
	 * PVR holds the CPU-type and version while
         * HID1 holds the PLL configuration
	 */
	__asm ("mfpvr %0; mfspr %1,1009" : "=r"(pvr), "=r"(hid1));

	/* Amiga types which can run a PPC board */
	if (is_a4000())
		mach = "Amiga 4000";
	else if (is_a3000())
		mach = "Amiga 3000";
	else
		mach = "Amiga 1200";

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
		cpu = 604;
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

	/* compute cpuclock based on PLL configuration */
	if (cpu == 603)
		cpuclock = busclock * pll603[hid1>>28 & 0xf] / 10;
	else /* 604 */
		cpuclock = busclock * pll604[hid1>>28 & 0xf] / 10;

	snprintf(model, sizeof(model),
	    "%s %s (%s v%d.%d %lu MHz, busclk %lu MHz)",
	    mach, pup, cpuname, (pvr>>8) & 0xf, (pvr >> 0) & 0xf,
	    cpuclock / 1000000, busclock / 1000000);

	/* set timebase */
	ticks_per_sec = busclock / 4;
	ns_per_tick = 1000000000 / ticks_per_sec;
}

static void
amigappc_reboot(void)
{

	/* reboot CSPPC/BPPC */
	if (!is_a1200()) {
		P5write(P5_REG_LOCK,0x60);
		P5write(P5_REG_LOCK,0x50);
		P5write(P5_REG_LOCK,0x30);
		P5write(P5_REG_SHADOW,P5_SET_CLEAR|P5_SHADOW);
		P5write(P5_REG_LOCK,0x00);
	} else
		P5write(P5_BPPC_MAGIC,0x00);

	P5write(P5_REG_LOCK,0x60);
	P5write(P5_REG_LOCK,0x50);
	P5write(P5_REG_LOCK,0x30);
	P5write(P5_REG_SHADOW,P5_SELF_RESET);
	P5write(P5_REG_RESET,P5_AMIGA_RESET);
}

static void
amigappc_bat_add(paddr_t pa, register_t len, register_t prot)
{
	static int nd = 0, ni = 0;
	const uint32_t i = pa >> 28;

	battable[i].batl = BATL(pa, prot, BAT_PP_RW);
	battable[i].batu = BATU(pa, len, BAT_Vs);

	/*
	 * Let's start loading the BAT registers.
	 */
	if (!(prot & (BAT_I | BAT_G))) {
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
	register_t msr;

	msr = mfmsr();

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
		register_t len, prot;

		len = va_arg(ap, register_t);
		prot = va_arg(ap, register_t);
		amigappc_bat_add(pa, len, prot);
		pa = va_arg(ap, paddr_t);
	}
	va_end(ap);
}

void
initppc(u_int startkernel, u_int endkernel)
{
	struct boot_memseg *ms;
	u_int i, r;

	/*
	 * amigappc memory region set
	 */
	ms = &memlist->m_seg[0];
	for (i = 0, r = 0; i < memlist->m_nseg; i++, ms++) {
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
	 * The CSPPC RAM (A3000/A4000) always starts at 0x08000000 and is
	 * up to 128MB big.
	 * The BPPC RAM (A1200) can be up to 256MB and may start at nearly
	 * any address between 0x40000000 and 0x80000000 depending on which
	 * RAM module of which size was inserted into which bank:
	 * The RAM module in bank 1 is located from 0x?8000000 downwards.
	 * The RAM module in bank 2 is located from 0x?8000000 upwards.
	 * Whether '?' is 4, 5, 6 or 7 probably depends on the size.
	 * So we have to use the 'startkernel' symbol for BAT-mapping
	 * our RAM.
	 */
	if (is_a1200()) {
		amigappc_batinit(0x00000000, BAT_BL_16M, BAT_I|BAT_G,
		    (startkernel & 0xf0000000), BAT_BL_256M, 0,
		    0xfff00000, BAT_BL_512K, 0,
		    ~0);
	} else {
		/* A3000 or A4000 */
		amigappc_batinit(0x00000000, BAT_BL_16M, BAT_I|BAT_G,
		    (startkernel & 0xf8000000), BAT_BL_128M, 0,
		    0xfff00000, BAT_BL_512K, 0,
		    0x40000000, BAT_BL_256M, BAT_I|BAT_G,
		    ~0);
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
	oea_startup(model);

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
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP) {
		oea_dumpsys();
		/* XXX dumpsys doesn't work, so give a chance to debug */
		Debugger();
	}

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
 * used by several amiga scsi drivers.
 */
int
dma_cachectl(void *addr, int len)
{
	paddr_t pa, end;
	int inc;

	if (addr == NULL || len == 0)
		return 0;

	pa = kvtop(addr);
	inc = curcpu()->ci_ci.dcache_line_size;

	for (end = pa + len; pa < end; pa += inc)
		__asm volatile("dcbf 0,%0" :: "r"(pa));
	__asm volatile("sync");

#if 0 /* XXX not needed, we don't have instructions in DMA buffers */
	pa = kvtop(addr);
	for (end = pa + len; pa < end; pa += inc)
		__asm volatile("icbi 0,%0" :: "r"(pa));
	__asm volatile("isync");
#endif

	return 0;
}
