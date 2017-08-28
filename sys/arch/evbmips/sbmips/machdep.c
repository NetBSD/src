/* $NetBSD: machdep.c,v 1.2.2.2 2017/08/28 17:51:36 skrll Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.2.2.2 2017/08/28 17:51:36 skrll Exp $");

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/intr.h>
#include <sys/kcore.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/psl.h>
#include <mips/pte.h>
#include <mips/reg.h>

#include <mips/cfe/cfe_api.h>

#include <evbmips/sbmips/autoconf.h>
#include <evbmips/sbmips/swarm.h>
#include <evbmips/sbmips/systemsw.h>

#if 0 /* XXXCGD */
#include <evbmips/sbmips/nvram.h>
#endif /* XXXCGD */
#include <evbmips/sbmips/leds.h>

#include <mips/sibyte/dev/sbbuswatchvar.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <mips/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define	ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <dev/cons.h>

#if NKSYMS || defined(DDB) || defined(MODULAR)
/* start and end of kernel symbol table */
void	*ksym_start, *ksym_end;
#endif

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

char	bootstring[512];	/* Boot command */
int	netboot;		/* Are we netbooting? */
int	cfe_present;

struct bootinfo_v1 bootinfo;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(long, long, long, long);

extern void *esym;

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(long fwhandle, long magic, long bootdata, long reserved)
{
	void *kernend;
	extern char edata[], end[];
	uint32_t config;

	/* XXX this code must run on the target CPU */
	config = mips3_cp0_config_read();
	config &= ~MIPS3_CONFIG_K0_MASK;
	config |= 0x05;				/* XXX.  cacheable coherent */
	mips3_cp0_config_write(config);

	/* Zero BSS.  XXXCGD: uh, is this really necessary still?  */
	memset(edata, 0, end - edata);

	/*
	 * Copy the bootinfo structure from the boot loader.
	 * this has to be done before mips_vector_init is
	 * called because we may need CFE's TLB handler
	 */

	if (magic == BOOTINFO_MAGIC)
		memcpy(&bootinfo, (struct bootinfo_v1 *)bootdata,
		    sizeof bootinfo);
	else if (reserved == CFE_EPTSEAL) {
		magic = BOOTINFO_MAGIC;
		memset(&bootinfo, 0, sizeof bootinfo);
		bootinfo.version = BOOTINFO_VERSION;
		bootinfo.fwhandle = fwhandle;
		bootinfo.fwentry = bootdata;
		bootinfo.ssym = (vaddr_t)end;
		bootinfo.esym = (vaddr_t)end;
	}

	kernend = (void *)mips_round_page(end);
#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (magic == BOOTINFO_MAGIC) {
		ksym_start = (void *)(intptr_t)bootinfo.ssym;
		ksym_end   = (void *)(intptr_t)bootinfo.esym;
		kernend = (void *)mips_round_page((vaddr_t)ksym_end);
	}
#endif

	consinit();

	uvm_md_init();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
#ifdef MULTIPROCESSOR
	mips_vector_init(NULL, true);
#else
	mips_vector_init(NULL, false);
#endif

	mips_locoresw.lsw_bus_error = sibyte_bus_watch_check;

	sb1250_ipl_map_init();

#ifdef DEBUG
	printf("fwhandle=%08X magic=%08X bootdata=%08X reserved=%08X\n",
	    (u_int)fwhandle, (u_int)magic, (u_int)bootdata, (u_int)reserved);
