/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

/*
 * Interface to new debugger.
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h> /* just for boothowto --eichin */
#include <setjmp.h>

#include <vm/vm.h>

#include <machine/trap.h>
#include <machine/db_machdep.h>

extern jmp_buf	*db_recover;

int	db_active = 0;

/*
 * Received keyboard interrupt sequence.
 */
kdb_kintr(regs)
	register struct mc68020_saved_state *regs;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		kdb_trap(-1, regs);
	}
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
kdb_trap(type, regs)
	int	type;
	register struct mc68020_saved_state *regs;
{
	switch (type) {
	case T_TRACE:		/* single-step */
	case T_BREAKPOINT:	/* breakpoint */
/*      case T_WATCHPOINT:*/
		break;
	case -1:
		break;
	default:
		kdbprinttrap(type, 0);
		if (db_recover != 0) {
			db_error("Caught exception in ddb.\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb's own stack here. */

	ddb_regs = *regs;

	db_active++;
	cnpollc(TRUE);
/*	(void) setvideoenable(1);*/
	db_trap(type, 0);
	cnpollc(FALSE);
	db_active--;

	*regs = ddb_regs;

	/*
	 * Indicate that single_step is for KDB.
	 * But lock out interrupts to prevent TRACE_KDB from setting the
	 * trace bit in the current SR (and trapping while exiting KDB).
	 */
	(void) spl7();
/*	if (!USERMODE(regs->sr) && (regs->sr & SR_T1) && (current_thread())) {
		current_thread()->pcb->pcb_flag |= TRACE_KDB;
	}*/
/*	if ((regs->sr & SR_T1) && (current_thread())) {
		current_thread()->pcb->flag |= TRACE_KDB;
	}*/

	return(1);
}

extern char *trap_type[];
extern int trap_types;

/*
 * Print trap reason.
 */
kdbprinttrap(type, code)
	int	type, code;
{
	printf("kernel: ");
	if (type >= trap_types || type < 0)
		printf("type %d", type);
	else
		printf("%s", trap_type[type]);
	printf(" trap\n");
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	register int	size;
	register char	*data;
{
	register char	*src;

	src = (char *)addr;
	while (--size >= 0)
		*data++ = *src++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	register int	size;
	register char	*data;
{
	register char	*dst;

	int		oldmap0 = 0;
	int		oldmap1 = 0;
	vm_offset_t	addr1;
	extern char	etext;

	if (addr >= VM_MIN_KERNEL_ADDRESS &&
	    addr <= (vm_offset_t)&etext)
	{
#ifdef 0	/* XXX - needs to be cpu_dependent, probably */
		oldmap0 = getpgmap(addr);
		setpgmap(addr, (oldmap0 & ~PG_PROT) | PG_KW);

		addr1 = sun_trunc_page(addr + size - 1);
		if (sun_trunc_page(addr) != addr1) {
			/* data crosses a page boundary */

			oldmap1 = getpgmap(addr1);
			setpgmap(addr1, (oldmap1 & ~PG_PROT) | PG_KW);
		}
#endif
	}

	dst = (char *)addr;

	while (--size >= 0)
		*dst++ = *data++;

#if 0
	if (oldmap0) {
		setpgmap(addr, oldmap0);
		if (oldmap1)
			setpgmap(addr1, oldmap1);
	}
#endif
}

int
Debugger()
{
	asm ("trap #15");
}
