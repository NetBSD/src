/*	$NetBSD: db_trace.c,v 1.5 1997/03/20 12:00:43 matthias Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND HELSINKI UNIVERSITY OF TECHNOLOGY ALLOW FREE USE
 * OF THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIM ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
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
 * 	File: ns532/db_trace.c
 *	Author: Tero Kivinen, Tatu Ylonen
 *	Helsinki University of Technology 1992.
 *
 *	Stack trace and special register support for debugger.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>
#include <machine/cpufunc.h>

#include <ddb/db_sym.h>
#include <ddb/db_access.h>
#include <ddb/db_variables.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>

struct ns532_frame;
static void	db_nextframe __P((struct ns532_frame **, db_addr_t *,
					int *, int));
static int	db_numargs __P((struct ns532_frame *));
static int	db_spec_regs __P((struct db_variable *, db_expr_t *, int));

/*
 * Machine register set.
 */
struct db_variable db_regs[] = {
	{ "r0", 	(long *)&ddb_regs.tf_regs.r_r0, FCN_NULL },
	{ "r1", 	(long *)&ddb_regs.tf_regs.r_r1, FCN_NULL },
	{ "r2", 	(long *)&ddb_regs.tf_regs.r_r2, FCN_NULL },
	{ "r3", 	(long *)&ddb_regs.tf_regs.r_r3, FCN_NULL },
	{ "r4", 	(long *)&ddb_regs.tf_regs.r_r4, FCN_NULL },
	{ "r5", 	(long *)&ddb_regs.tf_regs.r_r5, FCN_NULL },
	{ "r6", 	(long *)&ddb_regs.tf_regs.r_r6, FCN_NULL },
	{ "r7", 	(long *)&ddb_regs.tf_regs.r_r7, FCN_NULL },
	{ "sp", 	(long *)&ddb_regs.tf_regs.r_sp, FCN_NULL },
	{ "fp", 	(long *)&ddb_regs.tf_regs.r_fp, FCN_NULL },
	{ "sb", 	(long *)&ddb_regs.tf_regs.r_sb, FCN_NULL },
	{ "pc", 	(long *)&ddb_regs.tf_regs.r_pc, FCN_NULL },
	{ "psr",	(long *)&ddb_regs.tf_regs.r_psr,FCN_NULL },
	{ "tear",	(long *)&ddb_regs.tf_tear,	FCN_NULL },
	{ "msr",	(long *)&ddb_regs.tf_msr,	FCN_NULL },
	{ "ipl",	(long *)&db_active_ipl, 	FCN_NULL },

	{ "intbase",	(long *) 0, db_spec_regs },
	{ "ptb",	(long *) 0, db_spec_regs },
	{ "ivar",	(long *) 0, db_spec_regs },
	{ "rtear",	(long *) 0, db_spec_regs }, /* Current reg value */
	{ "mcr",	(long *) 0, db_spec_regs },
	{ "rmsr",	(long *) 0, db_spec_regs }, /* Current reg value */
	{ "dcr",	(long *) 0, db_spec_regs },
	{ "dsr",	(long *) 0, db_spec_regs },
	{ "car",	(long *) 0, db_spec_regs },
	{ "bpc",	(long *) 0, db_spec_regs },
	{ "cfg",	(long *) 0, db_spec_regs },
};
struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]) - 1;

/*
 * Stack trace.
 */
#define	INKERNEL(va)	(((vm_offset_t)(va)) >= VM_MIN_KERNEL_ADDRESS)

struct ns532_frame {
	struct ns532_frame	*f_frame;
	int			f_retaddr;
	int			f_arg0;
};

enum { NONE, TRAP, INTERRUPT, SYSCALL };

db_addr_t	db_trap_symbol_value = 0;
db_addr_t	db_syscall_symbol_value = 0;
db_addr_t	db_intr_symbol_value = 0;
boolean_t	db_trace_symbols_found = FALSE;

/*
 * Figure out how many arguments were passed into the frame at "fp".
 */
int db_numargs_default = 5;

int
db_numargs(fp)
	struct ns532_frame *fp;
{
	db_addr_t	argp;
	int		inst;
	int		args;
	int		i;
	extern char	etext[];

	argp = (db_addr_t) db_get_value((db_addr_t)&fp->f_retaddr, 4, FALSE);
	if (argp < (db_addr_t)VM_MIN_KERNEL_ADDRESS || argp > (db_addr_t)etext)
		return(db_numargs_default);

