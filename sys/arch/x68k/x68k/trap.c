/*	$NetBSD: trap.c,v 1.98 2009/11/23 00:11:45 rmind Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: trap.c 1.37 92/12/20$
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/4/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: trap.c 1.37 92/12/20$
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.98 2009/11/23 00:11:45 rmind Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_execfmt.h"
#include "opt_compat_sunos.h"
#include "opt_m680x0.h"
#include "opt_fpu_emulate.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/userret.h>
#include <sys/kauth.h>

#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/db_machdep.h>
#include <m68k/cacheops.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#ifdef FPU_EMULATE
#include <m68k/fpe/fpu_emulate.h>
#endif

#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_exec.h>
#include <compat/sunos/sunos_syscall.h>
#endif

int	writeback(struct frame *, int);
void	trap(struct frame *, int, u_int, u_int);

#if defined(M68040) || defined(M68060)
#ifdef DEBUG
static void dumpssw(u_short);
static void dumpwb(int, u_short, u_int, u_int);
#endif
#endif

static inline void userret(struct lwp *, struct frame *, u_quad_t, u_int, int);

int	astpending;

const char *trap_type[] = {
	"Bus error",
	"Address error",
	"Illegal instruction",
	"Zero divide",
	"CHK instruction",
	"TRAPV instruction",
	"Privilege violation",
	"Trace trap",
	"MMU fault",
	"SSIR trap",
	"Format error",
	"68881 exception",
	"Coprocessor violation",
	"Async system trap"
};
int	trap_types = sizeof trap_type / sizeof trap_type[0];

/*
 * Size of various exception stack frames (minus the standard 8 bytes)
 */
short	exframesize[] = {
	FMT0SIZE,	/* type 0 - normal (68020/030/040/060) */
	FMT1SIZE,	/* type 1 - throwaway (68020/030/040) */
	FMT2SIZE,	/* type 2 - normal 6-word (68020/030/040/060) */
	FMT3SIZE,	/* type 3 - FP post-instruction (68040/060) */
	FMT4SIZE,	/* type 4 - access error/fp disabled (68060) */
	-1, -1,		/* type 5-6 - undefined */
	FMT7SIZE,	/* type 7 - access error (68040) */
	58,		/* type 8 - bus fault (68010) */
	FMT9SIZE,	/* type 9 - coprocessor mid-instruction (68020/030) */
	FMTASIZE,	/* type A - short bus fault (68020/030) */
	FMTBSIZE,	/* type B - long bus fault (68020/030) */
	-1, -1, -1, -1	/* type C-F - undefined */
};

#ifdef M68060
#define	KDFAULT_060(c)	(cputype == CPU_68060 && ((c) & FSLW_TM_SV))
#define	WRFAULT_060(c)	(cputype == CPU_68060 && ((c) & FSLW_RW_W))
#else
#define	KDFAULT_060(c)	0
#define	WRFAULT_060(c)	0
#endif

#ifdef M68040
#define	KDFAULT_040(c)	(cputype == CPU_68040 && \
			 ((c) & SSW4_TMMASK) == SSW4_TMKD)
#define	WRFAULT_040(c)	(cputype == CPU_68040 && \
			 ((c) & (SSW4_LK|SSW4_RW)) != SSW4_RW)
#else
#define	KDFAULT_040(c)	0
#define	WRFAULT_040(c)	0
#endif

#if defined(M68030) || defined(M68020)
#define	KDFAULT_OTH(c)	(cputype <= CPU_68030 && \
			 ((c) & (SSW_DF|SSW_FCMASK)) == (SSW_DF|FC_SUPERD))
#define	WRFAULT_OTH(c)	(cputype <= CPU_68030 && \
			 (((c) & SSW_DF) != 0 && \
			 ((((c) & SSW_RW) == 0) || (((c) & SSW_RM) != 0))))
#else
#define	KDFAULT_OTH(c)	0
#define	WRFAULT_OTH(c)	0
#endif

#define	KDFAULT(c)	(KDFAULT_060(c) || KDFAULT_040(c) || KDFAULT_OTH(c))
#define	WRFAULT(c)	(WRFAULT_060(c) || WRFAULT_040(c) || WRFAULT_OTH(c))

