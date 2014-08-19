/*	$NetBSD: machdep.c,v 1.16.12.1 2014/08/20 00:02:58 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.16.12.1 2014/08/20 00:02:58 tls Exp $");

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

#include <mips/locore.h>
#include <mips/psl.h>

#include <mips/bonito/bonitoreg.h>
#include <evbmips/gdium/gdiumvar.h>

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

int	comcnrate = 38400;	/* XXX should be config option */
#endif /* NCOM > 0 */

struct gdium_config gdium_configuration = {
	.gc_bonito = {
		.bc_adbase = 11,	/* magic */
	},
};

/* For sysctl_hw. */

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int	netboot;		/* Are we netbooting? */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(int, char **, char **, void *);

/*
 * For some reason, PMON doesn't assign a real address to the Ralink's BAR.
 * So we have to do it.
 */
static void
gdium_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	const pcitag_t high_dev = pci_make_tag(pba->pba_pc, 0, 17, 1);
	const pcitag_t ralink_dev = pci_make_tag(pba->pba_pc, 0, 13, 0);
	bus_size_t high_size, ralink_size;
	pcireg_t v;

	/*
	 * Get the highest PCI addr used from the last PCI dev.
	 */
	v = pci_conf_read(pba->pba_pc, high_dev, PCI_MAPREG_START);
	v &= PCI_MAPREG_MEM_ADDR_MASK;

	/*
	 * Get the sizes of the map registers.
	 */
	pci_mapreg_info(pba->pba_pc, high_dev, PCI_MAPREG_START,
	    PCI_MAPREG_MEM_TYPE_32BIT, NULL, &high_size, NULL);
	pci_mapreg_info(pba->pba_pc, ralink_dev, PCI_MAPREG_START,
	    PCI_MAPREG_MEM_TYPE_32BIT, NULL, &ralink_size, NULL);

	/*
	 * Position the ralink register space after the last device.
	 */
	v = (v + high_size + ralink_size - 1) & ~(ralink_size - 1);

	/*
	 * Set the mapreg.
	 */
	pci_conf_write(pba->pba_pc, ralink_dev, PCI_MAPREG_START, v);

#if 0
	/*
	 * Why does linux do this?
	 */
	for (int dev = 15; dev <= 17; dev +=2) {
		for (int func = 0; func <= 1; func++) {
			pcitag_t usb_dev = pci_make_tag(pba->pba_pc, 0, dev, func);
			v = pci_conf_read(pba->pba_pc, usb_dev, 0xe0);
			v |= 3;
			pci_conf_write(pba->pba_pc, usb_dev, 0xe0, v);
		}
	}
#endif
}

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int argc, char **argv, char **envp32, void *callvec)
{
	struct gdium_config *gc = &gdium_configuration;
	void *kernend;
#ifdef NOTYET
	char *cp;
	int howto;
#endif
	int i;
	psize_t memsize;
	char *envp[128];
	int32_t *eptrs = (int32_t *)envp32; 
	extern char edata[], end[];

	/*
	 * Clear the BSS segment.
	 */
	kernend = (void *)mips_round_page(end);
	memset(edata, 0, (char *)kernend - edata);

	/*
	 * the pointer array in envp32 is 32bit - we need to sign extend them
	 * and put them into a list of actual pointers
	 * Only strictly necessary on LP64 but it doesn't hurt in LP32, runs only
	 * once at startup and I'd rather not pollute this file with another
	 * #ifdef orgy
	 */
	i = 0;
	while ((eptrs[i] != 0) && (i < 128)) {
		envp[i] = (char *)(intptr_t)eptrs[i];	/* sign extend */
		i++;
	}

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

	memsize = 256*1024*1024;
	physmem = btoc(memsize);

	bonito_pci_init(&gc->gc_pc, &gc->gc_bonito);
	/*
	 * Override the null bonito_pci_attach_hook with our own to we can
	 * fix the ralink (device 13).
	 */
	gc->gc_pc.pc_attach_hook = gdium_pci_attach_hook;
	gdium_bus_io_init(&gc->gc_iot, gc);
	gdium_bus_mem_init(&gc->gc_memt, gc);
	gdium_dma_init(gc);
	gdium_cnattach(gc);

	/*
	 * Disable the 2nd PCI window since we don't need it.
	 */
	mips3_sd((uint64_t *)MIPS_PHYS_TO_KSEG1(BONITO_REGBASE + 0x158), 0xe);
	pci_conf_write(&gc->gc_pc, pci_make_tag(&gc->gc_pc, 0, 0, 0), 18, 0);

	/*
	 * Get the timer from PMON.
	 */
	for (i = 0; envp[i] != NULL; i++) {
		if (!strncmp(envp[i], "cpuclock=", 9)) {
			curcpu()->ci_cpu_freq =
			    strtoul(&envp[i][9], NULL, 10);
			break;
		}
	}

	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq /= 2;

	/* Compute the number of ticks for hz. */
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;

	/* Compute the delay divisor. */
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + 500000) / 1000000);

	/*
	 * Get correct cpu frequency if the CPU runs at twice the
	 * external/cp0-count frequency.
	 */
	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq *= 2;

#ifdef DEBUG
	printf("Timer calibration: %lu cycles/sec\n",
	    curcpu()->ci_cpu_freq);
#endif

#if NCOM > 0
	/*
	 * Delay to allow firmware putchars to complete.
	 * FIFO depth * character time.
	 * character time = (1000000 / (defaultrate / 10))
	 */
	delay(160000000 / comcnrate);
	if (comcnattach(&gc->gc_iot, MALTA_UART0ADR, comcnrate,
	    COM_FREQ, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
		panic("malta: unable to initialize serial console");
#endif /* NCOM > 0 */

	mem_clusters[0].start = 0;
	mem_clusters[0].size = ctob(physmem);
	mem_cluster_cnt = 1;

	cpu_setmodel("Gdium Liberty 1000");

	/*
	 * XXX: check argv[0] - do something if "gdb"???
	 */

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
#ifdef NOTYET
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
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
	 *  Do the common startup items.
	 */
	cpu_startup_common();

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	gdium_configuration.gc_mallocsafe = 1;

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

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		/*
		 * Turning on GPIO1 as output will cause a powerdown.
		 */
		REGVAL(BONITO_GPIODATA) |= 2;
		REGVAL(BONITO_GPIOIE) &= ~2;
	}

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");

	/*
	 * Turning off GPIO2 as output will cause a reset.
	 */
	REGVAL(BONITO_GPIODATA) &= ~4;
	REGVAL(BONITO_GPIOIE) &= ~4;

	__asm__ __volatile__ (
		"\t.long 0x3c02bfc0\n"
		"\t.long 0x00400008\n"
	    ::: "v0");
}
