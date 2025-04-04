/*	$NetBSD: db_machdep.c,v 1.60.2.1 2025/04/04 16:19:39 martin Exp $	*/

/*
 * :set tabs=4
 *
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
 *
 *	db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 *
 * VAX enhancements by cmcmanis@mcmanis.com no rights reserved :-)
 *
 */

/*
 * Interface to new debugger.
 * Taken from i386 port and modified for vax.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.60.2.1 2025/04/04 16:19:39 martin Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>		 /* just for boothowto --eichin */
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/proc.h>
#include <sys/reboot.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/rpb.h>
#include <vax/vax/gencons.h>

#include <ddb/db_active.h>
#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_variables.h>
#include <ddb/db_proc.h>

#include "ioconf.h"

db_regs_t ddb_regs;

void	kdbprinttrap(int, int);

int	db_active = 0;

static	int splsave; /* IPL before entering debugger */

#ifdef MULTIPROCESSOR
static struct cpu_info *stopcpu;
/*
 * Only the master CPU is allowed to enter DDB, but the correct frames
 * must still be there. Keep the state-machine here.
 */
static int
pause_cpus(void)
{
	volatile struct cpu_info *ci = curcpu();

	if (stopcpu == NULL) {
		stopcpu = curcpu();
		cpu_send_ipi(IPI_DEST_ALL, IPI_DDB);
	}
	if ((ci->ci_flags & CI_MASTERCPU) == 0) {
		ci->ci_flags |= CI_STOPPED;
		while (ci->ci_flags & CI_STOPPED)
			;
		return 1;
	} else
		return 0;
}

static void
resume_cpus(void)
{
	struct cpu_info *ci;
	int i;

	stopcpu = NULL;
	for (i = 0; i < cpu_cd.cd_ndevs; i++) {
		if ((ci = device_lookup_private(&cpu_cd, i)) == NULL)
			continue;
		ci->ci_flags &= ~CI_STOPPED;
	}
}
#endif
/*
 * VAX Call frame on the stack, this from
 * "Computer Programming and Architecture, The VAX-11"
 *		Henry Levy & Richard Eckhouse Jr.
 *			ISBN 0-932376-07-X
 */
typedef struct __vax_frame {
	u_int	vax_cond;		/* condition handler		   */
	u_int	vax_psw:16;		/* 16 bit processor status word	   */
	u_int	vax_regs:12;		/* Register save mask.		   */
	u_int	vax_zero:1;		/* Always zero			   */
	u_int	vax_calls:1;		/* True if CALLS, false if CALLG   */
	u_int	vax_spa:2;		/* Stack pointer alignment	   */
	u_int	*vax_ap;		/* argument pointer		   */
	struct __vax_frame *vax_fp;	/* frame pointer of previous frame */
	u_int	vax_pc;			/* program counter		   */
	u_int	vax_args[1];		/* 0 or more arguments		   */
} VAX_CALLFRAME;

/*
 * DDB is called by either <ESC> - D on keyboard, via a TRACE or
 * BPT trap or from kernel, normally as a result of a panic.
 * If it is the result of a panic, set the ddb register frame to
 * contain the registers when panic was called. (easy to debug).
 */
