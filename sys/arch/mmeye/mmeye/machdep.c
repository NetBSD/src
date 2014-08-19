/*	$NetBSD: machdep.c,v 1.55.14.2 2014/08/20 00:03:14 tls Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.55.14.2 2014/08/20 00:03:14 tls Exp $");

#include "opt_ddb.h"
#include "opt_memsize.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <sh3/bscreg.h>
#include <sh3/cpgreg.h>
#include <sh3/cache_sh3.h>
#include <sh3/exception.h>
#include <sh3/mmu.h>

#include <machine/bootinfo.h>
#include <machine/mmeye.h>
#include <machine/intr.h>
#include <machine/pcb.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(MODULAR) || defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <sys/exec_elf.h>
#endif

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* mmeye */
char machine_arch[] = MACHINE_ARCH;	/* sh3eb */

char bootinfo[BOOTINFO_SIZE];

void initSH3(void *, u_int, int32_t);
void LoadAndReset(const char *);
void XLoadAndReset(char *);
void consinit(void);
void sh3_cache_on(void);
void InitializeBsc(void);
void *lookup_bootinfo(unsigned int);

struct mmeye_intrhand {
	void *intc_ih;
	int irq;
} mmeye_intrhand[_INTR_N];

/*
 * Machine-dependent startup code
 *
 * This is called from main() in kern/main.c.
 */
void
cpu_startup(void)
{

	sh_startup();
}

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_loadandreset(SYSCTLFN_ARGS)
{
	const char *osimage;
	int error;

	error = sysctl_lookup(SYSCTLFN_CALL(__UNCONST(rnode)));
	if (error || newp == NULL)
		return (error);

	osimage = (const char *)(*(const u_long *)newp);
	LoadAndReset(osimage);
	/* not reach here */
	return (0);
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
/*
<atatat> okay...your turn to play.
<atatat> pick a number.
<kjk> 98752.
*/
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "load_and_reset", NULL,
		       sysctl_machdep_loadandreset, 98752, NULL, 0,
		       CTL_MACHDEP, CPU_LOADANDRESET, CTL_EOL);
}

int waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

#if defined(MMEYE_EPC_WDT)
	epc_watchdog_timer_reset(NULL);
#endif

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/* resettodr(); */
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
#if !defined(MMEYE_EPC_WDT)
	cpu_reset();
#endif
	for(;;) ;
	/*NOTREACHED*/
}

void
initSH3(void *pc, u_int bim, int32_t bip)	/* XXX return address */
{
	extern char edata[], end[];
	struct btinfo_howto *bi_howto;
	size_t symbolsize;
	vaddr_t kernend;
	const char *bi_msg;

	/* Clear bss */
	memset(edata, 0, end - edata);

	/* Check for valid bootinfo passed from bootstrap */
	if (bim == BOOTINFO_MAGIC) {
		struct btinfo_magic *bi_magic;

		memcpy(bootinfo, (void *)bip, BOOTINFO_SIZE);
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL) {
			bi_msg = "missing bootinfo structure";
			bim = (uintptr_t)bip;
		} else if (bi_magic->magic != BOOTINFO_MAGIC) {
			bi_msg = "invalid bootinfo structure";
			bim = bi_magic->magic;
		} else
			bi_msg = NULL;
	} else {
		bi_msg = "invalid bootinfo (standalone boot?)";
	}

	symbolsize = 0;
#if NKSYMS || defined(MODULAR) || defined(DDB)
	if (memcmp(&end, ELFMAG, SELFMAG) == 0) {
		Elf_Ehdr *eh = (void *)end;
		Elf_Shdr *sh = (void *)(end + eh->e_shoff);
		int i;
		for (i = 0; i < eh->e_shnum; i++, sh++)
			if (sh->sh_offset > 0 &&
			    (sh->sh_offset + sh->sh_size) > symbolsize)
				symbolsize = sh->sh_offset + sh->sh_size;
	}
