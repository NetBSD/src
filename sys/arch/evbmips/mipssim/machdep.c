/* $NetBSD: machdep.c,v 1.4 2022/12/28 11:50:25 he Exp $ */

/*-
 * Copyright (c) 2001,2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Simon Burge.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.4 2022/12/28 11:50:25 he Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kcore.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/ic/comvar.h>
#include <dev/ic/ns16450reg.h>

#include <evbmips/mipssim/mipssimreg.h>
#include <evbmips/mipssim/mipssimvar.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>


#define	COMCNRATE	115200		/* not important, emulated device */
#define	COM_FREQ	1843200		/* not important, emulated device */

/*
 * QEMU/mipssim sets the CPU frequency to 6 MHz for 64-bit guests and
 * 12 MHz for 32-bit guests.
 */
#ifdef _LP64
#define	CPU_FREQ	6	/* MHz */
#else
#define	CPU_FREQ	12	/* MHz */
#endif

/* XXX move phys map decl to a general mips location */
/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

struct mipssim_config mipssim_configuration;

/* XXX move mem cluster decls to a general mips location */
int mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

/* XXX move mach_init() prototype to general mips header file */
void		mach_init(u_long, u_long, u_long, u_long);
static void	mach_init_memory(void);

/*
 * Provide a very simple output-only console driver so that we can
 * use printf() before the "real" console is initialised.
 */
static void uart_putc(dev_t, int);
static struct consdev early_console = {
	.cn_putc = uart_putc,
	.cn_dev = makedev(0, 0),
	.cn_pri = CN_DEAD
};

static void
uart_putc(dev_t dev, int c)
{
	volatile uint8_t *data = (void *)MIPS_PHYS_TO_KSEG1(
	    MIPSSIM_ISA_IO_BASE + MIPSSIM_UART0_ADDR + com_data);

	*data = (uint8_t)c;
	/* emulated UART, don't need to wait for output to drain */
}

static void
cal_timer(void)
{
	uint32_t cntfreq;

	cntfreq = curcpu()->ci_cpu_freq = CPU_FREQ * 1000 * 1000;

	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		cntfreq /= 2;

	curcpu()->ci_cctr_freq = cntfreq;
	curcpu()->ci_cycles_per_hz = (cntfreq + hz / 2) / hz;

	/* Compute number of cycles per 1us (1/MHz). 0.5MHz is for roundup. */
	curcpu()->ci_divisor_delay = ((cntfreq + 500000) / 1000000);
}

/*
 * 
 */
void
mach_init(u_long arg0, u_long arg1, u_long arg2, u_long arg3)
{
	struct mipssim_config *mcp = &mipssim_configuration;
	void *kernend;
	extern char edata[], end[];	/* XXX */

	kernend = (void *)mips_round_page(end);

	/* Zero BSS.  QEMU appears to leave some memory uninitialised. */
	memset(edata, 0, end - edata);

	/* enough of a console for printf() to work */
	cn_tab = &early_console;

	/* set CPU model info for sysctl_hw */
	cpu_setmodel("MIPSSIM");

	mips_vector_init(NULL, false);

	/* must be after CPU is identified in mips_vector_init() */
	cal_timer();

	uvm_md_init();

	/*
	 * Initialize bus space tags and bring up the main console.
	 */
	mipssim_bus_io_init(&mcp->mc_iot, mcp);
	mipssim_dma_init(mcp);

	if (comcnattach(&mcp->mc_iot, MIPSSIM_UART0_ADDR, COMCNRATE,
	    COM_FREQ, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
		panic("unable to initialize serial console");

	/*
	 * No way of passing arguments in mipssim.
	 */
	boothowto = RB_AUTOBOOT;
#ifdef KADB
	boothowto |= RB_KDB;
#endif

	mach_init_memory();

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

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * qemu for mipssim doesn't have a way of passing in the memory size, so
 * we probe.  lwp0 hasn't been set up this early, so use a dummy pcb to
 * allow badaddr() to function.  Limit total RAM to just before the IO
 * memory at MIPSSIM_ISA_IO_BASE.
 */
static void
mach_init_memory(void)
{
	struct lwp *l = curlwp;
	struct pcb dummypcb;
	psize_t memsize;
	size_t addr;
	uint32_t *memptr;
	extern char end[];	/* XXX */
#ifdef MIPS64
	size_t highaddr;
#endif

	l->l_addr = &dummypcb;
	memsize = roundup2(MIPS_KSEG0_TO_PHYS((uintptr_t)(end)), 1024 * 1024);

	for (addr = memsize; addr < MIPSSIM_ISA_IO_BASE; addr += 1024 * 1024) {
#ifdef MEM_DEBUG
		printf("test %zd MB\n", addr / 1024 * 1024);
#endif
		memptr = (void *)MIPS_PHYS_TO_KSEG1(addr - sizeof(*memptr));

		if (badaddr(memptr, sizeof(uint32_t)) < 0)
			break;

		memsize = addr;
	}
	l->l_addr = NULL;

	physmem = btoc(memsize);

	mem_clusters[0].start = PAGE_SIZE;
	mem_clusters[0].size = memsize - PAGE_SIZE;
	mem_cluster_cnt = 1;

#ifdef _LP64
	/* probe for more memory above ISA I/O "hole" */
	l->l_addr = &dummypcb;

	for (highaddr = addr = MIPSSIM_MORE_MEM_BASE;
	    addr < MIPSSIM_MORE_MEM_END;
	    addr += 1024 * 1024) {
		memptr = (void *)MIPS_PHYS_TO_XKPHYS(CCA_CACHEABLE,
						addr - sizeof(*memptr));
		if (badaddr(memptr, sizeof(uint32_t)) < 0)
			break;

		highaddr = addr;
#ifdef MEM_DEBUG
		printf("probed %zd MB\n", (addr - MIPSSIM_MORE_MEM_BASE)
					/ 1024 * 1024);
#endif
	}
	l->l_addr = NULL;

	if (highaddr != MIPSSIM_MORE_MEM_BASE) {
		mem_clusters[1].start = MIPSSIM_MORE_MEM_BASE;
		mem_clusters[1].size = highaddr - MIPSSIM_MORE_MEM_BASE;
		mem_cluster_cnt++;
		physmem += btoc(mem_clusters[1].size);
		memsize += mem_clusters[1].size;
	}
#endif /* _LP64 */

	printf("Memory size: 0x%" PRIxPSIZE " (%" PRIdPSIZE " MB)\n",
	    memsize, memsize / 1024 / 1024);
}

void
consinit(void)
{
	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
}

void
cpu_startup(void)
{

	/*
	 * Do the common startup items.
	 */
	cpu_startup_common();
}

/* XXX try to make this evbmips generic */
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

	printf("resetting...\n\n");
	__asm volatile("jr	%0" :: "r"(MIPS_RESET_EXC_VEC));
	printf("Oops, back from reset\n\nSpinning...");
	for (;;)
		/* spin forever */ ;	/* XXX */
	/*NOTREACHED*/
}
