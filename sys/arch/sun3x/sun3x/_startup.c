/*	$NetBSD: _startup.c,v 1.2.2.2 1997/01/16 21:53:08 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross, and Jeremy Cooper.
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

#include <machine/cpu.h>
#include <machine/db_machdep.h>
#include <machine/dvma.h>
#include <machine/mon.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/idprom.h>
#include <machine/obio.h>

#include "vector.h"
#include "interreg.h"
#include "machdep.h"

/* This is defined in locore.s */
extern char kernel_text[];

/* This is set by locore.s with the monitor's root ptr. */
extern struct mmu_rootptr mon_crp;

/* These are defined by the linker */
extern char etext[], edata[], end[];
char *esym;	/* DDB */

/*
 * Now our own stuff.
 */
int boothowto = RB_KDB; 	/* XXX - For now... */
int cold = 1;
void **old_vector_table;

unsigned char cpu_machine_id = 0;
char *cpu_string = NULL;
int cpu_has_vme = 0;
int has_iocache = 0;

int msgbufmapped = 0;
struct msgbuf *msgbufp = NULL;

struct user *proc0paddr;	/* proc[0] pcb address (u-area VA) */
extern struct pcb *curpcb;

/* First C code called by locore.s */
void _bootstrap __P((struct exec));

static void _monitor_hooks __P((void));
static void _verify_hardware __P((void));
static void _vm_init __P((struct exec *kehp));
static void tracedump __P((int));
static void v_handler __P((int addr, char *str));


/*
 * Prepare for running the PROM monitor
 */
static void
sun3x_mode_monitor __P((void))
{
	/* Install PROM vector table and enable NMI clock. */
	/* XXX - Disable watchdog action? */
	set_clk_mode(0, IREG_CLOCK_ENAB_5, 0);
	setvbr(old_vector_table);
	set_clk_mode(IREG_CLOCK_ENAB_7, 0, 1);

	loadcrp((int)&mon_crp);
}

/*
 * Prepare for running the kernel
 */
static void
sun3x_mode_normal __P((void))
{
	struct pcb *pcb;

	/* Install our vector table and disable the NMI clock. */
	set_clk_mode(0, IREG_CLOCK_ENAB_7, 0);
	setvbr((void**)vector_table);
	set_clk_mode(IREG_CLOCK_ENAB_5, 0, 1);

	pcb = curpcb ? curpcb :
		pcb = &proc0paddr->u_pcb;

	loadcrp(pcb->pcb_mmuctx);
}

/*
 * This function takes care of restoring enough of the
 * hardware state to allow the PROM to run normally.
 * The PROM needs: NMI enabled, it's own vector table.
 * In case of a temporary "drop into PROM", this will
 * also put our hardware state back into place after
 * the PROM "c" (continue) command is given.
 */
void sun3x_mon_abort()
{
	int s = splhigh();

	sun3x_mode_monitor();
	mon_printf("kernel stop: enter c to continue or g0 to panic\n");
	delay(100000);

	/*
	 * Drop into the PROM in a way that allows a continue.
	 * That's what the PROM function (romp->abortEntry) is for,
	 * but that wants to be entered as a trap hander, so just
	 * stuff it into the PROM interrupt vector for trap zero
	 * and then do a trap.  Needs PROM vector table in RAM.
	 */
	old_vector_table[32] = romVectorPtr->abortEntry;
	asm(" trap #0 ; _sun3x_mon_continued: nop");

	/* We have continued from a PROM abort! */

	sun3x_mode_normal();
	splx(s);
}

void sun3x_mon_halt()
{
	(void) splhigh();
	sun3x_mode_monitor();
	mon_exit_to_mon();
	/*NOTREACHED*/
}

/*
 * Caller must pass a string that is in our data segment.
 */
void sun3x_mon_reboot(bootstring)
	char *bootstring;
{
#ifdef	DIAGNOSTIC
	if (bootstring > end)
		bootstring = "?";
#endif

	(void) splhigh();
	sun3x_mode_monitor();
	mon_reboot(bootstring);
	mon_exit_to_mon();
	/*NOTREACHED*/
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
	mon_printf("_save_symtab: ");
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

	/* Check the string table length. */
	strsz = (int*)endp;
	if ((*strsz < 4) || (*strsz > 0x80000)) {
		errdesc = "strsize";
		goto err;
	}

	/* Success!  We have a valid symbol table! */
	endp += *strsz;			/* past strings */
	esym = endp;
	mon_printf(" found %d + %d\n", *symsz, *strsz);
	return;

 err:
	mon_printf(" no symbols (bad %s)\n", errdesc);
}
#endif	/* DDB && !SYMTAB_SPACE */

/*
 * This function is called from _bootstrap() to initialize
 * pre-vm-sytem virtual memory.  All this really does is to
 * set virtual_avail to the first page following preloaded
 * data (i.e. the kernel and its symbol table) and special
 * things that may be needed very early (proc0 upages).
 * Once that is done, pmap_bootstrap() is called to do the
 * usual preparations for our use of the MMU.
 */
static void
_vm_init(kehp)
	struct exec *kehp;	/* kernel exec header */
{
	vm_offset_t nextva;

