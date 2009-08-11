/* $NetBSD: machdep.c,v 1.11 2009/08/11 17:04:17 matt Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 * 	from: Utah Hdr: machdep.c 1.63 91/04/24
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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 * 	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.11 2009/08/11 17:04:17 matt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "opt_memsize.h"
#include "opt_modular.h"
#include "opt_ethaddr.h"

#include "opt_pci.h"
#include "pci.h"

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

#include <net/if.h>
#include <net/if_ether.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <mips/cache.h>
#include <mips/locore.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_extiovar.h>
#include <mips/adm5120/include/adm5120_obiovar.h>
#include <mips/adm5120/include/adm5120_mainbusvar.h>
#include <mips/adm5120/include/adm5120_pcivar.h>
#include <mips/adm5120/dev/uart.h>

#ifndef	MEMSIZE
#define	MEMSIZE 4 * 1024 * 1024
#endif /* !MEMSIZE */

struct	user *proc0paddr;

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int maxmem;			/* max memory per process */

int mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

struct adm5120_config adm5120_configuration;

void adm5120_setcpufreq(void);

void
adm5120_setcpufreq(void)
{
	uint32_t v, freq;

	v = SW_READ(SW_CODE_REG);
	switch (v & CLKS_MASK) {
	case CLKS_175MHZ:
		freq = 175 * 1000 * 1000;
		break;
	case CLKS_200MHZ:
		freq = 200 * 1000 * 1000;
		break;
	default:
		panic("adm5120: cannot determine CPU clock speed");
	}

	curcpu()->ci_cpu_freq = freq;
	curcpu()->ci_cycles_per_hz = (freq + hz / 2) / hz / 2;
	curcpu()->ci_divisor_delay = ((freq + 500000) / 1000000) / 2;
}

void	mach_init(int, char **, void *, void *); /* XXX */

static void
copy_args(int argc, char **argv)
{
	struct adm5120_config *admc = &adm5120_configuration;
	int i;
	char *buf;
	size_t buflen, rc;

	buf = admc->args;
	buflen = sizeof(admc->args);

	if (argc >= __arraycount(admc->argv))
		panic("%s: too many boot args\n", __func__);

	for (i = 0; buflen > 0 && i < argc && argv[i] != NULL; i++) {
		admc->argv[i] = buf;
		if ((rc = strlcpy(buf, argv[i], buflen)) >= buflen)
			panic("%s: boot args too long\n", __func__);

		buf += rc;
		buflen -= rc;
		*buf++ = '\0';
		buflen--;
	}
	if (i < argc)
		panic("%s: boot args too long\n", __func__);

	admc->argc = argc;
}

static void
parse_args(prop_dictionary_t properties, int argc, char **argv,
    uint32_t *memsizep)
{
	char buf[32];
	char *key, *val, *valend;
	unsigned long tmp;
	int i;
	uint8_t enaddr[ETHER_ADDR_LEN];

	if (memsizep != NULL)
		*memsizep = MEMSIZE;

	for (i = 0; i < argc && argv[i] != NULL; i++) {
		if (strlcpy(buf, argv[i], sizeof(buf)) >= sizeof(buf))
			goto err;
		val = buf;
		key = strsep(&val, "=");
		if (val == NULL)
			goto err;
		if (strcmp(key, "mem") == 0) {
			tmp = strtoul(val, &valend, 10);
			if (val == valend || *valend != 'M')
				goto err;
			if (memsizep != NULL)
				*memsizep = tmp * 1024 * 1024;
		} else if (strcmp(key, "HZ") == 0)
			;
		else if (strcmp(key, "gpio") == 0) {
			prop_number_t pn;

			tmp = strtoul(val, &valend, 10);
			if (val == valend || *valend != '\0')
				goto err;
			if (properties == NULL)
				continue;
			pn = prop_number_create_unsigned_integer(tmp);
			if (pn == NULL) {
				printf(
				   "%s: prop_number_create_unsigned_integer\n",
				   __func__);
				continue;
			}
			if (!prop_dictionary_set(properties, "initial-gpio",
			    pn)) {
				printf("%s: prop_dictionary_set(gpio)\n",
				    __func__);
			}
			prop_object_release(pn);
		} else if (strcmp(key, "kmac") == 0) {
			prop_data_t pd;

			ether_nonstatic_aton(enaddr, val);
			if (properties == NULL)
				continue;
			pd = prop_data_create_data(enaddr, sizeof(enaddr));
			if (pd == NULL) {
				printf("%s: prop_data_create_data\n", __func__);
				continue;
			}
			if (!prop_dictionary_set(properties, "mac-addr", pd)) {
				printf("%s: prop_dictionary_set(mac)\n",
				    __func__);
			}
			prop_object_release(pd);
		} else if (strcmp(key, "board") == 0)
			printf("Routerboard %s\n", val);
		else if (strcmp(key, "boot") == 0)
			;
		continue;
	err:
		printf("bad argv[%d] (%s)\n", i, argv[i]);
	}
}

