/*	$NetBSD: machdep.c,v 1.59.10.3 2009/06/20 07:20:02 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.59.10.3 2009/06/20 07:20:02 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_memsize.h"
#include "opt_initbsc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/ksyms.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <sh3/bscreg.h>
#include <sh3/cpgreg.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>
#include <sh3/exception.h>

#include <machine/bus.h>
#include <machine/intr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* evbsh3 */
char machine_arch[] = MACHINE_ARCH;	/* sh3eb or sh3el */

void initSH3(void *);
void LoadAndReset(const char *);
void XLoadAndReset(char *);

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

void
cpu_reboot(int howto, char *bootstr)
{
	static int waittime = -1;

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
	cpu_reset();
	for(;;)
		;
	/*NOTREACHED*/
}

void
initSH3(void *pc)	/* XXX return address */
{
	extern char edata[], end[];
	vaddr_t kernend;

	/* Clear bss */
	memset(edata, 0, end - edata);

	/* Initilize CPU ops. */
#if defined(SH3) && defined(SH4)
#error "don't define both SH3 and SH4"
#elif defined(SH3)
#if defined(SH7708)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708);
#elif defined(SH7708S)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708S);
#elif defined(SH7708R)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708R);
#elif defined(SH7709)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7709);
#elif defined(SH7709A)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7709A);
#elif defined(SH7706)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7706);
#else
#error "unsupported SH3 variants"
#endif
#elif defined(SH4)
#if defined(SH7750)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750);	
#elif defined(SH7750S)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750S);
#elif defined(SH7750R)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750R);
#elif defined(SH7751)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7751);
#elif defined(SH7751R)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7751R);
#else
#error "unsupported SH4 variants"
#endif
#else
#error "define SH3 or SH4"
#endif
	/* Console */
	consinit();

	/* Load memory to UVM */
	kernend = atop(round_page(SH3_P1SEG_TO_PHYS(end)));
	physmem = atop(IOM_RAM_SIZE);
	uvm_page_physload(
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		VM_FREELIST_DEFAULT);

	/* Initialize proc0 u-area */
	sh_proc0_init();

	/* Initialize pmap and start to address translation */
	pmap_bootstrap();

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
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
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

int
bus_space_map (bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags, bus_space_handle_t *bshp)
{

	*bshp = (bus_space_handle_t)addr;

	return 0;
}

int
sh_memio_subregion(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

int
sh_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
	       bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	*bshp = *bpap = rstart;

	return (0);
}

void
sh_memio_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

}

void
sh_memio_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	return;
}

#ifdef SH4_PCMCIA

int
shpcmcia_memio_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags, bus_space_handle_t *bshp)
{
	int error;
	struct extent *ex;
	bus_space_tag_t pt = t & ~SH3_BUS_SPACE_PCMCIA_8BIT;

	if (pt != SH3_BUS_SPACE_PCMCIA_IO && 
	    pt != SH3_BUS_SPACE_PCMCIA_MEM &&
	    pt != SH3_BUS_SPACE_PCMCIA_ATT) {
		*bshp = (bus_space_handle_t)bpa;

		return 0;
	}

	ex = iomem_ex;

#if 0
	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size,
				    EX_NOWAIT | EX_MALLOCOK );
	if (error){
		printf("sh3_pcmcia_memio_map:extent_alloc_region error\n");
		return (error);
	}
#endif

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = shpcmcia_mem_add_mapping(bpa, size, (int)t, bshp );
#if 0
	if (error) {
		if (extent_free(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK )) {
			printf("sh3_pcmcia_memio_map: pa 0x%lx, size 0x%lx\n",
			       bpa, size);
			printf("sh3_pcmcia_memio_map: can't free region\n");
		}
	}
#endif

	return (error);
}

int
shpcmcia_mem_add_mapping(bus_addr_t bpa, bus_size_t size, int type, bus_space_handle_t *bshp)
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;
	unsigned int m = 0;
	int io_type = type & ~SH3_BUS_SPACE_PCMCIA_8BIT;

	pa = sh3_trunc_page(bpa);
	endpa = sh3_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("sh3_pcmcia_mem_add_mapping: overflow");
#endif

	va = uvm_km_alloc(kernel_map, endpa - pa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0){
		printf("shpcmcia_add_mapping: nomem \n");
		return (ENOMEM);
	}

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

