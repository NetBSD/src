/*	$NetBSD: trap.c,v 1.137 2009/11/21 04:16:53 rmind Exp $	*/

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
 *	from: Utah Hdr: trap.c 1.37 92/12/20
 *	from: @(#)trap.c	8.5 (Berkeley) 1/4/94
 */

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	from: Utah Hdr: trap.c 1.37 92/12/20
 *	from: @(#)trap.c	8.5 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.137 2009/11/21 04:16:53 rmind Exp $");

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_fpu_emulate.h"
#include "opt_kgdb.h"
#include "opt_compat_aout_m68k.h"
#include "opt_compat_sunos.h"

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
#ifdef	KGDB
#include <sys/kgdb.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/endian.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/reg.h>
#include <m68k/cacheops.h>

#include <sun3/sun3/fc.h>
#include <sun3/sun3/machdep.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_syscall.h>
extern struct emul emul_sunos;
#endif

#ifdef COMPAT_AOUT_M68K
extern struct emul emul_netbsd_aoutm68k;
#endif

/*
 * The sun3 wants faults to go through the pmap code, but
 * the sun3x just goes directly to the common VM code.
 */
#ifdef	_SUN3X_
# define _pmap_fault(map, va, ftype) \
	uvm_fault(map, va, ftype)
#endif	/* SUN3X */

/* Special labels in m68k/copy.s */
extern char fubail[], subail[];

/* These are called from locore.s */
void trap(struct trapframe *, int type, u_int code, u_int v);
void trap_kdebug(int type, struct trapframe tf);
int _nodb_trap(int type, struct trapframe *);
void straytrap(struct trapframe);

static void userret(struct lwp *, struct trapframe *, u_quad_t);

int astpending;

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
	"Async system trap",
	"Unused? (14)",
	"Breakpoint",
	"FPU instruction",
	"FPU data format",
};
u_int trap_types = sizeof(trap_type) / sizeof(trap_type[0]);

/*
 * Size of various exception stack frames (minus the standard 8 bytes)
 */
short	exframesize[] = {
	FMT0SIZE,	/* type 0 - normal (68020/030/040/060) */
	FMT1SIZE,	/* type 1 - throwaway (68020/030/040) */
	FMT2SIZE,	/* type 2 - normal 6-word (68020/030/040/060) */
	FMT3SIZE,	/* type 3 - FP post-instruction (68040/060) */
	FMT4SIZE,	/* type 4 - access error/fp disabled (68060) */
	-1, -1, 	/* type 5-6 - undefined */
	FMT7SIZE,	/* type 7 - access error (68040) */
	58,		/* type 8 - bus fault (68010) */
	FMT9SIZE,	/* type 9 - coprocessor mid-instruction (68020/030) */
	FMTASIZE,	/* type A - short bus fault (68020/030) */
	FMTBSIZE,	/* type B - long bus fault (68020/030) */
	-1, -1, -1, -1	/* type C-F - undefined */
};

#define KDFAULT(c)	(((c) & (SSW_DF|SSW_FCMASK)) == (SSW_DF|FC_SUPERD))
#define WRFAULT(c)	(((c) & SSW_DF) != 0 && \
			  ((((c) & SSW_RW) == 0) || (((c) & SSW_RM) != 0)))

/* #define	DEBUG XXX */

#ifdef DEBUG
int mmudebug = 0;
int mmupid = -1;
#define MDB_ISPID(p)	((p) == mmupid)
#define MDB_FOLLOW	1
#define MDB_WBFOLLOW	2
#define MDB_WBFAILED	4
#define MDB_CPFAULT 	8
#endif

/*
 * trap and syscall both need the following work done before
 * returning to user mode.
 */
static void 
userret(struct lwp *l, struct trapframe *tf, u_quad_t oticks)
{
	struct proc *p = l->l_proc;

	/* Invoke MI userret code */
	mi_userret(l);

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_stflag & PST_PROFIL) {
		extern int psratio;
		addupc_task(l, tf->tf_pc,
		            (int)(p->p_sticks - oticks) * psratio);
	}
}

