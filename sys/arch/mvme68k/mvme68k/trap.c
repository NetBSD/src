/*	$NetBSD: trap.c,v 1.61 2001/09/10 21:19:19 chris Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_kgdb.h"
#include "opt_compat_sunos.h"
#include "opt_compat_hpux.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/syslog.h>
#include <sys/user.h>

#ifdef DEBUG
#include <dev/cons.h>
#endif

#include <machine/db_machdep.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/cpu.h>
#include <machine/reg.h>

#include <m68k/cacheops.h>

#include <uvm/uvm_extern.h>

#ifdef COMPAT_HPUX
#include <compat/hpux/hpux.h>
#endif

#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_syscall.h>
#include <compat/sunos/sunos_exec.h>
#endif

int	writeback __P((struct frame *fp, int docachepush));
void	trap __P((int type, u_int code, u_int v, struct frame frame));

#if defined(M68040) || defined(M68060)
#ifdef DEBUG
void	dumpssw __P((u_short));
void	dumpwb __P((int, u_short, u_int, u_int));
#endif
#endif

static inline void userret __P((struct proc *p, struct frame *fp,
	    u_quad_t oticks, u_int faultaddr, int fromtrap));

int	astpending;

char	*trap_type[] = {
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
#if defined(M68020) || defined(M68030) || defined(M68040)
#define	KDFAULT_060(c)	(cputype == CPU_68060 && ((c) & FSLW_TM_SV))
#define	WRFAULT_060(c)	(cputype == CPU_68060 && ((c) & FSLW_RW_W))
#else
#define	KDFAULT_060(c)	((c) & FSLW_TM_SV)
#define	WRFAULT_060(c)	((c) & FSLW_RW_W)
#endif
#else
#define	KDFAULT_060(c)	0
#define	WRFAULT_060(c)	0
#endif

#ifdef M68040
#if defined(M68020) || defined(M68030) || defined(M68060)
#define	KDFAULT_040(c)	(cputype == CPU_68040 && \
			 ((c) & SSW4_TMMASK) == SSW4_TMKD)
#define	WRFAULT_040(c)	(cputype == CPU_68040 && \
			 ((c) & SSW4_RW) == 0)
#else
#define	KDFAULT_040(c)	(((c) & SSW4_TMMASK) == SSW4_TMKD)
#define	WRFAULT_040(c)	(((c) & SSW4_RW) == 0)
#endif
#else
#define	KDFAULT_040(c)	0
#define	WRFAULT_040(c)	0
#endif

#if defined(M68030) || defined(M68020)
#if defined(M68040) || defined(M68060)
#define	KDFAULT_OTH(c)	(cputype <= CPU_68030 && \
			 ((c) & (SSW_DF|SSW_FCMASK)) == (SSW_DF|FC_SUPERD))
#define	WRFAULT_OTH(c)	(cputype <= CPU_68030 && \
			 ((c) & (SSW_DF|SSW_RW)) == SSW_DF)
#else
#define	KDFAULT_OTH(c)	(((c) & (SSW_DF|SSW_FCMASK)) == (SSW_DF|FC_SUPERD))
#define	WRFAULT_OTH(c)	(((c) & (SSW_DF|SSW_RW)) == SSW_DF)
#endif
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
userret(p, fp, oticks, faultaddr, fromtrap)
	struct proc *p;
	struct frame *fp;
	u_quad_t oticks;
	u_int faultaddr;
	int fromtrap;
{
	int sig;
#ifdef M68040
	int beenhere = 0;

again:
#endif
	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		postsig(sig);
	p->p_priority = p->p_usrpri;
	if (want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(NULL);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, fp->f_pc,
			    (int)(p->p_sticks - oticks) * psratio);
	}
#ifdef M68040
	/*
	 * Deal with user mode writebacks (from trap, or from sigreturn).
	 * If any writeback fails, go back and attempt signal delivery.
	 * unless we have already been here and attempted the writeback
	 * (e.g. bad address with user ignoring SIGSEGV).  In that case
	 * we just return to the user without sucessfully completing
	 * the writebacks.  Maybe we should just drop the sucker?
	 */
	if (
#if defined(M68030) || defined(M68060)
	    cputype == CPU_68040 &&
#endif
	    fp->f_format == FMT7) {
		if (beenhere) {
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(fromtrap ?
		"pid %d(%s): writeback aborted, pc=%x, fa=%x\n" :
		"pid %d(%s): writeback aborted in sigreturn, pc=%x\n",
				    p->p_pid, p->p_comm, fp->f_pc, faultaddr);
#endif
		} else if ((sig = writeback(fp, fromtrap))) {
			beenhere = 1;
			oticks = p->p_sticks;
			trapsignal(p, sig, faultaddr);
			goto again;
		}
	}
