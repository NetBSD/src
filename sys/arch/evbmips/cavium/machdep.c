/*	$NetBSD: machdep.c,v 1.4.2.3 2015/09/22 12:05:41 skrll Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	@(#)machdep.c   8.3 (Berkeley) 1/12/94
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)machdep.c   8.3 (Berkeley) 1/12/94
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.4.2.3 2015/09/22 12:05:41 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <machine/psl.h>
#include <machine/locore.h>

#include <mips/cavium/autoconf.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/include/bootbusvar.h>

#include <mips/cavium/dev/octeon_uartreg.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_gpioreg.h>

#include <evbmips/cavium/octeon_uboot.h>

static void	mach_init_bss(void);
static void	mach_init_vector(void);
static void	mach_init_bus_space(void);
static void	mach_init_console(void);
static void	mach_init_memory(u_quad_t);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
int	comcnrate = 115200;	/* XXX should be config option */
#endif /* NCOM > 0 */

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int	netboot;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	mach_init(uint64_t, uint64_t, uint64_t, uint64_t);

struct octeon_config octeon_configuration;
struct octeon_btinfo octeon_btinfo;

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	uint64_t btinfo_paddr;
	u_quad_t memsize;
	int corefreq;

	mach_init_bss();

	KASSERT(MIPS_XKPHYS_P(arg3));
	btinfo_paddr = mips64_ld_a64(arg3 + OCTEON_BTINFO_PADDR_OFFSET);

	/* Should be in first 256MB segment */
	KASSERT(btinfo_paddr < 256 * 1024 * 1024);
	memcpy(&octeon_btinfo,
	    (struct octeon_btinfo *)MIPS_PHYS_TO_KSEG0(btinfo_paddr),
	    sizeof(octeon_btinfo));

	corefreq = octeon_btinfo.obt_eclock_hz;
	memsize = octeon_btinfo.obt_dram_size * 1024 * 1024;

	octeon_cal_timer(corefreq);

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case 0: cpu_setmodel("Cavium Octeon CN38XX/CN36XX"); break;
	case 1: cpu_setmodel("Cavium Octeon CN31XX/CN3020"); break;
	case 2: cpu_setmodel("Cavium Octeon CN3005/CN3010"); break;
	case 3: cpu_setmodel("Cavium Octeon CN58XX"); break;
	case 4: cpu_setmodel("Cavium Octeon CN5[4-7]XX"); break;
	case 6: cpu_setmodel("Cavium Octeon CN50XX"); break;
	case 7: cpu_setmodel("Cavium Octeon CN52XX"); break;
	default: cpu_setmodel("Cavium Octeon"); break;
	}

	mach_init_vector();

	/* set the VM page size */
	uvm_setpagesize();

	mach_init_bus_space();

	mach_init_console();

	mach_init_memory(memsize);

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

	boothowto = RB_AUTOBOOT;
	boothowto |= AB_VERBOSE;

#if defined(DDB)
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
}

void
mach_init_bss(void)
{
	extern char edata[], end[];

	/*
	 * Clear the BSS segment.
	 */
	memset(edata, 0, mips_round_page(end) - (uintptr_t)edata);
}

void
mach_init_vector(void)
{

	/* Make sure exception base at 0 (MIPS_COP_0_EBASE) */
	__asm __volatile("mtc0 %0, $15, 1" : : "r"(0x80000000) );

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 */
	mips_vector_init(NULL, true);
}

void
mach_init_bus_space(void)
{
	struct octeon_config *mcp = &octeon_configuration;

	octeon_dma_init(mcp);

	iobus_bootstrap(mcp);
	bootbus_bootstrap(mcp);
}

void
mach_init_console(void)
{
#if NCOM > 0
	struct octeon_config *mcp = &octeon_configuration;
	int status;
	extern int octeon_uart_com_cnattach(bus_space_tag_t, int, int);

	/*
	 * Delay to allow firmware putchars to complete.
	 * FIFO depth * character time.
	 * character time = (1000000 / (defaultrate / 10))
	 */
	delay(640000000 / comcnrate);

	status = octeon_uart_com_cnattach(
		&mcp->mc_iobus_bust,
		0,	/* XXX port 0 */
		comcnrate);
	if (status != 0)
		panic("can't initialize console!");	/* XXX print to nowhere! */
#else
	panic("octeon: not configured to use serial console");
#endif /* NCOM > 0 */
}

void
mach_init_memory(u_quad_t memsize)
{
	extern char kernel_text[];
	extern char end[];

	physmem = btoc(memsize);

	if (memsize <= 256 * 1024 * 1024) {
		mem_clusters[0].start = 0;
		mem_clusters[0].size = memsize;
		mem_cluster_cnt = 1;
	} else if (memsize <= 512 * 1024 * 1024) {
		mem_clusters[0].start = 0;
		mem_clusters[0].size = 256 * 1024 * 1024;
		mem_clusters[1].start = 0x410000000ULL;
		mem_clusters[1].size = memsize - 256 * 1024 * 1024;
		mem_cluster_cnt = 2;
	} else {
		mem_clusters[0].start = 0;
		mem_clusters[0].size = 256 * 1024 * 1024;
		mem_clusters[1].start = 0x20000000;
		mem_clusters[1].size = memsize - 512 * 1024 * 1024;
		mem_clusters[2].start = 0x410000000ULL;
		mem_clusters[2].size = 256 * 1024 * 1024;
		mem_cluster_cnt = 3;
	}


#ifdef MULTIPROCESSOR
	const u_int cores = mipsNN_cp0_ebase_read() & MIPS_EBASE_CPUNUM;
	mem_clusters[0].start = cores * 4096;
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	mips_page_physload(mips_trunc_page(kernel_text), mips_round_page(end),
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();
}

/*
 * cpu_startup
 * cpu_reboot
 */

int	waittime = -1;

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{
#ifdef MULTIPROCESSOR
	// Create a kcpuset so we can see on which CPUs the kernel was started.
	kcpuset_create(&cpus_booted, true);
#endif

	/*
	 * Do the common startup items.
	 */
	cpu_startup_common();

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	octeon_configuration.mc_mallocsafe = 1;
}

void
cpu_reboot(int howto, char *bootstr)
{

	/* Take a snapshot before clobbering any registers. */
	savectx(curpcb);

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && (waittime < 0)) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
	}

	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");

	/*
	 * Need a small delay here, otherwise we see the first few characters of
	 * the warning below.
	 */
	delay(80000);

	/* initiate chip soft-reset */
	uint64_t fuse = octeon_read_csr(CIU_FUSE);
	octeon_write_csr(CIU_SOFT_BIST, fuse);
	octeon_read_csr(CIU_SOFT_RST);
	octeon_write_csr(CIU_SOFT_RST, fuse);

	delay(1000000);

	printf("WARNING: reset failed!\nSpinning...");

	for (;;)
		/* spin forever */ ;	/* XXX */
}
