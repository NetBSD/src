/*	$NetBSD: db_trace.c,v 1.39 2003/05/21 23:12:18 kristerw Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.39 2003/05/21 23:12:18 kristerw Exp $");

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

/*
 * Machine register set.
 */

#define dbreg(xx) (long *)offsetof(db_regs_t, tf_ ## xx)

static int db_i386_regop(const struct db_variable *, db_expr_t *, int);

const struct db_variable db_regs[] = {
	{ "ds",		dbreg(ds),     db_i386_regop },
	{ "es",		dbreg(es),     db_i386_regop },
	{ "fs",		dbreg(fs),     db_i386_regop },
	{ "gs",		dbreg(gs),     db_i386_regop },
	{ "edi",	dbreg(edi),    db_i386_regop },
	{ "esi",	dbreg(esi),    db_i386_regop },
	{ "ebp",	dbreg(ebp),    db_i386_regop },
	{ "ebx",	dbreg(ebx),    db_i386_regop },
	{ "edx",	dbreg(edx),    db_i386_regop },
	{ "ecx",	dbreg(ecx),    db_i386_regop },
	{ "eax",	dbreg(eax),    db_i386_regop },
	{ "eip",	dbreg(eip),    db_i386_regop },
	{ "cs",		dbreg(cs),     db_i386_regop },
	{ "eflags",	dbreg(eflags), db_i386_regop },
	{ "esp",	dbreg(esp),    db_i386_regop },
	{ "ss",		dbreg(ss),     db_i386_regop },
};
const struct db_variable * const db_eregs =
    db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

static int
db_i386_regop (const struct db_variable *vp, db_expr_t *val, int opcode)
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
		panic("db_i386_regop: unknown op %d", opcode);
	}
	return 0;
}

/*
 * Stack trace.
 */
#define	INKERNEL(va)	(((vaddr_t)(va)) >= VM_MIN_KERNEL_ADDRESS)

struct i386_frame {
	struct i386_frame	*f_frame;
	int			f_retaddr;
	int			f_arg0;
};

#define	NONE		0
#define	TRAP		1
#define	SYSCALL		2
#define	INTERRUPT	3
#define INTERRUPT_TSS	4
#define TRAP_TSS	5

#define MAXNARG	16

db_addr_t	db_trap_symbol_value = 0;
db_addr_t	db_syscall_symbol_value = 0;
db_addr_t	db_kdintr_symbol_value = 0;
boolean_t	db_trace_symbols_found = FALSE;

void db_find_trace_symbols(void);
int db_numargs(int *);
int db_nextframe(int **, int **, int **, db_addr_t *, int *, int,
    void (*) (const char *, ...));
db_sym_t db_frame_info(int *, db_addr_t, char **, db_expr_t *, int *, int *);

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
 */
