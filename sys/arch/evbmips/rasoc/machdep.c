/*	$NetBSD: machdep.c,v 1.8.2.2 2017/12/03 11:36:10 jdolecek Exp $	*/

/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.8.2.2 2017/12/03 11:36:10 jdolecek Exp $");

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

/* structures we define/alloc for other files in the kernel */
struct vm_map *phys_map = NULL;

int mem_cluster_cnt = 0;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

void mach_init(void);

static inline uint32_t
sysctl_read(u_int offset)
{
	return *RA_IOREG_VADDR(RA_SYSCTL_BASE, offset);
}

static inline void
sysctl_write(u_int offset, uint32_t val)
{
	*RA_IOREG_VADDR(RA_SYSCTL_BASE, offset) = val;
}

static void
cal_timer(void)
{
	uint32_t cntfreq;

	cntfreq = curcpu()->ci_cpu_freq = RA_CLOCK_RATE;
	
	/* MIPS 4Kc CP0 counts every other clock */
	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		cntfreq /= 2;

	curcpu()->ci_cycles_per_hz = (cntfreq + hz / 2) / hz;

	/* Compute number of cycles per 1us (1/MHz). 0.5MHz is for roundup. */
	curcpu()->ci_divisor_delay = ((cntfreq + 500000) / 1000000);
}

void
mach_init(void)
{
	vaddr_t kernend;
	psize_t memsize;

	extern char kernel_text[];
	extern char edata[], end[];	/* From Linker */

	/* clear the BSS segment */
	kernend = mips_round_page(end);

	memset(edata, 0, kernend - (vaddr_t)edata);

#ifdef RALINK_CONSOLE_EARLY
	/*
	 * set up early console
	 *  cannot printf until sometime (?) in mips_vector_init
	 *  meanwhile can use the ra_console_putc primitive if necessary
	 */
	ralink_console_early();
#endif

	/* set CPU model info for sysctl_hw */
	uint32_t tmp1, tmp2;
	char id1[5], id2[5];
	tmp1 = sysctl_read(RA_SYSCTL_ID0);
	memcpy(id1, &tmp1, sizeof(tmp1));
	tmp2 = sysctl_read(RA_SYSCTL_ID1);
	memcpy(id2, &tmp2, sizeof(tmp2));
	id2[4] = id1[4] = '\0';
	if (id2[2] == ' ') {
		id2[2] = '\0';
	} else if (id2[3] == ' ') {
		id2[3] = '\0';
	} else {
		id2[4] = '\0';
	}
	cpu_setmodel("%s%s", id1, id2);

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Sets up mips_cpu_flags that may be queried by other
	 * functions called during startup.
	 * Also clears the I+D caches.
	 */
	mips_vector_init(NULL, false);

	/*
	 * Calibrate timers.
	 */
	cal_timer();

	uvm_md_init();

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
#ifdef KADB
	boothowto |= RB_KDB;
#endif

	/*
	 * Determine the memory size.
	 */
#if defined(MT7620) || defined(MT7628)
	memsize = 128 << 20;
#else
	memsize = *(volatile uint32_t *)
	    MIPS_PHYS_TO_KSEG1(RA_SYSCTL_BASE + RA_SYSCTL_CFG0);
	memsize = __SHIFTOUT(memsize, SYSCTL_CFG0_DRAM_SIZE);
	if (__predict_false(memsize == 0)) {
		memsize = 2 << 20;
	} else {
		memsize = 4 << (20 + memsize);
	}
#endif

	physmem = btoc(memsize);

	mem_clusters[mem_cluster_cnt].start = 0;
	mem_clusters[mem_cluster_cnt].size = memsize;
	mem_cluster_cnt++;

	/*
	 * Load the memory into the VM system
	 */
	mips_page_physload((vaddr_t)kernel_text, kernend,
	    mem_clusters, mem_cluster_cnt,
	    NULL, 0);

	/*
	 * Initialize message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Init mapping for u page(s) for proc0.
	 */
	mips_init_lwp0_uarea();

	/*
	 * Initialize busses.
	 */
	ra_bus_init();

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
cpu_startup(void)
{
	cpu_startup_common();
}

void
cpu_reboot(int howto, char *bootstr)
{
	static int waittime = -1;

	/* Take a snapshot before clobbering any registers. */
	savectx(lwp_getpcb(curlwp));

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

	sysctl_write(RA_SYSCTL_RST, 1);  /* SoC Reset */
	sysctl_write(RA_SYSCTL_RST, 0);

#if 0
	__asm volatile("jr	%0" :: "r"(MIPS_RESET_EXC_VEC));
#endif
	printf("Oops, back from reset\n\nSpinning...");
	for (;;)
		/* spin forever */ ;	/* XXX */
	/*NOTREACHED*/
}

#define NO_SECURITY_MAGIC	0x27051958
#define SERIAL_MAGIC		0x100000
int
ra_check_memo_reg(int key)
{
	uint32_t magic;

	/*
	 * These registers may be overwritten.  Keep the value around in case
	 * it is used later.  Bitmask 1 == security, 2 = serial
	 */
	static int keyvalue;

	switch (key) {
	case NO_SECURITY:
		magic = sysctl_read(RA_SYSCTL_MEMO0);
		if ((NO_SECURITY_MAGIC == magic) || ((keyvalue & 1) != 0)) {
			keyvalue |= 1;
			return 1;
		}
		return 0;
		break;

	case SERIAL_CONSOLE:
		magic = sysctl_read(RA_SYSCTL_MEMO1);
		if (magic == 0
		    || (SERIAL_MAGIC & magic) != 0
		    || (keyvalue & 2) != 0) {
			keyvalue |= 2;
			return 1;
		}
		return 0;
		break;

	default:
		return 0;
	}

}