void
kdb_trap(struct trapframe *tf)
{
	int s;
#ifdef MULTIPROCESSOR
	struct cpu_info *ci = curcpu();
#endif

	switch (tf->tf_trap) {
	case T_BPTFLT:	/* breakpoint */
	case T_TRCTRAP: /* single_step */
		break;

	/* XXX todo: should be migrated to use VAX_CALLFRAME at some point */
	case T_KDBTRAP:
#ifndef MULTIPROCESSOR	/* No fancy panic stack conversion here */
		if (panicstr) {
			struct	callsframe *pf, *df;

			df = (void *)tf->tf_fp; /* start of debug's calls */
			pf = (void *)df->ca_fp; /* start of panic's calls */
			memcpy(&ddb_regs.tf_r0, &pf->ca_argno, sizeof(int) * 12);
			ddb_regs.tf_fp = pf->ca_fp;
			ddb_regs.tf_pc = pf->ca_pc;
			ddb_regs.tf_ap = pf->ca_ap;
			ddb_regs.tf_sp = (unsigned)pf;
			ddb_regs.tf_psl = tf->tf_psl & ~0x1fffe0;
			ddb_regs.tf_psl |= pf->ca_maskpsw & 0xffe0;
			ddb_regs.tf_psl |= (splsave << 16);
		}
#endif
		break;

	default:
		if ((boothowto & RB_KDB) == 0)
			return;

		kdbprinttrap(tf->tf_trap, tf->tf_code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

#ifdef MULTIPROCESSOR
	ci->ci_ddb_regs = tf;
	if (pause_cpus())
		return;
#endif
#ifndef MULTIPROCESSOR
	if (!panicstr)
		memcpy(&ddb_regs, tf, sizeof(struct trapframe));
#else
	memcpy(&ddb_regs, stopcpu->ci_ddb_regs, sizeof(struct trapframe));
	printf("stopped on CPU %d\n", stopcpu->ci_cpuid);
#endif

	/* XXX Should switch to interrupt stack here, if needed. */

	s = splhigh();
	db_active++;
	cnpollc(true);
	db_trap(tf->tf_trap, tf->tf_code);
	cnpollc(false);
	db_active--;
	splx(s);

#ifndef MULTIPROCESSOR
	if (!panicstr)
		memcpy(tf, &ddb_regs, sizeof(struct trapframe));
#else
	memcpy(stopcpu->ci_ddb_regs, &ddb_regs, sizeof(struct trapframe));
#endif
	tf->tf_sp = mfpr(PR_USP);
#ifdef MULTIPROCESSOR
	rpb.wait = 0;
	resume_cpus();
#endif
}

extern const char * const traptypes[];
extern int no_traps;

/*
 * Print trap reason.
 */
void
kdbprinttrap(int type, int code)
{
	db_printf("kernel: ");
	if (type >= no_traps || type < 0)
		db_printf("type %d", type);
	else
		db_printf("%s", traptypes[type]);
	db_printf(" trap, code=%x\n", code);
}

/*
 * Check whether an address is accessible.
 */
bool
db_validate_address(vaddr_t addr)
{
	struct proc *p = curproc;
	struct pmap *pmap;

	if (!p || !p->p_vmspace || !p->p_vmspace->vm_map.pmap ||
	    addr >= VM_MIN_KERNEL_ADDRESS)
		pmap = pmap_kernel();
	else
		pmap = p->p_vmspace->vm_map.pmap;

	return (pmap_extract(pmap, addr, NULL));
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	if (!db_validate_address(addr)) {
		db_printf("address 0x%lx is inaccessible\n", addr);
		db_error(NULL);
		/*NOTREACHED*/
	}

	memcpy(data, (void *)addr, size);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	if (!db_validate_address(addr)) {
		db_printf("address 0x%lx is inaccessible\n", addr);
		db_error(NULL);
		/*NOTREACHED*/
	}

	memcpy((void *)addr, data, size);
}

void
Debugger(void)
{
	splsave = splx(0xe);	/* XXX WRONG (this can lower IPL) */
	setsoftddb();		/* beg for debugger */
	splx(splsave);
}

/*
 * Machine register set.
 */
const struct db_variable db_regs[] = {
	{"r0",	&ddb_regs.tf_r0,	FCN_NULL},
	{"r1",	&ddb_regs.tf_r1,	FCN_NULL},
	{"r2",	&ddb_regs.tf_r2,	FCN_NULL},
	{"r3",	&ddb_regs.tf_r3,	FCN_NULL},
	{"r4",	&ddb_regs.tf_r4,	FCN_NULL},
	{"r5",	&ddb_regs.tf_r5,	FCN_NULL},
	{"r6",	&ddb_regs.tf_r6,	FCN_NULL},
	{"r7",	&ddb_regs.tf_r7,	FCN_NULL},
	{"r8",	&ddb_regs.tf_r8,	FCN_NULL},
	{"r9",	&ddb_regs.tf_r9,	FCN_NULL},
	{"r10", &ddb_regs.tf_r10,	FCN_NULL},
	{"r11", &ddb_regs.tf_r11,	FCN_NULL},
	{"ap",	&ddb_regs.tf_ap,	FCN_NULL},
	{"fp",	&ddb_regs.tf_fp,	FCN_NULL},
	{"sp",	&ddb_regs.tf_sp,	FCN_NULL},
	{"pc",	&ddb_regs.tf_pc,	FCN_NULL},
	{"psl", &ddb_regs.tf_psl,	FCN_NULL},
};
const struct db_variable * const db_eregs = db_regs + __arraycount(db_regs);

#define IN_USERLAND(x)	(((u_int)(x) & 0x80000000) == 0)

/*
 * Dump a stack traceback. Takes two arguments:
 *	fp - CALL FRAME pointer
 *	pr - printf-like function pointer for output
 */
static void
db_dump_stack(VAX_CALLFRAME *fp, void (*pr)(const char *, ...))
{
	u_int nargs, arg_base;
	VAX_CALLFRAME *tmp_frame;
	db_expr_t	diff;
	db_sym_t	sym;
	const char	*symname;
	extern int	sret;
	extern unsigned int etext;

	(*pr)("Stack traceback:\n");
	if (IN_USERLAND(fp)) {
		(*pr)("\t Process is executing in user space.\n");
		return;
	}

	if (!db_validate_address((vaddr_t)fp)) {
		(*pr)("\t frame 0x%x inaccessible.\n", fp);
		return;
	}

	while (!IN_USERLAND(fp->vax_fp) &&
	    db_validate_address((vaddr_t)fp->vax_fp)) {
		u_int pc = fp->vax_pc;

		/*
		 * Figure out the arguments by using a bit of subtlety.
		 * As the argument pointer may have been used as a temporary
		 * by the callee ... recreate what it would have pointed to
		 * as follows:
		 * The vax_regs value is a 12 bit bitmask of the registers
		 * that were saved on the stack, corresponding to R0 to R11.
		 * By counting those bits, we get a value for arg_base so that
		 * args[arg_base] points to the longword that identifies the
		 * number of arguments.
		 * arg_base+1 - arg_base+n are the argument pointers/contents.
		 */

		/*
		 * If this was due to a trap/fault, pull the correct pc
		 * out of the trap frame.
		 */
		if (pc == (u_int) &sret && fp->vax_fp != 0) {
			struct trapframe *tf;

			/* Count saved register bits */
			arg_base = popcount(fp->vax_regs);

			tf = (struct trapframe *) &fp->vax_args[arg_base + 2];
			if (!db_validate_address((vaddr_t)tf)) {
				(*pr)("0x%lx: trap frame inaccessible.\n", tf);
				return;
			}

			(*pr)("0x%lx: trap type=0x%lx code=0x%lx pc=0x%lx "
				"psl=0x%lx\n", tf, tf->tf_trap, tf->tf_code,
				tf->tf_pc, tf->tf_psl);
			pc = tf->tf_pc;
		}

		diff = INT_MAX;
		symname = NULL;
		if (pc >= VM_MIN_KERNEL_ADDRESS && pc < (u_int) &etext) {
			sym = db_search_symbol(pc, DB_STGY_ANY, &diff);
			db_symbol_values(sym, &symname, 0);
		}
		if (symname != NULL)
			(*pr)("0x%lx: %s+0x%lx(", fp, symname, diff);
		else
			(*pr)("0x%lx: %#x(", fp, pc);

		/* First get the frame that called this function ... */
		tmp_frame = fp->vax_fp;

		/* Count saved register bits */
		arg_base = popcount(tmp_frame->vax_regs);

		/* number of arguments is then pointed to by vax_args[arg_base] */
		nargs = tmp_frame->vax_args[arg_base];
		if (nargs) {
			nargs--; /* reduce by one for formatting niceties */
			arg_base++; /* skip past the actual number of arguments */
			while (nargs--)
				(*pr)("%#x,", tmp_frame->vax_args[arg_base++]);

			/* now print out the last arg with closing brace and \n */
			(*pr)("%#x)\n", tmp_frame->vax_args[arg_base]);
		} else
			(*pr)("void)\n");
		/* move to the next frame */
		fp = fp->vax_fp;
	}
}

static void
db_trace_process(struct lwp *l, struct proc *p, void (*pr)(const char *, ...))
{
	struct pcb *pcb;

	KASSERT(l != NULL);
	KASSERT(p != NULL);

	if (!db_validate_address((vaddr_t)l)) {
		(*pr)("LWP at %p inaccessible\n", l);
		return;
	}

	if (!db_validate_address((vaddr_t)p)) {
		(*pr)("Process at %p inaccessible\n", p);
		return;
	}

	pcb = lwp_getpcb(l);
	if (!db_validate_address((vaddr_t)pcb)) {
		(*pr)("PCB at %p inaccessible\n", pcb);
		return;
	}

	(*pr)("Process %d.%d\n", p->p_pid, l->l_lid);
	(*pr)("\t PCB contents:\n");
	(*pr)(" KSP = 0x%x\n", (unsigned int)(pcb->KSP));
	(*pr)(" ESP = 0x%x\n", (unsigned int)(pcb->ESP));
	(*pr)(" SSP = 0x%x\n", (unsigned int)(pcb->SSP));
	(*pr)(" USP = 0x%x\n", (unsigned int)(pcb->USP));
	(*pr)(" R[00] = 0x%08x	  R[06] = 0x%08x\n",
		(unsigned int)(pcb->R[0]), (unsigned int)(pcb->R[6]));
	(*pr)(" R[01] = 0x%08x	  R[07] = 0x%08x\n",
		(unsigned int)(pcb->R[1]), (unsigned int)(pcb->R[7]));
	(*pr)(" R[02] = 0x%08x	  R[08] = 0x%08x\n",
		(unsigned int)(pcb->R[2]), (unsigned int)(pcb->R[8]));
	(*pr)(" R[03] = 0x%08x	  R[09] = 0x%08x\n",
		(unsigned int)(pcb->R[3]), (unsigned int)(pcb->R[9]));
	(*pr)(" R[04] = 0x%08x	  R[10] = 0x%08x\n",
		(unsigned int)(pcb->R[4]), (unsigned int)(pcb->R[10]));
	(*pr)(" R[05] = 0x%08x	  R[11] = 0x%08x\n",
		(unsigned int)(pcb->R[5]), (unsigned int)(pcb->R[11]));
	(*pr)(" AP = 0x%x\n", (unsigned int)(pcb->AP));
	(*pr)(" FP = 0x%x\n", (unsigned int)(pcb->FP));
	(*pr)(" PC = 0x%x\n", (unsigned int)(pcb->PC));
	(*pr)(" PSL = 0x%x\n", (unsigned int)(pcb->PSL));
	(*pr)(" Trap frame pointer: 0x%x\n", l->l_md.md_utf);

	db_dump_stack((VAX_CALLFRAME *)(pcb->FP), pr);

	return;
}

/*
 * Implement the trace command which has the form:
 *
 *	trace			<-- Trace panic (same as before)
 *	trace	0x88888		<-- Trace frame whose address is 888888
 *	trace/t			<-- Trace current process (0 if no current proc)
 *	trace/t 0tnn		<-- Trace process nn (0t for decimal)
 */
void
db_stack_trace_print(
	db_expr_t	addr,		/* Address parameter */
	bool		have_addr,	/* True if addr is valid */
	db_expr_t	count,		/* Optional count */
	const char	*modif,		/* pointer to flag modifier 't' */
	void		(*pr)(const char *, ...)) /* Print function */
{
	struct lwp	*l = curlwp;
	struct proc	*p = l->l_proc;
	int		trace_proc;
	int		trace_lwp;
	const char	*s;

	/* Check to see if we're tracing a process or lwp */
	trace_proc = 0;
	s = modif;
	do {
		switch (*s) {
		case 't':
			trace_proc++;
			break;

		case 'a':
			trace_lwp++;
			break;
		}
	} while (*s++ && !(trace_proc || trace_lwp));

	/*
	 * If we're here because of a panic, we'll always have an address of a
	 * frame.
	 *
	 * If the user typed an address, it can be one of the following:
	 * - the address of a struct lwp
	 * - the address of a struct proc
	 * - a PID, hex or decimal
	 * - the address of a frame
	 */
	if (have_addr) {
		if (trace_lwp) {
			l = (struct lwp *)addr;
			p = l->l_proc;
			db_trace_process(l, p, pr);
			return;
		} else if (trace_proc) {
			p = db_proc_find((int)addr);

			/*
			 * Try to be helpful by looking at it as if it were
			 * decimal.
			 */
			if (p == NULL) {
				u_int	tpid = 0;
				u_int	foo = addr;

				while (foo != 0) {
					int digit = (foo >> 28) & 0xf;
					if (digit > 9) {
						tpid = -1;
						break;
					}
					tpid = tpid * 10 + digit;
					foo = foo << 4;
				}
				p = db_proc_find(tpid);
			}

			/* Not a PID. If accessible, assume it's an address. */
			if (p == NULL) {
				if (db_validate_address(addr)) {
					p = (struct proc *)addr;
				} else {
					(*pr)("\t No such process.\n");
					return;
				}
			}

			db_trace_process(p->p_lwps.lh_first, p, pr);
			return;
		} else {
			db_dump_stack((VAX_CALLFRAME *)addr, pr);
			return;
		}
	} else {
		/* We got no address. Grab stuff from ddb_regs. */
		if (panicstr)
			db_dump_stack((VAX_CALLFRAME *)ddb_regs.tf_fp, pr);
		else
			(*pr)("\t trace what?\n");
	}
}

static int ddbescape = 0;

int
kdbrint(int tkn)
{
	if (ddbescape && ((tkn & 0x7f) == 'D')) {
		setsoftddb();
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

#ifdef MULTIPROCESSOR

static void
db_mach_cpu(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct cpu_info *ci;

	if ((addr < 0) || (addr >= cpu_cd.cd_ndevs))
		return db_printf("%ld: CPU out of range\n", addr);
	if ((ci = device_lookup_private(&cpu_cd, addr)) == NULL)
		return db_printf("%ld: CPU not configured\n", addr);

	if ((ci != curcpu()) && ((ci->ci_flags & CI_STOPPED) == 0))
		return db_printf("CPU %ld not stopped???\n", addr);

	memcpy(stopcpu->ci_ddb_regs, &ddb_regs, sizeof(struct trapframe));
	stopcpu = ci;
	memcpy(&ddb_regs, stopcpu->ci_ddb_regs, sizeof(struct trapframe));
	db_printf("using CPU %ld", addr);
	if (ci->ci_curlwp)
		db_printf(" in proc %d.%d (%s)\n",
		    ci->ci_curlwp->l_proc->p_pid,
		    ci->ci_curlwp->l_lid,
		    ci->ci_curlwp->l_proc->p_comm);
}
#endif

const struct db_command db_machine_command_table[] = {
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("cpu",	db_mach_cpu,	0,
	  "switch to another cpu", "cpu-no", NULL) },
#endif
	{ DDB_END_CMD },
};