#ifdef DEBUG
int mmudebug = 0;
int mmupid = -1;
#define MDB_FOLLOW	1
#define MDB_WBFOLLOW	2
#define MDB_WBFAILED	4
#define MDB_ISPID(p)	((p) == mmupid)
#endif

/*
 * trap and syscall both need the following work done before returning
 * to user mode.
 */
static inline void
userret(struct lwp *l, struct frame *fp, u_quad_t oticks, u_int faultaddr,
    int fromtrap)
{
	struct proc *p = l->l_proc;
#ifdef M68040
	int sig;
	int beenhere = 0;

again:
#endif
	/* Invoke MI userret code */
	mi_userret(l);

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_stflag & PST_PROFIL) {
		extern int psratio;

		addupc_task(l, fp->f_pc,
			    (int)(p->p_sticks - oticks) * psratio);
	}
#ifdef M68040
	/*
	 * Deal with user mode writebacks (from trap, or from sigreturn).
	 * If any writeback fails, go back and attempt signal delivery.
	 * unless we have already been here and attempted the writeback
	 * (e.g. bad address with user ignoring SIGSEGV).  In that case
	 * we just return to the user without successfully completing
	 * the writebacks.  Maybe we should just drop the sucker?
	 */
	if (cputype == CPU_68040 && fp->f_format == FMT7) {
		if (beenhere) {
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(fromtrap ?
		"pid %d(%s): writeback aborted, pc=%x, fa=%x\n" :
		"pid %d(%s): writeback aborted in sigreturn, pc=%x\n",
				    p->p_pid, p->p_comm, fp->f_pc, faultaddr);
#endif
		} else if ((sig = writeback(fp, fromtrap))) {
			ksiginfo_t ksi;
			beenhere = 1;
			oticks = p->p_sticks;
			(void)memset(&ksi, 0, sizeof(ksi));
			ksi.ksi_signo = sig;
			ksi.ksi_addr = (void *)faultaddr;
			ksi.ksi_code = BUS_OBJERR;
			trapsignal(l, &ksi);
			goto again;
		}
	}
#endif
}

/*
 * Used by the common m68k syscall() and child_return() functions.
 * XXX: Temporary until all m68k ports share common trap()/userret() code.
 */
void machine_userret(struct lwp *, struct frame *, u_quad_t);

void
machine_userret(struct lwp *l, struct frame *f, u_quad_t t)
{

	userret(l, f, t, 0, 0);
}

/*
 * Trap is called from locore to handle most types of processor traps,
 * including events such as simulated software interrupts/AST's.
 * System calls are broken out for efficiency.
 */
