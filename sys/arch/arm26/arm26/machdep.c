/* $NetBSD: machdep.c,v 1.3 2000/06/26 14:20:33 mrg Exp $ */

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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * machdep.c -- standard machine-dependent functions
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.3 2000/06/26 14:20:33 mrg Exp $");

#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <machine/memcreg.h>

int physmem;
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "Archimedes";

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

vm_map_t exec_map = NULL;
vm_map_t phys_map = NULL;
vm_map_t mb_map = NULL; /* and ever more shall be so */

void
cpu_reboot(howto, b)
	int howto;
	char *b;
{
	/* FIXME This needs much more sophistication */
	splhigh();
	/* XXX Temporary measure */
	howto |= RB_HALT;
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
		asm volatile("movs pc, #3"); /* reboot in SVC mode */
	}
	panic("cpu_reboot failed");
}

/*
 * This is provided because GCC seems to generate calls to it.
 */
void abort __P((void)) __attribute__((__noreturn__));

void
abort()
{

	panic("abort() called -- noreturn function returned?");
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	int i, base, residual;
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
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
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
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pgs(curbuf, &pg, 1);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

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
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

#if 0
	/* Test exception handlers */
	asm(".word 0x06000010"); /* undefined instruction */
	asm("swi 0"); /* SWI */
	(*(void (*)(void))(0x00008000))(); /* prefetch abort */
	*(volatile int *)(0x00008000) = 0; /* data abort */
	*(volatile int *)(0x10000000) = 0; /* address exception */
#endif
}
