/*	$NetBSD: trap.c,v 1.59 1997/06/16 05:37:39 jonathan Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */
#define	MINIDEBUG 1

#if !defined(MIPS1) && !defined(MIPS3)
#error  Neither  "MIPS1" (r2000 family), "MIPS3" (r4000 family) was configured.
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/buf.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/cpu.h>
#include <machine/pte.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <miscfs/procfs/procfs.h>

/* all this to get prototypes for ipintr() and arpintr() */
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <netinet/ip_var.h>

#include "ppp.h"

#if NPPP > 0
#include <net/ppp_defs.h>		/* decls of struct pppstat for..  */
#include <net/if_pppvar.h>		/* decl of enum for... */
#include <net/if_ppp.h>			/* pppintr() prototype */
#endif

/*
 * Port-specific hardware interrupt handler
 */

int (*mips_hardware_intr) __P((unsigned mask, unsigned pc, unsigned status,
			       unsigned cause)) =
	( int (*) __P((unsigned, unsigned, unsigned, unsigned)) ) 0;

/*
 * Exception-handling functions, called via ExceptionTable from locore
 */
extern void mips1_KernGenException __P((void));
extern void mips1_UserGenException __P((void));
extern void mips1_TLBMissException __P((void));
extern void mips1_SystemCall __P((void));
extern void mips1_KernIntr __P((void));
extern void mips1_UserIntr __P((void));
/* marks end of vector code */
extern void mips1_UTLBMiss	__P((void));
extern void mips1_exceptionentry_end __P((void));

#ifdef MIPS3
extern void mips3_KernGenException __P((void));
extern void mips3_UserGenException __P((void));
extern void mips3_SystemCall __P((void));
extern void mips3_KernIntr __P((void));
extern void mips3_UserIntr __P((void));
extern void mips3_TLBModException  __P((void));
extern void mips3_TLBMissException __P((void));
extern void mips3_TLBInvalidException __P((void));
extern void mips3_VCED __P((void));
extern void mips3_VCEI __P((void));

/* marks end of vector code */
extern void mips3_TLBMiss	__P((void));
extern void mips3_exceptionentry_end __P((void));
#endif


void (*mips1_ExceptionTable[]) __P((void)) = {
/*
 * The kernel exception handlers.
 */
    mips1_KernIntr,			/* 0 external interrupt */
    mips1_KernGenException,		/* 1 TLB modification */
    mips1_TLBMissException,		/* 2 TLB miss (load or instr. fetch) */
    mips1_TLBMissException,		/* 3 TLB miss (store) */
    mips1_KernGenException,		/* 4 address error (load or I-fetch) */
    mips1_KernGenException,		/* 5 address error (store) */
    mips1_KernGenException,		/* 6 bus error (I-fetch) */
    mips1_KernGenException,		/* 7 bus error (load or store) */
    mips1_KernGenException,		/* 8 system call */
    mips1_KernGenException,		/* 9 breakpoint */
    mips1_KernGenException,		/* 10 reserved instruction */
    mips1_KernGenException,		/* 11 coprocessor unusable */
    mips1_KernGenException,		/* 12 arithmetic overflow */
    mips1_KernGenException,		/* 13 r4k trap excpt, r3k reserved */
    mips1_KernGenException,		/* 14 r4k virt coherence, r3k reserved */
    mips1_KernGenException,		/* 15 r4k FP exception, r3k reserved */
    mips1_KernGenException,		/* 16 reserved */
    mips1_KernGenException,		/* 17 reserved */
    mips1_KernGenException,		/* 18 reserved */
    mips1_KernGenException,		/* 19 reserved */
    mips1_KernGenException,		/* 20 reserved */
    mips1_KernGenException,		/* 21 reserved */
    mips1_KernGenException,		/* 22 reserved */
    mips1_KernGenException,		/* 23 watch exception */
    mips1_KernGenException,		/* 24 reserved */
    mips1_KernGenException,		/* 25 reserved */
    mips1_KernGenException,		/* 26 reserved */
    mips1_KernGenException,		/* 27 reserved */
    mips1_KernGenException,		/* 28 reserved */
    mips1_KernGenException,		/* 29 reserved */
    mips1_KernGenException,		/* 30 reserved */
    mips1_KernGenException,		/* 31 virt. coherence exception data */
/*
 * The user exception handlers.
 */
    mips1_UserIntr,		        /*  0 */
    mips1_UserGenException,	        /*  1 */
    mips1_UserGenException,	        /*  2 */
    mips1_UserGenException,	        /*  3 */
    mips1_UserGenException,	        /*  4 */
    mips1_UserGenException,	        /*  5 */
    mips1_UserGenException,	        /*  6 */
    mips1_UserGenException,	        /*  7 */
    mips1_SystemCall,		        /*  8 */
    mips1_UserGenException,	        /*  9 */
    mips1_UserGenException,	        /* 10 */
    mips1_UserGenException,	        /* 11 */
    mips1_UserGenException,	        /* 12 */
    mips1_UserGenException,	        /* 13 */
    mips1_UserGenException,	        /* 14 */
    mips1_UserGenException,	        /* 15 */
    mips1_UserGenException,		/* 16 */
    mips1_UserGenException,		/* 17 */
    mips1_UserGenException,		/* 18 */
    mips1_UserGenException,		/* 19 */
    mips1_UserGenException,		/* 20 */
    mips1_UserGenException,		/* 21 */
    mips1_UserGenException,		/* 22 */
    mips1_UserGenException,		/* 23 */
    mips1_UserGenException,		/* 24 */
    mips1_UserGenException,		/* 25 */
    mips1_UserGenException,		/* 26 */
    mips1_UserGenException,		/* 27 */
    mips1_UserGenException,		/* 28 */
    mips1_UserGenException,		/* 29 */
    mips1_UserGenException,		/* 20 */
    mips1_UserGenException,		/* 31 */
};

#ifdef MIPS3		/* r4000 family (mips-III cpu) */

void (*mips3_ExceptionTable[]) __P((void)) = {
/*
 * The kernel exception handlers.
 */
    mips3_KernIntr,			/* 0 external interrupt */
    mips3_KernGenException,		/* 1 TLB modification */
    mips3_TLBInvalidException,		/* 2 TLB miss (load or instr. fetch) */
    mips3_TLBInvalidException,		/* 3 TLB miss (store) */
    mips3_KernGenException,		/* 4 address error (load or I-fetch) */
    mips3_KernGenException,		/* 5 address error (store) */
    mips3_KernGenException,		/* 6 bus error (I-fetch) */
    mips3_KernGenException,		/* 7 bus error (load or store) */
    mips3_KernGenException,		/* 8 system call */
    mips3_KernGenException,		/* 9 breakpoint */
    mips3_KernGenException,		/* 10 reserved instruction */
    mips3_KernGenException,		/* 11 coprocessor unusable */
    mips3_KernGenException,		/* 12 arithmetic overflow */
    mips3_KernGenException,		/* 13 r4k trap excpt, r3k reserved */
    mips3_VCEI,				/* 14 r4k virt coherence, r3k reserved */
    mips3_KernGenException,		/* 15 r4k FP exception, r3k reserved */
    mips3_KernGenException,		/* 16 reserved */
    mips3_KernGenException,		/* 17 reserved */
    mips3_KernGenException,		/* 18 reserved */
    mips3_KernGenException,		/* 19 reserved */
    mips3_KernGenException,		/* 20 reserved */
    mips3_KernGenException,		/* 21 reserved */
    mips3_KernGenException,		/* 22 reserved */
    mips3_KernGenException,		/* 23 watch exception */
    mips3_KernGenException,		/* 24 reserved */
    mips3_KernGenException,		/* 25 reserved */
    mips3_KernGenException,		/* 26 reserved */
    mips3_KernGenException,		/* 27 reserved */
    mips3_KernGenException,		/* 28 reserved */
    mips3_KernGenException,		/* 29 reserved */
    mips3_KernGenException,		/* 30 reserved */
    mips3_VCED,				/* 31 virt. coherence exception data */
/*
 * The user exception handlers.
 */
    mips3_UserIntr,		        /*  0 */
    mips3_UserGenException,	        /*  1 */
    mips3_UserGenException,	        /*  2 */
    mips3_UserGenException,	        /*  3 */
    mips3_UserGenException,	        /*  4 */
    mips3_UserGenException,	        /*  5 */
    mips3_UserGenException,	        /*  6 */
    mips3_UserGenException,	        /*  7 */
    mips3_SystemCall,		        /*  8 */
    mips3_UserGenException,	        /*  9 */
    mips3_UserGenException,	        /* 10 */
    mips3_UserGenException,	        /* 11 */
    mips3_UserGenException,	        /* 12 */
    mips3_UserGenException,	        /* 13 */
    mips3_VCEI,			        /* 14 */
    mips3_UserGenException,	        /* 15 */
    mips3_UserGenException,		/* 16 */
    mips3_UserGenException,		/* 17 */
    mips3_UserGenException,		/* 18 */
    mips3_UserGenException,		/* 19 */
    mips3_UserGenException,		/* 20 */
    mips3_UserGenException,		/* 21 */
    mips3_UserGenException,		/* 22 */
    mips3_UserGenException,		/* 23 */
    mips3_UserGenException,		/* 24 */
    mips3_UserGenException,		/* 25 */
    mips3_UserGenException,		/* 26 */
    mips3_UserGenException,		/* 27 */
    mips3_UserGenException,		/* 28 */
    mips3_UserGenException,		/* 29 */
    mips3_UserGenException,		/* 20 */
    mips3_VCED,				/* 31 virt. coherence exception data */
};
#endif	/* MIPS3 */


char	*trap_type[] = {
	"external interrupt",
	"TLB modification",
	"TLB miss (load or instr. fetch)",
	"TLB miss (store)",
	"address error (load or I-fetch)",
	"address error (store)",
	"bus error (I-fetch)",
	"bus error (load or store)",
	"system call",
	"breakpoint",
	"reserved instruction",
	"coprocessor unusable",
	"arithmetic overflow",
	"r4k trap/r3k reserved 13",
	"r4k virtual coherency instruction/r3k reserved 14",
	"r4k floating point/ r3k reserved 15",
	"reserved 16",
	"reserved 17",
	"reserved 18",
	"reserved 19",
	"reserved 20",
	"reserved 21",
	"reserved 22",
	"r4000 watch",
	"reserved 24",
	"reserved 25",
	"reserved 26",
	"reserved 27",
	"reserved 28",
	"reserved 29",
	"reserved 30",
	"r4000 virtual coherency data",
};
extern unsigned intrcnt[];

