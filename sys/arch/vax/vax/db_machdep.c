/*	$NetBSD: db_machdep.c,v 1.14 1999/01/01 21:25:03 ragge Exp $	*/

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
 *	db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 * Taken from i386 port and modified for vax.
 */
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h> /* just for boothowto --eichin */

#include <vm/vm.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/../vax/gencons.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_variables.h>

extern	label_t	*db_recover;

void	kdbprinttrap __P((int, int));

int	db_active = 0;

extern int qdpolling;
static	int splsave; /* IPL before entering debugger */

/*
 * DDB is called by either <ESC> - D on keyboard, via a TRACE or
 * BPT trap or from kernel, normally as a result of a panic.
 * If it is the result of a panic, set the ddb register frame to
 * contain the registers when panic was called. (easy to debug).
 */
void
kdb_trap(frame)
	struct trapframe *frame;
{
	int s;

	switch (frame->trap) {
	case T_BPTFLT:	/* breakpoint */
	case T_TRCTRAP:	/* single_step */
		break;

	case T_KDBTRAP:
		if (panicstr) {
			struct	callsframe *pf, *df;

			df = (void *)frame->fp; /* start of debug's calls */
			pf = (void *)df->ca_fp;	/* start of panic's calls */
			bcopy(&pf->ca_argno, &ddb_regs.r0, sizeof(int) * 12);
			ddb_regs.fp = pf->ca_fp;
			ddb_regs.pc = pf->ca_pc;
			ddb_regs.ap = pf->ca_ap;
			ddb_regs.sp = (unsigned)pf;
			ddb_regs.psl = frame->psl & ~0x1fffe0;
			ddb_regs.psl |= pf->ca_maskpsw & 0xffe0;
			ddb_regs.psl |= (splsave << 16);
		}
		break;

	default:
		if ((boothowto & RB_KDB) == 0)
			return;

		kdbprinttrap(frame->trap, frame->code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	if (!panicstr)
		bcopy(frame, &ddb_regs, sizeof(struct trapframe));

	/* XXX Should switch to interrupt stack here, if needed. */

	s = splddb();
	db_active++;
        cnpollc(TRUE);
        db_trap(frame->trap, frame->code);
        cnpollc(FALSE);
        db_active--;
	splx(s);

	if (!panicstr)
		bcopy(&ddb_regs, frame, sizeof(struct trapframe));
	frame->sp = mfpr(PR_USP);

	return;
}

extern char *traptypes[];
extern int no_traps;

/*
 * Print trap reason.
 */
void
kdbprinttrap(type, code)
	int type, code;
{
	db_printf("kernel: ");
	if (type >= no_traps || type < 0)
		db_printf("type %d", type);
	else
		db_printf("%s", traptypes[type]);
	db_printf(" trap, code=%x\n", code);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{

	bcopy((caddr_t)addr, data, size);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{

	memcpy((caddr_t)addr, data, size);
}

void
Debugger()
{
	splsave = splx(0xe);
	mtpr(0xf, PR_SIRR); /* beg for debugger */
	splx(splsave);
}

/*
 * Machine register set.
 */
struct db_variable db_regs[] = {
	{"r0",	&ddb_regs.r0,	FCN_NULL},
	{"r1",	&ddb_regs.r1,	FCN_NULL},
	{"r2",	&ddb_regs.r2,	FCN_NULL},
	{"r3",	&ddb_regs.r3,	FCN_NULL},
	{"r4",	&ddb_regs.r4,	FCN_NULL},
	{"r5",	&ddb_regs.r5,	FCN_NULL},
	{"r6",	&ddb_regs.r6,	FCN_NULL},
	{"r7",	&ddb_regs.r7,	FCN_NULL},
	{"r8",	&ddb_regs.r8,	FCN_NULL},
	{"r9",	&ddb_regs.r9,	FCN_NULL},
	{"r10",	&ddb_regs.r10,	FCN_NULL},
	{"r11",	&ddb_regs.r11,	FCN_NULL},
	{"ap",	&ddb_regs.ap,	FCN_NULL},
	{"fp",	&ddb_regs.fp,	FCN_NULL},
	{"sp",	&ddb_regs.sp,	FCN_NULL},
	{"pc",	&ddb_regs.pc,	FCN_NULL},
	{"psl",	&ddb_regs.psl,	FCN_NULL},
};
struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

void
db_stack_trace_cmd(addr, have_addr, count, modif)
        db_expr_t       addr;
        boolean_t       have_addr;
        db_expr_t       count;
        char            *modif;
{
	extern vaddr_t	proc0paddr, istack;
	struct proc	*p = curproc;
        db_expr_t	diff;
        db_sym_t	sym;
	vaddr_t		paddr;
	u_int		*cf, *tcf, abase, loop, i, stackbase;
        char		*symname;
 
	if (panicstr) {
		cf = (int *)ddb_regs.sp;
	} else {
		printf("Don't know what to do without panic\n");
		cf = (int *)ddb_regs.fp; /* XXX */
	}
	if (p)
		paddr = (u_int)p->p_addr;
	else
		paddr = proc0paddr;
	stackbase = (ddb_regs.psl & PSL_IS ? istack : paddr);
	while ((cf[3] > stackbase) && (cf[3] < (stackbase + USPACE))) {
		diff = INT_MAX;
		symname = NULL;
		abase = 0;
		sym = db_search_symbol(cf[4], DB_STGY_ANY, &diff);
		db_symbol_values(sym, &symname, 0);

		db_printf("%s+0x%lx(", symname, diff);
		/* Pass all saved regs */
		tcf = (int *)cf[3];
		loop = (tcf[1] >> 16) & 0xfff;
		while (loop) {
			if (loop & 1)
				abase++;
			loop >>= 1;
		}
		if (tcf[5+abase]) {
			for (i = 0; i < (tcf[5+abase] - 1); i++)
				db_printf("0x%x,", tcf[6+abase+i]);
			db_printf("0x%x)\n", tcf[6+abase+i]);
		} else
			db_printf("void)\n");
		cf = (u_int *)cf[3];
	}
}

static int ddbescape = 0;

int
kdbrint(tkn)
	int tkn;
{

	if (ddbescape && ((tkn & 0x7f) == 'D')) {
		mtpr(0xf, PR_SIRR);
		ddbescape = 0;
		return 1;
	}

	if ((ddbescape == 0) && ((tkn & 0x7f) == 27)) {
		ddbescape = 1;
		return 1;
	}

	if (ddbescape) {
		ddbescape = 0;
		return 2;
	}
	
	ddbescape = 0;
	return 0;
}


