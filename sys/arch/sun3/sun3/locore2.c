/*	$NetBSD: locore2.c,v 1.34 1995/02/13 22:24:26 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Adam Glass and Gordon Ross
 * 4. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <machine/control.h>
#include <machine/cpufunc.h>
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/control.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/idprom.h>
#include <machine/obio.h>
#include <machine/obmem.h>

#include "vector.h"
#include "interreg.h"

/* This is defined in locore.s */
extern char kernel_text[];

/* These are defined by the linker */
extern char etext[], edata[], end[];

/*
 * Globals shared with the pmap code.
 * XXX - should reexamine this...
 */ 
vm_offset_t virtual_avail, virtual_end;
vm_offset_t avail_start, avail_end;
/* used to skip the Sun3/50 video RAM */
vm_offset_t hole_start, hole_size;

/*
 * Now our own stuff.
 */
unsigned int *old_vector_table;

unsigned char cpu_machine_id = 0;
char *cpu_string = NULL;
int cpu_has_vme = 0;

/* XXX - Need real code to do this at startup. */
#ifdef	FPCOPROC
int fpu_type = 1;	/* XXX */
#else
int fpu_type = 0;	/* XXX */
#endif

vm_offset_t high_segment_free_start = 0;
vm_offset_t high_segment_free_end = 0;

int msgbufmapped = 0;
struct msgbuf *msgbufp = NULL;
extern vm_offset_t tmp_vpages[];
extern int physmem;
unsigned char *interrupt_reg;

vm_offset_t proc0_user_pa;
struct user *proc0paddr;	/* proc[0] pcb address (u-area VA) */
extern struct pcb *curpcb;

/*
 * Switch to our own interrupt vector table.
 */
static void initialize_vector_table()
{
	old_vector_table = getvbr();
	setvbr((unsigned int *) vector_table);
}

vm_offset_t high_segment_alloc(npages)
	int npages;
{
	int i;
	vm_offset_t va, tmp;
	
	if (npages == 0)
		mon_panic("panic: request for high segment allocation of 0 pages");
	if (high_segment_free_start == high_segment_free_end) return NULL;
	
	va = high_segment_free_start + (npages*NBPG);
	if (va > high_segment_free_end) return NULL;
	tmp = high_segment_free_start;
	high_segment_free_start = va;
	return tmp;
}

/*
 * Prepare for running the PROM monitor
 */
static void sun3_mode_monitor()
{
	/* Install PROM vector table and enable NMI clock. */
	/* XXX - Disable watchdog action? */
	set_clk_mode(0, IREG_CLOCK_ENAB_5, 0);
	setvbr(old_vector_table);
	set_clk_mode(IREG_CLOCK_ENAB_7, 0, 1);
}

/*
 * Prepare for running the kernel
 */
static void sun3_mode_normal()
{
	/* Install our vector table and disable the NMI clock. */
	set_clk_mode(0, IREG_CLOCK_ENAB_7, 0);
	setvbr((unsigned int *) vector_table);
	set_clk_mode(IREG_CLOCK_ENAB_5, 0, 1);
}

/*
 * This function takes care of restoring enough of the
 * hardware state to allow the PROM to run normally.
 * The PROM needs: NMI enabled, it's own vector table.
 * In case of a temporary "drop into PROM", this will
 * also put our hardware state back into place after
 * the PROM "c" (continue) command is given.
 */
void sun3_mon_abort()
{
	int s = splhigh();

	sun3_mode_monitor();
	mon_printf("kernel stop: enter c to continue or g0 to panic\n");
	delay(100000);

	/*
	 * Drop into the PROM in a way that allows a continue.
	 * That's what the PROM function (romp->abortEntry) is for,
	 * but that wants to be entered as a trap hander, so just
	 * stuff it into the PROM interrupt vector for trap zero
	 * and then do a trap.  Needs PROM vector table in RAM.
	 */
	old_vector_table[32] = (int)romp->abortEntry;
	asm(" trap #0 ; _sun3_mon_continued: nop");

	/* We have continued from a PROM abort! */

	sun3_mode_normal();
	splx(s);
}

void sun3_mon_halt()
{
	(void) splhigh();
	sun3_mode_monitor();
	mon_exit_to_mon();
	/*NOTREACHED*/
}