#ifdef DEBUG
#define TRAPSIZE	10
struct trapdebug {		/* trap history buffer for debugging */
	u_int	status;
	u_int	cause;
	u_int	vadr;
	u_int	pc;
	u_int	ra;
	u_int	sp;
	u_int	code;
} trapdebug[TRAPSIZE], *trp = trapdebug;

void trapDump __P((char * msg));
#endif	/* DEBUG */


#ifdef MINIDEBUG
void mips_dump_tlb __P((int, int));
void mdbpanic __P((void));
#endif

/*
 * Other forward declarations.
 */
unsigned MachEmulateBranch __P((unsigned *regsPtr,
			     unsigned instPC,
			     unsigned fpcCSR,
			     int allowNonBranch));

struct proc *fpcurproc;

/* extern functions used but not declared elsewhere */
extern void clearsoftclock __P((void));
extern void clearsoftnet __P((void));
extern void MachFPInterrupt __P((unsigned, unsigned, unsigned, int *));
extern void switchfpregs __P((struct proc *, struct proc *));

/* only called by locore */
extern void trap __P((u_int status, u_int cause, u_int vaddr,  u_int opc,
			 struct frame frame));
static void userret __P((struct proc *p, unsigned pc, u_quad_t sticks));
extern void syscall __P((unsigned status, unsigned cause, unsigned opc,
			 struct frame *frame));
extern void child_return __P((struct proc *p));
extern void interrupt __P((unsigned status, unsigned cause, unsigned pc,
			   struct frame *frame));
extern void ast __P((unsigned pc));

#ifdef DEBUG /* stack trace code, also useful to DDB one day */
int	kdbpeek __P((vm_offset_t addr));
extern void cpu_getregs __P((int *));
extern void stacktrace __P((void)); /*XXX*/
extern void logstacktrace __P((void)); /*XXX*/

/* extern functions printed by name in stack backtraces */
extern void idle __P((void)),  cpu_switch __P(( struct proc *p));
extern void MachEmptyWriteBuffer __P((void));
extern void MachUTLBMiss __P((void));
extern void setsoftclock __P((void));
extern int main __P((void*));
extern void am7990_meminit __P((void*)); /* XXX */
extern void savectx __P((struct user *));
#endif	/* DEBUG */


#ifdef MINIDEBUG
extern long mdbpeek __P((caddr_t addr));
extern void mdbpoke __P((caddr_t addr, long value));
extern int cngetc __P((void));
extern void cnputc __P((int));
extern int gethex __P((unsigned *val, unsigned dotval));
extern void dump __P((unsigned *addr, unsigned size));
extern void print_regs __P((struct frame *frame));
extern void set_break __P((u_int va));
extern void del_break __P((u_int va));
extern void break_insert __P((void));
extern void break_restore __P((void));
extern int break_find __P((u_int va));
extern void prt_break __P((void));
extern void mdbsetsstep __P((struct frame *frame));
extern int mdbclrsstep __P((struct frame *frame));
extern int mdbprintins __P((int ins, int mdbdot));
extern int mdb __P((int type, struct frame *frame));
#endif

/*
 * Index into intrcnt[], which is defined in locore
 */
typedef enum {
	SOFTCLOCK_INTR = 0,
	SOFTNET_INTR,
	SERIAL0_INTR,
	SERIAL1_INTR,
	SERIAL2_INTR,
	LANCE_INTR,
	SCSI_INTR,
	ERROR_INTR,
	HARDCLOCK,
  	FPU_INTR,
	SLOT0_INTR,
	SLOT1_INTR,
	SLOT2_INTR,
	DTOP_INTR,
	ISDN_INTR,
	FLOPPY_INTR,
	STRAY_INTR
} decstation_intr_t;

static void
userret(p, pc, sticks)
	struct proc *p;
	unsigned pc;
	u_quad_t sticks;
{
	int sig;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		postsig(sig);
	p->p_priority = p->p_usrpri;
	if (want_resched) {
		int s;
		/*
		 * Since we are curproc, a clock interrupt could
		 * change our priority without changing run queues
		 * (the running process is not kept on a run queue).
		 * If this happened after we setrunqueue ourselves but
		 * before we switch()'ed, we might not be on the queue
		 * indicated by our priority.
		 */
		s = splstatclock();
		setrunqueue(p);
		p->p_stats->p_ru.ru_nivcsw++;
		mi_switch();
		splx(s);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}
	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, pc, (int)(p->p_sticks - sticks) * psratio);
	}
	curpriority = p->p_priority;
}

#define DELAYBRANCH(x) ((x)<0)
/*
 * Process a system call.
 *
 * System calls are strange beasts.  They are passed the syscall number
 * in v0, and the arguments in the registers (as normal).  They return
 * an error flag in a3 (if a3 != 0 on return, the syscall had an error),
 * and the return value (if any) in v0 and possibly v1.
 */
void
syscall(status, cause, opc, frame)
	unsigned status, cause, opc;
	struct frame *frame;
{
	struct proc *p = curproc;
	u_quad_t sticks;
	int args[8], rval[2], error;
	size_t code, numsys, nsaved, argsiz;
	struct sysent *callp;

#ifdef DEBUG
	trp->status = status;
	trp->cause = cause;
	trp->vadr = 0;
	trp->pc = opc;
	trp->ra = frame->f_regs[RA];
	trp->sp = frame->f_regs[SP];
	trp->code = 0;
	if (++trp == &trapdebug[TRAPSIZE])
		trp = trapdebug;
#endif

	cnt.v_syscall++;

#ifdef MIPS3
	if (status & MIPS_SR_INT_IE)
#else
	if (status & MIPS1_SR_INT_ENA_PREV)
#endif
		splx(MIPS_SR_INT_IE | (status & MACH_HARD_INT_MASK));

	sticks = p->p_sticks;
	if (DELAYBRANCH(cause))
		frame->f_regs[PC] = MachEmulateBranch(frame->f_regs, opc, 0, 0);
	else
		frame->f_regs[PC] = opc + sizeof(int);
	callp = p->p_emul->e_sysent;
	numsys = p->p_emul->e_nsysent;
	code = frame->f_regs[V0];
	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = frame->f_regs[A0];
		args[0] = frame->f_regs[A1];
		args[1] = frame->f_regs[A2];
		args[2] = frame->f_regs[A3];
		nsaved = 3;
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = frame->f_regs[A0 + _QUAD_LOWWORD];
		args[0] = frame->f_regs[A2];
		args[1] = frame->f_regs[A3];
		nsaved = 2;
		break;
	default:
		args[0] = frame->f_regs[A0];
		args[1] = frame->f_regs[A1];
		args[2] = frame->f_regs[A2];
		args[3] = frame->f_regs[A3];
		nsaved = 4;
		break;
	}
	if (code >= p->p_emul->e_nsysent)
		callp += p->p_emul->e_nosys;
	else
		callp += code;
	argsiz = callp->sy_argsize / sizeof(int);
	if (argsiz > nsaved) {
		error = copyin(
			(caddr_t)((int *)frame->f_regs[SP] + 4),
			(caddr_t)(args + nsaved),
			(argsiz - nsaved) * sizeof(int));
		if (error)
			goto bad;
	}
#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, args);
#endif
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p->p_tracep, code, callp->sy_argsize, args);
#endif
	rval[0] = 0;
	rval[1] = frame->f_regs[V1];
#ifdef DEBUG
/* XXX save code */
#endif
	error = (*callp->sy_call)(p, args, rval);

#ifdef DEBUG
/* XXX save syscall result in trapdebug */
#endif
	switch (error) {
	case 0:
		frame->f_regs[V0] = rval[0];
		frame->f_regs[V1] = rval[1];
		frame->f_regs[A3] = 0;
		break;
	case ERESTART:
		frame->f_regs[PC] = opc;
		break;
	case EJUSTRETURN:
		break;	/* nothing to do */
	default:
	bad:
		frame->f_regs[V0] = error;
		frame->f_regs[A3] = 1;
		break;
	}
#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval);
#endif
	userret(p, opc, sticks);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, code, error, rval[0]);
#endif
}

/*
 * fork syscall returns directly to user process via proc_trampoline,
 * which will be called the very first time when child gets running.
 * no more FORK_BRAINDAMAGED.
 */
void
child_return(p)
	struct proc *p;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_regs[V0] = 0;
	frame->f_regs[V1] = 1;
	frame->f_regs[A3] = 0;
	userret(p, frame->f_regs[PC] - sizeof(int), 0); /* XXX */
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, SYS_fork, 0, 0);
#endif
}

#ifdef MIPS3
#define TRAPTYPE(x) (((x) & MIPS3_CR_EXC_CODE) >> MACH_CR_EXC_CODE_SHIFT)
#else
#define TRAPTYPE(x) (((x) & MIPS1_CR_EXC_CODE) >> MACH_CR_EXC_CODE_SHIFT)
#endif
#define KERNLAND(x) ((int)(x) < 0)

/*
 * Trap is called from locore to handle most types of processor traps.
 * System calls are broken out for efficiency.  MIPS can handle software
 * interrupts as a part of real interrupt processing.
 */
void
trap(status, cause, vaddr, opc, frame)
	u_int status;
	u_int cause;
	u_int vaddr;
	u_int opc;
	struct frame frame;
{
	int type, sig;
	int ucode = 0;
	u_quad_t sticks = 0;
	struct proc *p = curproc;
	vm_prot_t ftype;
	extern void fswintrberr __P((void));

#ifdef DEBUG
	trp->status = status;
	trp->cause = cause;
	trp->vadr = vaddr;
	trp->pc = opc;
	trp->ra = !USERMODE(status) ? frame.f_regs[RA] : p->p_md.md_regs[RA];
	trp->sp = (int)&frame;
	trp->code = 0;
	if (++trp == &trapdebug[TRAPSIZE])
		trp = trapdebug;
#endif
	cnt.v_trap++;
	type = TRAPTYPE(cause);
	if (USERMODE(status)) {
		type |= T_USER;
		sticks = p->p_sticks;
	}

#ifdef MIPS3
	if (status & MIPS_SR_INT_IE)
#else
	if (status & MIPS1_SR_INT_ENA_PREV)
#endif
		splx((status & MACH_HARD_INT_MASK) | MIPS_SR_INT_IE);

