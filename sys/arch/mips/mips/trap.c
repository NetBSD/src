/*	$NetBSD: trap.c,v 1.72 1997/07/20 19:40:19 jonathan Exp $	*/

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

#include <machine/cpu.h>
#include <mips/trap.h>
#include <machine/psl.h>
#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/pte.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <sys/cdefs.h>
#include <sys/syslog.h>
#include <miscfs/procfs/procfs.h>

#ifdef DDB
#include <mips/db_machdep.h>
#endif

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
#ifdef MIPS1
 extern void mips1_KernGenException __P((void));
extern void mips1_UserGenException __P((void));
extern void mips1_TLBMissException __P((void));
extern void mips1_SystemCall __P((void));
extern void mips1_KernIntr __P((void));
extern void mips1_UserIntr __P((void));
/* marks end of vector code */
extern void mips1_UTLBMiss	__P((void));
extern void mips1_exceptionentry_end __P((void));
#endif

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


#ifdef MIPS1
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
#endif	/* MIPS1 */

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


void mips1_dump_tlb __P((int, int, void (*printfn)(const char*, ...)));
void mips3_dump_tlb __P((int, int, void (*printfn)(const char*, ...)));
void mips_dump_tlb __P((int, int));

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

 /*
  *  stack trace code, also useful to DDB one day
  */
#if defined(DEBUG) || defined(MDB) || defined(DDB)
int	kdbpeek __P((vm_offset_t addr));
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
#endif	/* DEBUG || MDB */

#ifdef MDB
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