void sun3_mon_reboot(bootstring)
	char *bootstring;
{
	(void) splhigh();
	sun3_mode_monitor();
	mon_reboot(bootstring);
	mon_exit_to_mon();
	/*NOTREACHED*/
}

void sun3_context_equiv()
{
	unsigned int i, sme;
	int x;
	vm_offset_t va;

#ifdef	DIAGNOSTIC
	/* Near the beginning of locore.s we set context zero. */
	if (get_context() != 0)
		mon_panic("sun3_context_equiv: not in context zero?\n");
	/* Note: PROM setcxsegmap function needs sfc=dfs=FC_CONTROL */
	if (getsfc() != FC_CONTROL)
		mon_panic("sun3_context_equiv: sfc != FC_CONTROL?\n");
	if (getdfc() != FC_CONTROL)
		mon_panic("sun3_context_equiv: dfc != FC_CONTROL?\n");
#endif

	for (x = 1; x < NCONTEXT; x++) {
		for (va = 0; va < (vm_offset_t) (NBSG * NSEGMAP); va += NBSG) {
			sme = get_segmap(va);
			mon_setcxsegmap(x, va, sme);
		}
	}
}

static void
sun3_mon_init(sva, eva, keep)
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


/*
 * This is called just before pmap_bootstrap()
 * (from sun3_bootstrap(), below) to initialize enough
 * to allow the VM system to run normally.  This involves
 * allocating some physical pages and virtual space for
 * special purposes, etc. by advancing avail_start and
 * virtual_avail past the "stolen" pages.  Note that
 * the kernel should never take a fault on any page
 * between [ KERNBASE .. virtual_avail ] and this is
 * checked in trap.c for kernel-mode MMU faults.
 */