	switch (type) {
	default:
	dopanic:
		type &= ~T_USER;
		if (type >= 0 && type < 32)
			printf("trap: %s", trap_type[type]);
		else
			printf("trap: unknown %d", type);
		printf(" in %s mode\n", USERMODE(status) ? "user" : "kernel");
		printf("status=0x%x, cause=0x%x, pc=0x%x, v=0x%x\n",
		    status, cause, opc, vaddr);
		printf("usp=0x%x ksp=%p\n", p->p_md.md_regs[SP], (int*)&frame);
		if (curproc != NULL)
			printf("pid=%d cmd=%s\n", p->p_pid, p->p_comm);
		else
			printf("curproc == NULL\n");
#ifdef MINIDEBUG
		frame.f_regs[CAUSE] = cause;
		frame.f_regs[BADVADDR] = vaddr;
		mdb(type, (struct frame *)&frame.f_regs);
#else
#ifdef DEBUG
		stacktrace();
		trapDump("trap");
#endif
#endif
		panic("trap");
		/*NOTREACHED*/
	case T_TLB_MOD:
		if (KERNLAND(vaddr)) {
			pt_entry_t *pte;
			unsigned entry;
			vm_offset_t pa;

			pte = kvtopte(vaddr);
			entry = pte->pt_entry;
			if (!(entry & PG_V) || (entry & PG_M)) {
				mdb(type, (struct frame *)&frame.f_regs);
				panic("ktlbmod: invalid pte");
			}
/*XXX MIPS3? */		if (entry & PG_RO) {
				/* write to read only page in the kernel */
				ftype = VM_PROT_WRITE;
				goto kernelfault;
			}
			entry |= PG_M;
			pte->pt_entry = entry;
			vaddr &= ~PGOFSET;
			MachTLBUpdate(vaddr, entry);
			pa = pfn_to_vad(entry);
#ifdef ATTR
			pmap_attributes[atop(pa)] |= PMAP_ATTR_MOD;
#else
			if (!IS_VM_PHYSADDR(pa))
				panic("ktlbmod: unmanaged page");
			PHYS_TO_VM_PAGE(pa)->flags &= ~PG_CLEAN;
#endif
			return; /* KERN */
		}
		/*FALLTHROUGH*/
	case T_TLB_MOD+T_USER:
	    {
		pt_entry_t *pte;
		unsigned entry;
		vm_offset_t pa;
		pmap_t pmap;

		pmap  = p->p_vmspace->vm_map.pmap;
		if (!(pte = pmap_segmap(pmap, vaddr)))
			panic("utlbmod: invalid segmap");
		pte += (vaddr >> PGSHIFT) & (NPTEPG - 1);
		entry = pte->pt_entry;
		if (!(entry & PG_V) || (entry & PG_M))
			panic("utlbmod: invalid pte");

/*XXX MIPS3? */	if (entry & PG_RO) {
			/* write to read only page */
			ftype = VM_PROT_WRITE;
			goto pagefault;
		}
		entry |= PG_M;
		pte->pt_entry = entry;
		vaddr = (vaddr & ~PGOFSET) |
			(pmap->pm_tlbpid << VMMACH_TLB_PID_SHIFT);
		MachTLBUpdate(vaddr, entry);  
		pa = pfn_to_vad(entry);
#ifdef ATTR
		pmap_attributes[atop(pa)] |= PMAP_ATTR_MOD;
#else
		if (!IS_VM_PHYSADDR(pa))
			panic("utlbmod: unmanaged page");
		PHYS_TO_VM_PAGE(pa)->flags &= ~PG_CLEAN;
#endif
		if (type & T_USER)
			userret(p, opc, sticks);
		return; /* GEN */
	    }
	case T_TLB_LD_MISS:
	case T_TLB_ST_MISS:
		ftype = (type == T_TLB_LD_MISS) ? VM_PROT_READ : VM_PROT_WRITE;
		if (KERNLAND(vaddr))
			goto kernelfault;
		/*
		 * It is an error for the kernel to access user space except
		 * through the copyin/copyout routines.
		 */
		if (p->p_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		/* check for fuswintr() or suswintr() getting a page fault */
		if (p->p_addr->u_pcb.pcb_onfault == (caddr_t)fswintrberr) {
			frame.f_regs[PC] = (int)fswintrberr;
			return; /* KERN */
		}
		goto pagefault;
	case T_TLB_LD_MISS+T_USER:
		ftype = VM_PROT_READ;
		goto pagefault;
	case T_TLB_ST_MISS+T_USER:
		ftype = VM_PROT_WRITE;
	pagefault: ;
	    {
		vm_offset_t va;
		struct vmspace *vm; 
		vm_map_t map;
		int rv;

		vm = p->p_vmspace;
		map = &vm->vm_map;
		va = trunc_page((vm_offset_t)vaddr);
		rv = vm_fault(map, va, ftype, FALSE);
#ifdef VMFAULT_TRACE
		printf(
		"vm_fault(%p (pmap %p), %p (%p), %p, %d) -> %p at pc %p\n",
		map, vm->vm_map.pmap, va, vaddr, ftype, FALSE, rv, opc);
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((caddr_t)va >= vm->vm_maxsaddr) {
			if (rv == KERN_SUCCESS) {
				unsigned nss;

				nss = clrnd(btoc(USRSTACK-(unsigned)va));
				if (nss > vm->vm_ssize)
					vm->vm_ssize = nss;
			}
			else if (rv == KERN_PROTECTION_FAILURE) 
				rv = KERN_INVALID_ADDRESS;
		}
		if (rv == KERN_SUCCESS) {
			if (type & T_USER)
				userret(p, opc, sticks);
			return; /* GEN */
		}
		if ((type & T_USER) == 0) 
			goto copyfault;
		sig = (rv == KERN_PROTECTION_FAILURE) ? SIGBUS : SIGSEGV;
		ucode = vaddr;
		break; /* SIGNAL */  
		}
	kernelfault: ;
	    {
		vm_offset_t va;
		int rv; 

		va = trunc_page((vm_offset_t)vaddr);
		rv = vm_fault(kernel_map, va, ftype, FALSE);
		if (rv == KERN_SUCCESS)
			return; /* KERN */
		/*FALLTHROUGH*/
	    }
	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
	case T_BUS_ERR_LD_ST:	/* BERR asserted to cpu */
	copyfault:
		if (p->p_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		frame.f_regs[PC] = (int)p->p_addr->u_pcb.pcb_onfault;
		return; /* KERN */

	case T_ADDR_ERR_LD+T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST+T_USER:	/* misaligned or kseg access */
	case T_BUS_ERR_IFETCH+T_USER:	/* BERR asserted to cpu */
	case T_BUS_ERR_LD_ST+T_USER:	/* BERR asserted to cpu */
		sig = SIGSEGV;
		ucode = vaddr;
		break; /* SIGNAL */
	case T_BREAK+T_USER:
	    {
		unsigned va, instr; 
		int rv;

		/* compute address of break instruction */
		va = (DELAYBRANCH(cause)) ? opc + sizeof(int) : opc;

		/* read break instruction */
		instr = fuiword((caddr_t)va);

		if (p->p_md.md_ss_addr != va || instr != MACH_BREAK_SSTEP) {
			sig = SIGTRAP; 
			break;
		}
			/*
		 * Restore original instruction and clear BP
			 */
#ifndef NO_PROCFS_SUBR 
		rv = suiword((caddr_t)va, p->p_md.md_ss_instr);
		if (rv < 0) {
			vm_offset_t sa, ea;
			sa = trunc_page((vm_offset_t)va);
			ea = round_page((vm_offset_t)va + sizeof(int) - 1);
			rv = vm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_DEFAULT, FALSE);
			if (rv == KERN_SUCCESS) {
				rv = suiword((caddr_t)va, MACH_BREAK_SSTEP);
				(void)vm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, FALSE);
				}
			}
#else
		{
		struct uio uio;
		struct iovec iov;
		iov.iov_base = (caddr_t)&p->p_md.md_ss_instr;
		iov.iov_len = sizeof(int);
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = (off_t)va;
		uio.uio_resid = sizeof(int);
		uio.uio_segflg = UIO_SYSSPACE;
		uio.uio_rw = UIO_WRITE; 
		uio.uio_procp = curproc;
		rv = procfs_domem(p, p, NULL, &uio);
		MachFlushCache();  
		}
#endif
		if (rv < 0)
			printf("Warning: can't restore instruction at 0x%x: 0x%x\n",
				p->p_md.md_ss_addr, p->p_md.md_ss_instr);
		p->p_md.md_ss_addr = 0;
		sig = SIGTRAP;
		break; /* SIGNAL */
				}
	case T_RES_INST+T_USER:
		sig = SIGILL;
		break; /* SIGNAL */
	case T_COP_UNUSABLE+T_USER:
		if ((cause & MACH_CR_COP_ERR) != 0x10000000) {
			sig = SIGILL;	/* only FPU instructions allowed */
			break; /* SIGNAL */
			}
		switchfpregs(fpcurproc, p);
		fpcurproc = p;
		p->p_md.md_regs[PS] |= MACH_SR_COP_1_BIT;
		p->p_md.md_flags |= MDP_FPUSED;
		userret(p, opc, sticks);
		return; /* GEN */
	case T_FPE+T_USER:
		MachFPInterrupt(status, cause, opc, p->p_md.md_regs);  
		userret(p, opc, sticks);
		return; /* GEN */ 
	case T_OVFLOW+T_USER:
		sig = SIGFPE;
		break; /* SIGNAL */ 
		}
	p->p_md.md_regs[CAUSE] = cause;
	p->p_md.md_regs[BADVADDR] = vaddr;
	trapsignal(p, sig, ucode);
	if ((type & T_USER) == 0)
		panic("trapsignal");
	userret(p, opc, sticks);
	return; 
}

