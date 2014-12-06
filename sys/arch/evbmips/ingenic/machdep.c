/*	$NetBSD: machdep.c,v 1.2 2014/12/06 14:30:11 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.2 2014/12/06 14:30:11 macallan Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/cpu.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <mips/ingenic/ingenic_regs.h>
#include <mips/ingenic/ingenic_var.h>

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int maxmem;			/* max memory per process */

int mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

void	mach_init(void); /* XXX */
void	ingenic_reset(void);

void	ingenic_putchar_init(void);
void	ingenic_puts(const char *);
void	ingenic_com_cnattach(void);

static void
cal_timer(void)
{
	uint32_t	cntfreq;
	volatile uint32_t junk;

	/*
	 * The manual seems to imply that EXCCLK is 12MHz, although in real
	 * life it appears to be 48MHz. Either way, we want a 12MHz counter.
	 */
	curcpu()->ci_cpu_freq = 1200000000;	/* for now */
	cntfreq = 12000000;	/* EXTCLK / 4 */
	
	curcpu()->ci_cctr_freq = cntfreq;
	curcpu()->ci_cycles_per_hz = (cntfreq + hz / 2) / hz;

	/* Compute number of cycles per 1us (1/MHz). 0.5MHz is for roundup. */
	curcpu()->ci_divisor_delay = ((cntfreq + 500000) / 1000000);

	/* actually start the counter now */
	/* stop OS timer */
	writereg(JZ_TC_TECR, TESR_OST);
	/* zero everything */
	writereg(JZ_OST_CTRL, 0);
	writereg(JZ_OST_CNT_LO, 0);
	writereg(JZ_OST_CNT_HI, 0);
	writereg(JZ_OST_DATA, 0xffffffff);
	/* use EXTCLK, don't reset */
	writereg(JZ_OST_CTRL, OSTC_EXT_EN | OSTC_MODE | OSTC_DIV_4);
	/* start the timer */
	writereg(JZ_TC_TESR, TESR_OST);
	/* make sure the timer actually runs */
	junk = readreg(JZ_OST_CNT_LO);
	do {} while (junk == readreg(JZ_OST_CNT_LO));
}

void
mach_init(void)
{
	void *kernend;
	uint32_t memsize;
	extern char edata[], end[];	/* XXX */

	/* clear the BSS segment */
	kernend = (void *)mips_round_page(end);

	memset(edata, 0, (char *)kernend - edata);

	/* setup early console */
	ingenic_putchar_init();

	/* set CPU model info for sysctl_hw */
	cpu_setmodel("Ingenic XBurst");
	mips_vector_init(NULL, false);
	cal_timer();
	uvm_setpagesize();
	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
#ifdef KADB
	boothowto |= RB_KDB;
#endif

	/*
	 * Determine the memory size.
	 *
	 * Note: Reserve the first page!  That's where the trap
	 * vectors are located.
	 */
	memsize = 0x40000000;

	printf("Memory size: 0x%08x\n", memsize);
	physmem = btoc(memsize);

	/* XXX this is CI20 specific */
	mem_clusters[0].start = PAGE_SIZE;
	mem_clusters[0].size = 0x10000000 - PAGE_SIZE;
	mem_clusters[1].start = 0x30000000;
	mem_clusters[1].size = 0x30000000;
	mem_cluster_cnt = 2;

	/*
	 * Load the available pages into the VM system.
	 */
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t)kernend,
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

	apbus_init();
	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
consinit(void)
{
	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
	ingenic_com_cnattach();
}

void
cpu_startup(void)
{
	char pbuf[9];
	vaddr_t minaddr, maxaddr;
#ifdef DEBUG
	extern int pmapdebug;		/* XXX */
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_getmodel());
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.
	 */

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

void
cpu_reboot(int howto, char *bootstr)
{
	static int waittime = -1;

	/* Take a snapshot before clobbering any registers. */
	savectx(curpcb);

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT)
		howto |= RB_HALT;

	boothowto = howto;

	/* If system is cold, just halt. */
	if (cold) {
		boothowto |= RB_HALT;
		goto haltsys;
	}

	if ((boothowto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;

		/*
		 * Synchronize the disks....
		 */
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	if (boothowto & RB_DUMP)
		dumpsys();

haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

#if 0
	if ((boothowto & RB_POWERDOWN) == RB_POWERDOWN)
		if (board && board->ab_poweroff)
			board->ab_poweroff();
#endif

	/*
	 * Firmware may autoboot (depending on settings), and we cannot pass
	 * flags to it (at least I haven't figured out how to yet), so
	 * we "pseudo-halt" now.
	 */
	if (boothowto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("reseting board...\n\n");
	mips_icache_sync_all();
	mips_dcache_wbinv_all();
	ingenic_reset();
	__asm volatile("jr	%0" :: "r"(MIPS_RESET_EXC_VEC));
	printf("Oops, back from reset\n\nSpinning...");
	for (;;)
		/* spin forever */ ;	/* XXX */
	/*NOTREACHED*/
}

void
ingenic_reset(void)
{
	/*
	 * for now, provoke a watchdog reset in about a second, so UART buffers
	 * have a fighting chance to flush before we pull the plug
	 */
	writereg(JZ_WDOG_TCER, 0);	/* disable watchdog */
	writereg(JZ_WDOG_TCNT, 0);	/* reset counter */
	writereg(JZ_WDOG_TDR, 128);	/* wait for ~1s */
	writereg(JZ_WDOG_TCSR, TCSR_RTC_EN | TCSR_DIV_256);
	writereg(JZ_WDOG_TCER, TCER_ENABLE);	/* fire! */	
}
