/*	$NetBSD: machdep.c,v 1.51.2.1 2012/04/17 00:05:52 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.51.2.1 2012/04/17 00:05:52 yamt Exp $");

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h" 
#include "opt_algor_p6032.h"

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "opt_memsize.h"
#include "opt_ethaddr.h"

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/lwp.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <mips/locore.h>
#include <mips/pcb.h>

#ifdef DDB
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <machine/pmon.h>
#include <algor/autoconf.h>

#include <algor/pci/vtpbcvar.h>

#include "ksyms.h"

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

int	comcnrate = TTYDEF_SPEED;
#endif /* NCOM > 0 */

#if defined(ALGOR_P4032) + \
    defined(ALGOR_P5064) + \
    defined(ALGOR_P6032) + \
    0 != 1
#error Must configure exactly one platform.
#endif

#ifdef ALGOR_P4032
#include <algor/algor/algor_p4032reg.h> 
#include <algor/algor/algor_p4032var.h> 
struct p4032_config p4032_configuration;
#endif

#ifdef ALGOR_P5064
#include <algor/algor/algor_p5064reg.h>
#include <algor/algor/algor_p5064var.h>
struct p5064_config p5064_configuration;
#endif

#ifdef ALGOR_P6032
#include <algor/algor/algor_p6032reg.h>
#include <algor/algor/algor_p6032var.h>
struct p6032_config p6032_configuration;
#endif 

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int	maxmem;			/* max memory per process */

int	mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

char	algor_ethaddr[ETHER_ADDR_LEN];

void	mach_init(int, char *[], char *[]);	/* XXX */

int	cpuspeed = 150;		/* XXX XXX XXX */