#endif

	cpu_setmodel("sb1250");

	if (magic == BOOTINFO_MAGIC) {
		int idx;
		int added;
		uint64_t start, len, type;

		cfe_init(bootinfo.fwhandle, bootinfo.fwentry);
		cfe_present = 1;

		idx = 0;
		physmem = 0;
		mem_cluster_cnt = 0;
		while (cfe_enummem(idx, 0, &start, &len, &type) == 0) {
			added = 0;
			printf("Memory Block #%d start %08"PRIx64"X len %08"PRIx64"X: %s: ",
			    idx, start, len, (type == CFE_MI_AVAILABLE) ?
			    "Available" : "Reserved");
			if ((type == CFE_MI_AVAILABLE) &&
			    (mem_cluster_cnt < VM_PHYSSEG_MAX)) {
				/*
				 * XXX Ignore memory above 256MB for now, it
				 * XXX needs special handling.
				 */
				if (start < (256*1024*1024)) {
				    physmem += btoc(((int) len));
				    mem_clusters[mem_cluster_cnt].start =
					(long) start;
				    mem_clusters[mem_cluster_cnt].size =
					(long) len;
				    mem_cluster_cnt++;
				    added = 1;
				}
			}
			if (added)
				printf("added to map\n");
			else
				printf("not added to map\n");
			idx++;
		}

	} else {
		/*
		 * Handle the case of not being called from the firmware.
		 */
		/* XXX hardwire to 32MB; should be kernel config option */
		physmem = 32 * 1024 * 1024 / 4096;
		mem_clusters[0].start = 0;
		mem_clusters[0].size = ctob(physmem);
		mem_cluster_cnt = 1;
	}


	for (u_int i = 0; i < sizeof(bootinfo.boot_flags); i++) {
		switch (bootinfo.boot_flags[i]) {
		case '\0':
			break;
		case ' ':
			continue;
		case '-':
			while (bootinfo.boot_flags[i] != ' ' &&
			    bootinfo.boot_flags[i] != '\0') {
				switch (bootinfo.boot_flags[i]) {
				case 'a':
					boothowto |= RB_ASKNAME;
					break;
				case 'd':
					boothowto |= RB_KDB;
					break;
				case 's':
					boothowto |= RB_SINGLE;
					break;
				}
				i++;
			}
		}
	}

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t) kernend,
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	/*
	 * Allocate uarea for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf(((uintptr_t)ksym_end - (uintptr_t)ksym_start),
	    ksym_start, ksym_end);
#endif

	if (boothowto & RB_KDB) {
#if defined(DDB)
		Debugger();
#endif
	}

#ifdef MULTIPROCESSOR
	mips_fixup_exceptions(mips_fixup_zero_relative);
#endif
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{
	/*
	 * Just do the common stuff.
	 */
	cpu_startup_common();
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

	printf("rebooting...\n\n");

	if (cfe_present) {
		/*
		 * XXX
		 * For some reason we can't return to CFE with
		 * and do a warm start.  Need to look into this...
		 */
		cfe_exit(0, (howto & RB_DUMP) ? 1 : 0);
		printf("cfe_exit didn't!\n");
	}

	printf("WARNING: reboot failed!\n");

	for (;;);
}

static void
cswarm_setled(u_int index, char c)
{
	volatile u_char *led_ptr =
	    (void *)MIPS_PHYS_TO_KSEG1(SWARM_LEDS_PHYS);

	if (index < 4)
		led_ptr[0x20 + ((3 - index) << 3)] = c;
}

void
cswarm_setleds(const char *str)
{
	int i;

	for (i = 0; i < 4 && str[i]; i++)
		cswarm_setled(i, str[i]);
	for (; i < 4; i++)
		cswarm_setled(' ', str[i]);
}

int
sbmips_cca_for_pa(paddr_t pa)
{
	int rv;

	rv = 2;			/* Uncached. */

	/* Check each DRAM region. */
	if ((pa >= 0x0000000000   && pa <= 0x000fffffff) ||	/* DRAM 0 */
	    (pa >= 0x0080000000   && pa <= 0x008fffffff) ||	/* DRAM 1 */
	    (pa >= 0x0090000000   && pa <= 0x009fffffff) ||	/* DRAM 2 */
	    (pa >= 0x00c0000000   && pa <= 0x00cfffffff) ||	/* DRAM 3 */
#ifdef _MIPS_PADDR_T_64BIT
	    (pa >= 0x0100000000LL && pa <= 0x07ffffffffLL) ||	/* DRAM exp */
#endif
	   0) {
		rv = 5;		/* Cacheable coherent. */
	}

	return (rv);
}