void
mach_init(int argc, char **argv, void *a2, void *a3)
{
	struct adm5120_config *admc = &adm5120_configuration;
	uint32_t memsize;
	vaddr_t kernend;
	u_long first, last;
	vaddr_t v;

	extern char edata[], end[];	/* XXX */

	/* clear the BSS segment */
	kernend = mips_round_page(end);
	memset(edata, 0, kernend - (vaddr_t)edata);

	/* set CPU model info for sysctl_hw */
	strcpy(cpu_model, "Infineon ADM5120");

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Sets up mips_cpu_flags that may be queried by other
	 * functions called during startup.
	 * Also clears the I+D caches.
	 */
	mips_vector_init();

	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();

	adm5120_setcpufreq();

	/*
	 * Initialize bus space tags.
	 */
	obio_bus_mem_init(&admc->obio_space, admc);
	extio_bus_mem_init(&admc->extio_space, admc);
#if NPCI > 0
	pciio_bus_mem_init(&admc->pciio_space, admc);
	pcimem_bus_mem_init(&admc->pcimem_space, admc);
#endif

	/*
	 * Initialize bus DMA tag.
	 */
	obio_dma_init(&admc->obio_dmat);

	/*
	 * Attach serial console.
	 */
	uart_cnattach();

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
#ifdef KADB
	boothowto |= RB_KDB;
#endif

	parse_args(NULL, argc, argv, &memsize);

	/*
	 * Determine the memory size.
	 *
	 * Note: Reserve the first page!  That's where the trap
	 * vectors are located.
	 */

#if 0
	if (GET_MEMSIZE(memsize) == 0) {
		uint32_t val;

		/* This does not seem to work... --dyoung */
		val = SW_READ(SW_MEMCONT_REG);
		printf("SW_MEMCONT_REG: 0x%08" PRIx32 "\n", val);
		switch (val & SDRAM_SIZE_MASK) {
		case SDRAM_SIZE_4MBYTES:
			memsize = 4 * 1024 * 1024;
			break;
		case SDRAM_SIZE_8MBYTES:
			memsize = 8 * 1024 * 1024;
			break;
		case SDRAM_SIZE_16MBYTES:
			memsize = 16 * 1024 * 1024;
			break;
		case SDRAM_SIZE_64MBYTES:
			memsize = 64 * 1024 * 1024;
			break;
		case SDRAM_SIZE_128MBYTES:
			memsize = 128 * 1024 * 1024;
			break;
		default:
			panic("adm5120: cannot determine memory size");
		}
	}
#endif

	physmem = btoc(memsize);

	mem_clusters[mem_cluster_cnt].start = PAGE_SIZE;
	mem_clusters[mem_cluster_cnt].size =
	    memsize - mem_clusters[mem_cluster_cnt].start;
	mem_cluster_cnt++;

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
	    VM_FREELIST_DEFAULT);

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
	v = uvm_pageboot_alloc(USPACE);
	lwp0.l_addr = proc0paddr = (struct user *)v;
	lwp0.l_md.md_regs = (struct frame *)(v + USPACE) - 1;
	proc0paddr->u_pcb.pcb_context[11] =
	    MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	copy_args(argc, argv);
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
	struct adm5120_config *admc = &adm5120_configuration;
	char pbuf[9];
	vaddr_t minaddr, maxaddr;
#ifdef DEBUG
	extern int pmapdebug;		/* XXX */
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */
#endif

	if ((admc->properties = prop_dictionary_create()) == NULL)
		printf("%s: prop_dictionary_create\n", __func__);
	parse_args(admc->properties, admc->argc, admc->argv, NULL);

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
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
	if (curproc)
		savectx((struct user *)curpcb);

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
	 * Routerboard BIOS may autoboot, so "pseudo-halt".
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
	SW_WRITE(SW_SFTRES_REG, 0);			/* reset */
	for (;;)
		/* spin forever */ ;	/* XXX */
	/*NOTREACHED*/
}