/*ARGSUSED*/
void
trap(struct frame *fp, int type, unsigned code, unsigned v)
{
	extern char fubail[], subail[];
	struct lwp *l;
	struct proc *p;
	struct pcb *pcb;
	ksiginfo_t ksi;
	int s;
	u_quad_t sticks = 0 /* XXX initializer works around compiler bug */;

	uvmexp.traps++;
	l = curlwp;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_trap = type & ~T_USER;

	p = l->l_proc;
	pcb = lwp_getpcb(l);
	KASSERT(pcb != NULL);

	if (USERMODE(fp->f_sr)) {
		type |= T_USER;
		sticks = p->p_sticks;
		l->l_md.md_regs = fp->f_regs;
		LWP_CACHE_CREDS(l, p);
	}
	switch (type) {

	default:
	dopanic:
		printf("trap type %d, code = 0x%x, v = 0x%x\n", type, code, v);
		printf("%s program counter = 0x%x\n",
		    (type & T_USER) ? "user" : "kernel", fp->f_pc);
		/*
		 * Let the kernel debugger see the trap frame that
		 * caused us to panic.  This is a convenience so
		 * one can see registers at the point of failure.
		 */
		s = splhigh();
#ifdef KGDB
		/* If connected, step or cont returns 1 */
		if (kgdb_trap(type, fp))
			goto kgdb_cont;
#endif
#ifdef DDB
		(void)kdb_trap(type, (db_regs_t *)fp);
#endif
#ifdef KGDB
	kgdb_cont:
#endif
		splx(s);
		if (panicstr) {
			printf("trap during panic!\n");
#ifdef DEBUG
			/* XXX should be a machine-dependent hook */
			printf("(press a key)\n"); (void)cngetc();
#endif
		}
		regdump((struct trapframe *)fp, 128);
		type &= ~T_USER;
		if ((u_int)type < trap_types)
			panic(trap_type[type]);
		panic("trap");

	case T_BUSERR:		/* kernel bus error */
		if (pcb->pcb_onfault == 0)
			goto dopanic;
		/* FALLTHROUGH */

	copyfault:
		/*
		 * If we have arranged to catch this fault in any of the
		 * copy to/from user space routines, set PC to return to
		 * indicated location and set flag informing buserror code
		 * that it may need to clean up stack frame.
		 */
		fp->f_stackadj = exframesize[fp->f_format];
		fp->f_format = fp->f_vector = 0;
		fp->f_pc = (int)pcb->pcb_onfault;
		return;

	case T_BUSERR|T_USER:	/* bus error */
	case T_ADDRERR|T_USER:	/* address error */
		ksi.ksi_addr = (void *)v;
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_code = (type == (T_BUSERR|T_USER)) ?
			BUS_OBJERR : BUS_ADRERR;
		break;

	case T_COPERR:		/* kernel coprocessor violation */
	case T_FMTERR|T_USER:	/* do all RTE errors come in as T_USER? */
	case T_FMTERR:		/* ...just in case... */
	/*
	 * The user has most likely trashed the RTE or FP state info
	 * in the stack frame of a signal handler.
	 */
		printf("pid %d: kernel %s exception\n", p->p_pid,
		       type==T_COPERR ? "coprocessor" : "format");
		type |= T_USER;

		mutex_enter(p->p_lock);
		SIGACTION(p, SIGILL).sa_handler = SIG_DFL;
		sigdelset(&p->p_sigctx.ps_sigignore, SIGILL);
		sigdelset(&p->p_sigctx.ps_sigcatch, SIGILL);
		sigdelset(&l->l_sigmask, SIGILL);
		mutex_exit(p->p_lock);

		ksi.ksi_signo = SIGILL;
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was ILL_RESAD_FAULT */
		ksi.ksi_code = (type == T_COPERR) ?
			ILL_COPROC : ILL_ILLOPC;
		break;

	case T_COPERR|T_USER:	/* user coprocessor violation */
	/* What is a proper response here? */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = FPE_FLTINV;
		break;

	case T_FPERR|T_USER:	/* 68881 exceptions */
	/*
	 * We pass along the 68881 status which locore stashed
	 * in code for us.
	 */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = fpsr2siginfocode(code);
		break;

	/*
	 * FPU faults in supervisor mode.
	 */
	case T_ILLINST:	/* fnop generates this, apparently. */
	case T_FPEMULI:
	case T_FPEMULD: {
		extern label_t *nofault;

		if (nofault)	/* If we're probing. */
			longjmp(nofault);
		if (type == T_ILLINST)
			printf("Kernel Illegal Instruction trap.\n");
		else
			printf("Kernel FPU trap.\n");
		goto dopanic;
	}

	/*
	 * Unimplemented FPU instructions/datatypes.
	 */
	case T_FPEMULI|T_USER:	/* unimplemented FP instruction */
	case T_FPEMULD|T_USER:	/* unimplemented FP data type */
#ifdef FPU_EMULATE
		if (fpu_emulate(fp, &pcb->pcb_fpregs, &ksi) == 0)
			; /* XXX - Deal with tracing? (fp->f_sr & PSL_T) */
		break;
#elif defined(M68040) || defined(M68060)
		/* XXX need to FSAVE */
		printf("pid %d(%s): unimplemented FP %s at %x (EA %x)\n",
		       p->p_pid, p->p_comm,
		       fp->f_format == 2 ? "instruction" : "data type",
		       fp->f_pc, fp->f_fmt2.f_iaddr);
		/* XXX need to FRESTORE */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = FPE_FLTINV;
		break;
#endif

	case T_ILLINST|T_USER:	/* illegal instruction fault */
	case T_PRIVINST|T_USER:	/* privileged instruction fault */
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was ILL_PRIVIN_FAULT */
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = (type == (T_PRIVINST|T_USER)) ?
			ILL_PRVOPC : ILL_ILLOPC;
		break;

	case T_ZERODIV|T_USER:	/* Divide by zero */
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was FPE_INTDIV_TRAP */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = FPE_FLTDIV;
		break;

	case T_CHKINST|T_USER:	/* CHK instruction trap */
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was FPE_SUBRNG_TRAP */
		ksi.ksi_signo = SIGFPE;
		break;

	case T_TRAPVINST|T_USER:	/* TRAPV instruction trap */
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was FPE_INTOVF_TRAP */
		ksi.ksi_signo = SIGFPE;
		break;

	/*
	 * XXX: Trace traps are a nightmare.
	 *
	 *	HP-UX uses trap #1 for breakpoints,
	 *	NetBSD/m68k uses trap #2,
	 *	SUN 3.x uses trap #15,
	 *	DDB and KGDB uses trap #15 (for kernel breakpoints;
	 *	handled elsewhere).
	 *
	 * NetBSD and HP-UX traps both get mapped by locore.s into T_TRACE.
	 * SUN 3.x traps get passed through as T_TRAP15 and are not really
	 * supported yet.
	 *
	 * XXX: We should never get kernel-mode T_TRAP15
	 * XXX: because locore.s now gives them special treatment.
	 */
	case T_TRAP15:		/* kernel breakpoint */
#ifdef DEBUG
		printf("unexpected kernel trace trap, type = %d\n", type);
		printf("program counter = 0x%x\n", fp->f_pc);
#endif
		fp->f_sr &= ~PSL_T;
		return;

	case T_TRACE|T_USER:	/* user trace trap */
#ifdef COMPAT_SUNOS
		/*
		 * SunOS uses Trap #2 for a "CPU cache flush".
		 * Just flush the on-chip caches and return.
		 */
		if (p->p_emul == &emul_sunos) {
			ICIA();
			DCIU();
			return;
		}
#endif
		/* FALLTHROUGH */
	case T_TRACE:		/* tracing a trap instruction */
	case T_TRAP15|T_USER:	/* SUN user trace trap */
		fp->f_sr &= ~PSL_T;
		ksi.ksi_signo = SIGTRAP;
		break;

	case T_ASTFLT:		/* system async trap, cannot happen */
		goto dopanic;

	case T_ASTFLT|T_USER:	/* user async trap */
		astpending = 0;
		/*
		 * We check for software interrupts first.  This is because
		 * they are at a higher level than ASTs, and on a VAX would
		 * interrupt the AST.  We assume that if we are processing
		 * an AST that we must be at IPL0 so we don't bother to
		 * check.  Note that we ensure that we are at least at SIR
		 * IPL while processing the SIR.
		 */
		spl1();
		/* fall into... */

	case T_SSIR:		/* software interrupt */
	case T_SSIR|T_USER:
		/*
		 * If this was not an AST trap, we are all done.
		 */
		if (type != (T_ASTFLT|T_USER)) {
			uvmexp.traps--;
			return;
		}
		spl0();
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}
		if (curcpu()->ci_want_resched)
			preempt();
		goto out;

	case T_MMUFLT:		/* kernel mode page fault */
		/*
		 * If we were doing profiling ticks or other user mode
		 * stuff from interrupt code, Just Say No.
		 */
		if (pcb->pcb_onfault == fubail || pcb->pcb_onfault == subail)
			goto copyfault;
		/* fall into ... */

	case T_MMUFLT|T_USER:	/* page fault */
	    {
		vaddr_t va;
		struct vmspace *vm = p->p_vmspace;
		struct vm_map *map;
		int rv;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
		printf("trap: T_MMUFLT pid=%d, code=%x, v=%x, pc=%x, sr=%x\n",
		       p->p_pid, code, v, fp->f_pc, fp->f_sr);
#endif
		/*
		 * It is only a kernel address space fault iff:
		 * 	1. (type & T_USER) == 0  and
		 * 	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space data fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if ((type & T_USER) == 0 &&
		    ((pcb->pcb_onfault == 0) || KDFAULT(code)))
			map = kernel_map;
		else {
			map = vm ? &vm->vm_map : kernel_map;
			if ((l->l_flag & LW_SA)
			    && (~l->l_pflag & LP_SA_NOBLOCK)) {
				l->l_savp->savp_faultaddr = (vaddr_t)v;
				l->l_pflag |= LP_SA_PAGEFAULT;
			}
		}

		if (WRFAULT(code))
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

		va = trunc_page((vaddr_t)v);

		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel %s access at 0x%x\n",
			    (ftype & VM_PROT_WRITE) ? "read/write" :
			    "read", v);
			goto dopanic;
		}

		rv = uvm_fault(map, va, ftype);
#ifdef DEBUG
		if (rv && MDB_ISPID(p->p_pid))
			printf("uvm_fault(%p, 0x%lx, 0x%x) -> 0x%x\n",
			    map, va, ftype, rv);
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if (rv == 0) {
			if (map != kernel_map && (void *)va >= vm->vm_maxsaddr)
				uvm_grow(p, va);

			if (type == T_MMUFLT) {
#ifdef M68040
				if (cputype == CPU_68040)
					(void) writeback(fp, 1);
#endif
				return;
			}
			l->l_pflag &= ~LP_SA_PAGEFAULT;
			goto out;
		}
		if (rv == EACCES) {
			ksi.ksi_code = SEGV_ACCERR;
			rv = EFAULT;
		} else
			ksi.ksi_code = SEGV_MAPERR;
		if (type == T_MMUFLT) {
			if (pcb->pcb_onfault)
				goto copyfault;
			printf("uvm_fault(%p, 0x%lx, 0x%x) -> 0x%x\n",
			    map, va, ftype, rv);
			printf("  type %x, code [mmu,,ssw]: %x\n",
			       type, code);
			goto dopanic;
		}
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		ksi.ksi_addr = (void *)v;
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       l->l_cred ?
			       kauth_cred_geteuid(l->l_cred) : -1);
			ksi.ksi_signo = SIGKILL;
		} else {
			ksi.ksi_signo = SIGSEGV;
		}
		break;
	    }
	}
	if (ksi.ksi_signo)
		trapsignal(l, &ksi);
	if ((type & T_USER) == 0)
		return;
out:
	userret(l, fp, sticks, v, 1);
}

#ifdef M68040
#ifdef DEBUG
struct writebackstats {
	int calls;
	int cpushes;
	int move16s;
	int wb1s, wb2s, wb3s;
	int wbsize[4];
} wbstats;

const char *f7sz[] = { "longword", "byte", "word", "line" };
const char *f7tt[] = { "normal", "MOVE16", "AFC", "ACK" };
const char *f7tm[] = { "d-push", "u-data", "u-code", "M-data",
		 "M-code", "k-data", "k-code", "RES" };
const char wberrstr[] =
    "WARNING: pid %d(%s) writeback [%s] failed, pc=%x fa=%x wba=%x wbd=%x\n";
#endif

int
writeback(struct frame *fp, int docachepush)
{
	struct fmt7 *f = &fp->f_fmt7;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pcb *pcb = lwp_getpcb(l);
	int err = 0;
	u_int fa;
	void *oonfault = pcb->pcb_onfault;
	paddr_t pa;

#ifdef DEBUG
	if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid)) {
		printf(" pid=%d, fa=%x,", p->p_pid, f->f_fa);
		dumpssw(f->f_ssw);
	}
	wbstats.calls++;
#endif
	/*
	 * Deal with special cases first.
	 */
	if ((f->f_ssw & SSW4_TMMASK) == SSW4_TMDCP) {
		/*
		 * Dcache push fault.
		 * Line-align the address and write out the push data to
		 * the indicated physical address.
		 */
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid)) {
			printf(" pushing %s to PA %x, data %x",
			       f7sz[(f->f_ssw & SSW4_SZMASK) >> 5],
			       f->f_fa, f->f_pd0);
			if ((f->f_ssw & SSW4_SZMASK) == SSW4_SZLN)
				printf("/%x/%x/%x",
				       f->f_pd1, f->f_pd2, f->f_pd3);
			printf("\n");
		}
		if (f->f_wb1s & SSW4_WBSV)
			panic("writeback: cache push with WB1S valid");
		wbstats.cpushes++;
#endif
		/*
		 * XXX there are security problems if we attempt to do a
		 * cache push after a signal handler has been called.
		 */
		if (docachepush) {
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(f->f_fa), VM_PROT_WRITE,
			    VM_PROT_WRITE|PMAP_WIRED);
			pmap_update(pmap_kernel());
			fa = (u_int)&vmmap[m68k_page_offset(f->f_fa) & ~0xF];
			memcpy((void *)fa, (void *)&f->f_pd0, 16);
			(void) pmap_extract(pmap_kernel(), (paddr_t)fa, &pa);
			DCFL(pa);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
				    (vaddr_t)&vmmap[PAGE_SIZE]);
			pmap_update(pmap_kernel());
		} else
			printf("WARNING: pid %d(%s) uid %d: CPUSH not done\n",
			       p->p_pid, p->p_comm, kauth_cred_geteuid(l->l_cred));
	} else if ((f->f_ssw & (SSW4_RW|SSW4_TTMASK)) == SSW4_TTM16) {
		/*
		 * MOVE16 fault.
		 * Line-align the address and write out the push data to
		 * the indicated virtual address.
		 */
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			printf(" MOVE16 to VA %x(%x), data %x/%x/%x/%x\n",
			       f->f_fa, f->f_fa & ~0xF, f->f_pd0, f->f_pd1,
			       f->f_pd2, f->f_pd3);
		if (f->f_wb1s & SSW4_WBSV)
			panic("writeback: MOVE16 with WB1S valid");
		wbstats.move16s++;
#endif
		if (KDFAULT(f->f_wb1s))
			memcpy((void *)(f->f_fa & ~0xF),
			    (void *)&f->f_pd0, 16);
		else
			err = suline((void *)(f->f_fa & ~0xF), (void *)&f->f_pd0);
		if (err) {
			fa = f->f_fa & ~0xF;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(wberrstr, p->p_pid, p->p_comm,
				       "MOVE16", fp->f_pc, f->f_fa,
				       f->f_fa & ~0xF, f->f_pd0);
#endif
		}
	} else if (f->f_wb1s & SSW4_WBSV) {
		/*
		 * Writeback #1.
		 * Position the "memory-aligned" data and write it out.
		 */
		u_int wb1d = f->f_wb1d;
		int off;

#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			dumpwb(1, f->f_wb1s, f->f_wb1a, f->f_wb1d);
		wbstats.wb1s++;
		wbstats.wbsize[(f->f_wb2s&SSW4_SZMASK)>>5]++;
#endif
		off = (f->f_wb1a & 3) * 8;
		switch (f->f_wb1s & SSW4_SZMASK) {
		case SSW4_SZLW:
			if (off)
				wb1d = (wb1d >> (32 - off)) | (wb1d << off);
			if (KDFAULT(f->f_wb1s))
				*(long *)f->f_wb1a = wb1d;
			else
				err = suword((void *)f->f_wb1a, wb1d);
			break;
		case SSW4_SZB:
			off = 24 - off;
			if (off)
				wb1d >>= off;
			if (KDFAULT(f->f_wb1s))
				*(char *)f->f_wb1a = wb1d;
			else
				err = subyte((void *)f->f_wb1a, wb1d);
			break;
		case SSW4_SZW:
			off = (off + 16) % 32;
			if (off)
				wb1d = (wb1d >> (32 - off)) | (wb1d << off);
			if (KDFAULT(f->f_wb1s))
				*(short *)f->f_wb1a = wb1d;
			else
				err = susword((void *)f->f_wb1a, wb1d);
			break;
		}
		if (err) {
			fa = f->f_wb1a;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(wberrstr, p->p_pid, p->p_comm,
				       "#1", fp->f_pc, f->f_fa,
				       f->f_wb1a, f->f_wb1d);
#endif
		}
	}
	/*
	 * Deal with the "normal" writebacks.
	 *
	 * XXX writeback2 is known to reflect a LINE size writeback after
	 * a MOVE16 was already dealt with above.  Ignore it.
	 */
	if (err == 0 && (f->f_wb2s & SSW4_WBSV) &&
	    (f->f_wb2s & SSW4_SZMASK) != SSW4_SZLN) {
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			dumpwb(2, f->f_wb2s, f->f_wb2a, f->f_wb2d);
		wbstats.wb2s++;
		wbstats.wbsize[(f->f_wb2s&SSW4_SZMASK)>>5]++;
#endif
		switch (f->f_wb2s & SSW4_SZMASK) {
		case SSW4_SZLW:
			if (KDFAULT(f->f_wb2s))
				*(long *)f->f_wb2a = f->f_wb2d;
			else
				err = suword((void *)f->f_wb2a, f->f_wb2d);
			break;
		case SSW4_SZB:
			if (KDFAULT(f->f_wb2s))
				*(char *)f->f_wb2a = f->f_wb2d;
			else
				err = subyte((void *)f->f_wb2a, f->f_wb2d);
			break;
		case SSW4_SZW:
			if (KDFAULT(f->f_wb2s))
				*(short *)f->f_wb2a = f->f_wb2d;
			else
				err = susword((void *)f->f_wb2a, f->f_wb2d);
			break;
		}
		if (err) {
			fa = f->f_wb2a;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED) {
				printf(wberrstr, p->p_pid, p->p_comm,
				       "#2", fp->f_pc, f->f_fa,
				       f->f_wb2a, f->f_wb2d);
				dumpssw(f->f_ssw);
				dumpwb(2, f->f_wb2s, f->f_wb2a, f->f_wb2d);
			}
#endif
		}
	}
	if (err == 0 && (f->f_wb3s & SSW4_WBSV)) {
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			dumpwb(3, f->f_wb3s, f->f_wb3a, f->f_wb3d);
		wbstats.wb3s++;
		wbstats.wbsize[(f->f_wb3s&SSW4_SZMASK)>>5]++;
#endif
		switch (f->f_wb3s & SSW4_SZMASK) {
		case SSW4_SZLW:
			if (KDFAULT(f->f_wb3s))
				*(long *)f->f_wb3a = f->f_wb3d;
			else
				err = suword((void *)f->f_wb3a, f->f_wb3d);
			break;
		case SSW4_SZB:
			if (KDFAULT(f->f_wb3s))
				*(char *)f->f_wb3a = f->f_wb3d;
			else
				err = subyte((void *)f->f_wb3a, f->f_wb3d);
			break;
		case SSW4_SZW:
			if (KDFAULT(f->f_wb3s))
				*(short *)f->f_wb3a = f->f_wb3d;
			else
				err = susword((void *)f->f_wb3a, f->f_wb3d);
			break;
#ifdef DEBUG
		case SSW4_SZLN:
			panic("writeback: wb3s indicates LINE write");
#endif
		}
		if (err) {
			fa = f->f_wb3a;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(wberrstr, p->p_pid, p->p_comm,
				       "#3", fp->f_pc, f->f_fa,
				       f->f_wb3a, f->f_wb3d);
#endif
		}
	}
	pcb->pcb_onfault = oonfault;
	if (err)
		err = SIGSEGV;
	return (err);
}