/*
 * Used by the common m68k syscall() and child_return() functions.
 * XXX: Temporary until all m68k ports share common trap()/userret() code.
 */
void machine_userret(struct lwp *, struct frame *, u_quad_t);

void 
machine_userret(struct lwp *l, struct frame *f, u_quad_t t)
{

	userret(l, &f->F_t, t);
}

/*
 * Trap is called from locore to handle most types of processor traps,
 * including events such as simulated software interrupts/AST's.
 * System calls are broken out for efficiency.
 */
/*ARGSUSED*/
void 
trap(struct trapframe *tf, int type, u_int code, u_int v)
{
	struct lwp *l;
	struct proc *p;
	struct pcb *pcb;
	ksiginfo_t ksi;
	int tmp;
	u_quad_t sticks;
	void *onfault;

	uvmexp.traps++;
	l = curlwp;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_trap = type & ~T_USER;

	p = l->l_proc;
	pcb = lwp_getpcb(l);

	KASSERT(pcb != NULL);

	if (USERMODE(tf->tf_sr)) {
		type |= T_USER;
		sticks = p->p_sticks;
		l->l_md.md_regs = tf->tf_regs;
		LWP_CACHE_CREDS(l, p);
	} else {
		sticks = 0;
		/* XXX: Detect trap recursion? */
	}

	switch (type) {
	default:
	dopanic:
		printf("trap type=0x%x, code=0x%x, v=0x%x\n", type, code, v);
		/*
		 * Let the kernel debugger see the trap frame that
		 * caused us to panic.  This is a convenience so
		 * one can see registers at the point of failure.
		 */
		tmp = splhigh();
#ifdef KGDB
		/* If connected, step or cont returns 1 */
		if (kgdb_trap(type, tf))
			goto kgdb_cont;
#endif
#ifdef	DDB
		(void) kdb_trap(type, (db_regs_t *) tf);
#endif
#ifdef KGDB
	kgdb_cont:
#endif
		splx(tmp);
		if (panicstr) {
			/*
			 * Note: panic is smart enough to do:
			 *   boot(RB_AUTOBOOT | RB_NOSYNC, NULL)
			 * if we call it again.
			 */
			panic("trap during panic!");
		}
		regdump(tf, 128);
		type &= ~T_USER;
		if ((u_int)type < trap_types)
			panic(trap_type[type]);
		panic("trap type 0x%x", type);

	case T_BUSERR:		/* kernel bus error */
		if (pcb->pcb_onfault == NULL)
			goto dopanic;
		/*FALLTHROUGH*/

	copyfault:
		/*
		 * If we have arranged to catch this fault in any of the
		 * copy to/from user space routines, set PC to return to
		 * indicated location and set flag informing buserror code
		 * that it may need to clean up stack frame.
		 */
		tf->tf_stackadj = exframesize[tf->tf_format];
		tf->tf_format = tf->tf_vector = 0;
		tf->tf_pc = (int)pcb->pcb_onfault;
		goto done;

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
		ksi.ksi_addr = (void *)(int)tf->tf_format;
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
		 * We pass along the 68881 status register which locore stashed
		 * in code for us.
		 */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = fpsr2siginfocode(code);
		break;

	case T_FPEMULI:		/* FPU faults in supervisor mode */
	case T_FPEMULD:
		if (nofault)	/* Doing FPU probe? */
			longjmp(nofault);
		goto dopanic;

	case T_FPEMULI|T_USER:	/* unimplemented FP instruction */
	case T_FPEMULD|T_USER:	/* unimplemented FP data type */
#ifdef	FPU_EMULATE
		if (fpu_emulate(tf, &pcb->pcb_fpregs, &ksi) == 0)
			; /* XXX - Deal with tracing? (tf->tf_sr & PSL_T) */
#else
		uprintf("pid %d killed: no floating point support\n", p->p_pid);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC;
#endif
		break;

	case T_ILLINST|T_USER:	/* illegal instruction fault */
	case T_PRIVINST|T_USER:	/* privileged instruction fault */
		ksi.ksi_addr = (void *)(int)tf->tf_format;
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = (type == (T_PRIVINST|T_USER)) ?
			ILL_PRVOPC : ILL_ILLOPC;
		break;

	case T_ZERODIV|T_USER:	/* Divide by zero */
		ksi.ksi_code = FPE_FLTDIV;
	case T_CHKINST|T_USER:	/* CHK instruction trap */
	case T_TRAPVINST|T_USER:	/* TRAPV instruction trap */
		ksi.ksi_addr = (void *)(int)tf->tf_format;
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
		tf->tf_sr &= ~PSL_T;
		goto done;

	case T_TRACE|T_USER:	/* user trace trap */
#ifdef COMPAT_SUNOS
		/*
		 * SunOS uses Trap #2 for a "CPU cache flush"
		 * Just flush the on-chip caches and return.
		 * XXX - Too bad NetBSD uses trap 2...
		 */
		if (p->p_emul == &emul_sunos) {
			ICIA();
			DCIU();
			/* get out fast */
			goto done;
		}
#endif
		/* FALLTHROUGH */
	case T_TRACE:		/* tracing a trap instruction */
	case T_TRAP15|T_USER:	/* SUN user trace trap */
		tf->tf_sr &= ~PSL_T;
		ksi.ksi_signo = SIGTRAP;
		break;

	case T_ASTFLT:		/* system async trap, cannot happen */
		goto dopanic;

	case T_ASTFLT|T_USER:	/* user async trap */
		astpending = 0;
		/* T_SSIR is not used on a Sun3. */
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}
		if (curcpu()->ci_want_resched)
			preempt();
		goto douret;

	case T_MMUFLT:		/* kernel mode page fault */
		/* Hacks to avoid calling VM code from debugger. */
#ifdef	DDB
		if (db_recover != 0)
			goto dopanic;
#endif
#ifdef	KGDB
		if (kgdb_recover != 0)
			goto dopanic;
#endif
		/*
		 * If we were doing profiling ticks or other user mode
		 * stuff from interrupt code, Just Say No.
		 */
		if (pcb->pcb_onfault == (void *)fubail ||
		    pcb->pcb_onfault == (void *)subail)
		{
#ifdef	DEBUG
			if (mmudebug & MDB_CPFAULT) {
				printf("trap: copyfault fu/su bail\n");
				Debugger();
			}
#endif
			goto copyfault;
		}
		/*FALLTHROUGH*/

	case T_MMUFLT|T_USER: { 	/* page fault */
		vaddr_t va;
		struct vmspace *vm = p->p_vmspace;
		struct vm_map *map;
		int rv;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
		printf("trap: T_MMUFLT pid=%d, code=0x%x, v=0x%x, pc=0x%x, sr=0x%x\n",
		       p->p_pid, code, v, tf->tf_pc, tf->tf_sr);
#endif

		/*
		 * It is only a kernel address space fault iff:
		 * 	1. (type & T_USER) == 0  and: (2 or 3)
		 * 	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space data fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		map = &vm->vm_map;
		if ((type & T_USER) == 0) {
			/* supervisor mode fault */
			if (pcb->pcb_onfault == NULL || KDFAULT(code))
				map = kernel_map;
		} else if (l->l_flag & LW_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)v;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}

		if (WRFAULT(code))
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;
		va = m68k_trunc_page((vaddr_t)v);

		/*
		 * Need to resolve the fault.
		 *
		 * We give the pmap code a chance to resolve faults by
		 * reloading translations that it was forced to unload.
		 * This function does that, and calls vm_fault if it
		 * could not resolve the fault by reloading the MMU.
		 * This function may also, for example, disallow any
		 * faults in the kernel text segment, etc.
		 */

		onfault = pcb->pcb_onfault;
		pcb->pcb_onfault = NULL;
		rv = _pmap_fault(map, va, ftype);
		pcb->pcb_onfault = onfault;