void
mach_init(int argc, char *argv[], char *envp[])
{
	extern char kernel_text[], edata[], end[];
	vaddr_t kernstart, kernend;
	vsize_t size;
	const char *cp;
	char *cp0;
	size_t i;

	/*
	 * First, find the start and end of the kernel and clear
	 * the BSS segment.  Account for a bit of space for the
	 * bootstrap stack.
	 */
	led_display('b', 's', 's', ' ');
	kernstart = (vaddr_t) mips_trunc_page(kernel_text) - 2 * NBPG;
	kernend   = (vaddr_t) mips_round_page(end);
	memset(edata, 0, kernend - (vaddr_t)edata);

	/*
	 * Copy the exception-dispatch code down to the exception vector.
	 * Initialize the locore function vector.  Clear out the I- and
	 * D-caches.
	 *
	 * We can no longer call into PMON after this.
	 */
	led_display('v', 'e', 'c', 'i');
	mips_vector_init(NULL, false);

	/*
	 * Initialize PAGE_SIZE-dependent variables.
	 */
	led_display('p', 'g', 's', 'z');
	uvm_setpagesize();

	/*
	 * Initialize bus space tags and bring up the console.
	 */
#if defined(ALGOR_P4032)
	    {
		struct p4032_config *acp = &p4032_configuration;
		struct vtpbc_config *vt = &vtpbc_configuration; 
		bus_space_handle_t sh;

		strcpy(cpu_model, "Algorithmics P-4032");

		vt->vt_addr = MIPS_PHYS_TO_KSEG1(P4032_V962PBC);
		vt->vt_cfgbase = MIPS_PHYS_TO_KSEG1(P4032_PCICFG);
		vt->vt_adbase = 11;

		led_display('v', '9', '6', '2');
		vtpbc_init(&acp->ac_pc, vt);

		led_display('l', 'i', 'o', ' ');
		algor_p4032loc_bus_io_init(&acp->ac_lociot, acp);

		led_display('i', 'o', ' ', ' ');
		algor_p4032_bus_io_init(&acp->ac_iot, acp);

		led_display('m', 'e', 'm', ' ');
		algor_p4032_bus_mem_init(&acp->ac_memt, acp);

		led_display('d', 'm', 'a', ' ');
		algor_p4032_dma_init(acp);
#if NCOM > 0
		/*
		 * Delay to allow firmware putchars to complete.
		 * FIFO depth * character time.
		 * character time = (1000000 / (defaultrate / 10))
		 */
		led_display('c', 'o', 'n', 's');
		DELAY(160000000 / comcnrate);
		if (comcnattach(&acp->ac_lociot, P4032_COM1, comcnrate,
		    COM_FREQ, COM_TYPE_NORMAL,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
			panic("p4032: unable to initialize serial console");
#else
		panic("p4032: not configured to use serial console");
#endif /* NCOM > 0 */

		led_display('h', 'z', ' ', ' ');
		bus_space_map(&acp->ac_lociot, P4032_RTC, 2, 0, &sh);
		algor_p4032_cal_timer(&acp->ac_lociot, sh);
		bus_space_unmap(&acp->ac_lociot, sh, 2);
	    }
#elif defined(ALGOR_P5064)
	    {
		struct p5064_config *acp = &p5064_configuration;
		struct vtpbc_config *vt = &vtpbc_configuration;
		bus_space_handle_t sh;

		strcpy(cpu_model, "Algorithmics P-5064");

		vt->vt_addr = MIPS_PHYS_TO_KSEG1(P5064_V360EPC);
		vt->vt_cfgbase = MIPS_PHYS_TO_KSEG1(P5064_PCICFG);
		vt->vt_adbase = 24;

		led_display('v', '3', '6', '0');
		vtpbc_init(&acp->ac_pc, vt);

		led_display('i', 'o', ' ', ' ');
		algor_p5064_bus_io_init(&acp->ac_iot, acp);

		led_display('m', 'e', 'm', ' ');
		algor_p5064_bus_mem_init(&acp->ac_memt, acp);

		led_display('d', 'm', 'a', ' ');
		algor_p5064_dma_init(acp);
#if NCOM > 0
		/*
		 * Delay to allow firmware putchars to complete.
		 * FIFO depth * character time.
		 * character time = (1000000 / (defaultrate / 10))
		 */
		led_display('c', 'o', 'n', 's');
		DELAY(160000000 / comcnrate);  
		if (comcnattach(&acp->ac_iot, 0x3f8, comcnrate,
		    COM_FREQ, COM_TYPE_NORMAL,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
			panic("p5064: unable to initialize serial console");
#else
		panic("p5064: not configured to use serial console");
#endif /* NCOM > 0 */

		led_display('h', 'z', ' ', ' ');
		bus_space_map(&acp->ac_iot, 0x70, 2, 0, &sh);
		algor_p5064_cal_timer(&acp->ac_iot, sh);
		bus_space_unmap(&acp->ac_iot, sh, 2);
	    }
#elif defined(ALGOR_P6032)
	    {
		struct p6032_config *acp = &p6032_configuration;
		struct bonito_config *bc = &acp->ac_bonito;
		bus_space_handle_t sh;

		strcpy(cpu_model, "Algorithmics P-6032");

		bc->bc_adbase = 11;
		
		led_display('b','n','t','o');
		bonito_pci_init(&acp->ac_pc, bc);

		led_display('i','o',' ',' ');
		algor_p6032_bus_io_init(&acp->ac_iot, acp);

		led_display('m','e','m',' ');
		algor_p6032_bus_mem_init(&acp->ac_memt, acp);

		led_display('d','m','a',' ');
		algor_p6032_dma_init(acp);
#if NCOM > 0
		/*
		 * Delay to allow firmware putchars to complete.
		 * FIFO depth * character time.
		 * character time = (1000000 / (defaultrate / 10))
		 */
		led_display('c','o','n','s');
		DELAY(160000000 / comcnrate);
		if (comcnattach(&acp->ac_iot, 0x3f8, comcnrate,
		    COM_FREQ, COM_TYPE_NORMAL,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
			panic("p6032: unable to initialize serial console");
#else
		panic("p6032: not configured to use serial console");
#endif /* NCOM > 0 */

		led_display('h','z',' ',' ');
		bus_space_map(&acp->ac_iot, 0x70, 2, 0, &sh);
		algor_p6032_cal_timer(&acp->ac_iot, sh);
		bus_space_unmap(&acp->ac_iot, sh, 2);
	    }
#endif /* ALGOR_P4032 || ALGOR_P5064 || ALGOR_P6032 */

	/*
	 * The Algorithmics boards have PMON firmware; set up our
	 * PMON state.
	 */
	led_display('p', 'm', 'o', 'n');
	pmon_init(envp);

	/*
	 * Get the Ethernet address of the on-board Ethernet.
	 */
#if defined(ETHADDR)
	cp = ETHADDR;
#else
	cp = pmon_getenv("ethaddr");
#endif
	if (cp != NULL) {
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			algor_ethaddr[i] = strtoul(cp, &cp0, 16);
			cp = cp0 + 1;
		}
	}

	/*
	 * Get the boot options.
	 */
	led_display('b', 'o', 'p', 't');
	boothowto = 0;
	if (argc > 1) {
#ifdef _LP64
		cp = (void *)(intptr_t)((int32_t *)argv)[1];
#else
		cp = argv[1];
#endif
		for (; cp != NULL && *cp != '\0'; cp++) {
			switch (*cp) {
#if defined(KGDB) || defined(DDB)
			case 'd':	/* break into kernel debugger */
				boothowto |= RB_KDB;
				break;
#endif

			case 'h':	/* always halt, never reboot */
				boothowto |= RB_HALT;
				break;

			case 'n':	/* askname */
				boothowto |= RB_ASKNAME;
				break;

			case 's':	/* single-user mode */
				boothowto |= RB_SINGLE;
				break;

			case 'q':	/* quiet boot */
				boothowto |= AB_QUIET;
				break;

			case 'v':	/* verbose boot */
				boothowto |= AB_VERBOSE;
				break;

			case '-':
				/*
				 * Just ignore this.  It's not required,
				 * but it's common for it to be passed
				 * regardless.
				 */
				break;

			default:
				printf("Unrecognized boto flag '%c'.\n", *cp);
				break;
			}
		}
	}

	/*
	 * Determine the memory size.  Use the `memsize' PMON
	 * variable.  If that's not available, panic.
	 *
	 * Note: Reserve the first page!  That's where the trap
	 * vectors are located.
	 */
#if defined(MEMSIZE)
	size = MEMSIZE;
#else
	if ((cp = pmon_getenv("memsize")) != NULL)
		size = strtoul(cp, NULL, 0);
	else {
		printf("FATAL: `memsize' PMON variable not set.  Set it to\n");
		printf("       the amount of memory (in MB) and try again.\n");
		printf("       Or, build a kernel with the `MEMSIZE' "
		    "option.\n");
		panic("algor_init");
	}
#endif /* MEMSIZE */
	/*
	 * Deal with 2 different conventions of the contents
	 * of the memsize variable -- if it's > 1024, assume
	 * it's already in bytes, not megabytes.
	 */
	if (size < 1024) {
		printf("Memory size: %#"PRIxVSIZE" (%"PRIxVSIZE")\n",
		    size * 1024 * 1024, size);
		size *= 1024 * 1024;
	} else
		printf("Memory size: %#"PRIxVSIZE"\n", size);

	mem_clusters[mem_cluster_cnt].start = PAGE_SIZE;
	mem_clusters[mem_cluster_cnt].size =
	    size - mem_clusters[mem_cluster_cnt].start;
	mem_cluster_cnt++;

	/*
	 * Load the physical memory clusters into the VM system.
	 */
	led_display('v', 'm', 'p', 'g');
	for (i = 0; i < mem_cluster_cnt; i++) {
		physmem += atop(mem_clusters[i].size);
	}
	if (physmem == 0)
		panic("can't happen: system seems to have no memory!");
	maxmem = physmem;

	static const struct mips_vmfreelist isadma = {
		.fl_start = 8*1024*1024,
		.fl_end = 16*1024*1024,
		.fl_freelist = VM_FREELIST_ISADMA,
	};
	mips_page_physload(kernstart, kernend,
	    mem_clusters, mem_cluster_cnt, &isadma, 1);

	/*
	 * Initialize message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	led_display('p', 'm', 'a', 'p');
	pmap_bootstrap();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	led_display('u', 's', 'p', 'c');
	mips_init_lwp0_uarea();

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
	if (boothowto & RB_KDB) {
#if defined(DDB)
		Debugger();
#endif
	}
}

void
consinit(void)
{

	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
	led_display('N', 'B', 'S', 'D');
}

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ptoa(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
#if defined(ALGOR_P4032)
	    {
		struct p4032_config *acp = &p4032_configuration;

		acp->ac_mallocsafe = 1;
	    }
#elif defined(ALGOR_P5064)
	    {
		struct p5064_config *acp = &p5064_configuration;

		acp->ac_mallocsafe = 1;
	    }
#elif defined(ALGOR_P6032)
	    {
		struct p6032_config *acp = &p6032_configuration;

		acp->ac_mallocsafe = 1;
	    }
#endif

	minaddr = 0;

	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocate via the pool allocator, and we use KSEG0 to
	 * map those pages.
	 */

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

int	waittime = -1;
struct pcb dumppcb;

void
cpu_reboot(int howto, char *bootstr)
{
	int tmp;

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
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	(void) splhigh();

	if (boothowto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if (boothowto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to return to the monitor.\n\n");
		led_display('h','a','l','t');
		cnpollc(1);
		(void) cngetc();
		cnpollc(0);
	}

	printf("Returning to the monitor...\n\n");
	led_display('r', 'v', 'e', 'c');
	/* Jump to the reset vector. */
	__asm volatile("li %0, 0xbfc00000; jr %0; nop"
		: "=r" (tmp)
		: /* no inputs */
		: "memory");
	led_display('R', 'S', 'T', 'F');
	for (;;)
		/* spin forever */ ;
}
