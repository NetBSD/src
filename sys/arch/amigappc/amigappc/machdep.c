/* $NetBSD: machdep.c,v 1.30.26.2 2007/03/12 05:46:47 rmind Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.30.26.2 2007/03/12 05:46:47 rmind Exp $");

#include "opt_ddb.h"
#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/powerpc.h>
#include <machine/bat.h>
#include <machine/mtpr.h>
#include <machine/trap.h>
#include <machine/hid.h>
#include <machine/cpu.h>

#include <amiga/amiga/cc.h>
#include <amiga/amiga/cia.h>

/*
 * Global variables used here and there
 * from macppc/machdep.c
 */
struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;

extern struct user *proc0paddr;

/* from amiga/machdep.c */
char cpu_model[80];
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

/* XXX: should be in extintr.c */
volatile int cpl, ipending, astpending, tickspending;
int imask[NIPL];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

struct bat battable[16];
extern int aga_enable, eclockfreq;

#define PPCMEMREGIONS 32
static struct mem_region PPCmem[PPCMEMREGIONS + 1], PPCavail[PPCMEMREGIONS + 3];

void show_me_regs(void);

void
initppc(startkernel, endkernel)
        u_int startkernel, endkernel;
{
	extern void cpu_fail(void);
	extern adamint, adamintsize;
	extern extint, extsize;
	extern trapcode, trapsize;
	extern alitrap, alisize;
	extern dsitrap, dsisize;
	extern isitrap, isisize;
	extern decrint, decrsize;
	extern tlbimiss, tlbimsize;
	extern tlbdlmiss, tlbdlmsize;
	extern tlbdsmiss, tlbdsmsize;
#ifdef DDB
	extern ddblow, ddbsize;
#endif
#ifdef IPKDB
	extern ipkdblow, ipkdbsize;
#endif
	int exc, scratch;

	/* force memory mapping */
	PPCmem[0].start = 0x8000000;
	PPCmem[0].size  = 0x5f80000;
	PPCmem[1].start = 0x7c00000;
	PPCmem[1].size  = 0x0400000;
	PPCmem[2].start = 0x0;
	PPCmem[2].size  = 0x0;

	PPCavail[0].start = 0x8000000;
	PPCavail[0].size  = 0x5f80000;
	PPCavail[1].start = (0x7c00000 + endkernel + PGOFSET) & ~PGOFSET;
	PPCavail[1].size  = 0x8000000 - PPCavail[1].start;
/*
	PPCavail[1].start = 0x7c00000;
	PPCavail[1].size  = 0x0400000;
*/
	PPCavail[2].start = 0x0;
	PPCavail[2].size  = 0x0;

	CHIPMEMADDR = 0x0;
	chipmem_start = 0x0;
	chipmem_end  = 0x200000;

	CIAADDR = 0xbfd000;
	CIAAbase = CIAADDR + 0x1001;
	CIABbase = CIAADDR;

	CUSTOMADDR = 0xdff000;
	CUSTOMbase = CUSTOMADDR;

	eclockfreq = 709379;

	aga_enable = 1;
	machineid = 4000 << 16;

	/* Initialize BAT tables */
	battable[0].batl = BATL(0x00000000, BAT_I|BAT_G, BAT_PP_RW);
	battable[0].batu = BATU(0x00000000, BAT_BL_16M, BAT_Vs);
	battable[1].batl = BATL(0x08000000, 0, BAT_PP_RW);
	battable[1].batu = BATU(0x08000000, BAT_BL_128M, BAT_Vs);
	battable[2].batl = BATL(0x07000000, 0, BAT_PP_RW);
	battable[2].batu = BATU(0x07000000, BAT_BL_16M, BAT_Vs);
	battable[3].batl = BATL(0xfff00000, 0, BAT_PP_RW);
	battable[3].batu = BATU(0xfff00000, BAT_BL_512K, BAT_Vs);

	/* Load BAT registers */
	__asm volatile ("mtibatl 0,%0; mtibatu 0,%1;"
		"mtdbatl 0,%0; mtdbatu 0,%1" ::
		"r"(battable[0].batl), "r"(battable[0].batu));
	__asm volatile ("mtibatl 1,%0; mtibatu 1,%1;"
		"mtdbatl 1,%0; mtdbatu 1,%1" ::
		"r"(battable[1].batl), "r"(battable[1].batu));
	__asm volatile ("mtibatl 2,%0; mtibatu 2,%1;"
		"mtdbatl 2,%0; mtdbatu 2,%1" ::
		"r"(battable[2].batl), "r"(battable[2].batu));
	__asm volatile ("mtibatl 3,%0; mtibatu 3,%1;"
		"mtdbatl 3,%0; mtdbatu 3,%1" ::
		"r"(battable[3].batl), "r"(battable[3].batu));

	proc0.p_addr = proc0paddr;
	bzero(proc0.p_addr, sizeof *proc0.p_addr);
	curpcb = &proc0paddr->u_pcb;
	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

	/*
	 * Set up trap vectors
	 */
	for (exc = EXC_RSVD + EXC_UPPER; exc <= EXC_LAST + EXC_UPPER; exc += 0x100) {
		switch (exc - EXC_UPPER) {
		default:
			memcpy((void *)exc, &trapcode, (size_t)&trapsize);
			break;
		case EXC_MCHK:
			memcpy((void *)exc, &adamint, (size_t)&adamintsize);
			break;
		case EXC_EXI:
			memcpy((void *)exc, &extint, (size_t)&extsize);
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
		case EXC_ALI:
			memcpy((void *)exc, &alitrap, (size_t)&alisize);
			break;
		case EXC_DSI:
			memcpy((void *)exc, &dsitrap, (size_t)&dsisize);
			break;
		case EXC_ISI:
			memcpy((void *)exc, &isitrap, (size_t)&isisize);
			break;
		case EXC_DECR:
			memcpy((void *)exc, &decrint, (size_t)&decrsize);
			break;
		case EXC_IMISS:
			memcpy((void *)exc, &tlbimiss, (size_t)&tlbimsize);
			break;
		case EXC_DLMISS:
			memcpy((void *)exc, &tlbdlmiss, (size_t)&tlbdlmsize);
			break;
		case EXC_DSMISS:
			memcpy((void *)exc, &tlbdsmiss, (size_t)&tlbdsmsize);
			break;

#if defined(DDB) || defined(IPKDB)
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#else
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#endif
			break;
#endif /* DDB || IPKDB */
		}
	}

	/* External interrupt handler install
	*/
	__syncicache((void *)(EXC_RST + EXC_UPPER), EXC_LAST - EXC_RST + 0x100);

	/*
	 * Enable translation and interrupts
	 */
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync" :
		"=r"(scratch) : "K"(PSL_EE|PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	/*
	 * Set the page size
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module
	 */
	pmap_bootstrap(startkernel, endkernel);
}


/* XXX: for a while here, from intr.h */
volatile int
splraise(ncpl)
	int ncpl;
{
	int ocpl;
	volatile unsigned char *p5_ipl = (void *)0xf60030;

	ocpl = ~(*p5_ipl) & 7;

	__asm volatile("sync; eieio\n");	/* don't reorder.... */

	/*custom.intreq = 0x7fff;*/
	if (ncpl > ocpl) {
		/* disable int */
		/*p5_ipl = 0xc0;*/
		/* clear bits */
		*p5_ipl = 7;
		/* set new priority */
		*p5_ipl = 0x80 | (~ncpl & 7);
		/* enable int */
		/*p5_ipl = 0x40;*/
	}

	__asm volatile("sync; eieio\n");	/* reorder protect */
	return (ocpl);
}

volatile void
splx(ncpl)
	int ncpl;
{
	volatile unsigned char *p5_ipl = (void *)0xf60030;

	__asm volatile("sync; eieio\n");	/* reorder protect */

/*	if (ipending & ~ncpl)
		do_pending_int();*/

	custom.intreq = custom.intreqr;
	/* disable int */
	/*p5_ipl = 0xc0;*/
	/* clear bits */
	*p5_ipl = 0x07;
	/* set new priority */
	*p5_ipl = 0x80 | (~ncpl & 7);
	/* enable int */
	/*p5_ipl = 0x40;*/

	__asm volatile("sync; eieio\n");	/* reorder protect */
}

volatile int
spllower(ncpl)
	int ncpl;
{
	int ocpl;
	volatile unsigned char *p5_ipl = (void *)0xf60030;

	ocpl = ~(*p5_ipl) & 7;

	__asm volatile("sync; eieio\n");	/* reorder protect */

/*	if (ipending & ~ncpl)
		do_pending_int();*/

	/*custom.intreq = 0x7fff;*/
	if (ncpl < ocpl) {
		/* disable int */
		/*p5_ipl = 0xc0;*/
		/* clear bits */
		*p5_ipl = 7;
		/* set new priority */
		*p5_ipl = 0x80 | (~ncpl & 7);
		/* enable int */
		/*p5_ipl = 0x40;*/
	}

	__asm volatile("sync; eieio\n");	/* reorder protect */
	return (ocpl);
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
volatile void
softintr(ipl)
	int ipl;
{
	int msrsave;

	__asm volatile("mfmsr %0" : "=r"(msrsave));
	__asm volatile("mtmsr %0" :: "r"(msrsave & ~PSL_EE));
	ipending |= 1 << ipl;
	__asm volatile("mtmsr %0" :: "r"(msrsave));
}
/* XXX: end of intr.h */


/* show PPC registers */
void show_me_regs()
{
	register u_long	scr0, scr1, scr2, scr3;

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


/*
 * This is called during initppc, before the system is really initialized.
 * It shall provide the total and the available regions of RAM.
 * Both lists must have a zero-size entry as terminator.
 * The available regions need not take the kernel into account, but needs
 * to provide space for two additional entry beyond the terminating one.
 */
void
mem_regions(memp, availp)
	struct mem_region **memp, **availp;
{
	*memp = PPCmem;
	*availp = PPCavail;
}


/* XXX */
void
do_pending_int(void)
{
	__asm volatile ("sync");
}

/*
 * Interrupt handler
 */
void
intrhand()
{
	register unsigned short ireq;

	ireq = custom.intreqr;

	/* transmit buffer empty */
	if (ireq & INTF_TBE) {
#if NSER > 0
		ser_outintr();
#else
		custom.intreq = INTF_TBE;
#endif
	}

	/* disk block */
	if (ireq & INTF_DSKBLK) {
#if NFD > 0
		fdintr(0);
#endif
		custom.intreq = INTF_DSKBLK;
	}

	/* software */
	if (ireq & INTF_SOFTINT) {
		custom.intreq = INTF_SOFTINT;
	}

	/* ports */
	if (ireq & INTF_PORTS) {
		ciaa_intr();
		custom.intreq = INTF_PORTS;
	}

	/* vertical blank */
	if (ireq & INTF_VERTB) {
		vbl_handler();
	}

	/* blitter */
	if (ireq & INTF_BLIT) {
		blitter_handler();
	}

	/* copper */
	if (ireq & INTF_COPER) {
		copper_handler();
	}
}


struct isr *isr_ports;
struct isr *isr_exter;

void
add_isr(isr)
	struct isr *isr;
{
	struct isr **p, *q;

	p = isr->isr_ipl == 2 ? &isr_ports : &isr_exter;

	while ((q = *p) != NULL) {
		p = &q->isr_forw;
	}
	isr->isr_forw = NULL;
	*p = isr;
	/* enable interrupt */
	custom.intena = isr->isr_ipl == 2 ? INTF_SETCLR | INTF_PORTS :
						INTF_SETCLR | INTF_EXTER;
}

void
remove_isr(isr)
	struct isr *isr;
{
	struct isr **p, *q;

	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;

	while ((q = *p) != NULL && q != isr) {
		p = &q->isr_forw;
	}
	if (q) {
		*p = q->isr_forw;
	}
	else {
		panic("remove_isr: handler not registered");
	}

	/* disable interrupt if no more handlers */
	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;
	if (*p == NULL) {
		custom.intena = isr->isr_ipl == 6 ? INTF_EXTER : INTF_PORTS;
	}
}


/*
 * this is a handy package to have asynchronously executed
 * function calls executed at very low interrupt priority.
 * Example for use is keyboard repeat, where the repeat 
 * handler running at splclock() triggers such a (hardware
 * aided) software interrupt.
 * Note: the installed functions are currently called in a
 * LIFO fashion, might want to change this to FIFO
 * later.
 */
struct si_callback {
	struct si_callback *next;
	void (*function) __P((void *rock1, void *rock2));
	void *rock1, *rock2;
};
static struct si_callback *si_callbacks;
static struct si_callback *si_free;
#ifdef DIAGNOSTIC
static int ncb;		/* number of callback blocks allocated */
static int ncbd;	/* number of callback blocks dynamically allocated */
#endif

void
alloc_sicallback()
{
	struct si_callback *si;
	int s;

	si = (struct si_callback *)malloc(sizeof(*si), M_TEMP, M_NOWAIT);
	if (si == NULL)	{
		return;
	}
	s = splhigh();
	si->next = si_free;
	si_free = si;
	splx(s);
#ifdef DIAGNOSTIC
	++ncb;
#endif
}


/*
int
sys_sysarch()
{
return 0;
}*/

void
identifycpu()
{
	register int pvr, hid1;
	char *mach, *pup, *cpu;
	static const char pll[] = {10, 10, 70, 0, 20, 65, 25, 45,
			30, 55, 40, 50, 15, 60, 35, 0};
	const char *p5type_p = (const char *)0xf00010;
	int cpuclock, busclock;

	/* Amiga type */
	if (is_a4000()) {
		mach = "Amiga 4000";
	}
	else if (is_a3000()) {
		mach = "Amiga 3000";
	}
	else {
		mach = "Amiga 1200";
	}

	__asm ("mfpvr %0; mfspr %1,1009" : "=r"(pvr), "=r"(hid1));

	/* XXX removethis */printf("p5serial = %8s\n", p5type_p);
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

	switch (p5type_p[1]) {
	case 'A':
		busclock = 60000000/4;
		cpuclock = 600;
		break;
	/* case B, C, D */
	default:
		busclock = 66000000/4;
		cpuclock = 666;
		break;
	}
	/*
	 * compute cpuclock based on PLL configuration in HID1
	 * XXX: based on 604e, should work for 603e
	 */
	hid1 = hid1>>28 & 0xf;
	cpuclock = cpuclock*pll[hid1]/100;

	/* find CPU type */
	switch (pvr >> 16) {
	case 1:
		cpu = "601";
		break;
	case 3:
		cpu = "603";
		break;
	case 4:
		cpu = "604";
		break;
	case 5:
		cpu = "602";
		break;
	case 6:
		cpu = "603e";
		break;
	case 7:
		cpu = "603e+";
		break;
	case 8:
		cpu = "750";
		break;
	case 9:
	case 10:
		cpu = "604e";
		break;
	case 12:
		cpu = "7400";
		break;
	case 20:
		cpu = "620";
		break;
	default:
		cpu = "unknown";
		break;
	}

	snprintf(cpu_model, sizeof(cpu_model), 
		"%s %s (%s v%d.%d %d MHz, busclk %d kHz)", mach, pup, cpu,
		pvr>>8 & 0xff, pvr & 0xff, cpuclock, busclock / 1000);
	printf("%s\n", cpu_model);
}

/*
 * Machine dependent startup code
 */
void
cpu_startup()
{
	void *	v;
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	initmsgbuf((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

	proc0.p_addr = proc0paddr;
	v = (void *)proc0paddr + USPACE;

	printf("%s%s", copyright, version);
	identifycpu();

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				16*NCARGS, VM_MAP_PAGEABLE, false, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
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
cpu_reboot(howto, what)
	int howto;
	char *what;
{
	static int syncing;
	static char str[256];

	howto = 0;
}

int
lcsplx(ipl)
	int ipl;
{
	return spllower(ipl);   /* XXX */
}

/*
 * Convert kernel VA to physical address
 */
int
kvtop(addr)
	void *addr;
{
	vaddr_t va;
	paddr_t pa;
	int off;
	extern char end[];

	if (addr < end)
		return (int)addr;

	va = trunc_page((vaddr_t)addr);
	off = (int)addr - va;

	if (pmap_extract(pmap_kernel(), va, &pa) == false) {
		/*printf("kvtop: zero page frame (va=0x%x)\n", addr);*/
		return (int)addr;
	}

	return((int)pa + off);
}