#ifdef	DEBUG
		if (rv && MDB_ISPID(p->p_pid)) {
			printf("vm_fault(%p, 0x%lx, 0x%x) -> 0x%x\n",
			       map, va, ftype, rv);
			if (mmudebug & MDB_WBFAILED)
				Debugger();
		}
#endif	/* DEBUG */

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

			if ((type & T_USER) != 0)
				l->l_pflag &= ~LP_SA_PAGEFAULT;
			goto finish;
		}
		if (rv == EACCES) {
			ksi.ksi_code = SEGV_ACCERR;
			rv = EFAULT;
		} else
			ksi.ksi_code = SEGV_MAPERR;
		if ((type & T_USER) == 0) {
			/* supervisor mode fault */
			if (pcb->pcb_onfault) {
#ifdef	DEBUG
				if (mmudebug & MDB_CPFAULT) {
					printf("trap: copyfault pcb_onfault\n");
					Debugger();
				}
#endif
				goto copyfault;
			}
			printf("vm_fault(%p, 0x%lx, 0x%x) -> 0x%x\n",
			       map, va, ftype, rv);
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
		} /* T_MMUFLT */
	} /* switch */

finish:
	/* If trap was from supervisor mode, just return. */
	if ((type & T_USER) == 0)
		goto done;
	/* Post a signal if necessary. */
	if (ksi.ksi_signo)
		trapsignal(l, &ksi);
