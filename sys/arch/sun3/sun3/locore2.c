/*	$NetBSD: locore2.c,v 1.97 2009/11/29 15:06:40 pooka Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: locore2.c,v 1.97 2009/11/29 15:06:40 pooka Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#define ELFSIZE 32
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/db_machdep.h>
#include <machine/dvma.h>
#include <machine/idprom.h>
#include <machine/leds.h>
#include <machine/mon.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun3/sun3/control.h>
#include <sun3/sun3/interreg.h>
#include <sun3/sun3/machdep.h>
#include <sun3/sun3/obmem.h>
#include <sun68k/sun68k/vector.h>

#include "ksyms.h"

/* This is defined in locore.s */
extern char kernel_text[];

/* These are defined by the linker */
extern char etext[], edata[], end[];
int nsym;
char *ssym, *esym;

/* Basically a flag: "Do we have a VAC?" */
int cache_size;

/*
 * XXX: m68k common code needs these...
 * ... but this port does not need to deal with anything except
 * an mc68020, so these two variables are always ignored.
 */
int cputype = CPU_68020;
int mmutype = MMU_SUN;

/*
 * Now our own stuff.
 */

u_char cpu_machine_id = 0;
const char *cpu_string = NULL;
int cpu_has_vme = 0;

/*
 * XXX - Should empirically estimate the divisor...
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuclock	(where cpuclock is in MHz).
 */
int delay_divisor = 82;		/* assume the fastest (3/260) */

extern struct pcb *curpcb;

/* First C code called by locore.s */
void _bootstrap(void);

static void _verify_hardware(void);
static void _vm_init(void);

#if NKSYMS || defined(DDB) || defined(MODULAR)
static void _save_symtab(void);

/*
 * Preserve DDB symbols and strings by setting esym.
 */
static void 
_save_symtab(void)
{
	int i;
	Elf_Ehdr *ehdr;
	Elf_Shdr *shp;
	vaddr_t minsym, maxsym;

	/*
	 * Check the ELF headers.
	 */

	ehdr = (void *)end;
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		mon_printf("_save_symtab: bad ELF magic\n");
		return;
	}

	/*
	 * Find the end of the symbols and strings.
	 */

	maxsym = 0;
	minsym = ~maxsym;
	shp = (Elf_Shdr *)(end + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shp[i].sh_type != SHT_SYMTAB &&
		    shp[i].sh_type != SHT_STRTAB) {
			continue;
		}
		minsym = min(minsym, (vaddr_t)end + shp[i].sh_offset);
		maxsym = max(maxsym, (vaddr_t)end + shp[i].sh_offset +
			     shp[i].sh_size);
	}

	/*
	 * The Sun3/50 has further restrictions on the
	 * size of the kernel boot image.  If preserving
	 * the symbol table would take us over the limit,
	 * then just ignore the symbols.
	 */

	if ((cpu_machine_id == ID_SUN3_50) &&
	    ((vaddr_t)maxsym > (KERNBASE + OBMEM_BW50_ADDR - USPACE))) {
		mon_printf("_save_symtab: too large for 3/50");
		return;
	}

	nsym = 1;
	ssym = (char *)ehdr;
	esym = (char *)maxsym;
}
#endif	/* DDB */

/*
 * This function is called from _bootstrap() to initialize
 * pre-vm-sytem virtual memory.  All this really does is to
 * set virtual_avail to the first page following preloaded
 * data (i.e. the kernel and its symbol table) and special
 * things that may be needed very early (lwp0 upages).
 * Once that is done, pmap_bootstrap() is called to do the
 * usual preparations for our use of the MMU.
 */
