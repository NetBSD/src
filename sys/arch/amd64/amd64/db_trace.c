/*	$NetBSD: db_trace.c,v 1.3 2004/04/16 14:21:56 fvdl Exp $	*/

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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.3 2004/04/16 14:21:56 fvdl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h> 

#include <machine/db_machdep.h>
#include <machine/frame.h>
#include <machine/trap.h>

#include <ddb/db_sym.h>
#include <ddb/db_access.h>
#include <ddb/db_variables.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>

#define dbreg(xx) (long *)offsetof(db_regs_t, tf_ ## xx)

static int db_x86_64_regop(const struct db_variable *, db_expr_t *, int);

/*
 * Machine register set.
 */
const struct db_variable db_regs[] = {
	{ "ds",		dbreg(ds),     db_x86_64_regop },
	{ "es",		dbreg(es),     db_x86_64_regop },
	{ "fs",		dbreg(fs),     db_x86_64_regop },
	{ "gs",		dbreg(gs),     db_x86_64_regop },
	{ "rdi",	dbreg(rdi),    db_x86_64_regop },
	{ "rsi",	dbreg(rsi),    db_x86_64_regop },
	{ "rbp",	dbreg(rbp),    db_x86_64_regop },
	{ "rbx",	dbreg(rbx),    db_x86_64_regop },
	{ "rdx",	dbreg(rdx),    db_x86_64_regop },
	{ "rcx",	dbreg(rcx),    db_x86_64_regop },
	{ "rax",	dbreg(rax),    db_x86_64_regop },
	{ "r8",		dbreg(r8),     db_x86_64_regop },
	{ "r9",		dbreg(r9),     db_x86_64_regop },
	{ "r10",	dbreg(r10),    db_x86_64_regop },
	{ "r11",	dbreg(r11),    db_x86_64_regop },
	{ "r12",	dbreg(r12),    db_x86_64_regop },
	{ "r13",	dbreg(r13),    db_x86_64_regop },
	{ "r14",	dbreg(r14),    db_x86_64_regop },
	{ "r15",	dbreg(r15),    db_x86_64_regop },
	{ "rip",	dbreg(rip),    db_x86_64_regop },
	{ "cs",		dbreg(cs),     db_x86_64_regop },
	{ "rflags",	dbreg(rflags), db_x86_64_regop },
	{ "rsp",	dbreg(rsp),    db_x86_64_regop },
	{ "ss",		dbreg(ss),     db_x86_64_regop },
};
const struct db_variable * const db_eregs =
	db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

static int
db_x86_64_regop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
        db_expr_t *regaddr =
            (db_expr_t *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));
        
        switch (opcode) {
        case DB_VAR_GET:
                *val = *regaddr;
                break;
        case DB_VAR_SET:
                *regaddr = *val;
                break;
        default:
                panic("db_x86_64_regop: unknown op %d", opcode);
        }
        return 0;
}

/*
 * Stack trace.
 */
#define	INKERNEL(va)	(((vaddr_t)(va)) >= VM_MIN_KERNEL_ADDRESS)

struct x86_64_frame {
	struct x86_64_frame	*f_frame;
	long			f_retaddr;
	long			f_arg0;
};

#define	NONE		0
#define	TRAP		1
#define	SYSCALL		2
#define	INTERRUPT	3

db_addr_t	db_trap_symbol_value = 0;
db_addr_t	db_syscall_symbol_value = 0;
db_addr_t	db_kdintr_symbol_value = 0;
boolean_t	db_trace_symbols_found = FALSE;

void db_find_trace_symbols(void);
int db_numargs(long *);
int db_nextframe(long **, long **, long **, db_addr_t *, long *, int,
    void (*) (const char *, ...));
db_sym_t db_frame_info(long *, db_addr_t, char **, db_expr_t *, int *, int *);


void
db_find_trace_symbols()
{
	db_expr_t	value;

	if (db_value_of_name("_trap", &value))
		db_trap_symbol_value = (db_addr_t) value;
	if (db_value_of_name("_kdintr", &value))
		db_kdintr_symbol_value = (db_addr_t) value;
	if (db_value_of_name("_syscall", &value))
		db_syscall_symbol_value = (db_addr_t) value;
	db_trace_symbols_found = TRUE;
}

/*
 * Figure out how many arguments were passed into the frame at "fp".
 * We can probably figure out how many arguments where passed above
 * the first 6 (which are in registers), but since we can't
 * reliably determine the values currently, just return 0.
 */