#endif

	/* Start to determine heap area */
	kernend = (vaddr_t)sh3_round_page(end + symbolsize);

	/* Initilize CPU ops. */
#if defined(SH7708R)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708R);
#elif defined(SH7708)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708);
#elif defined(SH7750R)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750R);
#else
#warning "unknown product"
#endif
	consinit();

	if (bi_msg != NULL)
		printf("%s: magic=%#x\n", bi_msg, bim);

	bi_howto = lookup_bootinfo(BTINFO_HOWTO);
	if (bi_howto != NULL)
		boothowto = bi_howto->bi_howto;

	/* Load memory to UVM */
	physmem = atop(IOM_RAM_SIZE);
	kernend = atop(round_page(SH3_P1SEG_TO_PHYS(kernend)));
	uvm_page_physload(
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		VM_FREELIST_DEFAULT);

	/* Initialize proc0 u-area */
	sh_proc0_init();

	/* Initialize pmap and start to address translation */
	pmap_bootstrap();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (symbolsize)
		ksyms_addsyms_elf(symbolsize, &end, end + symbolsize);
#endif
#if defined(DDB)
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * XXX We can't return here, because we change stack pointer.
	 *     So jump to return address directly.
	 */
	__asm volatile (
		"jmp	@%0;"
		"mov	%1, r15"
		:: "r"(pc),"r"(lwp0.l_md.md_pcb->pcb_sf.sf_r7_bank));
}

/*
 * consinit:
 * initialize the system console.
 */
void
consinit(void)
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	cninit();
}

/*
 * InitializeBsc
 * : BSC(Bus State Controller)
 */
void
InitializeBsc(void)
{
#ifdef SH3
#ifdef NOPCMCIA
	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = DRAM, Area5 = Normal memory
	 * Area4 = Normal Memory
	 * Area6 = Normal memory
	 */
	_reg_write_2(SH3_BCR1, 0x1010);
#else /* NOPCMCIA */
	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = DRAM, Area5 = PCMCIA
	 * Area4 = Normal Memory
	 * Area6 = PCMCIA
	 */
	_reg_write_2(SH3_BCR1, 0x1013);
#endif /* NOPCMCIA */

#define PCMCIA_16
#ifdef PCMCIA_16
	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 16bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	_reg_write_2(SH3_BCR2, 0x2af4);
#else /* PCMCIA16 */
	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 8bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	_reg_write_2(SH3_BCR2, 0x16f4);
#endif /* PCMCIA16 */
	/*
	 * Idle cycle number in transition area and read to write
	 * Area6 = 3, Area5 = 3, Area4 = 3, Area3 = 3, Area2 = 3
	 * Area1 = 3, Area0 = 3
	 */
	_reg_write_2(SH3_WCR1, 0x3fff);

#if 0
	/*
	 * Wait cycle
	 * Area 6,5 = 2
	 * Area 4 = 10
	 * Area 3 = 2
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
	_reg_write_2(SH3_WCR2, 0x4bdd);
#else
	/*
	 * Wait cycle
	 * Area 6 = 6
	 * Area 5 = 2
	 * Area 4 = 10
	 * Area 3 = 3
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
	_reg_write_2(SH3_WCR2, 0xabfd);
#endif

	/*
	 * RAS pre-charge = 2cycle, RAS-CAS delay = 3 cycle,
	 * write pre-charge = 1cycle
	 * CAS before RAS refresh RAS assert time = 3  cycle
	 * Disable burst, Bus size=32bit, Column Address=10bit,Refresh ON
	 * CAS before RAS refresh ON, EDO DRAM
	 */
	_reg_write_2(SH3_MCR, 0x6135);
	/* SHREG_MCR = 0x4135; */

	/* DRAM Control Register */
	_reg_write_2(SH3_DCR, 0x0000);

	/*
	 * PCMCIA Control Register
	 * OE/WE assert delay 3.5 cycle
	 * OE/WE negate-address delay 3.5 cycle
	 */
	_reg_write_2(SH3_PCR, 0x00ff);
#endif
#ifdef SH4
	/*
	 * mmEye-WLF sets these values to registers.
	 * Its values takes from that original kernel.
	 */
	_reg_write_4(SH4_BCR1, 0x0008000d);
	_reg_write_2(SH4_BCR2, 0x6af9);
	_reg_write_4(SH4_WCR1, 0x11111111);
	_reg_write_4(SH4_WCR2, 0xdd7845a7);
	_reg_write_4(SH4_WCR3, 0x05555555);
	_reg_write_4(SH4_MCR, 0x500921f4);
	_reg_write_2(SH4_PCR, 0x4b2d);
#endif

	/*
	 * Refresh Timer Control/Status Register
	 * Disable interrupt by CMF, closk 1/16, Disable OVF interrupt
	 * Count Limit = 1024
	 * In following statement, the reason why high byte = 0xa5(a4 in RFCR)
	 * is the rule of SH3 in writing these register .
	 */
	_reg_write_2(SH_(RTCSR), 0xa594);

	/*
	 * Refresh Timer Counter
	 * initialize to 0
	 */
	_reg_write_2(SH_(RTCNT), 0xa500);

	/*
	 * set Refresh Time Constant Register
	 */
#ifdef SH3
	_reg_write_2(SH3_RTCOR, 0xa50d);
#elif SH4
	_reg_write_2(SH4_RTCOR, 0xa575);
#endif

	/*
	 * init Refresh Count Register
	 */
	_reg_write_2(SH_(RFCR), 0xa400);

	/*
	 * Set Clock mode (make internal clock double speed)
	 */
#if defined(SH7708R)
	_reg_write_2(SH3_FRQCR, 0xa100); /* 100MHz */
#elif defined(SH7708)
	_reg_write_2(SH3_FRQCR, 0x0112); /* 60MHz */
#elif defined(SH7750R)
	_reg_write_2(SH4_FRQCR, 0x0e0a);
#endif

#ifndef MMEYE_NO_CACHE
	/* Cache ON */
	_reg_write_4(SH3_CCR, SH3_CCR_CE);
#endif
}