#endif
	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
}

/*
 * Used by the common m68k syscall() and child_return() functions.
 * XXX: Temporary until all m68k ports share common trap()/userret() code.
 */
void machine_userret(struct proc *, struct frame *, u_quad_t);

void
machine_userret(p, f, t)
	struct proc *p;
	struct frame *f;
	u_quad_t t;
{

	userret(p, f, t, 0, 0);
}

/*
 * Trap is called from locore to handle most types of processor traps,
 * including events such as simulated software interrupts/AST's.
 * System calls are broken out for efficiency.
 */
/*ARGSUSED*/
void
trap(type, code, v, frame)
	int type;
	unsigned code;
	unsigned v;
	struct frame frame;
{
	extern char fubail[], subail[];
	struct proc *p;
	int i, s;
	u_int ucode;
	u_quad_t sticks = 0 /* XXX initialiser works around compiler bug */;
	static int panicing = 0;

	uvmexp.traps++;
	p = curproc;
	ucode = 0;

	/* I have verified that this DOES happen! -gwr */
	if (p == NULL)
		p = &proc0;
#ifdef DIAGNOSTIC
	if (p->p_addr == NULL)
		panic("trap: no pcb");
#endif

	if (USERMODE(frame.f_sr)) {
		type |= T_USER;
		sticks = p->p_sticks;
		p->p_md.md_regs = frame.f_regs;
	}
	switch (type) {

	default:
	dopanic:
		/*
		 * Let the kernel debugger see the trap frame that
		 * caused us to panic.  This is a convenience so
		 * one can see registers at the point of failure.
		 */
		s = splhigh();
		panicing = 1;
		printf("trap type %d, code = 0x%x, v = 0x%x\n", type, code, v);
		printf("%s program counter = 0x%x\n",
		    (type & T_USER) ? "user" : "kernel", frame.f_pc);
#ifdef KGDB
		/* If connected, step or cont returns 1 */
		if (kgdb_trap(type, &frame))
			goto kgdb_cont;
#endif
#ifdef DDB
		(void)kdb_trap(type, (db_regs_t *)&frame);
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
		regdump((struct trapframe *)&frame, 128);
		type &= ~T_USER;
		if ((u_int)type < trap_types)
			panic(trap_type[type]);
		panic("trap");

	case T_BUSERR:		/* kernel bus error */
		if (p->p_addr->u_pcb.pcb_onfault == 0)
			goto dopanic;
		/* FALLTHROUGH */

	copyfault:
		/*
		 * If we have arranged to catch this fault in any of the
		 * copy to/from user space routines, set PC to return to
		 * indicated location and set flag informing buserror code
		 * that it may need to clean up stack frame.
		 */
		frame.f_stackadj = exframesize[frame.f_format];
		frame.f_format = frame.f_vector = 0;
		frame.f_pc = (int) p->p_addr->u_pcb.pcb_onfault;
		return;

	case T_BUSERR|T_USER:	/* bus error */
	case T_ADDRERR|T_USER:	/* address error */
		ucode = v;
		i = SIGBUS;
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
		SIGACTION(p, SIGILL).sa_handler = SIG_DFL;
		sigdelset(&p->p_sigctx.ps_sigignore, SIGILL);
		sigdelset(&p->p_sigctx.ps_sigcatch, SIGILL);
		sigdelset(&p->p_sigctx.ps_sigmask, SIGILL);
		i = SIGILL;
		ucode = frame.f_format;	/* XXX was ILL_RESAD_FAULT */
		break;

	case T_COPERR|T_USER:	/* user coprocessor violation */
	/* What is a proper response here? */
		ucode = 0;
		i = SIGFPE;
		break;

	case T_FPERR|T_USER:	/* 68881 exceptions */
	/*
	 * We pass along the 68881 status register which locore stashed
	 * in code for us.  Note that there is a possibility that the
	 * bit pattern of this register will conflict with one of the
	 * FPE_* codes defined in signal.h.  Fortunately for us, the
	 * only such codes we use are all in the range 1-7 and the low
	 * 3 bits of the status register are defined as 0 so there is
	 * no clash.
	 */
		ucode = code;
		i = SIGFPE;
		break;

	/*
	 * FPU faults in supervisor mode.
	 */
	case T_ILLINST:	/* fnop generates this, apparently. */
	case T_FPEMULI:
	case T_FPEMULD:
	{
		extern label_t *nofault;

		if (nofault)	/* If we're probing. */
			longjmp(nofault);
		if (type == T_ILLINST)
			printf("Kernel Illegal Instruction trap.\n");
		else
			printf("Kernel FPU trap.\n");
		goto dopanic;
	}

	case T_FPEMULI|T_USER:	/* unimplemented FP instuction */
	case T_FPEMULD|T_USER:	/* unimplemented FP data type */
#if defined(M68040) || defined(M68060)
		/* XXX need to FSAVE */
		printf("pid %d(%s): unimplemented FP %s at %x (EA %x)\n",
		       p->p_pid, p->p_comm,
		       frame.f_format == 2 ? "instruction" : "data type",
		       frame.f_pc, frame.f_fmt2.f_iaddr);
		/* XXX need to FRESTORE */
		i = SIGFPE;
		break;
#endif

	case T_ILLINST|T_USER:	/* illegal instruction fault */
#ifdef COMPAT_HPUX
		if (p->p_emul == &emul_hpux) {
			ucode = HPUX_ILL_ILLINST_TRAP;
			i = SIGILL;
			break;
		}
		/* fall through */
#endif
	case T_PRIVINST|T_USER:	/* privileged instruction fault */
#ifdef COMPAT_HPUX
		if (p->p_emul == &emul_hpux)
			ucode = HPUX_ILL_PRIV_TRAP;
		else
#endif
		ucode = frame.f_format;	/* XXX was ILL_PRIVIN_FAULT */
		i = SIGILL;
		break;

	case T_ZERODIV|T_USER:	/* Divide by zero */
#ifdef COMPAT_HPUX
		if (p->p_emul == &emul_hpux)
			ucode = HPUX_FPE_INTDIV_TRAP;
		else
#endif
		ucode = frame.f_format;	/* XXX was FPE_INTDIV_TRAP */
		i = SIGFPE;
		break;

	case T_CHKINST|T_USER:	/* CHK instruction trap */
#ifdef COMPAT_HPUX
		if (p->p_emul == &emul_hpux) {
			/* handled differently under hp-ux */
			i = SIGILL;
			ucode = HPUX_ILL_CHK_TRAP;
			break;
		}
#endif
		ucode = frame.f_format;	/* XXX was FPE_SUBRNG_TRAP */
		i = SIGFPE;
		break;

	case T_TRAPVINST|T_USER:	/* TRAPV instruction trap */
#ifdef COMPAT_HPUX
		if (p->p_emul == &emul_hpux) {
			/* handled differently under hp-ux */
			i = SIGILL;
			ucode = HPUX_ILL_TRAPV_TRAP;
			break;
		}
#endif
		ucode = frame.f_format;	/* XXX was FPE_INTOVF_TRAP */
		i = SIGFPE;
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
		printf("program counter = 0x%x\n", frame.f_pc);
#endif
		frame.f_sr &= ~PSL_T;
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
		frame.f_sr &= ~PSL_T;
		i = SIGTRAP;
		break;

	case T_ASTFLT:		/* system async trap, cannot happen */
		goto dopanic;

	case T_ASTFLT|T_USER:	/* user async trap */
		astpending = 0;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		goto out;

	case T_MMUFLT:		/* kernel mode page fault */
		/*
		 * If we were doing profiling ticks or other user mode
		 * stuff from interrupt code, Just Say No.
		 */
		if (p->p_addr->u_pcb.pcb_onfault == fubail ||
		    p->p_addr->u_pcb.pcb_onfault == subail)
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
		       p->p_pid, code, v, frame.f_pc, frame.f_sr);
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
		    ((p->p_addr->u_pcb.pcb_onfault == 0) || KDFAULT(code)))
			map = kernel_map;
		else
			map = vm ? &vm->vm_map : kernel_map;

		if (WRFAULT(code))
			ftype = VM_PROT_READ | VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

		va = trunc_page((vaddr_t)v);

		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel %s access at 0x%x\n",
			    (ftype & VM_PROT_WRITE) ? "read/write" :
			    "read", v);
			goto dopanic;
		}