#include <net/netisr.h>
#include "ether.h"
#include "ppp.h"

/*
 * Handle an interrupt.
 * Called from MachKernIntr() or MachUserIntr()
 * Note: curproc might be NULL.
 */
void
interrupt(status, cause, pc, frame)
	unsigned status, cause, pc;
	struct frame *frame;  
{
	unsigned mask;

#ifdef DEBUG
	trp->status = status;
	trp->cause = cause;
	trp->vadr = 0;
	trp->pc = pc;
	trp->ra = 0;
	trp->sp = /* (int)&args */ 0;	/* XXX pass args in */
	trp->code = 0;
	if (++trp == &trapdebug[TRAPSIZE])
		trp = trapdebug;
#endif

	cnt.v_intr++;
	mask = cause & status;	/* pending interrupts & enable mask */

	/* Device interrupt */
	if (mips_hardware_intr)
		splx((*mips_hardware_intr)(mask, pc, status, cause));
	if (mask & MACH_INT_MASK_5) {
		intrcnt[FPU_INTR]++;
		if (USERMODE(status))
			MachFPInterrupt(status, cause, pc, frame->f_regs);
		else {
			printf("FPU interrupt: PC %x CR %x SR %x\n",
				pc, cause, status);
		}
	}

	/* Network software interrupt */
	if ((mask & MACH_SOFT_INT_MASK_1)
	    || (netisr && (status & MACH_SOFT_INT_MASK_1))) {
		register isr;
		isr = netisr; netisr = 0;	/* XXX need protect? */
		clearsoftnet();
		cnt.v_soft++;
		intrcnt[SOFTNET_INTR]++;
#ifdef INET
#if NETHER > 0
		if (isr & (1 << NETISR_ARP)) arpintr();
#endif
		if (isr & (1 << NETISR_IP)) ipintr();
#endif
#ifdef NS
		if (isr & (1 << NETISR_NS)) nsintr();
#endif
#ifdef ISO
		if (isr & (1 << NETISR_ISO)) clnlintr();
#endif
#if NPPP > 0
		if (isr & (1 << NETISR_PPP)) pppintr();
#endif
	}

	/* Software clock interrupt */
	if (mask & MACH_SOFT_INT_MASK_0) {
		clearsoftclock();
		cnt.v_soft++;
		intrcnt[SOFTCLOCK_INTR]++;
		softclock();
	}
}

/*
 * Handle asynchronous software traps.
 * This is called from MachUserIntr() either to deliver signals or
 * to make involuntary context switch (preemption).
 */
void
ast(pc)
	unsigned pc;		/* program counter where to continue */
{
	struct proc *p = curproc;

	cnt.v_soft++;
	astpending = 0;
	if (p->p_flag & P_OWEUPC) {
		p->p_flag &= ~P_OWEUPC;
		ADDUPROF(p);
	}
	userret(p, pc, p->p_sticks);
}


#ifdef DEBUG
void
trapDump(msg)
	char *msg;
{
	register int i;
	int s;

	s = splhigh();
	printf("trapDump(%s)\n", msg);
	for (i = 0; i < TRAPSIZE; i++) {
		if (trp == trapdebug)
			trp = &trapdebug[TRAPSIZE - 1];
		else
			trp--;
		if (trp->cause == 0)
			break;
		/* XXX mips3 cause is strictly bigger than mips1 */
		printf("%s: ADR %x PC %x CR %x SR %x\n",
			trap_type[(trp->cause & MACH_CR_EXC_CODE) >>
				MACH_CR_EXC_CODE_SHIFT],
			trp->vadr, trp->pc, trp->cause, trp->status);
		printf("   RA %x SP %x code %d\n", trp->ra, trp->sp, trp->code);
	}
	bzero(trapdebug, sizeof(trapdebug));
	trp = trapdebug;
	splx(s);
}
#endif

/*
 * forward declaration
 */
static unsigned GetBranchDest __P((InstFmt *InstPtr));


/*
 * Compute destination of a branch instruction.
 * XXX  Compute desination of r4000 squashed branches?
 */
static unsigned
GetBranchDest(InstPtr)
	InstFmt *InstPtr;
{
	return ((unsigned)InstPtr + 4 + ((short)InstPtr->IType.imm << 2));
}


/*
 * Return the resulting PC as if the branch was executed.
 */
unsigned
MachEmulateBranch(regsPtr, instPC, fpcCSR, allowNonBranch)
	unsigned *regsPtr;
	unsigned instPC;
	unsigned fpcCSR;
	int allowNonBranch;
{
	InstFmt inst;
	unsigned retAddr;
	int condition;

	inst.word = (instPC < MACH_CACHED_MEMORY_ADDR) ?
		fuiword((caddr_t)instPC) : *(unsigned*)instPC;

#if 0
	printf("regsPtr=%x PC=%x Inst=%x fpcCsr=%x\n", regsPtr, instPC,
		inst.word, fpcCSR); /* XXX */
#endif
	switch ((int)inst.JType.op) {
	case OP_SPECIAL:
		switch ((int)inst.RType.func) {
		case OP_JR:
		case OP_JALR:
			retAddr = regsPtr[inst.RType.rs];
			break;

		default:
			if (!allowNonBranch)
				panic("MachEmulateBranch: Non-branch");
			retAddr = instPC + 4;
			break;
		}
		break;

	case OP_BCOND:
		switch ((int)inst.IType.rt) {
		case OP_BLTZ:
		case OP_BLTZAL:
		case OP_BLTZL:		/* squashed */
		case OP_BLTZALL:	/* squashed */

			if ((int)(regsPtr[inst.RType.rs]) < 0)
				retAddr = GetBranchDest((InstFmt *)instPC);
			else
				retAddr = instPC + 8;
			break;

		case OP_BGEZ:
		case OP_BGEZAL:
		case OP_BGEZL:		/* squashed */
		case OP_BGEZALL:	/* squashed */

			if ((int)(regsPtr[inst.RType.rs]) >= 0)
				retAddr = GetBranchDest((InstFmt *)instPC);
			else
				retAddr = instPC + 8;
			break;

		default:
			panic("MachEmulateBranch: Bad branch cond");
		}
		break;

	case OP_J:
	case OP_JAL:
		retAddr = (inst.JType.target << 2) | 
			((unsigned)instPC & 0xF0000000);
		break;

	case OP_BEQ:
	case OP_BEQL:			/* squashed */

		if (regsPtr[inst.RType.rs] == regsPtr[inst.RType.rt])
			retAddr = GetBranchDest((InstFmt *)instPC);
		else
			retAddr = instPC + 8;
		break;

	case OP_BNE:
	case OP_BNEL:			/* squashed */

		if (regsPtr[inst.RType.rs] != regsPtr[inst.RType.rt])
			retAddr = GetBranchDest((InstFmt *)instPC);
		else
			retAddr = instPC + 8;
		break;

	case OP_BLEZ:
	case OP_BLEZL:				/* squashed */

		if ((int)(regsPtr[inst.RType.rs]) <= 0)
			retAddr = GetBranchDest((InstFmt *)instPC);
		else
			retAddr = instPC + 8;
		break;

	case OP_BGTZ:
	case OP_BGTZL:				/* squashed */

		if ((int)(regsPtr[inst.RType.rs]) > 0)
			retAddr = GetBranchDest((InstFmt *)instPC);
		else
			retAddr = instPC + 8;
		break;

	case OP_COP1:
		switch (inst.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			if ((inst.RType.rt & COPz_BC_TF_MASK) == COPz_BC_TRUE)
				condition = fpcCSR & MACH_FPC_COND_BIT;
			else
				condition = !(fpcCSR & MACH_FPC_COND_BIT);
			if (condition)
				retAddr = GetBranchDest((InstFmt *)instPC);
			else
				retAddr = instPC + 8;
			break;

		default:
			if (!allowNonBranch)
				panic("MachEmulateBranch: Bad coproc branch instruction");
			retAddr = instPC + 4;
		}
		break;

	default:
		if (!allowNonBranch)
			panic("MachEmulateBranch: Non-branch instruction");
		retAddr = instPC + 4;
	}
#if 0
	printf("Target addr=%x\n", retAddr); /* XXX */
#endif
	return (retAddr);
}


/*
 * This routine is called by procxmt() to single step one instruction.
 * We do this by storing a break instruction after the current instruction,
 * resuming execution, and then restoring the old instruction.
 */
int
mips_singlestep(p)
	struct proc *p;
{
#ifdef NO_PROCFS_SUBR
        struct frame *f = (struct frame *)p->p_md.md_regs;
        unsigned va = 0;
        int rv; 

        if (p->p_md.md_ss_addr) {
		printf("SS %s (%d): breakpoint already set at %x (va %x)\n",
			p->p_comm, p->p_pid, p->p_md.md_ss_addr, va); /* XXX */
                return EFAULT;
	}
        if (fuiword((caddr_t)f->f_regs[PC]) != 0) /* not a NOP instruction */
                va = MachEmulateBranch(f->f_regs, f->f_regs[PC],
                        p->p_addr->u_pcb.pcb_fpregs.r_regs[32], 1);
        else
                va = f->f_regs[PC] + sizeof(int);
	p->p_md.md_ss_addr = va;
	p->p_md.md_ss_instr = fuiword((caddr_t)va);
        rv = suiword((caddr_t)va, MACH_BREAK_SSTEP);
        if (rv < 0) {
		vm_offset_t sa, ea;
		sa = trunc_page((vm_offset_t)va);
                ea = round_page((vm_offset_t)va + sizeof(int) - 1);
                rv = vm_map_protect(&p->p_vmspace->vm_map,
                        sa, ea, VM_PROT_DEFAULT, FALSE);
		if (rv == KERN_SUCCESS) {
                        rv = suiword((caddr_t)va, MACH_BREAK_SSTEP);
                        (void)vm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, FALSE);
		}
	}
#else
        struct frame *f = (struct frame *)p->p_md.md_regs;
        unsigned pc = f->f_regs[PC];
        unsigned va = 0;
        int rv;
        int curinstr, bpinstr = MACH_BREAK_SSTEP;
	struct uio uio;
	struct iovec iov;