int
db_numargs(int *retaddrp)
{
	int	*argp;
	int	inst;
	int	args;
	extern char	etext[];

	argp = (int *)db_get_value((int)retaddrp, 4, FALSE);
	if (argp < (int *)VM_MIN_KERNEL_ADDRESS || argp > (int *)etext) {
		args = 5;
	} else {
		inst = db_get_value((int)argp, 4, FALSE);
		if ((inst & 0xff) == 0x59)	/* popl %ecx */
			args = 1;
		else if ((inst & 0xffff) == 0xc483)	/* addl %n, %esp */
			args = ((inst >> 16) & 0xff) / 4;
		else
			args = 5;
	}
	return (args);
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
db_nextframe(int **nextframe, int **retaddr, int **arg0, db_addr_t *ip,
	     int *argp, int is_trap, void (*pr) __P((const char *, ...)))
{
	struct trapframe *tf;
	struct i386tss *tss;
	struct i386_frame *fp;
	struct intrframe *ifp;
	int traptype, trapno, err, i;

	switch (is_trap) {
	    case NONE:
		*ip = (db_addr_t)
			db_get_value((int)*retaddr, 4, FALSE);
		fp = (struct i386_frame *)
			db_get_value((int)*nextframe, 4, FALSE);
		if (fp == NULL)
			return 0;
		*nextframe = (int *)&fp->f_frame;
		*retaddr = (int *)&fp->f_retaddr;
		*arg0 = (int *)&fp->f_arg0;
		break;

	    case TRAP_TSS:
	    case INTERRUPT_TSS:
		tss = (struct i386tss *)*argp;
		*ip = tss->__tss_eip;
		fp = (struct i386_frame *)tss->tss_ebp;
		if (fp == NULL)
			return 0;
		*nextframe = (int *)&fp->f_frame;
		*retaddr = (int *)&fp->f_retaddr;
		*arg0 = (int *)&fp->f_arg0;
		if (is_trap == INTERRUPT_TSS)
			(*pr)("--- interrupt via task gate ---\n");
		else
			(*pr)("--- trap via task gate ---\n");
		break;

	    case TRAP:
	    case SYSCALL:
	    case INTERRUPT:
	    default:

		/* The only argument to trap() or syscall() is the trapframe. */
		tf = (struct trapframe *)argp;
		*ip = (db_addr_t)tf->tf_eip;
		switch (is_trap) {
		case TRAP:
			(*pr)("--- trap (number %d) ---\n", tf->tf_trapno);
			break;
		case SYSCALL:
			(*pr)("--- syscall (number %d) ---\n", tf->tf_eax);
			break;
		case INTERRUPT:
			(*pr)("--- interrupt ---\n");
			break;
		}
		fp = (struct i386_frame *)tf->tf_ebp;
		if (fp == NULL)
			return 0;
		*nextframe = (int *)&fp->f_frame;
		*retaddr = (int *)&fp->f_retaddr;
		*arg0 = (int *)&fp->f_arg0;
		break;
	}

	/*
	 * A bit of a hack. Since %ebp may be used in the stub code,
	 * walk the stack looking for a valid interrupt frame. Such
	 * a frame can be recognized by always having err 0 and
	 * trapno T_ASTFLT.
	 */
	if (db_frame_info(*nextframe, (db_addr_t)*ip, NULL, NULL, &traptype,
	    NULL) != (db_sym_t)0
	    && traptype == INTERRUPT) {
		for (i = 0; i < 4; i++) {
			ifp = (struct intrframe *)(argp + i);
			err = db_get_value((int)&ifp->__if_err, 4, FALSE);
			trapno = db_get_value((int)&ifp->__if_trapno, 4, FALSE);
			if (err == 0 && trapno == T_ASTFLT) {
				*nextframe = (int *)ifp - 1;
				break;
			}
		}
	}
	return 1;
}

db_sym_t
db_frame_info(int *frame, db_addr_t callpc, char **namep, db_expr_t *offp,
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
	narg = MAXNARG;

	if (INKERNEL((int)frame) && name) {
		/*
		 * XXX traps should be based off of the Xtrap*
		 * locations rather than on trap, since some traps
		 * (e.g., npxdna) don't go through trap()
		 */
#ifdef __ELF__
		if (!strcmp(name, "trap_tss")) {
			*is_trap = TRAP_TSS;
			narg = 0;
		} else if (!strcmp(name, "trap")) {
			*is_trap = TRAP;
			narg = 0;
		} else if (!strcmp(name, "syscall")) {
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
			} else if (!strncmp(name, "Xtss_", 5)) {
				*is_trap = INTERRUPT_TSS;
				narg = 0;
			}
		}
#else
		if (!strcmp(name, "_trap_tss")) {
			*is_trap = TRAP_TSS;
			narg = 0;
		} else if (!strcmp(name, "_trap")) {
			*is_trap = TRAP;
			narg = 0;
		} else if (!strcmp(name, "_syscall")) {
			*is_trap = SYSCALL;
			narg = 0;
		} else if (name[0] == '_' && name[1] == 'X') {
			if (!strncmp(name, "_Xintr", 6) ||
			    !strncmp(name, "_Xresume", 8) ||
			    !strncmp(name, "_Xstray", 7) ||
			    !strncmp(name, "_Xhold", 6) ||
			    !strncmp(name, "_Xrecurse", 9) ||
			    !strcmp(name, "_Xdoreti") ||
			    !strncmp(name, "_Xsoft", 6)) {
				*is_trap = INTERRUPT;
				narg = 0;
			} else if (!strncmp(name, "_Xtss_", 6)) {
				*is_trap = INTERRUPT_TSS;
				narg = 0;
			}
		}
#endif /* __ELF__ */
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
	int *frame, *lastframe;
	int *retaddr, *arg0;
	int		*argp;
	db_addr_t	callpc;
	int		is_trap = 0;
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
		frame = (int *)ddb_regs.tf_ebp;
		callpc = (db_addr_t)ddb_regs.tf_eip;
		retaddr = frame + 1;
		arg0 = frame + 2;
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
			frame = (int *)u->u_pcb.pcb_ebp;
			(*pr)("at %p\n", frame);
		} else
			frame = (int *)addr;
		callpc = (db_addr_t)
			 db_get_value((int)(frame + 1), 4, FALSE);
		frame = (int *)*frame; /* XXXfvdl db_get_value? */
		retaddr = frame + 1;
		arg0 = frame + 2;
	}

	lastframe = 0;
	while (count && frame != 0) {
		int		narg;
		char *	name;
		db_expr_t	offset;
		db_sym_t	sym;
		char	*argnames[MAXNARG], **argnp = NULL;
		db_addr_t	lastcallpc;

		sym = db_frame_info(frame, callpc, &name, &offset, &is_trap,
				    &narg);

		if (lastframe == 0 && sym == (db_sym_t)0) {
			/* Symbol not found, peek at code */
			int	instr = db_get_value(callpc, 4, FALSE);

			offset = 1;
			if ((instr & 0x00ffffff) == 0x00e58955 ||
					/* enter: pushl %ebp, movl %esp, %ebp */
			    (instr & 0x0000ffff) == 0x0000e589
					/* enter+1: movl %esp, %ebp */) {
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
			 * Use %esp instead
			 */
			argp = &((struct i386_frame *)(ddb_regs.tf_esp-4))->f_arg0;
		} else {
			argp = frame + 2;
		}

		while (narg) {
			if (argnp)
				(*pr)("%s=", *argnp++);
			(*pr)("%lx", db_get_value((int)argp, 4, FALSE));
			argp++;
			if (--narg != 0)
				(*pr)(",");
		}
		(*pr)(") at ");
		db_printsym(callpc, DB_STGY_PROC, pr);
		(*pr)("\n");

		if (lastframe == 0 && offset == 0 && !have_addr) {
			/* Frame really belongs to next callpc */
			struct i386_frame *fp = (void *)(ddb_regs.tf_esp-4);

			lastframe = (int *)fp;
			callpc = (db_addr_t)
			    db_get_value((db_addr_t)&fp->f_retaddr, 4, FALSE);
			continue;
		}

		lastframe = frame;
		lastcallpc = callpc;
		if (!db_nextframe(&frame, &retaddr, &arg0,
		   &callpc, frame + 2, is_trap, pr))
			break;

		if (INKERNEL((int)frame)) {
			/* staying in kernel */
			if (frame < lastframe ||
			    (frame == lastframe && callpc == lastcallpc)) {
				(*pr)("Bad frame pointer: %p\n", frame);
				break;
			}
		} else if (INKERNEL((int)lastframe)) {
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
