/*	$NetBSD: db_interface.c,v 1.4 1995/02/01 21:51:48 pk Exp $ */

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
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

#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <machine/bsd_openprom.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>

extern jmp_buf	*db_recover;

int	db_active = 0;

extern char *trap_type[];

/*
 * Received keyboard interrupt sequence.
 */
kdb_kbd_trap(regs)
	struct sparc_saved_state *regs;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		kdb_trap(-1, 0, regs);
	}
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
kdb_trap(type, regs)
	int	type;
	register struct sparc_saved_state *regs;
{

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		printf("kernel: %s trap", trap_type[type & 0xff]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */

	ddb_regs = *regs;

	db_active++;
	cnpollc(TRUE);
	db_trap(type, 0/*code*/);
	cnpollc(FALSE);
	db_active--;

	*regs = ddb_regs;

	return (1);
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
 * XXX - stolen from pmap.c
 */
#define	getpte(va)		lda(va, ASI_PTE)
#define	setpte(va, pte)		sta(va, ASI_PTE, pte)
#define	splpmap() splimp()

static void
db_write_text(dst, ch)
	unsigned char *dst;
	int ch;
{        
	int s, pte0, pte;
	vm_offset_t va;

	s = splpmap();
	va = (unsigned long)dst & (~PGOFSET);
	pte0 = getpte(va);

	if ((pte0 & PG_V) == 0) { 
		db_printf(" address 0x%x not a valid page\n", dst);
		splx(s);
		return;
	}

	pte = pte0 | PG_W;
	setpte(va, pte);

	*dst = (unsigned char)ch;

	setpte(va, pte0);
	splx(s);
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
	extern char	etext[];
	register char	*dst;

	dst = (char *)addr;
	while (--size >= 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS) && (dst < etext))
			db_write_text(dst, *data);
		else
			*dst = *data;
		dst++, data++;
	}

}

int
Debugger()
{
	asm("ta 0x81");
}

void
db_prom_cmd()
{
	extern struct promvec *promvec;
	promvec->pv_abort();
}

struct db_command sparc_db_command_table[] = {
	{ "prom",	db_prom_cmd,	0,	0 },
	{ (char *)0, }
};

void
db_machine_init()
{
	db_machine_commands_install(sparc_db_command_table);
}