#define MODE(t, s)							\
	(t) & SH3_BUS_SPACE_PCMCIA_8BIT ?				\
		_PG_PCMCIA_ ## s ## 8 :					\
		_PG_PCMCIA_ ## s ## 16
	switch (io_type) {
	default:
		panic("unknown pcmcia space.");
		/* NOTREACHED */
	case SH3_BUS_SPACE_PCMCIA_IO:
		m = MODE(type, IO);
		break;
	case SH3_BUS_SPACE_PCMCIA_MEM:
		m = MODE(type, MEM);
		break;
	case SH3_BUS_SPACE_PCMCIA_ATT:
		m = MODE(type, ATTR);
		break;
	}
#undef MODE

	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		pte = __pmap_kpte_lookup(va);
		KDASSERT(pte);
		*pte |= m;  /* PTEA PCMCIA assistant bit */
		sh_tlb_update(0, va, *pte);
	}

	return 0;
}

void
shpcmcia_memio_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	struct extent *ex;
	u_long va, endva;
	bus_addr_t bpa;
	bus_space_tag_t pt = t & ~SH3_BUS_SPACE_PCMCIA_8BIT;

	if (pt != SH3_BUS_SPACE_PCMCIA_IO && 
	    pt != SH3_BUS_SPACE_PCMCIA_MEM &&
	    pt != SH3_BUS_SPACE_PCMCIA_ATT) {
		return ;
	}

	ex = iomem_ex;

	va = sh3_trunc_page(bsh);
	endva = sh3_round_page(bsh + size);

#ifdef DIAGNOSTIC
	if (endva <= va)
		panic("sh3_pcmcia_memio_unmap: overflow");
#endif

	pmap_extract(pmap_kernel(), va, &bpa);
	bpa += bsh & PGOFSET;

	/*
	 * Free the kernel virtual mapping.
	 */
	pmap_kremove(va, endva - va);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);

#if 0
	if (extent_free(ex, bpa, size,
			EX_NOWAIT | EX_MALLOCOK)) {
		printf("sh3_pcmcia_memio_unmap: %s 0x%lx, size 0x%lx\n",
		       "pa", bpa, size);
		printf("sh3_pcmcia_memio_unmap: can't free region\n");
	}
#endif
}

void    
shpcmcia_memio_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	/* sh3_pcmcia_memio_unmap() does all that we need to do. */
	shpcmcia_memio_unmap(t, bsh, size);
}

