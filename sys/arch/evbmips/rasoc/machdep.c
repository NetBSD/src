/*	$NetBSD: machdep.c,v 1.1.2.2 2012/02/14 01:25:52 matt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1.2.2 2012/02/14 01:25:52 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

/* structures we define/alloc for other files in the kernel */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;
struct cpu_info cpu_info_store;
int physmem;		/* # pages of physical memory */
u_int mem_cluster_cnt = 0;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

/* structures others define for us */
extern struct user *proc0paddr;

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

#ifdef RA_CONSOLE_EARLY
	/*
	 * set up early console
	 *  cannot printf until sometime (?) in mips_vector_init
	 *  meanwhile can use the ra_console_putc primitive if necessary
	 */
	ra_console_early();
#endif

	/* set CPU model info for sysctl_hw */
	uint32_t tmp;
	tmp = sysctl_read(RA_SYSCTL_ID0);
	memcpy(&cpu_model[0], &tmp, 4);
	tmp = sysctl_read(RA_SYSCTL_ID1);
	memcpy(&cpu_model[4], &tmp, 4);
	cpu_model[9] = 0;

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

	/*
	 * Set the VM page size.
	 */
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
	memsize = RTMEMSIZE * 1024 * 1024;

	physmem = btoc(memsize);

	mem_clusters[mem_cluster_cnt].start = PAGE_SIZE;
	mem_clusters[mem_cluster_cnt].size =
		memsize - mem_clusters[mem_cluster_cnt].start;
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

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(0, 0, 0);
#endif

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
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
	printf("Boot processor: %s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), RTMEMSIZE * 1024 * 1024);
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate a submap for physio
	 */
	minaddr = 0;
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
	if (curproc)
		savectx((struct pcb *)curpcb);

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
		if (((SERIAL_MAGIC & magic) != 0) || ((keyvalue & 2) != 0)) {
			keyvalue |= 2;
			return 1;
		}
		return 0;
		break;

	default:
		return 0;
	}

}