	for (i = 0; i < 5; i++) {
		/*
		 * After a bsr gcc may emit the following instructions
		 * to remove the arguments from the stack:
		 *   cmpqd 0,tos 	- to remove 4 bytes from the stack
		 *   cmpd tos,tos	- to remove 8 bytes from the stack
		 *   adjsp[bwd] -n	- to remove n bytes from the stack
		 * Gcc sometimes delays emitting these instructions and
		 * may even throw a branch between our feet.
		 */
		inst = db_get_value((db_addr_t) argp	, 4, FALSE);
		args = db_get_value((db_addr_t) argp + 2, 4, FALSE);
		if ((inst & 0xff) == 0xea) {		/* br */
			args = ((inst >> 8) & 0xffffff) | (args << 24);
			if (args & 0x80) {
				if (args & 0x40) {
					args = ntohl(args);
				} else {
					args = ntohs(args & 0xffff);
					if (args & 0x2000)
						args |= 0xc000;
				}
			} else {
				args = args & 0xff;
				if (args & 0x40)
					args |= 0x80;
			}
			argp += args;
			continue;
		}
		if ((inst & 0xffff) == 0xb81f)		/* cmpqd 0,tos */
			return(1);
		else if ((inst & 0xffff) == 0xbdc7)	/* cmpd tos,tos */
			return(2);
		else if ((inst & 0xfffc) == 0xa57c) {	/* adjsp[bwd] */
			switch (inst & 3) {
			case 0:
				args = ((args & 0xff) + 0x80);
				break;
			case 1:
				args = ((ntohs(args) & 0xffff) + 0x8000);
				break;
			case 3:
				args = -ntohl(args);
				break;
			default:
				return(db_numargs_default);
			}
			if (args / 4 > 10 || (args & 3) != 0)
				continue;
			return(args / 4);
		}
		argp += db_dasm_ns32k(NULL, argp);
	}
	return(db_numargs_default);
}

/*
 * Figure out the next frame up in the call stack.
 * For trap(), we print the address of the faulting instruction and
 *   proceed with the calling frame.  We return the ip that faulted.
 *   If the trap was caused by jumping through a bogus pointer, then
 *   the next line in the backtrace will list some random function as
 *   being called.  It should get the argument list correct, though.
 *   It might be possible to dig out from the next frame up the name
 *   of the function that faulted, but that could get hairy.
 */
static void
db_nextframe(fp, ip, argp, is_trap)
	struct ns532_frame	**fp;		/* in/out */
	db_addr_t		*ip;		/* out */
	int			*argp;		/* in */
	int			is_trap;	/* in */
{
	struct trapframe *tf;
	struct syscframe *sf;

	switch (is_trap) {
	case INTERRUPT:
		db_printf("--- interrupt ---\n");
	case NONE:
		*ip = (db_addr_t)
		      db_get_value((int) &(*fp)->f_retaddr, 4, FALSE);
		*fp = (struct ns532_frame *)
		      db_get_value((int) &(*fp)->f_frame, 4, FALSE);
		break;

	/* The only argument to trap() or syscall() is the trapframe. */
	case TRAP:
		tf = (struct trapframe *)argp;
		db_printf("--- trap (number %ld) ---\n", tf->tf_trapno);
		*fp = (struct ns532_frame *)tf->tf_regs.r_fp;
		*ip = (db_addr_t)tf->tf_regs.r_pc;
		break;

	case SYSCALL:
		sf = (struct syscframe *)argp;
		db_printf("--- syscall (number %d) ---\n", sf->sf_regs.r_r0);
		*fp = (struct ns532_frame *)sf->sf_regs.r_fp;
		*ip = (db_addr_t)sf->sf_regs.r_pc;
		break;
	}
}

