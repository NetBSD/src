/* $NetBSD: machdep.c,v 1.4 2002/03/17 11:37:55 simonb Exp $ */

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
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
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

#include "opt_ddb.h"
#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/device.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/swarm.h>
#include <mips/locore.h>

#include <mips/cfe/cfe_xiocb.h>
#include <mips/cfe/cfe_api.h>

#if 0 /* XXXCGD */
#include <machine/nvram.h>
#endif /* XXXCGD */
#include <machine/leds.h>

#ifdef DDB
#include <machine/db_machdep.h>
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

#ifdef IKOS
#include <sbmips/ikos/ikosvar.h>
#endif

/* For sysctl. */
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "sb1250";

/* Our exported CPU info.  Only one for now */
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* Total physical memory */

char	bootstring[512];	/* Boot command */
int	netboot;		/* Are we netbooting? */
int	cfe_present;

struct bootinfo_v1 bootinfo;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(long,long,long,long);

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS_INT_MASK | MIPS_SR_INT_IE;

extern caddr_t esym;
extern struct user *proc0paddr;

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(fwhandle,magic,bootdata,reserved)
	long fwhandle;
	long magic;
	long bootdata;
	long reserved;
{
	caddr_t kernend, v, p0;
	u_long first, last;
	vsize_t size;
	extern char edata[], end[];
	int i;
	u_int memsize;
	uint32_t config;

	/* XXX this code must run on the target cpu */
	config = mips3_cp0_config_read();
	config &= ~MIPS3_CONFIG_K0_MASK;
	config |= 0x05;				/* XXX.  cacheable coherent */
	mips3_cp0_config_write(config);

	/* XXXCGD */
#ifdef SBMIPS_MEM_SIZE
	memsize = SBMIPS_MEM_SIZE;
#else
	memsize = 32 * 1024 * 1024;
#endif
	memsize += MIPS_KSEG0_START;

	/*
	 * Clear the BSS segment.
	 */
#ifdef DDB
	if (memcmp(((Elf_Ehdr *)end)->e_ident, ELFMAG, SELFMAG) == 0 &&
	    ((Elf_Ehdr *)end)->e_ident[EI_CLASS] == ELFCLASS) {
		esym = end;
		esym += ((Elf_Ehdr *)end)->e_entry;
		kernend = (caddr_t)mips_round_page(esym);
		bzero(edata, end - edata);
	} else
#endif
	{
		kernend = (caddr_t)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

	/*
	 * Copy the bootinfo structure from the boot loader.
	 * this has to be done before mips_vector_init is
	 * called because we may need CFE's TLB handler
	 */

	cfe_present = 0;
	if (magic == BOOTINFO_MAGIC)
		bcopy((struct bootinfo_v1 *)bootdata, &bootinfo,
		    sizeof bootinfo);
	else if (reserved == CFE_EPTSEAL) {
		magic = BOOTINFO_MAGIC;
		bzero(&bootinfo, sizeof bootinfo);
		bootinfo.version = BOOTINFO_VERSION;
		bootinfo.fwhandle = fwhandle;
		bootinfo.fwentry = bootdata;
	}

#ifdef IKOS
	ikoscons_output_bufsize = IKOSCONS_OUTPUT_BUFSIZE;
	memsize -= ikoscons_output_bufsize;
	ikoscons_output_buf =
	    (void *)MIPS_PHYS_TO_KSEG1(MIPS_KSEG0_TO_PHYS(memsize));
	memset(ikoscons_output_buf, 0, ikoscons_output_bufsize);
#endif

	/*
	 * physmem is measured in pages
	 * XXX does this need to be set for 'consinit', 'uvm_setpagesize',
	 * XXX or 'mips_vector_init'?  not that I can tell right now...
	 * XXX if CFE is present, this will be recalculated.
	 * XXX
	 * XXX Does 'physmem' want to be the total amount of RAM in the
	 * XXX system, or the amount that is available to us?
	 */
	physmem = btoc(memsize - MIPS_KSEG0_START);

	consinit();

	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

#if 0
*(unsigned long long *)0xb0020440 = 3;
#endif

	printf("fwhandle=%08X magic=%08X bootdata=%08X reserved=%08X\n",
	    (u_int) fwhandle,(u_int) magic,(u_int) bootdata,(u_int) reserved);

	if (magic == BOOTINFO_MAGIC) {
		int idx;
		int added;
		cfe_xuint_t start,len,type;

		cfe_init(fwhandle);
		cfe_present = 1;

		idx = 0;
		physmem = 0;
		mem_cluster_cnt = 0;
		while (cfe_getmeminfo(idx,&start,&len,&type) == 0) {
			added = 0;
			printf("Memory Block #%d start %08llX len %08llX: %s: ",
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
		mem_clusters[0].start = 0;
		mem_clusters[0].size = ctob(physmem);
		mem_cluster_cnt = 1;
	}


	for (i = 0; i < sizeof(bootinfo.boot_flags); i++) {
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
		if (memcmp("single", &bootinfo.boot_flags[i], 5) == 0)
			boothowto |= RB_SINGLE;
		if (memcmp("nfsroot=", &bootinfo.boot_flags[i], 8) == 0)
			netboot = 1;
		/*
		 * XXX Select root device from 'root=/dev/hd[abcd][1234]' too.
		 */
	}

	/*
	 * Load the rest of the available pages into the VM system.
	 * The first chunk is tricky because we have to avoid the
	 * kernel, but the rest are easy.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
		VM_FREELIST_DEFAULT);

	for (i = 1; i < mem_cluster_cnt; i++) {
		first = round_page(mem_clusters[i].start);
		last = mem_clusters[i].start + mem_clusters[i].size;
		uvm_page_physload(atop(first), atop(last), atop(first),
		    atop(last), VM_FREELIST_DEFAULT);
	}

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Allocate space for proc0's USPACE
	 */
	p0 = (caddr_t)pmap_steal_memory(USPACE, NULL, NULL);
	proc0.p_addr = proc0paddr = (struct user *)p0;
	proc0.p_md.md_regs = (struct frame *)(p0 + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	size = (vsize_t)allocsys(NULL, NULL);
	v = (caddr_t)pmap_steal_memory(size, NULL, NULL);
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");

	pmap_bootstrap();

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if defined(DDB)
	ddb_init(0, 0, 0);
#endif

	if (boothowto & RB_KDB) {
#if defined(DDB)
		Debugger();
#endif
	}
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("%s memory", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
		    UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base + 1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
					"buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr, 16 * NCARGS,
	    VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr, VM_PHYS_SIZE,
	    0, FALSE, NULL);


	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf(", %s free", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf(", %s in %d buffers\n", pbuf, nbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}
}

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	/* Take a snapshot before clobbering any registers. */
	if (curproc)
		savectx((struct user *)curpcb);

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
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n\n");

	if (cfe_present) {
		cfe_exit(1, (howto & RB_DUMP) ? 1 : 0);
		printf("cfe_exit didn't!\n");
	}

	delay(500000);

#if defined(IKOS) && defined(REALLY_IKOS)
	*(volatile char *)MIPS_PHYS_TO_KSEG1(0x408000) = 0;
#else
    {
	static int broken;

	if (!broken) {
		broken = 1;
		__asm__ ( "move $4, %0 ; break 0x3ff" : : "r"(howto) );
	}
    }
#endif

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

	/* Check each DRAM region. */
	if ((pa >= 0x0000000000 && pa <= 0x000fffffff) ||	/* DRAM 0 */
	    (pa >= 0x0080000000 && pa <= 0x008fffffff) ||	/* DRAM 1 */
	    (pa >= 0x0090000000 && pa <= 0x009fffffff) ||	/* DRAM 2 */
	    (pa >= 0x00c0000000 && pa <= 0x00cfffffff) ||	/* DRAM 3 */
#ifdef _MIPS_PADDR_T_64BIT
	    (pa >= 0x0100000000 && pa <= 0x07ffffffff) ||	/* DRAM exp */
#endif
	   0) {
		rv = 5;		/* Cacheable coherent. */
		goto done;
	}

	rv = 2;			/* Uncached. */
done:
	return (rv);
}