#define FETCH_INSTRUCTION(i, va) \
        iov.iov_base = (caddr_t)&(i);\
        iov.iov_len = sizeof(int); \
        uio.uio_iov = &iov;\
        uio.uio_iovcnt = 1;\
        uio.uio_offset = (off_t)(va);\
        uio.uio_resid = sizeof(int);\
        uio.uio_segflg = UIO_SYSSPACE;\
        uio.uio_rw = UIO_READ;\
        uio.uio_procp = curproc;\
        rv = procfs_domem(curproc, p, NULL, &uio)

#define STORE_INSTRUCTION(i, va) \
        iov.iov_base = (caddr_t)&(i);\
        iov.iov_len = sizeof(int);\
        uio.uio_iov = &iov;\
        uio.uio_iovcnt = 1;\
        uio.uio_offset = (off_t)(va);\
        uio.uio_resid = sizeof(int);\
        uio.uio_segflg = UIO_SYSSPACE;\
        uio.uio_rw = UIO_WRITE;\
        uio.uio_procp = curproc;\
        rv = procfs_domem(curproc, p, NULL, &uio)

        /* Fetch what's at the current location.  */
        FETCH_INSTRUCTION(curinstr, va);

	/* compute next address after current location */
        if (curinstr != 0)
                va = MachEmulateBranch(f->f_regs, pc,
                        p->p_addr->u_pcb.pcb_fpregs.r_regs[32], 1);
        else
                va = pc + sizeof(int);

	if (p->p_md.md_ss_addr) {
		printf("SS %s (%d): breakpoint already set at %x (va %x)\n",
			p->p_comm, p->p_pid, p->p_md.md_ss_addr, va); /* XXX */
		return (EFAULT);
	}
	p->p_md.md_ss_addr = va;

        /* Fetch what's at the current location. */
        FETCH_INSTRUCTION(p->p_md.md_ss_instr, va);

        /* Store breakpoint instruction at the "next" location now. */
        STORE_INSTRUCTION(bpinstr, va);
	MachFlushCache(); /* XXX memory barrier followed by flush icache? */
#endif
        if (rv < 0)
		return (EFAULT);
#if 0
	printf("SS %s (%d): breakpoint set at %x: %x (pc %x) br %x\n",
        printf("SS %s (%d): breakpoint set at %x: %x (pc %x) br %x\n",
		p->p_comm, p->p_pid, p->p_md.md_ss_addr,
                p->p_md.md_ss_instr, pc, fuword((caddr_t)va)); /* XXX */
#endif
        return 0;
}

#ifdef DEBUG
int
kdbpeek(addr)
	vm_offset_t addr;
{
	if (addr & 3) {
		printf("kdbpeek: unaligned address %lx\n", addr);
		return (-1);
	}
	return (*(int *)addr);
}

#define MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */

/* forward */
char *fn_name(unsigned addr);
void stacktrace_subr __P((int, int, int, int, void (*)(const char*, ...)));

/*
 * Do a stack backtrace.
 * (*printfn)()  prints the output to either the system log,
 * the console, or both.
 */
