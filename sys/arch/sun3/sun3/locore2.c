/*	$NetBSD: locore2.c,v 1.66 1997/06/10 19:25:28 veego Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/exec_aout.h>

#include <vm/vm.h>

#include <machine/control.h>
#include <machine/cpu.h>
#include <machine/db_machdep.h>
#include <machine/dvma.h>
#include <machine/mon.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/idprom.h>
#include <machine/obio.h>
#include <machine/obmem.h>
#include <machine/machdep.h>

#include <sun3/sun3/interreg.h>
#include <sun3/sun3/vector.h>

/* This is defined in locore.s */
extern char kernel_text[];

/* These are defined by the linker */
extern char etext[], edata[], end[];
char *esym;	/* DDB */


/*
 * Globals shared between pmap.c and here (sigh).
 * For simplicity, this interface retains the variables
 * that were used in the old interface (without NONCONTIG).
 */
vm_offset_t virtual_avail, virtual_end;
vm_offset_t avail_start, avail_end;
/* used to skip the Sun3/50 video RAM */
vm_offset_t hole_start, hole_size;
int cache_size;

/*
 * XXX: m68k common code needs these...
 * ... but this port does not need to deal with anything except
 * an mc68020, so these two variables are always ignored.
 * XXX: Need to do something about <m68k/include/cpu.h>
 */
int cputype = 0;	/* CPU_68020 */
int mmutype = 2;	/* MMU_SUN */

/*
 * Now our own stuff.
 */

unsigned char cpu_machine_id = 0;
char *cpu_string = NULL;
int cpu_has_vme = 0;

/*
 * XXX - Should empirically estimate the divisor...
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuclock	(where cpuclock is in MHz).
 */
int delay_divisor = 82;		/* assume the fastest (3/260) */

vm_offset_t high_segment_free_start = 0;
vm_offset_t high_segment_free_end = 0;

int msgbufmapped = 0;
struct msgbuf *msgbufp = NULL;
extern vm_offset_t tmp_vpages[];
extern int physmem;

struct user *proc0paddr;	/* proc[0] pcb address (u-area VA) */
extern struct pcb *curpcb;

extern vm_offset_t dumppage_pa;
extern vm_offset_t dumppage_va;

/* First C code called by locore.s */
void _bootstrap __P((struct exec));

static void _mon_init __P((vm_offset_t sva, vm_offset_t eva, int keep));
static void _verify_hardware __P((void));
static void _vm_init __P((struct exec *kehp));


/*
 * XXX - This can go away soon...
 */
vm_offset_t
high_segment_alloc(npages)
	int npages;
{
	vm_offset_t va, tmp;

	if (npages == 0) {
		mon_printf("high_segment_alloc: npages=0\n");
		sunmon_abort();
	}
	if (high_segment_free_start == high_segment_free_end)
		return NULL;

	va = high_segment_free_start + (npages*NBPG);
	if (va > high_segment_free_end) return NULL;
	tmp = high_segment_free_start;
	high_segment_free_start = va;
	return tmp;
}

/*
 * Duplicate all mappings in the current context into
 * every other context.  We have to let the PROM do the
 * actual segmap manipulation because we can only switch
 * the MMU context after we are sure that the kernel text
 * is identically mapped in all contexts.  The PROM can
 * do the job using hardware-dependent tricks...
 */
void
sun3_context_equiv __P((void))
{
	unsigned int sme;
	int x;
	vm_offset_t va;

#ifdef	DIAGNOSTIC
	/* Near the beginning of locore.s we set context zero. */
	if (get_context() != 0) {
		mon_printf("sun3_context_equiv: not in context zero?\n");
		sunmon_abort();
	}
	/* Note: PROM setcxsegmap function needs sfc=dfs=FC_CONTROL */
	if ((getsfc() != FC_CONTROL) || (getdfc() != FC_CONTROL)) {
		mon_printf("sun3_context_equiv: bad dfc or sfc\n");
		sunmon_abort();
	}
#endif

	for (x = 1; x < NCONTEXT; x++) {
		for (va = 0; va < (vm_offset_t) (NBSG * NSEGMAP); va += NBSG) {
			sme = get_segmap(va);
			mon_setcxsegmap(x, va, sme);
		}
	}
}

/*
 * Examine PMEGs used by the monitor, and either
 * reserve them (keep=1) or clear them (keep=0)
 */