void
sh3_cache_on(void)
{
#ifndef MMEYE_NO_CACHE
	/* Cache ON */
	_reg_write_4(SH3_CCR, SH3_CCR_CE);
	_reg_write_4(SH3_CCR, SH3_CCR_CF | SH3_CCR_CE);	/* cache clear */
	_reg_write_4(SH3_CCR, SH3_CCR_CE);
#endif
}

 /* XXX This value depends on physical available memory */
#define OSIMAGE_BUF_ADDR	(IOM_RAM_BEGIN + 0x00400000)

void
LoadAndReset(const char *osimage)
{
	void *buf_addr;
	u_long size;
	const u_long *src;
	u_long *dest;
	u_long csum = 0;
	u_long csum2 = 0;
	u_long size2;

	MMTA_IMASK = 0; /* mask all externel interrupt */

	printf("LoadAndReset: copy start\n");
	buf_addr = (void *)OSIMAGE_BUF_ADDR;

	size = *(const u_long *)osimage;
	src = (const u_long *)osimage;
	dest = buf_addr;

	size = (size + sizeof(u_long) * 2 + 3) >> 2;
	size2 = size;

	while (size--) {
		csum += *src;
		*dest++ = *src++;
	}

	dest = buf_addr;
	while (size2--)
		csum2 += *dest++;

	printf("LoadAndReset: copy end[%lx,%lx]\n", csum, csum2);
	printf("start XLoadAndReset\n");

	XLoadAndReset(buf_addr);
}

#ifdef sh3_tmp

#define	UART_BASE	0xa4000008

