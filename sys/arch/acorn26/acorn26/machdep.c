/* $NetBSD: machdep.c,v 1.8 2003/05/31 00:38:05 kristerw Exp $ */

/*-
 * Copyright (c) 1998 Ben Harris
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * machdep.c -- standard machine-dependent functions
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.8 2003/05/31 00:38:05 kristerw Exp $");

#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/memcreg.h>

int physmem;
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "Archimedes";

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;
struct vm_map *phys_map = NULL;
struct vm_map *mb_map = NULL; /* and ever more shall be so */

int waittime = -1;

void
cpu_reboot(howto, b)
	int howto;
	char *b;
{

	/* If "always halt" was specified as a boot flag, obey. */
	if ((boothowto & RB_HALT) != 0)
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
	splhigh();

#if 0
	/* XXX Need to implement this */
	if (boothowto & RB_DUMP)
		dumpsys();
#endif

	/* run any shutdown hooks */
	doshutdownhooks();

haltsys:
	if (howto & RB_HALT) {
		printf("system halted\n");
		for (;;);
	} else {
		printf("rebooting...");
		/*
		 * On a real reset, the ROM is mapped into address
		 * zero.  On RISC OS 3.11 at least, this can be faked
		 * by copying the first instruction in the ROM to
		 * address zero -- it immediately branches into the
		 * ROM at its usual address, and hence would remove
		 * the ROM mapped at zero anyway.
		 *
		 * In order to convince RISC OS to do a hard reset, we
		 * fake a *FX 200,2.  Note that I don't know if this
		 * will work on RISC OS <3.10.  This is slightly
		 * suboptimal as it causes RISC OS to zero the whole
		 * of RAM on startup (farewell, message buffer).  If
		 * anyone can tell me how to fake a control-reset in
		 * software, I'd be most grateful.
		 */
		*(volatile u_int8_t *)0x9c2 = 2; /* Zero page magic */
		*(volatile u_int32_t *)0
			= *(volatile u_int32_t *)MEMC_ROM_LOW_BASE;
		/* reboot in SVC mode, IRQs and FIQs disabled */
		__asm __volatile("movs pc, %0" : :
		    "r" (R15_MODE_SVC | R15_FIQ_DISABLE | R15_IRQ_DISABLE));
	}
	panic("cpu_reboot failed");
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	u_int i, base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

	/* Stuff to do here: */
	/* initmsgbuf() is called from start() */

	printf("%s", version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/* allocsys() is called from start() */

	/* Various boilerplate memory allocations. */

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("cpu_startup: cannot allocate VM for buffers");
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
		curbufsize = PAGE_SIZE * ((i < residual) ? (base+1) : base);

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
	pmap_update(pmap_kernel());

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	minaddr = kernel_map->min_offset;
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   512 * 1024, 0, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * PAGE_SIZE);
	printf("using %u buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	curpcb = &lwp0.l_addr->u_pcb;

#if 0
	/* Test exception handlers */
	asm(".word 0x06000010"); /* undefined instruction */
	asm("swi 0"); /* SWI */
	(*(void (*)(void))(0x00008000))(); /* prefetch abort */
	*(volatile int *)(0x00008000) = 0; /* data abort */
	*(volatile int *)(0x10000000) = 0; /* address exception */
#endif
}

/*
 * machine dependent system variables.
 */

int
cpu_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp, void *newp,
    size_t newlen, struct proc *p)
{

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	return (EOPNOTSUPP);
}