	/*
	 * First, reserve our symbol table which might have been
	 * loaded after our BSS area by the boot loader.  However,
	 * if DDB is not part of this kernel, ignore the symbols.
	 */
	esym = end;
#if defined(DDB) && !defined(SYMTAB_SPACE)
	/* This will advance esym past the symbols. */
	_save_symtab(kehp);
#endif

	/*
	 * Steal some special-purpose, already mapped pages.
	 * Note: msgbuf is setup in machdep.c:cpu_startup()
	 */
	nextva = sun3x_round_page(esym);

	/*
	 * Setup the u-area pages (stack, etc.) for proc0.
	 * This is done very early (here) to make sure the
	 * fault handler works in case we hit an early bug.
	 * (The fault handler may reference proc0 stuff.)
	 */
	proc0paddr = (struct user *) nextva;
	nextva += USPACE;
	bzero((caddr_t)proc0paddr, USPACE);
	proc0.p_addr = proc0paddr;

	/*
	 * Now that proc0 exists, make it the "current" one.
	 */
	curproc = &proc0;
	curpcb = &proc0paddr->u_pcb;

	/*
	 * Call pmap_bootstrap() so that we may begin using
	 * pmap_enter_kernel() and pmap_bootstrap_alloc().
	 */
	pmap_bootstrap(nextva);
}


/*
 * XXX - Should empirically estimate the divisor...
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuclock	(where cpuclock is in MHz).
 */
int delay_divisor = 82;		/* assume the fastest (3/260) */

static void
_verify_hardware()
{
	unsigned char machtype;
	int cpu_match = 0;

	machtype = identity_prom.idp_machtype;
	if ((machtype & CPU_ARCH_MASK) != SUN3X_ARCH)
		mon_panic("not a sun3x?\n");

	cpu_machine_id = machtype & SUN3X_IMPL_MASK;
	switch (cpu_machine_id) {

	case SUN3X_MACH_80 :
		cpu_match++;
		cpu_string = "80";
		delay_divisor = 102;	/* 20 MHz ? XXX */
		cpu_has_vme = FALSE;
		break;

	case SUN3X_MACH_470:
		cpu_match++;
		cpu_string = "470";
		delay_divisor = 82; 	/* 25 MHz ? XXX */
		cpu_has_vme = TRUE;
		break;

	default:
		mon_panic("unknown sun3x model\n");
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
static void
tracedump(x1)
	int x1;
{
	struct funcall_frame *fp = (struct funcall_frame *)(&x1 - 2);
	u_int stackpage = ((u_int)fp) & ~PGOFSET;

	mon_printf("Begin traceback...fp = %x\n", fp);
	do {
		if (fp == fp->fr_savfp) {
			mon_printf("FP loop at %x", fp);
			break;
		}
		mon_printf("Called from %x, fp=%x, args=%x %x %x %x\n",
				   fp->fr_savpc, fp->fr_savfp,
				   fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	} while ( (((u_int)fp) & ~PGOFSET) == stackpage);
	mon_printf("End traceback...\n");
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.  [lifted from freed cmu mach3 sun3 port]
 */
static void
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
			sun3x_mode_normal();
			panic("zero");
			/*NOTREACHED*/

		case 4:			/* old g4 */
			goto do_trace;

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

	do_trace:
	case 't':			/* 't'race kernel stack */
	case 'T':
		tracedump(addr);
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
 *
 * Note, args are always:
 * argv[0] = boot_device	(i.e. "sd(0,0,0)")
 * argv[1] = options	(i.e. "-ds" or NULL)
 * argv[2] = NULL
 */
static void
_monitor_hooks()
{
	MachMonRomVector *romp;
	MachMonBootParam *bpp;
	char **argp;
	char *p;

	romp = romVectorPtr;
	if (romp->romvecVersion >= 2)
		*romp->vector_cmd = v_handler;

	/* Set boothowto flags from PROM args. */
	bpp = *romp->bootParam;
	argp = bpp->argPtr;

	/* Skip argp[0] (the device string) */
	argp++;

	/* Have options? */
	if (*argp == NULL)
		return;
	p = *argp;
	if (*p == '-') {
		/* yes, parse options */
#ifdef	DEBUG
		mon_printf("boot option: %s\n", p);
#endif
		for (++p; *p; p++) {
			switch (*p) {
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
		argp++;
	}

#ifdef	DEBUG
	/* Have init name? */
	if (*argp == NULL)
		return;
	p = *argp;
	mon_printf("boot initpath: %s\n", p);
#endif
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

	/* set v_handler, get boothowto */
	_monitor_hooks();

	/* Find devices we need early (like the IDPROM). */
	obio_init();

	/* Note: must come after obio_init (for IDPROM). */
	_verify_hardware();	/* get CPU type, etc. */

	/* handle kernel mapping, pmap_bootstrap(), etc. */
	_vm_init(&keh);

	/*
	 * Point interrupts/exceptions to our vector table.
	 * (Until now, we use the one setup by the PROM.)
	 *
	 * This is done after obio_init() / intreg_init() finds
	 * the interrupt register and disables the NMI clock so
	 * it will not cause "spurrious level 7" complaints.
	 */
	old_vector_table = getvbr();
	setvbr((void **)vector_table);

	/* Interrupts are enabled later, after autoconfig. */
}