static void 
_vm_init(void)
{
	vaddr_t nextva;

	/*
	 * First preserve our symbol table, which might have been
	 * loaded after our BSS area by the boot loader.  However,
	 * if DDB is not part of this kernel, ignore the symbols.
	 */
	esym = end + 4;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* This will advance esym past the symbols. */
	_save_symtab();
#endif

	/*
	 * Steal some special-purpose, already mapped pages.
	 * Note: msgbuf is setup in machdep.c:cpu_startup()
	 */
	nextva = m68k_round_page(esym);

	/*
	 * Setup the u-area pages (stack, etc.) for lwp0.
	 * This is done very early (here) to make sure the
	 * fault handler works in case we hit an early bug.
	 * (The fault handler may reference lwp0 stuff.)
	 */
	uvm_lwp_setuarea(&lwp0, nextva);
	memset((void *)nextva, 0, USPACE);

	nextva += USPACE;

	/*
	 * Now that lwp0 exists, make it the "current" one.
	 */
	curlwp = &lwp0;
	curpcb = lwp_getpcb(&lwp0);

	/* This does most of the real work. */
	pmap_bootstrap(nextva);
}

/*
 * Determine which Sun3 model we are running on.
 * We have to do this very early on the Sun3 because
 * pmap_bootstrap() needs to know if it should avoid
 * the video memory on the Sun3/50.
 *
 * XXX: Just save idprom.idp_machtype here, and
 * XXX: move the rest of this to identifycpu().
 * XXX: Move cache_size stuff to cache.c.
 */
static void 
_verify_hardware(void)
{
	unsigned char machtype;
	int cpu_match = 0;

	machtype = identity_prom.idp_machtype;
	if ((machtype & IDM_ARCH_MASK) != IDM_ARCH_SUN3) {
		mon_printf("Bad IDPROM arch!\n");
		sunmon_abort();
	}

	cpu_machine_id = machtype;
	switch (cpu_machine_id) {

	case ID_SUN3_50 :
		cpu_match++;
		cpu_string = "50";
		delay_divisor = 128;	/* 16 MHz */
		break;

	case ID_SUN3_60 :
		cpu_match++;
		cpu_string = "60";
		delay_divisor = 102;	/* 20 MHz */
		break;

	case ID_SUN3_110:
		cpu_match++;
		cpu_string = "110";
		delay_divisor = 120;	/* 17 MHz */
		cpu_has_vme = true;
		break;

	case ID_SUN3_160:
		cpu_match++;
		cpu_string = "160";
		delay_divisor = 120;	/* 17 MHz */
		cpu_has_vme = true;
		break;

	case ID_SUN3_260:
		cpu_match++;
		cpu_string = "260";
		delay_divisor = 82; 	/* 25 MHz */
		cpu_has_vme = true;
#ifdef	HAVECACHE
		cache_size = 0x10000;	/* 64K */
#endif
		break;

	case ID_SUN3_E  :
		cpu_match++;
		cpu_string = "E";
		delay_divisor = 102;	/* 20 MHz  XXX: Correct? */
		cpu_has_vme = true;
		break;

	default:
		mon_printf("unknown sun3 model\n");
		sunmon_abort();
	}
	if (!cpu_match) {
		mon_printf("kernel not configured for the Sun 3 model\n");
		sunmon_abort();
	}
}

/*
 * This is called from locore.s just after the kernel is remapped
 * to its proper address, but before the call to main().  The work
 * done here corresponds to various things done in locore.s on the
 * hp300 port (and other m68k) but which we prefer to do in C code.
 * Also do setup specific to the Sun PROM monitor and IDPROM here.
 */
void 
_bootstrap(void)
{

	/* First, Clear BSS. */
	memset(edata, 0, end - edata);

	/* Set v_handler, get boothowto. */
	sunmon_init();

	/* Copy the IDPROM from control space. */
	idprom_init();

	/* Validate the Sun3 model (from IDPROM). */
	_verify_hardware();

	/* Handle kernel mapping, pmap_bootstrap(), etc. */
	_vm_init();

	/*
	 * Find and save OBIO mappings needed early,
	 * and call some init functions.
	 */
	obio_init();

	/*
	 * Point interrupts/exceptions to our vector table.
	 * (Until now, we use the one setup by the PROM.)
	 *
	 * This is done after obio_init() / intreg_init() finds
	 * the interrupt register and disables the NMI clock so
	 * it will not cause "spurrious level 7" complaints.
	 * Done after _vm_init so the PROM can debug that.
	 */
	setvbr((void **)vector_table);
	/* Interrupts are enabled later, after autoconfig. */

	/*
	 * Turn on the LEDs so we know power is on.
	 * Needs idprom_init and obio_init earlier.
	 */
	leds_init();
}