#define	IER	1
#define	IER_RBF	0x01
#define	IER_TBE	0x02
#define	IER_MSI	0x08
#define	FCR	2
#define	LCR	3
#define	LCR_DLAB	0x80
#define	DLM	1
#define	DLL	0
#define	MCR	4
#define	MCR_RTS	0x02
#define	MCR_DTR	0x01
#define	MCR_OUT2	0x08
#define	RTS_MODE	(MCR_RTS|MCR_DTR)
#define	LSR	5
#define	LSR_THRE	0x20
#define	LSR_ERROR	0x1e
#define	THR	0
#define	IIR	2
#define	IIR_II	0x06
#define	IIR_LSI	0x06
#define	IIR_MSI	0x00
#define	IIR_TBE	0x02
#define	IIR_PEND	0x01
#define	RBR	0
#define	MSR	6

#define	OUTP(port, val)	*(volatile unsigned char *)(UART_BASE+port) = val
#define	INP(port)	(*(volatile unsigned char *)(UART_BASE+port))

void
Init16550()
{
	int diviser;
	int tmp;

	/* Set speed */
	/* diviser = 12; */	/* 9600 bps */
	diviser = 6;	/* 19200 bps */

	OUTP(IER, 0);
	/* OUTP(FCR, 0x87); */	/* FIFO mode */
	OUTP(FCR, 0x00);	/* no FIFO mode */

	tmp = INP(LSR);
	tmp = INP(MSR);
	tmp = INP(IIR);
	tmp = INP(RBR);

	OUTP(LCR, INP(LCR) | LCR_DLAB);
	OUTP(DLM, 0xff & (diviser>>8));
	OUTP(DLL, 0xff & diviser);
	OUTP(LCR, INP(LCR) & ~LCR_DLAB);
	OUTP(MCR, 0);

	OUTP(LCR, 0x03);	/* 8 bit , no parity, 1 stop */

	/* start comm */
	OUTP(MCR, RTS_MODE | MCR_OUT2);
	/* OUTP(IER, IER_RBF | IER_TBE | IER_MSI); */
}

void
Send16550(int c)
{
	while (1) {
		OUTP(THR, c);

		while ((INP(LSR) & LSR_THRE) == 0)
			;

		if (c == '\n')
			c = '\r';
		else
			return;
	}
}
#endif /* sh3_tmp */


void
intc_intr(int ssr, int spc, int ssp)
{
	struct intc_intrhand *ih;
	int evtcode;

	curcpu()->ci_data.cpu_nintr++;

	evtcode = _reg_read_4(SH_(INTEVT));

	ih = EVTCODE_IH(evtcode);
	KDASSERT(ih->ih_func);
	/*
	 * On entry, all interrrupts are disabled,
	 * and exception is enabled for P3 access. (kernel stack is P3,
	 * SH3 may or may not cause TLB miss when access stack.)
	 * Enable higher level interrupt here.
	 */
	(void)_cpu_intr_resume(ih->ih_level);

	if (evtcode == SH_INTEVT_TMU0_TUNI0) {	/* hardclock */
		struct clockframe cf;
		cf.spc = spc;
		cf.ssr = ssr;
		cf.ssp = ssp;
		(*ih->ih_func)(&cf);
	} else {
		(*ih->ih_func)(ih->ih_arg);
	}
}

void *
mmeye_intr_establish(int irq, int trigger, int level, int (*func)(void *),
    void *arg)
{
	struct mmeye_intrhand *mh = &mmeye_intrhand[irq];

	mh->intc_ih = intc_intr_establish(0x200 + (irq << 5), IST_LEVEL, level,
	    func, arg);
	mh->irq = irq;

	MMTA_IMASK |= (1 << (15 - irq));

	return (mh);
}

void
mmeye_intr_disestablish(void *ih)
{
	struct mmeye_intrhand *mh = ih;

	MMTA_IMASK &= ~(1 << (15 - mh->irq));

	intc_intr_disestablish(mh->intc_ih);
}

