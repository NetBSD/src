/* $NetBSD: start.c,v 1.10 2001/05/13 13:48:11 bjh21 Exp $ */
/*-
 * Copyright (c) 1998, 2000 Ben Harris
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
 * start.c -- In the beginning...
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: start.c,v 1.10 2001/05/13 13:48:11 bjh21 Exp $");

#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <arm/armreg.h>
#include <arm/undefined.h>
#include <machine/boot.h>
#include <machine/machdep.h>
#include <machine/memcreg.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include "arcvideo.h"
#include "ioc.h"

#if NIOC > 0
#include <arch/arm26/iobus/iocreg.h>
#endif

extern void main __P((void)); /* XXX Should be in a header file */

struct bootconfig bootconfig;

struct user *proc0paddr;

/* We don't pass a command line yet. */
char *boot_args = "";
char *boot_file = "";

#ifdef DIAGNOSTIC
#define BOOT_SANITY 0x89345846
static int boot_sanity = BOOT_SANITY;
#endif

#ifndef __ELF__
/* Odd symbols declared by the linker */
extern char _stext_, _etext, _sdata_, _edata, _bss_start, _end;
#else /* ELF */
extern char __bss_start__[], __bss_end__[];
#endif

/*
 * This is where it all begins.  The bootloader is expected to enter
 * in an APCS-compliant manner so we don't have to mess around in
 * assembler to get things going.
 */
void
start(initbootconfig)
	struct bootconfig *initbootconfig;
{
	size_t size;
	caddr_t v;
	char pbuf[9];
	int onstack;

	/*
	 * State of the world as of BBBB 0.02:
	 * Registers per APCS
	 * physmem (set up by RISC OS and BBBB):	
	 * 0x02000000 -- 0x02080000 == Screen (potentially)
	 * 0x02080000 -- 0x02088000 == Zero page
	 * 0x02088000 -- 0x02090000 == Initial kernel stack
	 * 0x02090000 -- 0x02098000 == Kernel message buffer
	 * 0x02098000 -- freebase == Kernel image
	 * bss is cleared for us
	 * We re-map zero page to point at the start of the kernel image
	 * which is in locore.S.
	 */

#define ZP_PHYSADDR	((paddr_t)0x00098000)
#define MSGBUF_PHYSADDR	((paddr_t)0x00090000)

	/* We can't trust the BSS (at least not with my linker) */
	bzero(__bss_start__, __bss_end__ - __bss_start__);

	/* Save boot configuration somewhere */
	memcpy(&bootconfig, initbootconfig, sizeof(struct bootconfig));

	if (bootconfig.magic != BOOT_MAGIC)
		panic("bootloader is ancient");
	/* initialise kernel variables */
	boothowto = bootconfig.boothowto; 
	physmem = bootconfig.npages;
	uvmexp.pagesize = bootconfig.nbpp;
	uvm_setpagesize();
	
	/* Any others? */

	/* Set up kernel message buffer */
	/* XXX Should be in cpu_startup, yesno? */
	/* Probably not actually -- the sooner it's run, the better. */
	initmsgbuf(MEMC_PHYS_BASE + MSGBUF_PHYSADDR, MSGBUFSIZE);

#ifdef DIAGNOSTIC
	/*
	 * Check that the linker and loader agree about where
	 * everything should be loaded.
	 */

	if (boot_sanity != BOOT_SANITY)
		panic("Bootloader mislaid the data segment");
#endif

#ifndef DDB
	/* Throw away the symbol table to gain space. */
	if (bootconfig.freebase == bootconfig.esym) {
		bootconfig.freebase = bootconfig.ssym;
		bootconfig.ssym = bootconfig.esym = 0;
	}
#endif

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.  This trick is stolen from the alpha port.
	 */
	size = (vsize_t)allocsys(0, NULL);
	v = MEMC_PHYS_BASE + bootconfig.freebase;
	bootconfig.freebase += size;
	if (bootconfig.freebase > ptoa(physmem)) {
		format_bytes(pbuf, sizeof(pbuf), size);
		panic("start: out of memory (wanted %s)", pbuf);
	}
	bzero(v, size);
	if ((allocsys(v, NULL) - v) != size)
		panic("start: table size inconsistency");
	
	/* Tell UVM about memory */
#if NARCVIDEO == 0
	/*
	 * DMA-able space.  If the video driver's configured, it'll
	 * register this later once it knows how much it's using.
	 */
	uvm_page_physload(0, atop(MEMC_DMA_MAX), 0, atop(MEMC_DMA_MAX),
			  VM_FREELIST_LOW);
#endif
	/*
	 * Normal memory -- we expect the bootloader to tell us where
         * the kernel ends.
	 */
	/* Old zero page is unused. */
	uvm_page_physload(atop(MEMC_DMA_MAX), atop(MEMC_DMA_MAX) + 1,
			  atop(MEMC_DMA_MAX), atop(MEMC_DMA_MAX) + 1,
			  VM_FREELIST_DEFAULT);
	/* Stack, msgbuf, kernel image are used, space above it is unused. */
	uvm_page_physload(atop(MEMC_DMA_MAX) + 1, bootconfig.npages,
			  atop(round_page(bootconfig.freebase)),
			  bootconfig.npages,
			  VM_FREELIST_DEFAULT);

	/* printf("Memory registered with UVM.\n"); */

	/* Get the MEMC set up and map zero page */
	pmap_bootstrap(bootconfig.npages, ZP_PHYSADDR);

	/* Set up the undefined instruction handlers. */
	undefined_init();

	/*
	 * This is a nasty bit.  Because the kernel uses a 26-bit APCS
	 * variant, the CPU interrupt disable flags get munged on
	 * every function return.  Thus, we need to enable interrupts
	 * at the CPU now since this is the last function we control
	 * that won't return.  In order to be able to do this, we need
	 * to ensure we won't get any interrupts before we're ready
	 * for them.  For now, I'll assume we've got an IOC doing all
	 * this at the usual location, but it should be done more
	 * elegantly.
	 */

#if NIOC > 0
	*(volatile u_char *)(0x03200000 + (IOC_IRQMSKA << 2)) = 0;
	*(volatile u_char *)(0x03200000 + (IOC_IRQMSKB << 2)) = 0;
	*(volatile u_char *)(0x03200000 + (IOC_FIQMSK << 2)) = 0;
#endif
	int_on();

	/*
	 * Locate process 0's user structure, in the bottom of its kernel
	 * stack page.  That's our current stack page too.
	 */
	proc0paddr = (struct user *)(round_page((vaddr_t)&onstack) - USPACE);
	bzero(proc0paddr, sizeof(*proc0paddr));

	/* TODO: anything else? */
	
	main();

	panic("main() returned");
	/* NOTREACHED */
}
