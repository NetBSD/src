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
 * HISTORY
 * $Log: db_interface.c,v $
 * Revision 1.1  1993/12/15 03:28:56  briggs
 * Add files for ddb support.  I know they're at least partially broken.
 *
 * Revision 1.2  1993/08/10  08:42:52  glass
 * fixed problem that caused two consecutive segments to be using the same
 * pmeg unknowingly.  still too many printfs, not sure how many are actualy
 * in the machine dependent code.  reaches cpu_startup() where it stops
 * deliberately. next project: autoconfig(), maybe kgdb
 *
 * Revision 1.1  93/08/08  12:22:06  glass
 * lots of changes, too many printfs
 * 
 * Revision 2.6  92/01/03  20:31:01  dbg
 * 	Ignore RB_KDB - always enter DDB.
 * 	[91/11/06            dbg]
 * 
 * Revision 2.5  91/07/31  18:12:50  dbg
 * 	Stack switching support.
 * 	[91/07/12            dbg]
 * 
 * Revision 2.4  91/03/16  14:58:16  rpd
 * 	Replaced db_nofault with db_recover.
 * 	[91/03/14            rpd]
 * 
 * Revision 2.3  90/10/25  14:47:16  rwd
 * 	Added watchpoint support.
 * 	[90/10/16            rwd]
 * 
 * Revision 2.2  90/08/27  22:11:17  dbg
 * 	Reduce lint.
 * 	[90/08/07            dbg]
 * 	Created.
 * 	[90/07/25            dbg]
 * 
 */

/*
 * Interface to new debugger.
 */
#include "param.h"
#include "proc.h"
#include <machine/db_machdep.h>

#include <sys/reboot.h>
#include <vm/vm_statistics.h>
#include <vm/pmap.h>

#include <setjmp.h>
#include <sys/systm.h> /* just for boothowto --eichin */
#include "machine/trap.h"
/* #include "machine/mon.h" sun monitor-specific stuff. */

#define jmp_buf_t jmp_buf

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
#define T_BRKPT T_TRAP15

/*
 *  kdb_trap - field a TRACE or BPT trap
 */

extern jmp_buf_t *db_recover;

kdb_trap(type, regs)
	int	type;
	register struct mc68020_saved_state *regs;
{
    switch (type)
    {
	case T_TRACE:	/* single-step */
	case T_BRKPT:	/* breakpoint */
/*      case T_WATCHPOINT:*/
	    break;
#if 0
	case EXC_BREAKPOINT:
	    type = T_BRKPT;
	    break;
#endif
	case -1:
	    break;
	default:
	{
	    kdbprinttrap(type, 0);
	    if (db_recover != 0) {
		db_printf("Caught exception in ddb.\n");
		db_error("");
		/*NOTREACHED*/
	    }
	}
    }

    /*  Should switch to kdb's own stack here. */

    ddb_regs = *regs;

    db_active++;
    cnpollc(TRUE);
/*    (void) setvideoenable(1);*/

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
/*
    if (!USERMODE(regs->sr) && (regs->sr & SR_T1) && (current_thread())) {
	current_thread()->pcb->pcb_flag |= TRACE_KDB;
*/
/*    if ((regs->sr & SR_T1) && (current_thread())) {
	current_thread()->pcb->flag |= TRACE_KDB;
    }*/

    return(1);
}

extern char *	trap_type[];
#define TRAP_TYPES 15
/*extern int	TRAP_TYPES;*/

/*
 * Print trap reason.
 */
kdbprinttrap(type, code)
	int	type, code;
{
	printf("kernel: ");
	if (type >= TRAP_TYPES || type < 0)
	    printf("type %d", type);
	else
	    printf("%s", trap_type[type]);
	printf(" trap\n");
}

/*
 * Read bytes from kernel address space for debugger.
 */

extern jmp_buf_t	db_jmpbuf;

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
	    macconputstr("stupid db_write_bytes\n");
#ifdef 0
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
	    if (oldmap1) {
		setpgmap(addr1, oldmap1);
	    }
	}
#endif
}

int
Debugger()
{
    asm ("trap #15");
}

db_addr_t
db_disasm(loc, altfmt)
	db_addr_t	loc;
	boolean_t	altfmt;
{
	return loc+1;
}
