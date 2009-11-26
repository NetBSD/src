/* $NetBSD: start.c,v 1.16 2009/11/26 00:19:11 matt Exp $ */
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
/*
 * start.c -- In the beginning...
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: start.c,v 1.16 2009/11/26 00:19:11 matt Exp $");

#include "opt_modular.h"

#include <sys/msgbuf.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>
#include <acorn26/ioc/iociicvar.h>

#include <arm/armreg.h>
#include <arm/undefined.h>
#include <machine/boot.h>
#include <machine/machdep.h>
#include <machine/memcreg.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include "arcvideo.h"
#include "ioc.h"
#include "ksyms.h"

#if NIOC > 0
#include <arch/acorn26/iobus/iocreg.h>
#endif

extern void main(void); /* XXX Should be in a header file */

struct bootconfig bootconfig;

/* in machdep.h */
extern i2c_tag_t acorn26_i2c_tag;

/* We don't pass a command line yet. */
const char *boot_args = "";
const char *boot_file = "";

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
start(struct bootconfig *initbootconfig)
{
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
	memset(__bss_start__, 0, __bss_end__ - __bss_start__);

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
	initmsgbuf((void*)((paddr_t)MEMC_PHYS_BASE + MSGBUF_PHYSADDR),
		MSGBUFSIZE);

#ifdef DIAGNOSTIC
	/*
	 * Check that the linker and loader agree about where
	 * everything should be loaded.
	 */

	if (boot_sanity != BOOT_SANITY)
		panic("Bootloader mislaid the data segment");
#endif

#if !NKSYMS && !defined(DDB) && !defined(MODULAR)
	/* Throw away the symbol table to gain space. */
	if (bootconfig.freebase == bootconfig.esym) {
		bootconfig.freebase = bootconfig.ssym;
		bootconfig.ssym = bootconfig.esym = 0;
	}
#endif

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

	splhigh();
	fiq_off();

	/*
	 * Locate process 0's user structure, in the bottom of its kernel
	 * stack page.  That's our current stack page too.
	 */
	lwp0.l_addr = (struct user *)(round_page((vaddr_t)&onstack) - USPACE);
	memset(lwp0.l_addr, 0, sizeof(*lwp0.l_addr));

	/* TODO: anything else? */
	
	main();

	panic("main() returned");
	/* NOTREACHED */
}