int
shpcmcia_memio_subregion(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

#endif /* SH4_PCMCIA */

#if !defined(DONT_INIT_BSC)
/*
 * InitializeBsc
 * : BSC(Bus State Controller)
 */
void InitializeBsc(void);

void
InitializeBsc(void)
{

	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = SDRAM, Area5 = Normal memory
	 * Area4 = Normal Memory
	 * Area6 = Normal memory
	 */
#if defined(SH3)
	_reg_write_2(SH3_BCR1, BSC_BCR1_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_BCR1, BSC_BCR1_VAL);
#endif

	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 16bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	_reg_write_2(SH_(BCR2), BSC_BCR2_VAL);

	/*
	 * Idle cycle number in transition area and read to write
	 * Area6 = 3, Area5 = 3, Area4 = 3, Area3 = 3, Area2 = 3
	 * Area1 = 3, Area0 = 3
	 */
#if defined(SH3)
	_reg_write_2(SH3_WCR1, BSC_WCR1_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_WCR1, BSC_WCR1_VAL);
#endif

	/*
	 * Wait cycle
	 * Area 6 = 6
	 * Area 5 = 2
	 * Area 4 = 10
	 * Area 3 = 3
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
#if defined(SH3)
	_reg_write_2(SH3_WCR2, BSC_WCR2_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_WCR2, BSC_WCR2_VAL);
#endif

#if defined(SH4) && defined(BSC_WCR3_VAL)
	_reg_write_4(SH4_WCR3, BSC_WCR3_VAL);
#endif

	/*
	 * RAS pre-charge = 2cycle, RAS-CAS delay = 3 cycle,
	 * write pre-charge=1cycle
	 * CAS before RAS refresh RAS assert time = 3 cycle
	 * Disable burst, Bus size=32bit, Column Address=10bit, Refresh ON
	 * CAS before RAS refresh ON, EDO DRAM
	 */
#if defined(SH3)
	_reg_write_2(SH3_MCR, BSC_MCR_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_MCR, BSC_MCR_VAL);
#endif

#if defined(BSC_SDMR2_VAL)
	_reg_write_1(BSC_SDMR2_VAL, 0);
#endif

#if defined(BSC_SDMR3_VAL)
#if !(defined(COMPUTEXEVB) && defined(SH7709A))
	_reg_write_1(BSC_SDMR3_VAL, 0);
#else
	_reg_write_2(0x1a000000, 0);	/* ADDSET */
	_reg_write_1(BSC_SDMR3_VAL, 0);
	_reg_write_2(0x18000000, 0);	/* ADDRST */
#endif /* !(COMPUTEXEVB && SH7709A) */
#endif /* BSC_SDMR3_VAL */

	/*
	 * PCMCIA Control Register
	 * OE/WE assert delay 3.5 cycle
	 * OE/WE negate-address delay 3.5 cycle
	 */
#ifdef BSC_PCR_VAL
	_reg_write_2(SH_(PCR), BSC_PCR_VAL);
#endif

	/*
	 * Refresh Timer Control/Status Register
	 * Disable interrupt by CMF, closk 1/16, Disable OVF interrupt
	 * Count Limit = 1024
	 * In following statement, the reason why high byte = 0xa5(a4 in RFCR)
	 * is the rule of SH3 in writing these register.
	 */
	_reg_write_2(SH_(RTCSR), BSC_RTCSR_VAL);

	/*
	 * Refresh Timer Counter
	 * Initialize to 0
	 */
#ifdef BSC_RTCNT_VAL
	_reg_write_2(SH_(RTCNT), BSC_RTCNT_VAL);
#endif

	/* set Refresh Time Constant Register */
	_reg_write_2(SH_(RTCOR), BSC_RTCOR_VAL);

	/* init Refresh Count Register */
#ifdef BSC_RFCR_VAL
	_reg_write_2(SH_(RFCR), BSC_RFCR_VAL);
#endif

	/*
	 * Clock Pulse Generator
	 */
	/* Set Clock mode (make internal clock double speed) */
	_reg_write_2(SH_(FRQCR), FRQCR_VAL);

	/*
	 * Cache
	 */
#ifndef CACHE_DISABLE
	/* Cache ON */
	_reg_write_4(SH_(CCR), 0x1);
#endif
}
#endif /* !DONT_INIT_BSC */


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

	/* mask all externel interrupt (XXX) */

	XLoadAndReset(buf_addr);
}

void
intc_intr(int ssr, int spc, int ssp)
{
	struct intc_intrhand *ih;
	struct clockframe cf;
	int s, evtcode;

	switch (cpu_product) {
	case CPU_PRODUCT_7708:
	case CPU_PRODUCT_7708S:
	case CPU_PRODUCT_7708R:
		evtcode = _reg_read_4(SH3_INTEVT);
		break;
	case CPU_PRODUCT_7709:
	case CPU_PRODUCT_7709A:
	case CPU_PRODUCT_7706:
		evtcode = _reg_read_4(SH7709_INTEVT2);
		break;
	case CPU_PRODUCT_7750:
	case CPU_PRODUCT_7750S:
	case CPU_PRODUCT_7750R:
	case CPU_PRODUCT_7751:
	case CPU_PRODUCT_7751R:
		evtcode = _reg_read_4(SH4_INTEVT);
		break;
	default:
#ifdef DIAGNOSTIC
		panic("intr_intc: cpu_product %d unhandled!", cpu_product);
#endif
		return;
	}

	ih = EVTCODE_IH(evtcode);
	KDASSERT(ih->ih_func);
	/* 
	 * On entry, all interrrupts are disabled,
	 * and exception is enabled for P3 access. (kernel stack is P3,
	 * SH3 may or may not cause TLB miss when access stack.)
	 * Enable higher level interrupt here.
	 */
	s = _cpu_intr_resume(ih->ih_level);

	switch (evtcode) {
	default:
		(*ih->ih_func)(ih->ih_arg);
		break;
	case SH_INTEVT_TMU0_TUNI0:
		cf.spc = spc;
		cf.ssr = ssr;
		cf.ssp = ssp;
		(*ih->ih_func)(&cf);
		break;
	case SH_INTEVT_NMI:
		printf("NMI ignored.\n");
		break;
	}
}