#ifdef DEBUG
static void
dumpssw(u_short ssw)
{
	printf(" SSW: %x: ", ssw);
	if (ssw & SSW4_CP)
		printf("CP,");
	if (ssw & SSW4_CU)
		printf("CU,");
	if (ssw & SSW4_CT)
		printf("CT,");
	if (ssw & SSW4_CM)
		printf("CM,");
	if (ssw & SSW4_MA)
		printf("MA,");
	if (ssw & SSW4_ATC)
		printf("ATC,");
	if (ssw & SSW4_LK)
		printf("LK,");
	if (ssw & SSW4_RW)
		printf("RW,");
	printf(" SZ=%s, TT=%s, TM=%s\n",
	       f7sz[(ssw & SSW4_SZMASK) >> 5],
	       f7tt[(ssw & SSW4_TTMASK) >> 3],
	       f7tm[ssw & SSW4_TMMASK]);
}

static void
dumpwb(int num, u_short s, u_int a, u_int d)
{
	struct proc *p = curproc;
	paddr_t pa;

	printf(" writeback #%d: VA %x, data %x, SZ=%s, TT=%s, TM=%s\n",
	       num, a, d, f7sz[(s & SSW4_SZMASK) >> 5],
	       f7tt[(s & SSW4_TTMASK) >> 3], f7tm[s & SSW4_TMMASK]);
	printf("               PA ");
	if (pmap_extract(p->p_vmspace->vm_map.pmap, (vaddr_t)a, &pa) == false)
		printf("<invalid address>");
	else
		printf("%lx, current value %lx", pa, fuword((void *)a));
	printf("\n");
}
#endif
#endif