#define DELAYBRANCH(x) ((int)(x)<0)
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

	if (status & ((CPUISMIPS3) ? MIPS_SR_INT_IE : MIPS1_SR_INT_ENA_PREV))
		splx(MIPS_SR_INT_IE | (status & MIPS_HARD_INT_MASK));

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
#define TRAPTYPE(x) (((x) & MIPS3_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#else
#define TRAPTYPE(x) (((x) & MIPS1_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
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

	if (status & ((CPUISMIPS3) ? MIPS_SR_INT_IE : MIPS1_SR_INT_ENA_PREV))
		splx((status & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

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
#ifdef DEBUG
		stacktrace();
		trapDump("trap");
#endif	/* DEBUG */
		panic("trap");
		/*NOTREACHED*/
	case T_TLB_MOD:
		if (KERNLAND(vaddr)) {
			pt_entry_t *pte;
			unsigned entry;
			vm_offset_t pa;

			pte = kvtopte(vaddr);
			entry = pte->pt_entry;
			if (!mips_pg_v(entry) || (entry & mips_pg_m_bit())) {
				panic("ktlbmod: invalid pte");
			}
/*XXX MIPS3? */		if (entry & mips_pg_ro_bit()) {
				/* write to read only page in the kernel */
				ftype = VM_PROT_WRITE;
				goto kernelfault;
			}
			entry |= mips_pg_m_bit();
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
		if (!mips_pg_v(entry) || (entry & mips_pg_m_bit()))
			panic("utlbmod: invalid pte");

/*XXX MIPS3? */	if (entry & mips_pg_ro_bit()) {
			/* write to read only page */
			ftype = VM_PROT_WRITE;
			goto pagefault;
		}
		entry |= mips_pg_m_bit();
		pte->pt_entry = entry;
		vaddr = (vaddr & ~PGOFSET) |
			(pmap->pm_tlbpid << MIPS_TLB_PID_SHIFT);
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

	case T_BREAK:
#ifdef DDB
		kdb_trap(type, (struct frame *)&frame.f_regs);
		return;	/* KERN */
#else
		goto dopanic;
#endif
	case T_BREAK+T_USER:
	    {
		unsigned va, instr; 
		int rv;

		/* compute address of break instruction */
		va = (DELAYBRANCH(cause)) ? opc + sizeof(int) : opc;

		/* read break instruction */
		instr = fuiword((caddr_t)va);
#ifdef DEBUG
/*XXX*/		printf("break insn  0x%x\n", instr);
#endif

		if (p->p_md.md_ss_addr != va || instr != MIPS_BREAK_SSTEP) {
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
				rv = suiword((caddr_t)va, MIPS_BREAK_SSTEP);
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
		}
#endif
		MachFlushCache();

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
		if ((cause & MIPS_CR_COP_ERR) != 0x10000000) {
			sig = SIGILL;	/* only FPU instructions allowed */
			break; /* SIGNAL */
			}
		switchfpregs(fpcurproc, p);
		fpcurproc = p;
		p->p_md.md_regs[PS] |= MIPS_SR_COP_1_BIT;
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
	if (mask & MIPS_INT_MASK_5) {
		intrcnt[FPU_INTR]++;
		if (USERMODE(status))
			MachFPInterrupt(status, cause, pc, frame->f_regs);
		else {
			printf("FPU interrupt: PC %x CR %x SR %x\n",
				pc, cause, status);
		}
	}

	/* Network software interrupt */
	if ((mask & MIPS_SOFT_INT_MASK_1)
	    || (netisr && (status & MIPS_SOFT_INT_MASK_1))) {
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
	if (mask & MIPS_SOFT_INT_MASK_0) {
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
	int cause;

	s = splhigh();
	printf("trapDump(%s)\n", msg);
	for (i = 0; i < TRAPSIZE; i++) {
		if (trp == trapdebug)
			trp = &trapdebug[TRAPSIZE - 1];
		else
			trp--;
		if (trp->cause == 0)
			break;
		cause = (trp->cause & ((CPUISMIPS3) ?
		    MIPS3_CR_EXC_CODE : MIPS1_CR_EXC_CODE));
		printf("%s: ADR %x PC %x CR %x SR %x\n",
			trap_type[cause >> MIPS_CR_EXC_CODE_SHIFT],
			trp->vadr, trp->pc, trp->cause, trp->status);
		printf("   RA %x SP %x code %d\n", trp->ra, trp->sp, trp->code);
	}
#ifndef DDB
	bzero(trapdebug, sizeof(trapdebug));
	trp = trapdebug;
#endif
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

	inst.word = (instPC < MIPS_KSEG0_START) ?
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
				condition = fpcCSR & MIPS_FPU_COND_BIT;
			else
				condition = !(fpcCSR & MIPS_FPU_COND_BIT);
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
        rv = suiword((caddr_t)va, MIPS_BREAK_SSTEP);
        if (rv < 0) {
		vm_offset_t sa, ea;
		sa = trunc_page((vm_offset_t)va);
                ea = round_page((vm_offset_t)va + sizeof(int) - 1);
                rv = vm_map_protect(&p->p_vmspace->vm_map,
                        sa, ea, VM_PROT_DEFAULT, FALSE);
		if (rv == KERN_SUCCESS) {
                        rv = suiword((caddr_t)va, MIPS_BREAK_SSTEP);
                        (void)vm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, FALSE);
		}
	}
#else
        struct frame *f = (struct frame *)p->p_md.md_regs;
        unsigned pc = f->f_regs[PC];
        unsigned va = 0;
        int rv;
        int curinstr, bpinstr = MIPS_BREAK_SSTEP;
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

#if defined(DEBUG) || defined(MDB) || defined(DDB)
int
kdbpeek(addr)
	vm_offset_t addr;
{
	if (addr & 3) {
		printf("kdbpeek: unaligned address %lx\n", addr);
		/* We might have been called from DDB, so don\'t go there. */
		stacktrace();
		return (-1);
	}
	return (*(int *)addr);
}

#define MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */

/* forward */
char *fn_name(unsigned addr);
void stacktrace_subr __P((int a0, int a1, int a2, int a3,
			  u_int pc, u_int sp, u_int fp, u_int ra,
			  void (*)(const char*, ...)));

/*
 * Do a stack backtrace.
 * (*printfn)()  prints the output to either the system log,
 * the console, or both.
 */
void
stacktrace_subr(a0, a1, a2, a3, pc, sp, fp, ra, printfn)
	int a0, a1, a2, a3;
	u_int  pc, sp, fp, ra;
	void (*printfn) __P((const char*, ...));
{
	unsigned va, subr;
	unsigned instr, mask;
	InstFmt i;
	int more, stksize;
	extern char start[], edata[];
	unsigned int frames =  0;
	int foundframesize = 0;

/* Jump here when done with a frame, to start a new one */
loop:
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
	else if (pcBetween(mips1_KernGenException, mips1_UserGenException)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("------ kernel trap+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips1_KernGenException, a0, a1, a2);

		a0 = kdbpeek(sp + 40);
		a1 = kdbpeek(sp + 44);
		a2 = kdbpeek(sp + 48);
		a3 = kdbpeek(sp + 52);

		pc = kdbpeek(sp + 172);	/* exc_pc - pc at time of exception */
		ra = kdbpeek(sp + 148);	/* ra at time of exception */
		sp = sp + 176;
		goto specialframe;
	}
#endif	/* MIPS1 */

#ifdef MIPS3		/* r4000 family (mips-III cpu) */
	if (pcBetween(mips3_KernIntr, mips3_UserIntr)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("------ mips3 KernIntr+%x: (%x, %x ,%x) -------\n",
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
	else if (pcBetween(mips3_KernGenException, mips3_UserGenException)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("------ kernel trap+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips3_KernGenException, a0, a1, a2);

		a0 = kdbpeek(sp + 40);
		a1 = kdbpeek(sp + 44);
		a2 = kdbpeek(sp + 48);
		a3 = kdbpeek(sp + 52);

		pc = kdbpeek(sp + 172);	/* exc_pc - pc at time of exception */
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
	else if (pcBetween(bcopy, setrunqueue))	{
		subr = (unsigned) bcopy;
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
	foundframesize = 0;
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
			/* don't count pops for mcount */
			if (!foundframesize) {
				stksize = - ((short)i.IType.imm);
				foundframesize = 1;
			}				
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

#ifdef MIPS3
/*
 * Dump TLB contents on mips3 CPU.
 * called by mips3 locore after in-kernel TLB miss.
 */
void
mips3_dump_tlb(first, last, printfn)
	int first;
	int last;
	void (*printfn) __P((const char*, ...));
{
	int tlbno;
	struct tlb tlb;
	extern void mips3_TLBRead(int, struct tlb *);

	tlbno = first;

	while(tlbno <= last) {
		mips3_TLBRead(tlbno, &tlb);
		if (mips_pg_v(tlb.tlb_lo0) || mips_pg_v(tlb.tlb_lo1)) {
			(*printfn)("TLB %2d vad 0x%08x ", tlbno, tlb.tlb_hi);
		}
		else {
			(*printfn)("TLB*%2d vad 0x%08x ", tlbno, tlb.tlb_hi);
		}
		(*printfn)("0=0x%08lx ", pfn_to_vad(tlb.tlb_lo0));
		(*printfn)("%c", tlb.tlb_lo0 & mips_pg_m_bit() ? 'M' : ' ');
		(*printfn)("%c", tlb.tlb_lo0 & mips_pg_global_bit() ? 'G' : ' ');
		(*printfn)(" atr %x ", (tlb.tlb_lo0 >> 3) & 7);
		(*printfn)("1=0x%08lx ", pfn_to_vad(tlb.tlb_lo1));
		(*printfn)("%c", tlb.tlb_lo1 & mips_pg_m_bit() ? 'M' : ' ');
		(*printfn)("%c", tlb.tlb_lo1 & mips_pg_global_bit() ? 'G' : ' ');
		(*printfn)(" atr %x ", (tlb.tlb_lo1 >> 3) & 7);
		(*printfn)(" sz=%x\n", tlb.tlb_mask);

		tlbno++;
	}
}
#endif /* MIPS3 */

#ifdef MIPS1
/*
 * Dump mips1 TLB contents.
 * called by mips3 locore after in-kernel TLB miss.
 */
void
mips1_dump_tlb(first, last, printfn)
	int first;
	int last;
	void (*printfn) __P((const char*, ...));
{
	int tlbno;
	extern u_int tlbhi, tlblo;
	extern void mips1_TLBRead(int);

	tlbno = first;

	while(tlbno <= last) {
		mips1_TLBRead(tlbno);
		if (mips_pg_v(tlblo)) {
			(*printfn)("TLB %2d vad 0x%08x ", tlbno, tlbhi);
		}
		else {
			(*printfn)("TLB*%2d vad 0x%08x ", tlbno, tlbhi);
		}
		(*printfn)("0x%08x ", tlblo & MIPS1_PG_FRAME);
		(*printfn)("%c", tlblo & mips_pg_m_bit() ? 'M' : ' ');
		(*printfn)("%c", tlblo & mips_pg_global_bit() ? 'G' : ' ');
		(*printfn)("%c\n", tlblo & MIPS1_PG_N ? 'N' : ' ');

		tlbno++;
	}
}
#endif /* MIPS1 */

/*
 *  Dump TLB after panic.
 */
void
mips_dump_tlb(first, last)
	int first;
	int last;
{
	if (CPUISMIPS3) {
#ifdef MIPS3
		mips3_dump_tlb(first,last, printf);
#endif
	} else {
#ifdef MIPS1
		mips1_dump_tlb(first,last, printf);
#endif
	}
}