static void
_mon_init(sva, eva, keep)
	vm_offset_t sva, eva;
	int keep;	/* true: steal, false: clear */
{
	vm_offset_t pgva, endseg;
	int pte, valid;
	unsigned char sme;

	sva &= ~(NBSG-1);

	while (sva < eva) {
		sme = get_segmap(sva);
		if (sme != SEGINV) {
#ifdef	DEBUG
			mon_printf("mon va=0x%x seg=0x%x\n", sva, sme);
#endif
			valid = 0;
			endseg = sva + NBSG;
			for (pgva = sva; pgva < endseg; pgva += NBPG) {
				pte = get_pte(pgva);
				if (pte & PG_VALID) {
					valid++;
#ifdef	DEBUG
					mon_printf("mon va=0x%x pte=0x%x\n", pgva, pte);
#endif
				}
			}
			if (keep && valid)
				sun3_reserve_pmeg(sme);
			else set_segmap(sva, SEGINV);
		}
		sva += NBSG;
	}
}

#if defined(DDB) && !defined(SYMTAB_SPACE)
static void _save_symtab __P((struct exec *kehp));

/*
 * Preserve DDB symbols and strings by setting esym.
 */
static void
_save_symtab(kehp)
	struct exec *kehp;	/* kernel exec header */
{
	int x, *symsz, *strsz;
	char *endp;
	char *errdesc = "?";

	/*
	 * First, sanity-check the exec header.
	 */
	if ((kehp->a_midmag & 0xFFF0) != 0x0100) {
		errdesc = "magic";
		goto err;
	}
	/* Boundary between text and data varries a little. */
	x = kehp->a_text + kehp->a_data;
	if (x != (edata - kernel_text)) {
		errdesc = "a_text+a_data";
		goto err;
	}
	if (kehp->a_bss != (end - edata)) {
		errdesc = "a_bss";
		goto err;
	}
	if (kehp->a_entry != (int)kernel_text) {
		errdesc = "a_entry";
		goto err;
	}
	if (kehp->a_trsize || kehp->a_drsize) {
		errdesc = "a_Xrsize";
		goto err;
	}
	/* The exec header looks OK... */

	/* Check the symtab length word. */
	endp = end;
	symsz = (int*)endp;
	if (kehp->a_syms != *symsz) {
		errdesc = "a_syms";
		goto err;
	}
	endp += sizeof(int);	/* past length word */
	endp += *symsz;			/* past nlist array */

	/* Sanity-check the string table length. */
	strsz = (int*)endp;
	if ((*strsz < 4) || (*strsz > 0x80000)) {
		errdesc = "strsize";
		goto err;
	}

	/* Success!  We have a valid symbol table! */
	endp += *strsz;			/* past strings */
	esym = endp;
	return;

 err:
	mon_printf("_save_symtab: bad %s\n", errdesc);
}
#endif	/* DDB && !SYMTAB_SPACE */

/*
 * This is called just before pmap_bootstrap()
 * (from _bootstrap(), below) to initialize enough
 * to allow the VM system to run normally.  This involves
 * allocating some physical pages and virtual space for
 * special purposes, etc. by advancing avail_start and
 * virtual_avail past the "stolen" pages.
 */
static void
_vm_init(kehp)
	struct exec *kehp;	/* kernel exec header */
{
	MachMonRomVector *rvec;
	vm_offset_t va, eva, pte;
	unsigned int sme;

	rvec = romVectorPtr;

	/*
	 * Determine the range of kernel virtual space available.
	 * This is just page-aligned for now, so we can allocate
	 * some special-purpose pages before rounding to a segment.
	 */
	esym = end;
#if defined(DDB) && !defined(SYMTAB_SPACE)
	/* This will advance esym past the symbols. */
	_save_symtab(kehp);
#endif
	virtual_avail = m68k_round_page(esym);
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Determine the range of physical memory available.
	 * Physical memory at zero was remapped to KERNBASE.
	 */
	avail_start = virtual_avail - KERNBASE;
	if (rvec->romvecVersion < 1) {
		mon_printf("Warning: ancient PROM version=%d\n",
				   rvec->romvecVersion);
		/* Guess that PROM version 0.X used two pages. */
		avail_end = *rvec->memorySize - (2*NBPG);
	} else {
		/* PROM version 1 or later. */
		avail_end = *rvec->memoryAvail;
	}
	avail_end = m68k_trunc_page(avail_end);