void
stacktrace_subr(a0, a1, a2, a3, printfn)
	int a0, a1, a2, a3;
	void (*printfn) __P((const char*, ...));
{
	unsigned pc, sp, fp, ra, va, subr;
	unsigned instr, mask;
	InstFmt i;
	int more, stksize;
	int regs[3];
	extern char start[], edata[];
	unsigned int frames =  0;

	cpu_getregs(regs);

	/* get initial values from the exception frame */
	sp = regs[0];
	pc = regs[1];
	ra = 0;
	fp = regs[2];

/* Jump here when done with a frame, to start a new one */
loop:
	ra = 0;

/* Jump here after a nonstandard (interrupt handler) frame */
specialframe:
	stksize = 0;
	subr = 0;
	if	(frames++ > 100) {
		(*printfn)("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/*XXX*/
	}

	/* check for bad SP: could foul up next frame */
	if (sp & 3 || sp < 0x80000000) {
		(*printfn)("SP 0x%x: not in kernel\n", sp);
		ra = 0;
		subr = 0;
		goto done;
	}

/*
 * check for PC between two entry points
 */
# define Between(x, y, z) \
		( ((x) <= (y)) && ((y) < (z)) )
# define pcBetween(a,b) \
		Between((unsigned)a, pc, (unsigned)b)


	/* Backtraces should continue through interrupts from kernel mode */
#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
	if (pcBetween(mips1_KernIntr, mips1_UserIntr)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("r3000 KernIntr+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips1_KernIntr, a0, a1, a2);
		a0 = kdbpeek(sp + 40);
		a1 = kdbpeek(sp + 44);
		a2 = kdbpeek(sp + 48);
		a3 = kdbpeek(sp + 52);

		pc = kdbpeek(sp + 20);	/* exc_pc - pc at time of exception */
		ra = kdbpeek(sp + 148);	/* ra at time of exception */
		sp = sp + 176;
		goto specialframe;
	}
#endif	/* MIPS1 */

#ifdef MIPS3		/* r4000 family (mips-III cpu) */
	if (pcBetween(mips3_KernIntr, mips3_UserIntr)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("R4000 KernIntr+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips3_KernIntr, a0, a1, a2);
		a0 = kdbpeek(sp + 40);
		a1 = kdbpeek(sp + 44);
		a2 = kdbpeek(sp + 48);
		a3 = kdbpeek(sp + 52);

		pc = kdbpeek(sp + 20);	/* exc_pc - pc at time of exception */
		ra = kdbpeek(sp + 148);	/* ra at time of exception */
		sp = sp + 176;
		goto specialframe;
	}
#endif	/* MIPS3 */



	/*
	 * Check for current PC in  exception handler code that don't
	 * have a preceding "j ra" at the tail of the preceding function. 
	 * Depends on relative ordering of functions in locore.
	 */

	/* XXX fixup tests after cutting and pasting in locore.S */
	/* R4000  exception handlers */

#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
	if (pcBetween(mips1_KernGenException, mips1_UserGenException))
		subr = (unsigned) mips1_KernGenException;
	else if (pcBetween(mips1_UserGenException,mips1_SystemCall))
		subr = (unsigned) mips1_UserGenException;
	else if (pcBetween(mips1_SystemCall,mips1_KernIntr))
		subr = (unsigned) mips1_UserGenException;
	else if (pcBetween(mips1_KernIntr, mips1_UserIntr))
		subr = (unsigned) mips1_KernIntr;
	else if (pcBetween(mips1_UserIntr, mips1_TLBMissException))
		subr = (unsigned) mips1_UserIntr;
	else if (pcBetween(mips1_UTLBMiss, mips1_exceptionentry_end)) {
		(*printfn)("<<mips1 locore>>");
		goto done;
	}
	else
#endif /* MIPS1 */


#ifdef MIPS3		/* r4000 family (mips-III cpu) */
	/* R4000  exception handlers */
	if (pcBetween(mips3_KernGenException, mips3_UserGenException))
		subr = (unsigned) mips3_KernGenException;
	else if (pcBetween(mips3_UserGenException,mips3_SystemCall))
		subr = (unsigned) mips3_UserGenException;
	else if (pcBetween(mips3_SystemCall,mips3_KernIntr))
		subr = (unsigned) mips3_SystemCall;
	else if (pcBetween(mips3_KernIntr, mips3_UserIntr))
		subr = (unsigned) mips3_KernIntr;

	else if (pcBetween(mips3_UserIntr, mips3_TLBInvalidException))
		subr = (unsigned) mips3_UserIntr;
	else if (pcBetween(mips3_TLBMiss, mips3_exceptionentry_end)) {
		(*printfn)("<<mips3 locore>>");
		goto done;
	} else
#endif /* MIPS3 */


	if (pcBetween(splx, switchfpregs))
		subr = (unsigned) splx;
	else if (pcBetween(cpu_switch, savectx))
		subr = (unsigned) cpu_switch;
	else if (pcBetween(idle, cpu_switch))	{
		subr = (unsigned) idle;
		ra = 0;
		goto done;
	}


	/* Check for bad PC */
	if (pc & 3 || pc < 0x80000000 || pc >= (unsigned)edata) {
		(*printfn)("PC 0x%x: not in kernel space\n", pc);
		ra = 0;
		goto done;
	}
	if (!pcBetween(start, (unsigned) edata)) {
		(*printfn)("PC 0x%x: not in kernel text\n", pc);
		ra = 0;
		goto done;
	}

	/*
	 * Find the beginning of the current subroutine by scanning backwards
	 * from the current PC for the end of the previous subroutine.
	 */
	if (!subr) {
		va = pc - sizeof(int);
		while ((instr = kdbpeek(va)) != MIPS_JR_RA)
		va -= sizeof(int);
		va += 2 * sizeof(int);	/* skip back over branch & delay slot */
		/* skip over nulls which might separate .o files */
		while ((instr = kdbpeek(va)) == 0)
			va += sizeof(int);
		subr = va;
	}

	/*
	 * Jump here for locore entry pointsn for which the preceding
	 * function doesn't end in "j ra"
	 */
#if 0
stackscan:
#endif
	/* scan forwards to find stack size and any saved registers */
	stksize = 0;
	more = 3;
	mask = 0;
	for (va = subr; more; va += sizeof(int),
	     		      more = (more == 3) ? 3 : more - 1) {
		/* stop if hit our current position */
		if (va >= pc)
			break;
		instr = kdbpeek(va);
		i.word = instr;
		switch (i.JType.op) {
		case OP_SPECIAL:
			switch (i.RType.func) {
			case OP_JR:
			case OP_JALR:
				more = 2; /* stop after next instruction */
				break;

			case OP_SYSCALL:
			case OP_BREAK:
				more = 1; /* stop now */
			};
			break;

		case OP_BCOND:
		case OP_J:
		case OP_JAL:
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
			more = 2; /* stop after next instruction */
			break;

		case OP_COP0:
		case OP_COP1:
		case OP_COP2:
		case OP_COP3:
			switch (i.RType.rs) {
			case OP_BCx:
			case OP_BCy:
				more = 2; /* stop after next instruction */
			};
			break;

		case OP_SW:
			/* look for saved registers on the stack */
			if (i.IType.rs != 29)
				break;
			/* only restore the first one */
			if (mask & (1 << i.IType.rt))
				break;
			mask |= (1 << i.IType.rt);
			switch (i.IType.rt) {
			case 4: /* a0 */
				a0 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 5: /* a1 */
				a1 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 6: /* a2 */
				a2 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 7: /* a3 */
				a3 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 30: /* fp */
				fp = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 31: /* ra */
				ra = kdbpeek(sp + (short)i.IType.imm);
			}
			break;

		case OP_ADDI:
		case OP_ADDIU:
			/* look for stack pointer adjustment */
			if (i.IType.rs != 29 || i.IType.rt != 29)
				break;
			stksize = - ((short)i.IType.imm);
		}
	}

done:
	(*printfn)("%s+%x (%x,%x,%x,%x) ra %x sz %d\n",
		fn_name(subr), pc - subr, a0, a1, a2, a3, ra, stksize);

	if (ra) {
		if (pc == ra && stksize == 0)
			(*printfn)("stacktrace: loop!\n");
		else {
			pc = ra;
			sp += stksize;
			ra = 0;
			goto loop;
		}
	} else {
finish:
		if (curproc)
			(*printfn)("User-level: pid %d\n", curproc->p_pid);
		else
			(*printfn)("User-level: curproc NULL\n");
	}
}

/*
 * Functions ``special'' enough to print by name
 */
#ifdef __STDC__
#define Name(_fn)  { (void*)_fn, # _fn }
#else
#define Name(_fn) { _fn, "_fn"}
#endif
static struct { void *addr; char *name;} names[] = {
	Name(stacktrace),
	Name(stacktrace_subr),
	Name(main),
	Name(interrupt),
	Name(trap),
#ifdef pmax
	Name(am7990_meminit),
#endif

#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
	Name(mips1_KernGenException),
	Name(mips1_UserGenException),
	Name(mips1_SystemCall),
	Name(mips1_KernIntr),
	Name(mips1_UserIntr),
#endif	/* MIPS1 */

#ifdef MIPS3		/* r4000 family (mips-III cpu) */
	Name(mips3_KernGenException),
	Name(mips3_UserGenException),
	Name(mips3_SystemCall),
	Name(mips3_KernIntr),
	Name(mips3_UserIntr),
#endif	/* MIPS3 */

	Name(splx),
	Name(idle),
	Name(cpu_switch),
	{0, 0}
};

/*
 * Map a function address to a string name, if known; or a hex string.
 */
char *
fn_name(unsigned addr)
{
	static char buf[17];
	int i = 0;

	for (i = 0; names[i].name; i++)
		if (names[i].addr == (void*)addr)
			return (names[i].name);
	sprintf(buf, "%x", addr);
	return (buf);
}

#endif /* DEBUG */

#ifdef MINIDEBUG

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* XXX -- need onfault processing */
long mdbpeek(caddr_t addr) { return (*(long *)addr); }
void mdbpoke(caddr_t addr, long value) { *(long *)addr = value; }	

static char *op_name[64] = {
/* 0 */ "spec", "bcond","j",	"jal",	"beq",	"bne",	"blez", "bgtz",
/* 8 */ "addi", "addiu","slti", "sltiu","andi", "ori",	"xori", "lui",
/*16 */ "cop0", "cop1", "cop2", "cop3", "beql", "bnel", "blezl","bgtzl",
/*24 */ "daddi","daddiu","ldl", "ldr",	"op34", "op35", "op36", "op37",
/*32 */ "lb",	"lh",	"lwl",	"lw",	"lbu",	"lhu",	"lwr",	"lwu",
/*40 */ "sb",	"sh",	"swl",	"sw",	"sdl",	"sdr",	"swr",	"cache",
/*48 */ "ll",	"lwc1", "lwc2", "lwc3", "lld",	"ldc1", "ldc2", "ld",
/*56 */ "sc",	"swc1", "swc2", "swc3", "scd",	"sdc1", "sdc2", "sd"
};

static char *spec_name[64] = {
/* 0 */ "sll",	"spec01","srl", "sra",	"sllv", "spec05","srlv","srav",
/* 8 */ "jr",	"jalr", "spec12","spec13","syscall","break","spec16","sync",
/*16 */ "mfhi", "mthi", "mflo", "mtlo", "dsllv","spec25","dsrlv","dsrav",
/*24 */ "mult", "multu","div",	"divu", "dmult","dmultu","ddiv","ddivu",
/*32 */ "add",	"addu", "sub",	"subu", "and",	"or",	"xor",	"nor",
/*40 */ "spec50","spec51","slt","sltu", "dadd","daddu","dsub","dsubu",
/*48 */ "tge","tgeu","tlt","tltu","teq","spec65","tne","spec67",
/*56 */ "dsll","spec71","dsrl","dsra","dsll32","spec75","dsrl32","dsra32"
};

static char *bcond_name[32] = {
/* 0 */ "bltz", "bgez", "bltzl", "bgezl", "?", "?", "?", "?",
/* 8 */ "tgei", "tgeiu", "tlti", "tltiu", "teqi", "?", "tnei", "?",
/*16 */ "bltzal", "bgezal", "bltzall", "bgezall", "?", "?", "?", "?",
/*24 */ "?", "?", "?", "?", "?", "?", "?", "?",
};

static char *cop1_name[64] = {
/* 0 */ "fadd", "fsub", "fmpy", "fdiv", "fsqrt","fabs", "fmov", "fneg",
/* 8 */ "fop08","fop09","fop0a","fop0b","fop0c","fop0d","fop0e","fop0f",
/*16 */ "fop10","fop11","fop12","fop13","fop14","fop15","fop16","fop17",
/*24 */ "fop18","fop19","fop1a","fop1b","fop1c","fop1d","fop1e","fop1f",
/*32 */ "fcvts","fcvtd","fcvte","fop23","fcvtw","fop25","fop26","fop27",
/*40 */ "fop28","fop29","fop2a","fop2b","fop2c","fop2d","fop2e","fop2f",
/*48 */ "fcmp.f","fcmp.un","fcmp.eq","fcmp.ueq","fcmp.olt","fcmp.ult",
	"fcmp.ole","fcmp.ule",
/*56 */ "fcmp.sf","fcmp.ngle","fcmp.seq","fcmp.ngl","fcmp.lt","fcmp.nge",
	"fcmp.le","fcmp.ngt"
};

static char *fmt_name[16] = {
	"s",	"d",	"e",	"fmt3",
	"w",	"fmt5", "fmt6", "fmt7",
	"fmt8", "fmt9", "fmta", "fmtb",
	"fmtc", "fmtd", "fmte", "fmtf"
};

static char *reg_name[32] = {
	"zero", "at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",  
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7", 
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra"
};

static char *c0_opname[64] = {
	"c0op00","tlbr",  "tlbwi", "c0op03","c0op04","c0op05","tlbwr", "c0op07",
	"tlbp",	 "c0op11","c0op12","c0op13","c0op14","c0op15","c0op16","c0op17",
	"rfe",	 "c0op21","c0op22","c0op23","c0op24","c0op25","c0op26","c0op27",
	"eret","c0op31","c0op32","c0op33","c0op34","c0op35","c0op36","c0op37",
	"c0op40","c0op41","c0op42","c0op43","c0op44","c0op45","c0op46","c0op47",
	"c0op50","c0op51","c0op52","c0op53","c0op54","c0op55","c0op56","c0op57",
	"c0op60","c0op61","c0op62","c0op63","c0op64","c0op65","c0op66","c0op67",
	"c0op70","c0op71","c0op72","c0op73","c0op74","c0op75","c0op77","c0op77",
};

static char *c0_reg[32] = {
	"index","random","tlblo0","tlblo1","context","tlbmask","wired","c0r7",
	"badvaddr","count","tlbhi","c0r11","sr","cause","epc",	"prid",
	"config","lladr","watchlo","watchhi","xcontext","c0r21","c0r22","c0r23",
	"c0r24","c0r25","ecc","cacheerr","taglo","taghi","errepc","c0r31"
};

extern char *trap_type[];

#define MAXBRK 10
struct brk {
	int	inst;
	int	addr;
} brk_tab[MAXBRK];

/*
 * Mini debugger for kernel.
 */	
int
gethex(val, dotval)
	unsigned *val, dotval;
{
	unsigned c;

	*val = 0;
	while ((c = cngetc()) != '\e' && c != '\n' && c != '\r') {
		if (c >= '0' && c <= '9') {
			*val = (*val << 4) + c - '0';
			cnputc(c);
		}
		else if (c >= 'a' && c <= 'f') {
			*val = (*val << 4) + c - 'a' + 10;
			cnputc(c);
		}
		else if (c == '\b' || c == 0x7f) {	
			*val = *val >> 4;
			printf("\b \b");
		}
		else if (c == ',') {
			cnputc(c);
			return(c);
		}
		else if (c == '.') {
			*val = dotval;;
			cnputc(c);
		}
		else
			cnputc('\a');
	}
	if (c == '\r')
		c = '\n';
	return(c);
}

void dump(addr, size)
unsigned *addr, size;
{	
	int	cnt;

	cnt = 0;
	size = (size + 3) / 4;
	while (size--) {
		if ((cnt++ & 3) == 0)
			printf("\n%08x: ",(int)addr);
		printf("%08x ",*addr++);
	}
}

void print_regs(frame)
struct frame *frame;
{
	printf("\n");
	printf("T0-7 %08x %08x %08x %08x %08x %08x %08x %08x\n",
		frame->f_regs[T0], frame->f_regs[T1],
		frame->f_regs[T2], frame->f_regs[T3],
		frame->f_regs[T4], frame->f_regs[T5],
		frame->f_regs[T6], frame->f_regs[T7]);
	printf("T8-9 %08x %08x    A0-4 %08x %08x %08x %08x\n",
		frame->f_regs[T8], frame->f_regs[T9],
		frame->f_regs[A0], frame->f_regs[A1],
		frame->f_regs[A2], frame->f_regs[A3]);
	printf("S0-7 %08x %08x %08x %08x %08x %08x %08x %08x\n",
		frame->f_regs[S0], frame->f_regs[S1],
		frame->f_regs[S2], frame->f_regs[S3],
		frame->f_regs[S4], frame->f_regs[S5],
		frame->f_regs[S6], frame->f_regs[S7]);
	printf("  S8 %08x  V0-1 %08x %08x  GP %08x   SP %08x\n",
		frame->f_regs[S8], frame->f_regs[V0],
		frame->f_regs[V1], frame->f_regs[GP],
		frame->f_regs[SP]);
	printf("  AT %08x  PC   %08x           RA %08x   SR %08x",
		frame->f_regs[AST], frame->f_regs[PC],
		frame->f_regs[RA], frame->f_regs[SR]);
}

void
set_break(va)
	u_int va;
{
	int i;

	va = va & ~3;
	for (i = 0; i < MAXBRK; i++) {
		if (brk_tab[i].addr == 0) {
			brk_tab[i].addr = va;
			brk_tab[i].inst = *(unsigned *)va;
			return;
		}

	}
	printf(" Break table full!!");
}

void
del_break(va)
	u_int va;
{
	int i;
 
	va = va & ~3;
	for (i = 0; i < MAXBRK; i++) {
		if (brk_tab[i].addr == va) {
			brk_tab[i].addr = 0;
			return;
		}
	}	
	printf(" Break to remove not found!!");
}

void
break_insert()
{
	int i;	

	for (i = 0; i < MAXBRK; i++) {
		if (brk_tab[i].addr != 0) {
			brk_tab[i].inst = *(unsigned *)brk_tab[i].addr;
			*(unsigned *)brk_tab[i].addr = MACH_BREAK_BRKPT;
			MachFlushDCache(brk_tab[i].addr,4);
			MachFlushICache(brk_tab[i].addr,4);	 
		}
	}
}

void
break_restore() 
{
	int i;



	for (i = 0; i < MAXBRK; i++) {
		if (brk_tab[i].addr != 0) {
			*(unsigned *)brk_tab[i].addr = brk_tab[i].inst;
			MachFlushDCache(brk_tab[i].addr,4);
			MachFlushICache(brk_tab[i].addr,4);
	}
	}
}

int
break_find(va)
	u_int va;
{
	int i;

	for (i = 0; i < MAXBRK; i++) {
		if (brk_tab[i].addr == va) {
			return(i);
	}
	}
	return(-1);
}

void
prt_break()
{
	int i;


	for (i = 0; i < MAXBRK; i++) {
		if (brk_tab[i].addr != 0) {
			printf("\n    %08x\t", brk_tab[i].addr);
			mdbprintins(brk_tab[i].inst, brk_tab[i].addr);
		}
	}
}

unsigned mdb_ss_addr;
unsigned mdb_ss_instr;

void
mdbsetsstep(frame)
	struct frame *frame;
{
	register unsigned va;
	int	pc, allowbranch;

	pc = frame->f_regs[PC];

	/* compute next address after current location */
	if ((allowbranch = mdbpeek((caddr_t)pc)) != 0)
		va = MachEmulateBranch(frame->f_regs, pc, 0, allowbranch);
	else
		va = pc + 4;
	if (mdb_ss_addr) {
		printf("mdbsetsstep: breakpoint already set at %x (va %x)\n",
			mdb_ss_addr, va);
		return;
	}
	mdb_ss_addr = va;

	if ((int)va < 0) {
		/* kernel address */
		mdb_ss_instr = mdbpeek((caddr_t)va);
		mdbpoke((caddr_t)va, MACH_BREAK_SSTEP);
		MachFlushDCache(va, 4);
		MachFlushICache(va, 4);
	}
	return;
}

int
mdbclrsstep(frame)
	struct frame *frame;
{
	register unsigned pc, va;

	/* fix pc if break instruction is in the delay slot */
	pc = frame->f_regs[PC];
	if (frame->f_regs[CAUSE] < 0)
		pc += 4;


	/* check to be sure its the one we are expecting */
	va = mdb_ss_addr;
	if (!va || va != pc)
		return(FALSE);

	/* read break instruction */
	if (mdbpeek((caddr_t)va) != MACH_BREAK_SSTEP)
		return(FALSE);

	if ((int)va < 0) {
		/* kernel address */
		mdbpoke((caddr_t)va, mdb_ss_instr);
		MachFlushDCache(va, 4);
		MachFlushICache(va, 4);
		mdb_ss_addr = 0;
		return(TRUE);
	}

	printf("can't clear break at %x\n", va);
	mdb_ss_addr = 0;
	return(FALSE);
}

int
mdbprintins(ins, mdbdot)
	int ins, mdbdot;
{
	InstFmt i;
	int delay = 0;

	i.word = ins;

		switch (i.JType.op) {
		case OP_SPECIAL:
		if (i.word == 0) {
			printf("nop");
			break;
		}
		if (i.RType.func == OP_ADDU && i.RType.rt == 0) {
			printf("move\t%s,%s",
				reg_name[i.RType.rd],
				reg_name[i.RType.rs]);
			break;
		}
		printf("%s", spec_name[i.RType.func]);
			switch (i.RType.func) {
		case OP_SLL:
		case OP_SRL:
		case OP_SRA:
		case OP_DSLL:

		case OP_DSRL:
		case OP_DSRA:
		case OP_DSLL32:
		case OP_DSRL32:
		case OP_DSRA32:
			printf("\t%s,%s,%d",
				reg_name[i.RType.rd],
				reg_name[i.RType.rt],
				i.RType.shamt);
			break;

		case OP_SLLV:
		case OP_SRLV:
		case OP_SRAV:
		case OP_DSLLV:
		case OP_DSRLV:
		case OP_DSRAV:
			printf("\t%s,%s,%s",
				reg_name[i.RType.rd],
				reg_name[i.RType.rt],
				reg_name[i.RType.rs]);
			break;

		case OP_MFHI:
		case OP_MFLO:
			printf("\t%s", reg_name[i.RType.rd]);
			break;

			case OP_JR:
			case OP_JALR:
			delay = 1;
			/* FALLTHROUGH */
		case OP_MTLO:
		case OP_MTHI:
			printf("\t%s", reg_name[i.RType.rs]);
			break;

		case OP_MULT:
		case OP_MULTU:
		case OP_DMULT:
		case OP_DMULTU:
		case OP_DIV:
		case OP_DIVU:
		case OP_DDIV:
		case OP_DDIVU:
			printf("\t%s,%s",
				reg_name[i.RType.rs],
				reg_name[i.RType.rt]);
				break;


			case OP_SYSCALL:
		case OP_SYNC:
			break;

			case OP_BREAK:
			printf("\t%d", (i.RType.rs << 5) | i.RType.rt);
			break;

		default:
			printf("\t%s,%s,%s",
				reg_name[i.RType.rd],
				reg_name[i.RType.rs],
				reg_name[i.RType.rt]);
			};
			break;

		case OP_BCOND:
		printf("%s\t%s,", bcond_name[i.IType.rt],
			reg_name[i.IType.rs]);
		goto pr_displ;

		case OP_BLEZ:
	case OP_BLEZL:
		case OP_BGTZ:
	case OP_BGTZL:
		printf("%s\t%s,", op_name[i.IType.op],
			reg_name[i.IType.rs]);
		goto pr_displ;

	case OP_BEQ:
	case OP_BEQL:
		if (i.IType.rs == 0 && i.IType.rt == 0) {
			printf("b\t");
			goto pr_displ;
		}
		/* FALLTHROUGH */
	case OP_BNE:
	case OP_BNEL:
		printf("%s\t%s,%s,", op_name[i.IType.op],
			reg_name[i.IType.rs],
			reg_name[i.IType.rt]);
	pr_displ:
		delay = 1;
		printf("0x%08x", mdbdot + 4 + ((short)i.IType.imm << 2));
			break;

		case OP_COP0:
			switch (i.RType.rs) {
			case OP_BCx:
			case OP_BCy:

			printf("bc0%c\t",
				"ft"[i.RType.rt & COPz_BC_TF_MASK]);
			goto pr_displ;

		case OP_MT:
			printf("mtc0\t%s,%s",
				reg_name[i.RType.rt],
				c0_reg[i.RType.rd]);
			break;

		case OP_DMT:
			printf("dmtc0\t%s,%s",
				reg_name[i.RType.rt],
				c0_reg[i.RType.rd]);
				break;

		case OP_MF:
			printf("mfc0\t%s,%s",
				reg_name[i.RType.rt],
				c0_reg[i.RType.rd]);
				break;

		case OP_DMF:
			printf("dmfc0\t%s,%s",
				reg_name[i.RType.rt],
				c0_reg[i.RType.rd]);
				break;

		default:
			printf("%s", c0_opname[i.FRType.func]);
		};
				break;

	case OP_COP1:
		switch (i.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			printf("bc1%c\t",
				"ft"[i.RType.rt & COPz_BC_TF_MASK]);
			goto pr_displ;

		case OP_MT:
			printf("mtc1\t%s,f%d",
				reg_name[i.RType.rt],
				i.RType.rd);
				break;

		case OP_MF:
			printf("mfc1\t%s,f%d",

				reg_name[i.RType.rt],
				i.RType.rd);
				break;

		case OP_CT:
			printf("ctc1\t%s,f%d",
				reg_name[i.RType.rt],
				i.RType.rd);
				break;

		case OP_CF:
			printf("cfc1\t%s,f%d",
				reg_name[i.RType.rt],
				i.RType.rd);
				break;

		default:
			printf("%s.%s\tf%d,f%d,f%d",
				cop1_name[i.FRType.func],
				fmt_name[i.FRType.fmt],
				i.FRType.fd, i.FRType.fs, i.FRType.ft);
		};
				break;

	case OP_J:
	case OP_JAL:
		printf("%s\t", op_name[i.JType.op]);
		printf("0x%8x",(mdbdot & 0xF0000000) | (i.JType.target << 2));
		delay = 1;
		break;

	case OP_LWC1:
	case OP_SWC1:
		printf("%s\tf%d,", op_name[i.IType.op],
			i.IType.rt);
		goto loadstore;

	case OP_LB:
	case OP_LH:
	case OP_LW:
	case OP_LD:
	case OP_LBU:
	case OP_LHU:
	case OP_LWU:
	case OP_SB:
	case OP_SH:
	case OP_SW:
	case OP_SD:
		printf("%s\t%s,", op_name[i.IType.op],
			reg_name[i.IType.rt]);
	loadstore:
		printf("%d(%s)", (short)i.IType.imm,
			reg_name[i.IType.rs]);
				break;

	case OP_ORI:
	case OP_XORI:
		if (i.IType.rs == 0) {
			printf("li\t%s,0x%x",
				reg_name[i.IType.rt],
				i.IType.imm);
			break;
			}
		/* FALLTHROUGH */
	case OP_ANDI:
		printf("%s\t%s,%s,0x%x", op_name[i.IType.op],
			reg_name[i.IType.rt],
			reg_name[i.IType.rs],
			i.IType.imm);
		break;

	case OP_LUI:
		printf("%s\t%s,0x%x", op_name[i.IType.op],
			reg_name[i.IType.rt],
			i.IType.imm);
			break;

		case OP_ADDI:
	case OP_DADDI:
		case OP_ADDIU:
	case OP_DADDIU:
		if (i.IType.rs == 0) {
			printf("li\t%s,%d",
				reg_name[i.IType.rt],
				(short)i.IType.imm);
				break;
	}
		/* FALLTHROUGH */
	default:
		printf("%s\t%s,%s,%d", op_name[i.IType.op],
			reg_name[i.IType.rt],
			reg_name[i.IType.rs],
			(short)i.IType.imm);
	}
	return(delay);
}

#ifdef MIPS3
/*
 *	Dump TLB contents.
 */
void
mips_dump_tlb(first, last)
	int first;
	int last;
{
	int tlbno;
	struct tlb tlb;
	extern void mips3_TLBRead(int, struct tlb *);

	tlbno = first;

	while(tlbno <= last) {
		mips3_TLBRead(tlbno, &tlb);
		if(tlb.tlb_lo0 & PG_V || tlb.tlb_lo1 & PG_V) {
			printf("TLB %2d vad 0x%08x ", tlbno, tlb.tlb_hi);
		}
		else {
			printf("TLB*%2d vad 0x%08x ", tlbno, tlb.tlb_hi);
		}
		printf("0=0x%08x ", pfn_to_vad(tlb.tlb_lo0));
		printf("%c", tlb.tlb_lo0 & PG_M ? 'M' : ' ');
		printf("%c", tlb.tlb_lo0 & PG_G ? 'G' : ' ');
		printf(" atr %x ", (tlb.tlb_lo0 >> 3) & 7);
		printf("1=0x%08x ", pfn_to_vad(tlb.tlb_lo1));
		printf("%c", tlb.tlb_lo1 & PG_M ? 'M' : ' ');
		printf("%c", tlb.tlb_lo1 & PG_G ? 'G' : ' ');
		printf(" atr %x ", (tlb.tlb_lo1 >> 3) & 7);
		printf(" sz=%x\n", tlb.tlb_mask);

		tlbno++;
	}
}
#else /* MIPS3 */
/*
 *	Dump TLB contents.
 */
void
mips_dump_tlb(first, last)
	int first;
	int last;
{
	int tlbno;
	extern u_int tlbhi, tlblo;
	extern void mips1_TLBRead(int);

	tlbno = first;

	while(tlbno <= last) {
		mips1_TLBRead(tlbno);
		if(tlblo & PG_V) {
			printf("TLB %2d vad 0x%08x ", tlbno, tlbhi);
		}
		else {
			printf("TLB*%2d vad 0x%08x ", tlbno, tlbhi);
		}
		printf("0x%08x ", tlblo & PG_FRAME);
		printf("%c", tlblo & PG_M ? 'M' : ' ');
		printf("%c", tlblo & PG_G ? 'G' : ' ');
		printf("%c\n", tlblo & PG_N ? 'N' : ' ');

		tlbno++;
	}
}
#endif /* MIPS3 */

int
mdb(type, frame)
	int type;	
	struct frame *frame;	
{
	int c;
	int newaddr;
	int size;
static int ssandrun;	/* Single step and run flag (when cont at brk) */

	splhigh();
	newaddr = frame->f_regs[PC];
	switch (type) { 
	case T_BREAK:
		if (*(int *)newaddr == MACH_BREAK_SOVER) {
			break_restore();
			frame->f_regs[PC] += 4;
			printf("\nStop break (panic)\n# ");
			printf("    %08x\t", newaddr);
			(void)mdbprintins(*(int *)newaddr, newaddr);
			printf("\n# ");
			break;
		}
		if (*(int *)newaddr == MACH_BREAK_BRKPT) {
			break_restore();
			printf("\rBRK %08x\t", newaddr);
			if (mdbprintins(*(int *)newaddr, newaddr)) {
				newaddr += 4;
				printf("\n    %08x\t", newaddr);
				(void)mdbprintins(*(int *)newaddr, newaddr);
			}
			printf("\n# ");
			break; 
		}	
		if (mdbclrsstep(frame)) {
			if (ssandrun) { /* Step over bp before free run */
				ssandrun = 0;
				break_insert();
				return(TRUE);
			}
			printf("\r    %08x\t", newaddr);
			if (mdbprintins(*(int *)newaddr, newaddr)) {
				newaddr += 4;
				printf("\n    %08x\t",newaddr);
				(void)mdbprintins(*(int *)newaddr, newaddr);
			}
			printf("\n# ");
		}
		break;

	default:
		printf("\n-- %s --\n# ", trap_type[type]);
	}
	ssandrun = 0;
	break_restore();

	while (1) {
		c = cngetc();
		switch (c) {
		case 'T':
#ifdef DEBUG
			trapDump("Debugger");
#else
			printf("trapDump not available\n");
#endif
			break;
		case 'b':
			printf("break-");
			c = cngetc();
			switch (c) {
			case 's':
				printf("set at ");
				c = gethex(&newaddr, newaddr);
				if (c != '\e') {
					set_break(newaddr);
				}
				break;

			case 'd':
				printf("delete at ");
				c = gethex(&newaddr, newaddr);
				if (c != '\e') {
					del_break(newaddr);
				}
				break;

			case 'p':
				printf("print");
				prt_break(); 
				break;
			}
			break;

		case 'r':	
			print_regs(frame);
			break;

		case 'I':
			printf("Instruction at ");
			c = gethex(&newaddr, newaddr);
			while (c != '\e') {
				printf("\n    %08x\t",newaddr);
				mdbprintins(*(int *)newaddr, newaddr);
				newaddr += 4;
				c = cngetc();
			}
			break;
		
		case 'c':
			printf("continue\n");
			if (break_find(frame->f_regs[PC]) >= 0) {
				ssandrun = 1;
				mdbsetsstep(frame);
			}
			else {
				break_insert();
			}
			return TRUE;
				
		case 'S':
#ifdef DEBUG
			printf("Stack traceback:\n");
			stacktrace();
#else
			printf("Stack traceback not available\n");
#endif
			break;
		case 's':	
			set_break(frame->f_regs[PC] + 8);
			return TRUE;	
		case ' ':	
			mdbsetsstep(frame);
			return TRUE;
		     
		case 'd':
			printf("dump ");
			c = gethex(&newaddr, newaddr);
			if (c == ',') { 
				c = gethex(&size, 256);
			}	
			else {
				size = 16;
			}	
			if ((c == '\n' || c == '\r') && newaddr != 0) {
				dump((unsigned *)newaddr, size);
				newaddr += size;
			}
			break;	
		
		case 'm':
			printf("mod ");
			c = gethex(&newaddr, newaddr);
			while (c == ',') {
				c = gethex(&size, 0);
				if (c != '\e')
					*((unsigned *)newaddr)++ = size;
			}	
			break;	

		case 'i':	
			printf("in-");
			c = cngetc();
			switch (c) {
			case 'b':
				printf("byte ");
				c = gethex(&newaddr, newaddr);
				if (c == '\n') {
					printf("= %02x",
						*(u_char *)newaddr);
				}
				break;
			case 'h':
				printf("halfword ");
				c = gethex(&newaddr, newaddr);
				if (c == '\n') {
					printf("= %04x", 
						*(u_short *)newaddr);
				}
				break;
			case 'w':
				printf("word ");
				c = gethex(&newaddr, newaddr);
				if (c == '\n') {
					printf("= %08x",
						*(unsigned *)newaddr);
				} 
				break;
			}
			break;	
		   
		case 'o':
			printf("out-");
			c = cngetc();
			switch (c) {
			case 'b':
				printf("byte ");
				c = gethex(&newaddr, newaddr);
				if (c == ',') {
					c = gethex(&size, 0);
					if (c == '\n') {
						*(u_char *)newaddr = size;
					}
				}	
				break;
			case 'h':
				printf("halfword ");
				c = gethex(&newaddr, newaddr);
				if (c == ',') {
					c = gethex(&size, 0);
					if (c == '\n') {
						*(u_short *)newaddr = size;
					}
				} 
				break;
			case 'w':	
				printf("word ");
				c = gethex(&newaddr, newaddr);
				if (c == ',') {
					c = gethex(&size, 0);
					if (c == '\n') {
						*(unsigned *)newaddr = size;
					}

				}	
				break;	
			}	
			break;	
		   
		case 't':
			printf("tlb-dump\n");
#ifdef MIPS3
			mips_dump_tlb(0, 23);
			(void)cngetc();
			mips_dump_tlb(24, 47);
#else
			mips_dump_tlb(0, 22);
			(void)cngetc();
			mips_dump_tlb(23, 45);
			(void)cngetc();
			mips_dump_tlb(46, 63);
#endif
			break;	
				
		case 'f':	
			printf("flush-"); 
			c = cngetc();	
			switch (c) {		
			case 't':	
				printf("tlb");
				MachTLBFlush();
				break;
		     
			case 'c': 
				printf("cache");
				MachFlushCache();
				break;	
			}	 
			break;		
				
		default:	
			cnputc('\a');	
			break;	
		}		
		printf("\n# "); 
	}				
	return FALSE;			
}						
#endif