int
bus_space_map(bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{

#if defined(SH7750R)
	if (t == SH3_BUS_SPACE_PCMCIA_IO ||
	    t == SH3_BUS_SPACE_PCMCIA_IO8 ||
	    t == SH3_BUS_SPACE_PCMCIA_MEM ||
	    t == SH3_BUS_SPACE_PCMCIA_MEM8 ||
	    t == SH3_BUS_SPACE_PCMCIA_ATT ||
	    t == SH3_BUS_SPACE_PCMCIA_ATT8) {
		pt_entry_t *pte;
		vaddr_t va;
		u_long pa, endpa;
		uint32_t sa = 0;

		pa = sh3_trunc_page(addr);
		endpa = sh3_round_page(addr + size);

#ifdef DIAGNOSTIC
		if (endpa <= pa)
			panic("bus_space_map: overflow");
#endif

		va = uvm_km_alloc(kernel_map, endpa - pa, 0, UVM_KMF_VAONLY);
		if (va == 0) {
			printf("%s: nomem\n", __func__);
			return ENOMEM;
		}

		*bshp = (bus_space_handle_t)(va + (addr & PGOFSET));

		switch (t) {
		case SH3_BUS_SPACE_PCMCIA_IO:	sa = _PG_PCMCIA_IO16;	break;
		case SH3_BUS_SPACE_PCMCIA_IO8:	sa = _PG_PCMCIA_IO8;	break;
		case SH3_BUS_SPACE_PCMCIA_MEM:	sa = _PG_PCMCIA_MEM16;	break;
		case SH3_BUS_SPACE_PCMCIA_MEM8:	sa = _PG_PCMCIA_MEM8;	break;
		case SH3_BUS_SPACE_PCMCIA_ATT:	sa = _PG_PCMCIA_ATTR16;	break;
		case SH3_BUS_SPACE_PCMCIA_ATT8:	sa = _PG_PCMCIA_ATTR8;	break;
		}

		for ( ; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
			pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
			pte = __pmap_kpte_lookup(va);
			KDASSERT(pte);
			*pte |= sa;
			sh_tlb_update(0, va, *pte);
		}

		return 0;
	}
#endif

	*bshp = (bus_space_handle_t)addr;
	return 0;
}

void
sh_memio_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

#if defined(SH7750R)
	if (t == SH3_BUS_SPACE_PCMCIA_IO ||
	    t == SH3_BUS_SPACE_PCMCIA_IO8 ||
	    t == SH3_BUS_SPACE_PCMCIA_MEM ||
	    t == SH3_BUS_SPACE_PCMCIA_MEM8 ||
	    t == SH3_BUS_SPACE_PCMCIA_ATT ||
	    t == SH3_BUS_SPACE_PCMCIA_ATT8) {
		u_long va, endva;

		va = sh3_trunc_page(bsh);
		endva = sh3_round_page(bsh + size);

#ifdef DIAGNOSTIC
		if (endva <= va)
			panic("bus_space_unmap: overflow");
#endif

		pmap_kremove(va, endva - va);

		/*
		 * Free the kernel virtual mapping
		 */
		uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);

		return;
	}
#endif

	/* Nothing */
}

int
sh_memio_subregion(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

/*
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(unsigned int type)
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	if (((struct btinfo_common *)help)->next == 0)
		return NULL;		/* bootinfo not present */
	do {
		bt = (struct btinfo_common *)help;
		printf("Type %d @%p\n", bt->type, (void *)(intptr_t)bt);
		if (bt->type == type)
			return (void *)help;
		help += bt->next;
	} while (bt->next != 0 &&
	    (size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return NULL;
}

#ifdef MODULAR
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
}
#endif /* MODULAR */

#if defined(MMEYE_EPC_WDT)
callout_t epc_wdtc;

void
epc_watchdog_timer_reset(void *arg)
{

	callout_schedule(&epc_wdtc, 15 * hz);

	EPC_WDT = WDT_RDYCMD;
	EPC_WDT = WDT_CLRCMD;
}
#endif