	/*
	 * Steal some special-purpose, already mapped pages.
	 * First take pages that are already mapped as
	 * VA -> PA+KERNBASE since that's convenient.
	 */

	/*
	 * Message buffer page (msgbuf).
	 * This is put in physical page zero so it
	 * is always in the same place after reboot.
	 */
	va = KERNBASE;
	/* XXX: Make it non-cached? */
	pte = get_pte(va);
	pte |= PG_NC;
	set_pte(va, pte);
	/* Initialize msgbufp later, in machdep.c */

	/*
	 * Virtual and physical pages for proc[0] u-area (already mapped)
	 */
	proc0paddr = (struct user *) virtual_avail;
	virtual_avail += UPAGES*NBPG;
	avail_start   += UPAGES*NBPG;

	/*
	 * Virtual and physical page used by dumpsys()
	 */
	dumppage_va = virtual_avail;
	dumppage_pa = avail_start;
	virtual_avail += NBPG;
	avail_start   += NBPG;

	/*
	 * XXX - Make sure avail_start is within the low 1M range
	 * that the Sun PROM guarantees will be mapped in?
	 * Make sure it is below avail_end as well?
	 */

	/*
	 * Now steal some virtual addresses, but
	 * not the physical pages behind them.
	 */
	va = virtual_avail;	/* will clear PTEs from here */

	/*
	 * vpages array:  just some virtual addresses for
	 * temporary mappings in the pmap module (two pages)
	 */
	tmp_vpages[0] = virtual_avail;
	virtual_avail += NBPG;
	tmp_vpages[1] = virtual_avail;
	virtual_avail += NBPG;

	/*
	 * Done allocating PAGES of virtual space, so
	 * clean out the rest of the last used segment.
	 * After this point, virtual_avail is seg-aligned.
	 */
	virtual_avail = m68k_round_seg(virtual_avail);
	while (va < virtual_avail) {
		set_pte(va, PG_INVAL);
		va += NBPG;
	}

	/*
	 * Now that we are done stealing physical pages, etc.
	 * figure out which PMEGs are used by those mappings
	 * and reserve them -- but first, init PMEG management.
	 */
	sun3_pmeg_init();

	/*
	 * Reserve PMEGS for kernel text/data/bss
	 * and the misc pages taken above.
	 */
	va = VM_MIN_KERNEL_ADDRESS;
	while (va < virtual_avail) {
		sme = get_segmap(va);
		if (sme == SEGINV) {
			mon_printf("kernel text/data/bss not mapped\n");
			sunmon_abort();
		}
		sun3_reserve_pmeg(sme);
		va += NBSG;
	}

	/*
	 * Unmap kernel virtual space (only segments.  if it squished ptes,
	 * bad things might happen).  Also, make sure to leave no valid
	 * segmap entries in the MMU unless pmeg_array records them.
	 */
	va = virtual_avail;
	while (va < virtual_end) {
		set_segmap(va, SEGINV);
		va += NBSG;
	}

	/*
	 * Clear-out pmegs left in DVMA space by the PROM.
	 * DO NOT kill the last one! (owned by the PROM!)
	 */
	va  = m68k_trunc_seg(DVMA_SPACE_START);
	eva = m68k_trunc_seg(DVMA_SPACE_END);  /* Yes trunc! */
	while (va < eva) {
		set_segmap(va, SEGINV);
		va += NBSG;
	}

	/*
	 * Reserve PMEGs used by the PROM monitor:
	 *   need to preserve/protect mappings between
	 *		MONSTART, MONEND.
	 *   free up any pmegs in this range which have no mappings
	 *   deal with the awful MONSHORTSEG/MONSHORTPAGE
	 */
	_mon_init(MONSTART, MONEND, TRUE);

	/*
	 * Make sure the hole between MONEND, MONSHORTSEG is clear.
	 */
	_mon_init(MONEND, MONSHORTSEG, FALSE);

	/*
	 * MONSHORTSEG contains MONSHORTPAGE which is some stupid page
	 * allocated by the PROM monitor.  (PROM data)
	 * We use some of the segment for our u-area mapping.
	 */
	sme = get_segmap(MONSHORTSEG);
	sun3_reserve_pmeg(sme);
	high_segment_free_start = MONSHORTSEG;
	high_segment_free_end = MONSHORTPAGE;

	for (va = high_segment_free_start;
		 va < high_segment_free_end;
		 va += NBPG)
		set_pte(va, PG_INVAL);