#ifdef DIAGNOSTIC
		if (interrupt_depth && !panicing) {
			printf("trap: calling uvm_fault() from interrupt!\n");
			goto dopanic;
		}
#endif

#ifdef COMPAT_HPUX
		if (ISHPMMADDR(va)) {
			int pmap_mapmulti __P((pmap_t, vaddr_t));
			vaddr_t bva;

			rv = pmap_mapmulti(map->pmap, va);
			if (rv != 0) {
				bva = HPMMBASEADDR(va);
				rv = uvm_fault(map, bva, 0, ftype);
				if (rv == 0)
					(void) pmap_mapmulti(map->pmap, va);
			}
		} else
#endif
		rv = uvm_fault(map, va, 0, ftype);
#ifdef DEBUG
		if (rv && MDB_ISPID(p->p_pid))
			printf("uvm_fault(%p, 0x%lx, 0, 0x%x) -> 0x%x\n",
			    map, va, ftype, rv);
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((vm != NULL && (caddr_t)va >= vm->vm_maxsaddr)
		    && map != kernel_map) {
			if (rv == 0) {
				unsigned nss;

				nss = btoc(USRSTACK-(unsigned)va);
				if (nss > vm->vm_ssize)
					vm->vm_ssize = nss;
			} else if (rv == EACCES)
				rv = EFAULT;
		}
		if (rv == 0) {
			if (type == T_MMUFLT) {
#ifdef M68040
#if defined(M68030) || defined(M68060)
				if (cputype == CPU_68040)
#endif
					(void) writeback(&frame, 1);
#endif
				return;
			}
			goto out;
		}
		if (type == T_MMUFLT) {
			if (p->p_addr->u_pcb.pcb_onfault)
				goto copyfault;
			printf("uvm_fault(%p, 0x%lx, 0, 0x%x) -> 0x%x\n",
			    map, va, ftype, rv);
			printf("  type %x, code [mmu,,ssw]: %x\n",
			       type, code);
			goto dopanic;
		}
		ucode = v;
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			i = SIGKILL;
		} else {
			i = SIGSEGV;
		}
		break;
	    }
	}
	trapsignal(p, i, ucode);
	if ((type & T_USER) == 0)
		return;
