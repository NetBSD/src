/*	$NetBSD: machdep.c,v 1.42.2.1 2014/08/20 00:02:58 tls Exp $	*/

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
 * Copyright (c) 1988 University of Utah.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.42.2.1 2014/08/20 00:02:58 tls Exp $");

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kcore.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <machine/yamon.h>

#include <mips/locore.h>
#include <mips/psl.h>

#include <evbmips/malta/autoconf.h>
#include <evbmips/malta/maltareg.h>
#include <evbmips/malta/maltavar.h>

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

int	comcnrate = 38400;	/* XXX should be config option */
#endif /* NCOM > 0 */


#define REGVAL(x)       *((volatile u_int32_t *)(MIPS_PHYS_TO_KSEG1((x))))

struct malta_config malta_configuration;

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int	netboot;		/* Are we netbooting? */

yamon_env_var *yamon_envp;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(int, char **, yamon_env_var *, u_long);

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int argc, char **argv, yamon_env_var *envp, u_long memsize)
{
	struct malta_config *mcp = &malta_configuration;
	uint8_t * const brkres = (uint8_t *)MIPS_PHYS_TO_KSEG1(MALTA_BRKRES);
	bus_space_handle_t sh;
	void *kernend;
	int freqok;

	extern char edata[], end[];

	CTASSERT((intptr_t)MIPS_PHYS_TO_KSEG1(MALTA_BRKRES) < 0);

	*brkres = 0;	/* Disable BREAK==reset on console */

	/* Get the propaganda in early! */
	led_display_str("NetBSD");

	/*
	 * Clear the BSS segment.
	 */
	kernend = (void *)mips_round_page(end);
	memset(edata, 0, (char *)kernend - edata);

	/* save the yamon environment pointer */
	yamon_envp = envp;

	/* Use YAMON callbacks for early console I/O */
	cn_tab = &yamon_promcd;

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 */
	mips_vector_init(NULL, false);

	/* set the VM page size */
	uvm_setpagesize();

	physmem = btoc(memsize);

	/*
	 * Use YAMON's CPU frequency if available.
	 */
	freqok = yamon_setcpufreq(1);

	gt_pci_init(&mcp->mc_pc, &mcp->mc_gt);
	malta_bus_io_init(&mcp->mc_iot, mcp);
	malta_bus_mem_init(&mcp->mc_memt, mcp);
	malta_dma_init(mcp);

	/*
	 * Calibrate the timer if YAMON failed to tell us.
	 */
	if (!freqok) {
		bus_space_map(&mcp->mc_iot, MALTA_RTCADR, 2, 0, &sh);
		malta_cal_timer(&mcp->mc_iot, sh);
		bus_space_unmap(&mcp->mc_iot, sh, 2);
	}

#if NCOM > 0
	/*
	 * Delay to allow firmware putchars to complete.
	 * FIFO depth * character time.
	 * character time = (1000000 / (defaultrate / 10))
	 */
	delay(160000000 / comcnrate);
	if (comcnattach(&mcp->mc_iot, MALTA_UART0ADR, comcnrate,
	    COM_FREQ, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
		panic("malta: unable to initialize serial console");
#else
	panic("malta: not configured to use serial console");
#endif /* NCOM > 0 */

	mem_clusters[0].start = 0;
	mem_clusters[0].size = ctob(physmem);
	mem_cluster_cnt = 1;

	cpu_setmodel("MIPS Malta Evaluation Board");

	/*
	 * XXX: check argv[0] - do something if "gdb"???
	 */

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
#ifndef _LP64
	for (int i = 1; i < argc; i++) {
		for (char *cp = argv[i]; *cp; cp++) {
			int howto;
			/* Ignore superfluous '-', if there is one */
			if (*cp == '-')
				continue;

			howto = 0;
			BOOT_FLAG(*cp, howto);
			if (! howto)
				printf("bootflag '%c' not recognised\n", *cp);
			else
				boothowto |= howto;
		}
	}
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t)kernend,
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if defined(DDB)
	if (boothowto & RB_KDB)
		Debugger();
#endif

#ifdef MULTIPROCESSOR
	/*
	 * We can never be running on more than one processor but we can dream.
	 */
	mips_fixup_exceptions(mips_fixup_zero_relative);
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

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{
	/*
	 * Do the common startup items.
	 */
	cpu_startup_common();

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	malta_configuration.mc_mallocsafe = 1;
}

int	waittime = -1;

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

	pmf_system_shutdown(boothowto);

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");
	yamon_exit(boothowto);
	printf("Oops, back from yamon_exit()\n\nResetting...");

	REGVAL(MALTA_SOFTRES) = MALTA_GORESET;

	/*
	 * Need a small delay here, otherwise we see the first few characters of
	 * the warning below.
	 */
	delay(80000);

	printf("WARNING: reset failed!\nSpinning...");

	for (;;)
		/* spin forever */ ;	/* XXX */
}