douret:
	userret(l, tf, sticks);

done:;
	/* XXX: Detect trap recursion? */
}

/*
 * This is used if we hit a kernel breakpoint or trace trap
 * when there is no debugger installed (or not attached).
 * Drop into the PROM temporarily...
 */
int 
_nodb_trap(int type, struct trapframe *tf)
{

	printf("\r\nKernel ");
	if ((0 <= type) && (type < trap_types))
		printf("%s", trap_type[type]);
	else
		printf("trap 0x%x", type);
	printf(", frame=%p\r\n", tf);
	printf("No debugger; doing PROM abort.\r\n");
	printf("To continue, type: c <RETURN>\r\n");
	sunmon_abort();
	/* OK then, just resume... */
	tf->tf_sr &= ~PSL_T;
	return(1);
}

/*
 * This is called by locore for supervisor-mode trace and
 * breakpoint traps.  This is separate from trap() above
 * so that breakpoints in trap() will work.
 *
 * If we have both DDB and KGDB, let KGDB see it first,
 * because KGDB will just return 0 if not connected.
 */
void 
trap_kdebug(int type, struct trapframe tf)
{

#ifdef	KGDB
	/* Let KGDB handle it (if connected) */
	if (kgdb_trap(type, &tf))
		return;
#endif
#ifdef	DDB
	/* Let DDB handle it. */
	if (kdb_trap(type, &tf))
		return;
#endif

	/* Drop into the PROM temporarily... */
	(void)_nodb_trap(type, &tf);
}

/*
 * Called by locore.s for an unexpected interrupt.
 * XXX - Almost identical to trap_kdebug...
 */
void 
straytrap(struct trapframe tf)
{
	int type = -1;

	printf("unexpected trap; vector=0x%x at pc=0x%x\n",
		tf.tf_vector, tf.tf_pc);

#ifdef	KGDB
	/* Let KGDB handle it (if connected) */
	if (kgdb_trap(type, &tf))
		return;
#endif
#ifdef	DDB
	/* Let DDB handle it. */
	if (kdb_trap(type, &tf))
		return;
#endif

	/* Drop into the PROM temporarily... */
	(void)_nodb_trap(type, &tf);
}