int
db_numargs(long *retaddrp)
{
	return 0;
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
int
db_nextframe(long **nextframe, long **retaddr, long **arg0, db_addr_t *ip,
	     long *argp, int is_trap, void (*pr)(const char *, ...))
{
	struct trapframe *tf;
	struct x86_64_frame *fp;
	struct intrframe *ifp;
	int traptype, trapno, err, i;

	switch (is_trap) {
	    case NONE:
		*ip = (db_addr_t)
			db_get_value((long)*retaddr, 8, FALSE);
		fp = (struct x86_64_frame *)
			db_get_value((long)*nextframe, 8, FALSE);
		if (fp == NULL)
			return 0;
		*nextframe = (long *)&fp->f_frame;
		*retaddr = (long *)&fp->f_retaddr;
		*arg0 = (long *)&fp->f_arg0;
		break;

	    case TRAP:
	    case SYSCALL:
	    case INTERRUPT:
	    default:

		/* The only argument to trap() or syscall() is the trapframe. */
		tf = *(struct trapframe **)argp;
		switch (is_trap) {
		case TRAP:
			(*pr)("--- trap (number %d) ---\n", tf->tf_trapno);
			break;
		case SYSCALL:
			(*pr)("--- syscall (number %ld) ---\n", tf->tf_rax);
			break;
		case INTERRUPT:
			(*pr)("--- interrupt ---\n");
			tf = (struct trapframe *)argp;
			break;
		}
		*ip = (db_addr_t)tf->tf_rip;
		fp = (struct x86_64_frame *)tf->tf_rbp;
		if (fp == NULL)
			return 0;
		*nextframe = (long *)&fp->f_frame;
		*retaddr = (long *)&fp->f_retaddr;
		*arg0 = (long *)&fp->f_arg0;
		break;
	}

	/*
	 * A bit of a hack. Since %rbp may be used in the stub code,
	 * walk the stack looking for a valid interrupt frame. Such
	 * a frame can be recognized by always having
	 * err 0 or IREENT_MAGIC and trapno T_ASTFLT.
	 */
	if (db_frame_info(*nextframe, (db_addr_t)*ip, NULL, NULL, &traptype,
	    NULL) != (db_sym_t)0
	    && traptype == INTERRUPT) {
		for (i = 0; i < 4; i++) {
			ifp = (struct intrframe *)(argp + i);
			err = db_get_value((long)&ifp->__if_err, 8, FALSE);
			trapno = db_get_value((long)&ifp->__if_trapno,
			    8, FALSE);
			if ((err == 0 || err == IREENT_MAGIC) && trapno == T_ASTFLT) {
				*nextframe = (long *)ifp - 1;
				break;
			}
		}
		if (i == 4) {
			(*pr)("DDB lost frame for ");
			db_printsym(*ip, DB_STGY_ANY, pr);
			(*pr)(", trying %p\n",argp);
			*nextframe = argp;
		}
	}
	return 1;
}

db_sym_t
db_frame_info(long *frame, db_addr_t callpc, char **namep, db_expr_t *offp,
	      int *is_trap, int *nargp)
{
	db_expr_t	offset;
	db_sym_t	sym;
	int narg;
	char *name;

	sym = db_search_symbol(callpc, DB_STGY_ANY, &offset);
	db_symbol_values(sym, &name, NULL);
	if (sym == (db_sym_t)0)
		return (db_sym_t)0;

	*is_trap = NONE;
	narg = 0;

	if (INKERNEL((long)frame) && name) {
		/*
		 * XXX traps should be based off of the Xtrap*
		 * locations rather than on trap, since some traps
		 * (e.g., npxdna) don't go through trap()
		 */
		if (!strcmp(name, "trap")) {
			*is_trap = TRAP;
			narg = 0;
		} else if (!strcmp(name, "syscall_plain") ||
		           !strcmp(name, "syscall_fancy")) {
			*is_trap = SYSCALL;
			narg = 0;
		} else if (name[0] == 'X') {
			if (!strncmp(name, "Xintr", 5) ||
			    !strncmp(name, "Xresume", 7) ||
			    !strncmp(name, "Xstray", 6) ||
			    !strncmp(name, "Xhold", 5) ||
			    !strncmp(name, "Xrecurse", 8) ||
			    !strcmp(name, "Xdoreti") ||
			    !strncmp(name, "Xsoft", 5)) {
				*is_trap = INTERRUPT;
				narg = 0;
			}
		}
	}

	if (offp != NULL)
		*offp = offset;
	if (nargp != NULL)
		*nargp = narg;
	if (namep != NULL)
		*namep = name;
	return sym;
}


void
db_stack_trace_print(db_expr_t addr, boolean_t have_addr, db_expr_t count,
		     char *modif, void (*pr)(const char *, ...))
{
	long *frame, *lastframe;
	long *retaddr, *arg0;
	long		*argp;
	db_addr_t	callpc;
	int		is_trap;
	boolean_t	kernel_only = TRUE;
	boolean_t	trace_thread = FALSE;

#if 0
	if (!db_trace_symbols_found)
		db_find_trace_symbols();
#endif

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

	if (!have_addr) {
		frame = (long *)ddb_regs.tf_rbp;
		callpc = (db_addr_t)ddb_regs.tf_rip;
	} else {
		if (trace_thread) {
			struct proc *p;
			struct user *u;
			struct lwp *l;
			(*pr)("trace: pid %d ", (int)addr);
			p = pfind(addr);
			if (p == NULL) {
				(*pr)("not found\n");
				return;
			}	
			l = proc_representative_lwp(p); /* XXX NJWLWP */
			if (!(l->l_flag & L_INMEM)) {
				(*pr)("swapped out\n");
				return;
			}
			u = l->l_addr;
			if (p == curproc && l == curlwp) {
				frame = (long *)ddb_regs.tf_rbp;
				callpc = (db_addr_t)ddb_regs.tf_rip;
				(*pr)(" at %p\n", frame);
			} else {
				frame = (long *)u->u_pcb.pcb_rbp;
				callpc = (db_addr_t)
				    db_get_value((long)(frame + 1), 8, FALSE);
				(*pr)(" at %p\n", frame);
				frame = (long *)*frame; /* XXXfvdl db_get_value? */
			}
		} else {
			frame = (long *)addr;
			callpc = (db_addr_t)
			    db_get_value((long)(frame + 1), 8, FALSE);
			frame = (long *)*frame; /* XXXfvdl db_get_value? */
		}
	}
	retaddr = frame + 1;
	arg0 = frame + 2;

	lastframe = 0;
	while (count && frame != 0) {
		int		narg;
		char *	name;
		db_expr_t	offset;
		db_sym_t	sym;
		char	*argnames[16], **argnp = NULL;
		db_addr_t	lastcallpc;

		name = "?";
		is_trap = NONE;
		offset = 0;
		sym = db_frame_info(frame, callpc, &name, &offset, &is_trap,
				    &narg);

		if (lastframe == 0 && sym == (db_sym_t)0) {
			/* Symbol not found, peek at code */
			u_long	instr = db_get_value(callpc, 4, FALSE);

			offset = 1;
			if (instr  == 0xe5894855 ||
					/* enter: pushq %rbp, movq %rsp, %rbp */
			    (instr & 0x00ffffff) == 0x0048e589
					/* enter+1: movq %rsp, %rbp */) {
				offset = 0;
			}
		}

		if (is_trap == NONE) {
			if (db_sym_numargs(sym, &narg, argnames))
				argnp = argnames;
			else
				narg = db_numargs(frame);
		}

		(*pr)("%s(", name);

		if (lastframe == 0 && offset == 0 && !have_addr) {
			/*
			 * We have a breakpoint before the frame is set up
			 * Use %rsp instead
			 */
			argp = &((struct x86_64_frame *)(ddb_regs.tf_rsp-8))->f_arg0;
		} else {
			argp = frame + 2;
		}

		while (narg) {
			if (argnp)
				(*pr)("%s=", *argnp++);
			(*pr)("%lx", db_get_value((long)argp, 8, FALSE));
			argp++;
			if (--narg != 0)
				(*pr)(",");
		}
		(*pr)(") at ");
		db_printsym(callpc, DB_STGY_PROC, pr);
		(*pr)("\n");

		if (lastframe == 0 && offset == 0 && !have_addr) {
			/* Frame really belongs to next callpc */
			struct x86_64_frame *fp = (void *)(ddb_regs.tf_rsp-8);

			lastframe = (long *)fp;
			callpc = (db_addr_t)
			    db_get_value((db_addr_t)&fp->f_retaddr, 8, FALSE);
			continue;
		}

		lastframe = frame;
		lastcallpc = callpc;
		if (!db_nextframe(&frame, &retaddr, &arg0,
		   &callpc, frame + 2, is_trap, pr))
			break;

		if (INKERNEL((long)frame)) {
			/* staying in kernel */
			if (frame < lastframe ||
			    (frame == lastframe && callpc == lastcallpc)) {
				(*pr)("Bad frame pointer: %p\n", frame);
				break;
			}
		} else if (INKERNEL((long)lastframe)) {
			/* switch from user to kernel */
			if (kernel_only)
				break;	/* kernel stack only */
		} else {
			/* in user */
			if (frame <= lastframe) {
				(*pr)("Bad user frame pointer: %p\n",
					  frame);
				break;
			}
		}
		--count;
	}

	if (count && is_trap != NONE) {
		db_printsym(callpc, DB_STGY_XTRN, pr);
		(*pr)(":\n");
	}
}