void
db_stack_trace_cmd(addr, have_addr, count, modif)
	db_expr_t	addr;
	boolean_t	have_addr;
	db_expr_t	count;
	char		*modif;
{
	struct ns532_frame *frame, *lastframe;
	int		*argp;
	db_addr_t	callpc;
	int		is_trap = 0;
	boolean_t	kernel_only = TRUE;
	boolean_t	trace_thread = FALSE;

	{
		register char *cp = modif;
		register char c;

		while ((c = *cp++) != 0) {
			if (c == 't')
				trace_thread = TRUE;
			if (c == 'u')
				kernel_only = FALSE;
		}
	}

	if (count == -1)
		count = 65535;

	if (!have_addr) {
		frame = (struct ns532_frame *)ddb_regs.tf_regs.r_fp;
		callpc = (db_addr_t)ddb_regs.tf_regs.r_pc;
	} else if (trace_thread) {
		db_printf ("db_trace.c: can't trace thread\n");
	} else {
		frame = (struct ns532_frame *)addr;
		callpc = (db_addr_t)
			db_get_value((int)&frame->f_retaddr, 4, FALSE);
	}

	lastframe = 0;
	while (count && frame != 0) {
		int		narg;
		char *	name;
		db_expr_t	offset;
		db_sym_t	sym;
#define MAXNARG	16
		char	*argnames[MAXNARG], **argnp = NULL;

		sym = db_search_symbol(callpc, DB_STGY_ANY, &offset);
		db_symbol_values(sym, &name, NULL);

		if (lastframe == 0 && sym == NULL) {
			/* Symbol not found, peek at code */
			int	instr = db_get_value(callpc, 1, FALSE);

			offset = 1;
			if ((instr & 0xff) == 0x82) /* enter [],c */
				offset = 0;
		}
		if (INKERNEL((int)frame) && name) {
			if (!strcmp(name, "_trap")) {
				is_trap = TRAP;
			} else if (!strcmp(name, "_syscall")) {
				is_trap = SYSCALL;
			} else if (!strcmp(name, "_interrupt")) {
				is_trap = INTERRUPT;
			} else
				goto normal;
			narg = 0;
		} else {
		normal:
			is_trap = NONE;
			narg = MAXNARG;
			if (db_sym_numargs(sym, &narg, argnames))
				argnp = argnames;
			else
				narg = db_numargs(frame);
		}

		db_printf("%s(", name);

		if (lastframe == 0 && offset == 0 && !have_addr) {
			/*
			 * We have a breakpoint before the frame is set up
			 * Use sp instead
			 */
			argp = &((struct ns532_frame *)(ddb_regs.tf_regs.r_sp-4))->f_arg0;
		} else {
			argp = &frame->f_arg0;
		}

		while (narg) {
			if (argnp)
				db_printf("%s=", *argnp++);
			db_printf("%lx", db_get_value((int)argp, 4, FALSE));
			argp++;
			if (--narg != 0)
				db_printf(",");
		}
		db_printf(") at ");
		db_printsym(callpc, DB_STGY_PROC);
		db_printf("\n");

		if (lastframe == 0 && offset == 0 && !have_addr) {
			/* Frame really belongs to next callpc */
			lastframe = (struct ns532_frame *)(ddb_regs.tf_regs.r_sp-4);
			callpc = (db_addr_t)
				 db_get_value((int)&lastframe->f_retaddr, 4, FALSE);
			continue;
		}

		lastframe = frame;
		db_nextframe(&frame, &callpc, &frame->f_arg0, is_trap);

		if (frame == 0) {
			/* end of chain */
			break;
		}
		if (INKERNEL((int)frame)) {
			/* staying in kernel */
			if (frame <= lastframe) {
				db_printf("Bad frame pointer: %p\n", frame);
				break;
			}
		} else if (INKERNEL((int)lastframe)) {
			/* switch from user to kernel */
			if (kernel_only)
				break;	/* kernel stack only */
		} else {
			/* in user */
			if (frame <= lastframe) {
				db_printf("Bad user frame pointer: %p\n",
					  frame);
				break;
			}
		}
		--count;
	}

	if (count && is_trap != NONE) {
		db_printsym(callpc, DB_STGY_XTRN);
		db_printf(":\n");
	}
}

/**********************************************

  Get/Set value of special registers.

  *********************************************/

static int
db_spec_regs(vp, valp, what)
	struct db_variable *vp;
	db_expr_t *valp;
	int what;
{
	if (strcmp(vp->name, "intbase") == 0)
		if (what == DB_VAR_GET)
			sprd(intbase, *valp);
		else
			lprd(intbase, *valp);
	else if (strcmp(vp->name, "ptb") == 0)
		if (what == DB_VAR_GET)
			smr(ptb0, *valp);
		else
			load_ptb(*valp);
	else if (strcmp(vp->name, "ivar") == 0)
		if (what == DB_VAR_GET)
			*valp = 0;
		else {
			lmr(ivar0, *valp);
			lmr(ivar1, *valp);
		}
	else if (strcmp(vp->name, "rtear") == 0)
		if (what == DB_VAR_GET)
			smr(tear, *valp);
		else
			lmr(tear, *valp);
	else if (strcmp(vp->name, "mcr") == 0)
		if (what == DB_VAR_GET)
			smr(mcr, *valp);
		else
			lmr(mcr, *valp);
	else if (strcmp(vp->name, "rmsr") == 0)
		if (what == DB_VAR_GET)
			smr(msr, *valp);
		else
			lmr(msr, *valp);
	else if (strcmp(vp->name, "dcr") == 0)
		if (what == DB_VAR_GET)
			sprd(dcr, *valp);
		else
			lprd(dcr, *valp);
	else if (strcmp(vp->name, "dsr") == 0)
		if (what == DB_VAR_GET)
			sprd(dsr, *valp);
		else
			lprd(dsr, *valp);
	else if (strcmp(vp->name, "car") == 0)
		if (what == DB_VAR_GET)
			sprd(car, *valp);
		else
			lprd(car, *valp);
	else if (strcmp(vp->name, "bpc") == 0)
		if (what == DB_VAR_GET)
			sprd(bpc, *valp);
		else
			lprd(bpc, *valp);
	else if (strcmp(vp->name, "cfg") == 0)
		if (what == DB_VAR_GET)
			sprd(cfg, *valp);
		else
			lprd(cfg, *valp);
	else
		db_printf("Internal error, unknown register in db_spec_regs");
	return(0);
}