	/*
	 * Initialize the "u-area" pages.
	 * Must initialize p_addr before autoconfig or
	 * the fault handler will get a NULL reference.
	 */
	bzero((caddr_t)proc0paddr, USPACE);
	proc0.p_addr = proc0paddr;
	curproc = &proc0;
	curpcb = &proc0paddr->u_pcb;

	/*
	 * XXX  It might be possible to move much of what is
	 * XXX  done after this point into pmap_bootstrap...
	 */

	/*
	 * unmap user virtual segments
	 */
	va = 0;
	while (va < KERNBASE) {	/* starts and ends on segment boundries */
		set_segmap(va, SEGINV);
		va += NBSG;
	}

	/*
	 * Verify protection bits on kernel text/data/bss
	 * All of kernel text, data, and bss are cached.
	 * Text is read-only (except in db_write_ktext).
	 *
	 * Note that the Sun PROM initialized the memory
	 * mapping with everything non-cached...
	 */

	/* text */
	va = (vm_offset_t) kernel_text;
	eva = m68k_trunc_page(etext);
	while (va < eva) {
		pte = get_pte(va);
		if ((pte & (PG_VALID|PG_TYPE)) != PG_VALID) {
			mon_printf("invalid page at 0x%x\n", va);
		}
		pte &= ~(PG_WRITE|PG_NC);
		/* Kernel text is read-only */
		pte |= (PG_SYSTEM);
		set_pte(va, pte);
		va += NBPG;
	}

	/* data and bss */
	eva = m68k_round_page(end);
	while (va < eva) {
		pte = get_pte(va);
		if ((pte & (PG_VALID|PG_TYPE)) != PG_VALID) {
			mon_printf("invalid page at 0x%x\n", va);
		}
		pte &= ~(PG_NC);
		pte |= (PG_SYSTEM | PG_WRITE);
		set_pte(va, pte);
		va += NBPG;
	}

	/* Finally, duplicate the mappings into all contexts. */
	sun3_context_equiv();

	pmap_bootstrap();	/* bootstrap pmap module */
}

/*
 * Determine which Sun3 model we are running on.
 * We have to do this very early on the Sun3 because
 * pmap_bootstrap() needs to know if it should avoid
 * the video memory on the Sun3/50.
 */
static void
_verify_hardware()
{
	unsigned char machtype;
	int cpu_match = 0;

	idprom_init();

	machtype = identity_prom.idp_machtype;
	if ((machtype & CPU_ARCH_MASK) != SUN3_ARCH) {
		mon_printf("not a sun3?\n");
		sunmon_abort();
	}

	cpu_machine_id = machtype & SUN3_IMPL_MASK;
	switch (cpu_machine_id) {

	case SUN3_MACH_50 :
		cpu_match++;
		hole_start = OBMEM_BW50_ADDR;
		hole_size  = OBMEM_BW2_SIZE;
		cpu_string = "50";
		delay_divisor = 128;	/* 16 MHz */
		break;

	case SUN3_MACH_60 :
		cpu_match++;
		cpu_string = "60";
		delay_divisor = 102;	/* 20 MHz */
		break;

	case SUN3_MACH_110:
		cpu_match++;
		cpu_string = "110";
		delay_divisor = 120;	/* 17 MHz */
		cpu_has_vme = TRUE;
		break;

	case SUN3_MACH_160:
		cpu_match++;
		cpu_string = "160";
		delay_divisor = 120;	/* 17 MHz */
		cpu_has_vme = TRUE;
		break;

	case SUN3_MACH_260:
		cpu_match++;
		cpu_string = "260";
		delay_divisor = 82; 	/* 25 MHz */
		cpu_has_vme = TRUE;
#ifdef	HAVECACHE
		cache_size = 0x10000;	/* 64K */
#endif
		break;

	case SUN3_MACH_E  :
		cpu_match++;
		cpu_string = "E";
		delay_divisor = 102;	/* 20 MHz  XXX: Correct? */
		cpu_has_vme = TRUE;
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
_bootstrap(keh)
	struct exec keh;	/* kernel exec header */
{

	/* First, Clear BSS. */
	bzero(edata, end - edata);

	/* Set v_handler, get boothowto. */
	sunmon_init();

	/* Determine the Sun3 model. */
	_verify_hardware();

	/* handle kernel mapping, pmap_bootstrap(), etc. */
	_vm_init(&keh);

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
}