out:
	userret(p, &frame, sticks, v, 1);
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

char *f7sz[] = { "longword", "byte", "word", "line" };
char *f7tt[] = { "normal", "MOVE16", "AFC", "ACK" };
char *f7tm[] = { "d-push", "u-data", "u-code", "M-data",
		 "M-code", "k-data", "k-code", "RES" };
char wberrstr[] =
    "WARNING: pid %d(%s) writeback [%s] failed, pc=%x fa=%x wba=%x wbd=%x\n";
#endif

/*
 * Because calling memcpy() for 16 bytes is *way* too much overhead ...
 */
static __inline void fastcopy16(u_int *, u_int *);
static __inline void
fastcopy16(src, dst)
	u_int *src, *dst;
{
	*src++ = *dst++;
	*src++ = *dst++;
	*src++ = *dst++;
	*src = *dst;
}

int
writeback(fp, docachepush)
	struct frame *fp;
	int docachepush;
{
	struct fmt7 *f = &fp->f_fmt7;
	struct proc *p = curproc;
	int err = 0;
	u_int fa;
	caddr_t oonfault = p->p_addr->u_pcb.pcb_onfault;
	extern int suline(caddr_t, caddr_t);	/* locore.s */

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
			paddr_t pa;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(f->f_fa), VM_PROT_WRITE,
			    VM_PROT_WRITE|PMAP_WIRED);
			pmap_update(pmap_kernel());
			fa = (u_int)&vmmap[(f->f_fa & PGOFSET) & ~0xF];
			fastcopy16(&f->f_pd0, (u_int *)fa);
			(void) pmap_extract(pmap_kernel(), (vaddr_t)fa, &pa);
			DCFL_40(pa);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
				    (vaddr_t)&vmmap[NBPG]);
			pmap_update(pmap_kernel());
		} else
			printf("WARNING: pid %d(%s) uid %d: CPUSH not done\n",
			       p->p_pid, p->p_comm, p->p_ucred->cr_uid);
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
		if (KDFAULT_040(f->f_wb1s))
			fastcopy16(&f->f_pd0, (u_int *)(f->f_fa & ~0xF));
		else
			err = suline((caddr_t)(f->f_fa & ~0xF), (caddr_t)&f->f_pd0);
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
			if (KDFAULT_040(f->f_wb1s))
				*(long *)f->f_wb1a = wb1d;
			else
				err = suword((caddr_t)f->f_wb1a, wb1d);
			break;
		case SSW4_SZB:
			off = 24 - off;
			if (off)
				wb1d >>= off;
			if (KDFAULT_040(f->f_wb1s))
				*(char *)f->f_wb1a = wb1d;
			else
				err = subyte((caddr_t)f->f_wb1a, wb1d);
			break;
		case SSW4_SZW:
			off = (off + 16) % 32;
			if (off)
				wb1d = (wb1d >> (32 - off)) | (wb1d << off);
			if (KDFAULT_040(f->f_wb1s))
				*(short *)f->f_wb1a = wb1d;
			else
				err = susword((caddr_t)f->f_wb1a, wb1d);
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
			if (KDFAULT_040(f->f_wb2s))
				*(long *)f->f_wb2a = f->f_wb2d;
			else
				err = suword((caddr_t)f->f_wb2a, f->f_wb2d);
			break;
		case SSW4_SZB:
			if (KDFAULT_040(f->f_wb2s))
				*(char *)f->f_wb2a = f->f_wb2d;
			else
				err = subyte((caddr_t)f->f_wb2a, f->f_wb2d);
			break;
		case SSW4_SZW:
			if (KDFAULT_040(f->f_wb2s))
				*(short *)f->f_wb2a = f->f_wb2d;
			else
				err = susword((caddr_t)f->f_wb2a, f->f_wb2d);
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
			if (KDFAULT_040(f->f_wb3s))
				*(long *)f->f_wb3a = f->f_wb3d;
			else
				err = suword((caddr_t)f->f_wb3a, f->f_wb3d);
			break;
		case SSW4_SZB:
			if (KDFAULT_040(f->f_wb3s))
				*(char *)f->f_wb3a = f->f_wb3d;
			else
				err = subyte((caddr_t)f->f_wb3a, f->f_wb3d);
			break;
		case SSW4_SZW:
			if (KDFAULT_040(f->f_wb3s))
				*(short *)f->f_wb3a = f->f_wb3d;
			else
				err = susword((caddr_t)f->f_wb3a, f->f_wb3d);
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
	p->p_addr->u_pcb.pcb_onfault = oonfault;
	if (err)
		err = SIGSEGV;
	return (err);
}

#ifdef DEBUG
void
dumpssw(ssw)
	u_short ssw;
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

void
dumpwb(num, s, a, d)
	int num;
	u_short s;
	u_int a, d;
{
	struct proc *p = curproc;
	paddr_t pa;

	printf(" writeback #%d: VA %x, data %x, SZ=%s, TT=%s, TM=%s\n",
	       num, a, d, f7sz[(s & SSW4_SZMASK) >> 5],
	       f7tt[(s & SSW4_TTMASK) >> 3], f7tm[s & SSW4_TMMASK]);
	printf("               PA ");
	if (pmap_extract(p->p_vmspace->vm_map.pmap, (vaddr_t)a, &pa) == FALSE)
		printf("<invalid address>");
	else
		printf("%lx, current value %lx", pa, fuword((caddr_t)a));
	printf("\n");
}
#endif
#endif