void sun3_vm_init()
{
	vm_offset_t va, eva, sva, pte, temp_seg;
	vm_offset_t u_area_va;
	unsigned int sme;

	/*
	 * XXX - Reserve DDB symbols and strings?
	 */

	/*
	 * Determine the range of kernel virtual space available.
	 * This is just page-aligned for now, so we can allocate
	 * some special-purpose pages before rounding to a segment.
	 */
	virtual_avail = sun3_round_page(end);
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Determine the range of physical memory available.
	 * Physical memory at zero was remapped to KERNBASE.
	 */
	avail_start = virtual_avail - KERNBASE;
	if (romp->romvecVersion < 1) {
		mon_printf("Warning: ancient PROM version=%d\n",
				   romp->romvecVersion);
		/* Guess that PROM version 0.X used two pages. */
		avail_end = *romp->memorySize - (2*NBPG);
	} else {
		/* PROM version 1 or later. */
		avail_end = *romp->memoryAvail;
	}
	avail_end = sun3_trunc_page(avail_end);

	/*
	 * Steal some special-purpose, already mapped pages.
	 * First take pages that are already mapped as
	 * VA -> PA+KERNBASE since that's convenient.
	 */

	/*
	 * Message buffer page (msgbuf).
	 * XXX - Should put this in physical page zero.
	 * Also unmap a page at KERNBASE for redzone?
	 */
	msgbufp = (struct msgbuf *) virtual_avail;
	virtual_avail += NBPG;
	avail_start += NBPG;
	msgbufmapped = 1;
	/* Make it non-cached. */
	va = (vm_offset_t) msgbufp;
	pte = get_pte(va);
	pte |= PG_NC;
	set_pte(va, pte);

	/*
	 * Virtual and physical pages for proc[0] u-area (already mapped)
	 * Make these non-cached at their full-time mapping address.
	 * The running proc's upages are also mapped cached at kstack.
	 */
	proc0paddr = (struct user *) virtual_avail;
	proc0_user_pa = avail_start;
	virtual_avail += UPAGES*NBPG;
	avail_start += UPAGES*NBPG;
	/* Make them non-cached. */
	va = (vm_offset_t) proc0paddr;
	while (va < virtual_avail) {
		pte = get_pte(va);
		pte |= PG_NC;
		set_pte(va, pte);
		va += NBPG;
	}

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
	virtual_avail = sun3_round_seg(virtual_avail);
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
		if (sme == SEGINV)
			mon_panic("kernel text/data/bss not mapped\n");
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
	 */
	va = sun3_trunc_seg(DVMA_SPACE_START);
	while (va < DVMA_SPACE_END) {
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
	sun3_mon_init(MONSTART, MONEND, TRUE);

	/*
	 * Make sure the hole between MONEND, MONSHORTSEG is clear.
	 */
	sun3_mon_init(MONEND, MONSHORTSEG, FALSE);

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
	 * Reserve VA space for the u-area of the current process,
	 * and map proc[0]'s u-pages at the standard address.
	 */
	u_area_va = high_segment_alloc(UPAGES);
	if (u_area_va != UADDR)
		mon_panic("sun3_vm_init: u-area vaddr taken?\n");

	/* Initialize cached PTEs for u-area mapping. */
	save_u_area(&proc0, proc0paddr);

	/* map proc0's u-area at the standard address */
#ifdef	DIAGNOSTIC
	if (curproc != &proc0)
		panic("sun3_vm_init: bad curproc");
#endif
	load_u_area();

	bzero((caddr_t)UADDR, UPAGES*NBPG);

	curpcb = &proc0paddr->u_pcb;

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
	eva = sun3_trunc_page(etext);
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
	eva = sun3_round_page(end);
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
}

void kstack_fall_off()
{
	mon_printf("kstack: fell off\n");
}

void sun3_verify_hardware()
{
	unsigned char machtype;
	int cpu_match = 0;

	/* XXX - Should just measure this instead... */
	extern int cpuspeed;

	if (idprom_init())
		mon_panic("idprom_init failed\n");

	machtype = identity_prom.idp_machtype;
	if ((machtype & CPU_ARCH_MASK) != SUN3_ARCH)
		mon_panic("not a sun3?\n");

	cpu_machine_id = machtype & SUN3_IMPL_MASK;
	switch (cpu_machine_id) {

	case SUN3_MACH_50 :
#ifdef SUN3_50
		cpu_match++;
		hole_start = OBMEM_BW50_ADDR;
		hole_size  = OBMEM_BW2_SIZE;
#endif
		cpu_string = "50";
		cpuspeed = 16; /* MHz */
		break;

	case SUN3_MACH_60 :
#ifdef SUN3_60
		cpu_match++;
#endif
		cpu_string = "60";
		cpuspeed = 20; /* MHz */
		break;

	case SUN3_MACH_110:
#ifdef SUN3_110
		cpu_match++;
#endif
		cpu_string = "110";
		cpuspeed = 25; /* MHz */	/* XXX - Correct? */
		cpu_has_vme = TRUE;
		break;

	case SUN3_MACH_160:
#ifdef SUN3_160
		cpu_match++;
#endif
		cpu_string = "160";
		cpuspeed = 25; /* MHz */	/* XXX - Correct? */
		cpu_has_vme = TRUE;
		break;

	case SUN3_MACH_260:
#ifdef SUN3_260
		cpu_match++;
#endif
		cpu_string = "260";
		cpuspeed = 25; /* MHz */	/* XXX - Correct? */
		cpu_has_vme = TRUE;
		break;

	case SUN3_MACH_E  :
#ifdef SUN3_E
		cpu_match++;
#endif
		cpu_string = "E";
		cpuspeed = 30; /* MHz */	/* XXX - Correct? */
		cpu_has_vme = TRUE;
		break;

	default:
		mon_panic("unknown sun3 model\n");
	}
	if (!cpu_match)
		mon_panic("kernel not configured for the Sun 3 model\n");
}

/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing "g4" (for sun-2
 * compatibility) or "w trace".  This causes the monitor to call
 * the v_handler() routine which will call tracedump() for these cases.
 */
struct funcall_frame {
	struct funcall_frame *fr_savfp;
	int fr_savpc;
	int fr_arg[1];
};
/*VARARGS0*/
tracedump(x1)
	caddr_t x1;
{
	struct funcall_frame *fp = (struct funcall_frame *)(&x1 - 2);
	u_int tospage = btoc(fp);
	
	mon_printf("Begin traceback...fp = %x\n", fp);
	while (btoc(fp) == tospage) {
		if (fp == fp->fr_savfp) {
			mon_printf("FP loop at %x", fp);
			break;
		}
		mon_printf("Called from %x, fp=%x, args=%x %x %x %x\n",
				   fp->fr_savpc, fp->fr_savfp,
				   fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	mon_printf("End traceback...\n");
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.  [lifted from freed cmu mach3 sun3 port]
 */
void
v_handler(addr, str)
int addr;
char *str;
{
	
	switch (*str) {
	case '\0':
		/*
		 * No (non-hex) letter was specified on
		 * command line, use only the number given
		 */
		switch (addr) {
		case 0:			/* old g0 */
		case 0xd:		/* 'd'ump short hand */
			sun3_mode_normal();
			panic("zero");
			/*NOTREACHED*/
			
		case 4:			/* old g4 */
			tracedump();
			break;
			
		default:
			goto err;
		}
		break;
		
	case 'p':			/* 'p'rint string command */
	case 'P':
		mon_printf("%s\n", (char *)addr);
		break;
		
	case '%':			/* p'%'int anything a la printf */
		mon_printf(str, addr);
		mon_printf("\n");
		break;
		
	case 't':			/* 't'race kernel stack */
	case 'T':
		tracedump();
		break;
		
	case 'u':			/* d'u'mp hack ('d' look like hex) */
	case 'U':
		goto err;
		break;
		
	default:
	err:
		mon_printf("Don't understand 0x%x '%s'\n", addr, str);
	}
}

/*
 * Set the PROM vector handler (for g0, g4, etc.)
 * and set boothowto from the PROM arg strings.
 */
void sun3_monitor_hooks()
{
	MachMonBootParam *bpp;
	char *p;
	int i;

	if (romp->romvecVersion >= 2)
		*romp->vector_cmd = v_handler;

	/* Set boothowto flags from PROM args. */
	bpp = *romp->bootParam;
	for (i = 0; i < 8; i++) {
		p = bpp->argPtr[i];

		/* Null arg?  We're done. */
		if (p == NULL || *p == '\0')
			break;
#ifdef	DEBUG
		mon_printf("arg[%d]=\"%s\"\n", i, p);
#endif

		/* Not switches?  Skip it. */
		if (*p++ != '-')
			continue;

		while (*p) {
			switch (*p++) {
			case 'a':
				boothowto |= RB_ASKNAME;
				break;
			case 's':
				boothowto |= RB_SINGLE;
				break;
			case 'd':
				boothowto |= RB_KDB;
				break;
			}
		}
	}
#ifdef	DEBUG
	mon_printf("boothowto=0x%x\n", boothowto);
#endif
}

void set_interrupt_reg(value)
	unsigned int value;
{
	*interrupt_reg = (unsigned char) value;
}

unsigned int get_interrupt_reg()
{
	vm_offset_t pte;
	return (unsigned int) *interrupt_reg;
}

void internal_configure()
{
    obio_init();
	/* now other obio devices */
	zs_init();
	eeprom_init();
	isr_init();
	clock_init();
}

/* XXX - Move this into a real device driver. */
void fpu_init()
{
	int enab_reg;

	if (fpu_type) {
		enab_reg = get_control_byte((char *) SYSTEM_ENAB);
		enab_reg |= SYSTEM_ENAB_FPP;
		set_control_byte((char *) SYSTEM_ENAB, enab_reg);
	}
}

/*
 * This is called from locore.s just after the kernel is remapped
 * to its proper address, but before the call to main().
 */
void
sun3_bootstrap()
{
	int i;
	extern int cold;

	/* Clear BSS. */
	bzero(edata, end - edata);

	cold = 1;

	sun3_monitor_hooks();

	sun3_verify_hardware();

	sun3_vm_init();		/* handle kernel mapping problems, etc */

	pmap_bootstrap();		/* bootstrap pmap module */

	internal_configure();	/* stuff that can't wait for configure() */

	/*
	 * Point interrupts/exceptions to our table.
	 * This is done after internal_configure/isr_init finds
	 * the interrupt register and disables the NMI clock so
	 * it will not cause "spurrious level 7" complaints.
	 */
	initialize_vector_table();

	/* XXX - Move this into a real device driver. */
	fpu_init();
}
