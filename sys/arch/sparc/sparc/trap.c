/*	$NetBSD: trap.c,v 1.86.2.4 2001/03/12 13:29:25 bouyer Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by Harvard University.
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
 *	@(#)trap.c	8.4 (Berkeley) 9/23/93
 */

#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <uvm/uvm_extern.h>

#include <sparc/sparc/asm.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/trap.h>
#include <machine/instr.h>
#include <machine/pmap.h>

#ifdef DDB
#include <machine/db_machdep.h>
#else
#include <machine/frame.h>
#endif
#ifdef COMPAT_SVR4
#include <machine/svr4_machdep.h>
#endif
#ifdef COMPAT_SUNOS
extern struct emul emul_sunos;
#define SUNOS_MAXSADDR_SLOP (32 * 1024)
#endif

#include <sparc/fpu/fpu_extern.h>
#include <sparc/sparc/memreg.h>
#include <sparc/sparc/cpuvar.h>

#ifdef DEBUG
int	rwindow_debug = 0;
#endif

/*
 * Initial FPU state is all registers == all 1s, everything else == all 0s.
 * This makes every floating point register a signalling NaN, with sign bit
 * set, no matter how it is interpreted.  Appendix N of the Sparc V8 document
 * seems to imply that we should do this, and it does make sense.
 */
struct	fpstate initfpstate = {
	{ ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
	  ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0 }
};

/*
 * There are more than 100 trap types, but most are unused.
 *
 * Trap type 0 is taken over as an `Asynchronous System Trap'.
 * This is left-over Vax emulation crap that should be fixed.
 */
static const char T[] = "trap";
const char *trap_type[] = {
	/* non-user vectors */
	"ast",			/* 0 */
	"text fault",		/* 1 */
	"illegal instruction",	/* 2 */
	"privileged instruction",/*3 */
	"fp disabled",		/* 4 */
	"window overflow",	/* 5 */
	"window underflow",	/* 6 */
	"alignment fault",	/* 7 */
	"fp exception",		/* 8 */
	"data fault",		/* 9 */
	"tag overflow",		/* 0a */
	T, T, T, T, T, T,	/* 0b..10 */
	"level 1 int",		/* 11 */
	"level 2 int",		/* 12 */
	"level 3 int",		/* 13 */
	"level 4 int",		/* 14 */
	"level 5 int",		/* 15 */
	"level 6 int",		/* 16 */
	"level 7 int",		/* 17 */
	"level 8 int",		/* 18 */
	"level 9 int",		/* 19 */
	"level 10 int",		/* 1a */
	"level 11 int",		/* 1b */
	"level 12 int",		/* 1c */
	"level 13 int",		/* 1d */
	"level 14 int",		/* 1e */
	"level 15 int",		/* 1f */
	T, T, T, T, T, T, T, T,	/* 20..27 */
	T, T, T, T, T, T, T, T,	/* 28..2f */
	T, T, T, T, T, T,	/* 30..35 */
	"cp disabled",		/* 36 */
	T,			/* 37 */
	T, T, T, T, T, T, T, T,	/* 38..3f */
	"cp exception",		/* 40 */
	T, T, T, T, T, T, T,	/* 41..47 */
	T, T, T, T, T, T, T, T,	/* 48..4f */
	T, T, T, T, T, T, T, T,	/* 50..57 */
	T, T, T, T, T, T, T, T,	/* 58..5f */
	T, T, T, T, T, T, T, T,	/* 60..67 */
	T, T, T, T, T, T, T, T,	/* 68..6f */
	T, T, T, T, T, T, T, T,	/* 70..77 */
	T, T, T, T, T, T, T, T,	/* 78..7f */

	/* user (software trap) vectors */
	"syscall",		/* 80 */
	"breakpoint",		/* 81 */
	"zero divide",		/* 82 */
	"flush windows",	/* 83 */
	"clean windows",	/* 84 */
	"range check",		/* 85 */
	"fix align",		/* 86 */
	"integer overflow",	/* 87 */
	"svr4 syscall",		/* 88 */
	"4.4 syscall",		/* 89 */
	"kgdb exec",		/* 8a */
	T, T, T, T, T,		/* 8b..8f */
	T, T, T, T, T, T, T, T,	/* 9a..97 */
	T, T, T, T, T, T, T, T,	/* 98..9f */
	"svr4 getcc",		/* a0 */
	"svr4 setcc",		/* a1 */
	"svr4 getpsr",		/* a2 */
	"svr4 setpsr",		/* a3 */
	"svr4 gethrtime",	/* a4 */
	"svr4 gethrvtime",	/* a5 */
	T,			/* a6 */
	"svr4 gethrestime",	/* a7 */
};

#define	N_TRAP_TYPES	(sizeof trap_type / sizeof *trap_type)

static __inline void userret __P((struct proc *, int,  u_quad_t));
void trap __P((unsigned, int, int, struct trapframe *));
static __inline void share_fpu __P((struct proc *, struct trapframe *));
void mem_access_fault __P((unsigned, int, u_int, int, int, struct trapframe *));
void mem_access_fault4m __P((unsigned, u_int, u_int, struct trapframe *));
void syscall __P((register_t, struct trapframe *, register_t));

int ignore_bogus_traps = 1;

/*
 * Define the code needed before returning to user mode, for
 * trap, mem_access_fault, and syscall.
 */
static __inline void
userret(p, pc, oticks)
	struct proc *p;
	int pc;
	u_quad_t oticks;
{
	int sig;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		postsig(sig);
	p->p_priority = p->p_usrpri;
	if (want_ast) {
		want_ast = 0;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
	}
	if (want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(NULL);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	/*
	 * If profiling, charge recent system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL)
		addupc_task(p, pc, (int)(p->p_sticks - oticks));

	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
}

/*
 * If someone stole the FPU while we were away, do not enable it
 * on return.  This is not done in userret() above as it must follow
 * the ktrsysret() in syscall().  Actually, it is likely that the
 * ktrsysret should occur before the call to userret.
 */
static __inline void share_fpu(p, tf)
	struct proc *p;
	struct trapframe *tf;
{
	if ((tf->tf_psr & PSR_EF) != 0 && cpuinfo.fpproc != p)
		tf->tf_psr &= ~PSR_EF;
}

/*
 * Called from locore.s trap handling, for non-MMU-related traps.
 * (MMU-related traps go through mem_access_fault, below.)
 */
void
trap(type, psr, pc, tf)
	unsigned type;
	int psr, pc;
	struct trapframe *tf;
{
	struct proc *p;
	struct pcb *pcb;
	int n;
	char bits[64];
	u_quad_t sticks;
	int sig;
	u_long ucode;

	/* This steps the PC over the trap. */
#define	ADVANCE (n = tf->tf_npc, tf->tf_pc = n, tf->tf_npc = n + 4)

	uvmexp.traps++;		/* XXXSMP */
	/*
	 * Generally, kernel traps cause a panic.  Any exceptions are
	 * handled early here.
	 */
	if (psr & PSR_PS) {
#ifdef DDB
		if (type == T_BREAKPOINT) {
			write_all_windows();
			if (kdb_trap(type, tf)) {
				return;
			}
		}
#endif
#ifdef DIAGNOSTIC
		/*
		 * Currently, we allow DIAGNOSTIC kernel code to
		 * flush the windows to record stack traces.
		 */
		if (type == T_FLUSHWIN) {
			write_all_windows();
			ADVANCE;
			return;
		}
#endif
		/*
		 * Storing %fsr in cpu_attach will cause this trap
		 * even though the fpu has been enabled, if and only
		 * if there is no FPU.
		 */
		if (type == T_FPDISABLED && cold) {
			ADVANCE;
			return;
		}
	dopanic:
		printf("trap type 0x%x: pc=0x%x npc=0x%x psr=%s\n",
		       type, pc, tf->tf_npc, bitmask_snprintf(psr,
		       PSR_BITS, bits, sizeof(bits)));
#ifdef DDB
		(void) kdb_trap(type, tf);
#endif
		panic(type < N_TRAP_TYPES ? trap_type[type] : T);
		/* NOTREACHED */
	}
	if ((p = curproc) == NULL)
		p = &proc0;
	sticks = p->p_sticks;
	pcb = &p->p_addr->u_pcb;
	p->p_md.md_tf = tf;	/* for ptrace/signals */

#ifdef FPU_DEBUG
	if (type != T_FPDISABLED && (tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fpproc != p)
			panic("FPU enabled but wrong proc (0)");
		savefpstate(p->p_md.md_fpstate);
		p->p_md.md_fpumid = -1;
		cpuinfo.fpproc = NULL;
		tf->tf_psr &= ~PSR_EF;
		setpsr(getpsr() & ~PSR_EF);
	}
#endif

	sig = 0;
	ucode = 0;

	switch (type) {

	default:
		if (type < 0x80) {
			if (!ignore_bogus_traps)
				goto dopanic;
			printf("trap type 0x%x: pc=0x%x npc=0x%x psr=%s\n",
			       type, pc, tf->tf_npc, bitmask_snprintf(psr,
			       PSR_BITS, bits, sizeof(bits)));
			sig = SIGILL;
			ucode = type;
			break;
		}
#if defined(COMPAT_SVR4)
badtrap:
#endif
#ifdef DIAGNOSTIC
		/* the following message is gratuitous */
		/* ... but leave it in until we find anything */
		uprintf("%s[%d]: unimplemented software trap 0x%x\n",
			p->p_comm, p->p_pid, type);
#endif
		sig = SIGILL;
		ucode = type;
		break;

#ifdef COMPAT_SVR4
	case T_SVR4_GETCC:
	case T_SVR4_SETCC:
	case T_SVR4_GETPSR:
	case T_SVR4_SETPSR:
	case T_SVR4_GETHRTIME:
	case T_SVR4_GETHRVTIME:
	case T_SVR4_GETHRESTIME:
		if (!svr4_trap(type, p))
			goto badtrap;
		break;
#endif

	case T_AST:
		break;	/* the work is all in userret() */

	case T_ILLINST:
	case T_UNIMPLFLUSH:
		if ((sig = emulinstr(pc, tf)) == 0) {
			ADVANCE;
			break;
		}
		/* XXX - ucode? */
		break;

	case T_PRIVINST:
		sig = SIGILL;
		/* XXX - ucode? */
		break;

	case T_FPDISABLED: {
		struct fpstate *fs = p->p_md.md_fpstate;

#ifdef FPU_DEBUG
		if ((tf->tf_psr & PSR_PS) != 0) {
			printf("FPU fault from kernel mode, pc=%x\n", pc);
#if DDB
			Debugger();
#endif
		}
#endif

		KERNEL_PROC_LOCK(p);
		if (fs == NULL) {
			fs = malloc(sizeof *fs, M_SUBPROC, M_WAITOK);
			*fs = initfpstate;
			fs->fs_qsize = 0;
			p->p_md.md_fpstate = fs;
		}
		/*
		 * If we have not found an FPU, we have to emulate it.
		 */
		if (!cpuinfo.fpupresent) {
#ifdef notyet
			fpu_emulate(p, tf, fs);
#else
			sig = SIGFPE;
			/* XXX - ucode? */
#endif
			KERNEL_PROC_UNLOCK(p);
			break;
		}
		/*
		 * We may have more FPEs stored up and/or ops queued.
		 * If they exist, handle them and get out.  Otherwise,
		 * resolve the FPU state, turn it on, and try again.
		 */
		if (fs->fs_qsize) {
			fpu_cleanup(p, fs);
			KERNEL_PROC_UNLOCK(p);
			break;
		}
#if notyet
		simple_lock(&cpuinfo.fplock);
		if (cpuinfo.fpproc != p) {		/* we do not have it */
			if (cpuinfo.fpproc != NULL) {	/* someone else had it*/
				savefpstate(cpuinfo.fpproc->p_md.md_fpstate);
				cpuinfo.fpproc->p_md.md_fpumid = -1;
			}
			/*
			 * On MP machines, some of the other FPUs might
			 * still have our state. Tell the owning processor
			 * to save the process' FPU state.
			 */
			cpi = p->p_md.md_fpumid;
			if (cpi != NULL) {
				if (cpi->mid == cpuinfo.mid)
					panic("FPU on module %d\n", mid);
				simple_lock(&cpi->fplock);
				simple_lock(&cpi->msg.lock);
				cpi->msg.tag = XPMSG_SAVEFPU;
				raise_ipi(cpi);
			}
			loadfpstate(fs);
			cpuinfo.fpproc = p;		/* now we do have it */
			p->p_md.md_fpumid = cpuinfo.mid;
		}
		simple_unlock(&cpuinfo.fplock);
#else
		if (cpuinfo.fpproc != p) {		/* we do not have it */
			int mid;

			mid = p->p_md.md_fpumid;
			if (cpuinfo.fpproc != NULL) {	/* someone else had it*/
				savefpstate(cpuinfo.fpproc->p_md.md_fpstate);
				cpuinfo.fpproc->p_md.md_fpumid = -1;
			}
			/*
			 * On MP machines, some of the other FPUs might
			 * still have our state. We can't handle that yet,
			 * so panic if it happens. Possible solutions:
			 * (1) send an inter-processor message to have the
			 * other FPU save the state, or (2) don't do lazy FPU
			 * context switching at all.
			 */
			if (mid != -1 && mid != cpuinfo.mid) {
				printf("own FPU on module %d\n", mid);
				panic("fix this");
			}
			loadfpstate(fs);
			cpuinfo.fpproc = p;		/* now we do have it */
			p->p_md.md_fpumid = cpuinfo.mid;
		}
#endif
		KERNEL_PROC_UNLOCK(p);
		tf->tf_psr |= PSR_EF;
		break;
	}

	case T_WINOF:
		if (rwindow_save(p))
			sigexit(p, SIGILL);
		break;

#define read_rw(src, dst) \
	copyin((caddr_t)(src), (caddr_t)(dst), sizeof(struct rwindow))

	case T_RWRET:
		/*
		 * T_RWRET is a window load needed in order to rett.
		 * It simply needs the window to which tf->tf_out[6]
		 * (%sp) points.  There are no user or saved windows now.
		 * Copy the one from %sp into pcb->pcb_rw[0] and set
		 * nsaved to -1.  If we decide to deliver a signal on
		 * our way out, we will clear nsaved.
		 */
		KERNEL_PROC_LOCK(p);
		if (pcb->pcb_uw || pcb->pcb_nsaved)
			panic("trap T_RWRET 1");
#ifdef DEBUG
		if (rwindow_debug)
			printf("%s[%d]: rwindow: pcb<-stack: 0x%x\n",
				p->p_comm, p->p_pid, tf->tf_out[6]);
#endif
		if (read_rw(tf->tf_out[6], &pcb->pcb_rw[0]))
			sigexit(p, SIGILL);
		if (pcb->pcb_nsaved)
			panic("trap T_RWRET 2");
		pcb->pcb_nsaved = -1;		/* mark success */
		KERNEL_PROC_UNLOCK(p);
		break;

	case T_WINUF:
		/*
		 * T_WINUF is a real window underflow, from a restore
		 * instruction.  It needs to have the contents of two
		 * windows---the one belonging to the restore instruction
		 * itself, which is at its %sp, and the one belonging to
		 * the window above, which is at its %fp or %i6---both
		 * in the pcb.  The restore's window may still be in
		 * the cpu; we need to force it out to the stack.
		 */
		KERNEL_PROC_LOCK(p);
#ifdef DEBUG
		if (rwindow_debug)
			printf("%s[%d]: rwindow: T_WINUF 0: pcb<-stack: 0x%x\n",
				p->p_comm, p->p_pid, tf->tf_out[6]);
#endif
		write_user_windows();
		if (rwindow_save(p) || read_rw(tf->tf_out[6], &pcb->pcb_rw[0]))
			sigexit(p, SIGILL);
#ifdef DEBUG
		if (rwindow_debug)
			printf("%s[%d]: rwindow: T_WINUF 1: pcb<-stack: 0x%x\n",
				p->p_comm, p->p_pid, pcb->pcb_rw[0].rw_in[6]);
#endif
		if (read_rw(pcb->pcb_rw[0].rw_in[6], &pcb->pcb_rw[1]))
			sigexit(p, SIGILL);
		if (pcb->pcb_nsaved)
			panic("trap T_WINUF");
		pcb->pcb_nsaved = -1;		/* mark success */
		KERNEL_PROC_UNLOCK(p);
		break;

	case T_ALIGN:
		if ((p->p_md.md_flags & MDP_FIXALIGN) != 0) {
			KERNEL_PROC_LOCK(p);
			n = fixalign(p, tf);
			KERNEL_PROC_UNLOCK(p);
			if (n == 0) {
				ADVANCE;
				break;
			}
		}
		sig = SIGBUS;
		/* XXX - ucode? */
		break;

	case T_FPE:
		/*
		 * Clean up after a floating point exception.
		 * fpu_cleanup can (and usually does) modify the
		 * state we save here, so we must `give up' the FPU
		 * chip context.  (The software and hardware states
		 * will not match once fpu_cleanup does its job, so
		 * we must not save again later.)
		 */
		KERNEL_PROC_LOCK(p);
		if (p != cpuinfo.fpproc)
			panic("fpe without being the FP user");
		savefpstate(p->p_md.md_fpstate);
		cpuinfo.fpproc = NULL;
		/* tf->tf_psr &= ~PSR_EF; */	/* share_fpu will do this */
		fpu_cleanup(p, p->p_md.md_fpstate);
		KERNEL_PROC_UNLOCK(p);
		/* fpu_cleanup posts signals if needed */
#if 0		/* ??? really never??? */
		ADVANCE;
#endif
		break;

	case T_TAGOF:
		sig = SIGEMT;
		/* XXX - ucode? */
		break;

	case T_CPDISABLED:
		uprintf("coprocessor instruction\n");	/* XXX */
		sig = SIGILL;
		/* XXX - ucode? */
		break;

	case T_BREAKPOINT:
		sig = SIGTRAP;
		break;

	case T_DIV0:
	case T_IDIV0:
		ADVANCE;
		sig = SIGFPE;
		ucode = FPE_INTDIV_TRAP;
		break;

	case T_FLUSHWIN:
		write_user_windows();
#ifdef probably_slower_since_this_is_usually_false
		KERNEL_PROC_LOCK(p);
		if (pcb->pcb_nsaved && rwindow_save(p))
			sigexit(p, SIGILL);
		KERNEL_PROC_UNLOCK(p);
#endif
		ADVANCE;
		break;

	case T_CLEANWIN:
		uprintf("T_CLEANWIN\n");	/* XXX */
		ADVANCE;
		break;

	case T_RANGECHECK:
		uprintf("T_RANGECHECK\n");	/* XXX */
		ADVANCE;
		sig = SIGILL;
		/* XXX - ucode? */
		break;

	case T_FIXALIGN:
#ifdef DEBUG_ALIGN
		uprintf("T_FIXALIGN\n");
#endif
		/* User wants us to fix alignment faults */
		p->p_md.md_flags |= MDP_FIXALIGN;
		ADVANCE;
		break;

	case T_INTOF:
		uprintf("T_INTOF\n");		/* XXX */
		ADVANCE;
		sig = SIGFPE;
		ucode = FPE_INTOVF_TRAP;
		break;
	}
	if (sig != 0) {
		KERNEL_PROC_LOCK(p);
		trapsignal(p, sig, ucode);
		KERNEL_PROC_UNLOCK(p);
	}
	userret(p, pc, sticks);
	share_fpu(p, tf);
#undef ADVANCE
}

/*
 * Save windows from PCB into user stack, and return 0.  This is used on
 * window overflow pseudo-traps (from locore.s, just before returning to
 * user mode) and when ptrace or sendsig needs a consistent state.
 * As a side effect, rwindow_save() always sets pcb_nsaved to 0,
 * clobbering the `underflow restore' indicator if it was -1.
 *
 * If the windows cannot be saved, pcb_nsaved is restored and we return -1.
 */
int
rwindow_save(p)
	struct proc *p;
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct rwindow *rw = &pcb->pcb_rw[0];
	int i;

	i = pcb->pcb_nsaved;
	if (i < 0) {
		pcb->pcb_nsaved = 0;
		return (0);
	}
	if (i == 0)
		return (0);
#ifdef DEBUG
	if (rwindow_debug)
		printf("%s[%d]: rwindow: pcb->stack:", p->p_comm, p->p_pid);
#endif
	do {
#ifdef DEBUG
		if (rwindow_debug)
			printf(" 0x%x", rw[1].rw_in[6]);
#endif
		if (copyout((caddr_t)rw, (caddr_t)rw[1].rw_in[6],
		    sizeof *rw))
			return (-1);
		rw++;
	} while (--i > 0);
#ifdef DEBUG
	if (rwindow_debug)
		printf("\n");
#endif
	pcb->pcb_nsaved = 0;
	return (0);
}

/*
 * Kill user windows (before exec) by writing back to stack or pcb
 * and then erasing any pcb tracks.  Otherwise we might try to write
 * the registers into the new process after the exec.
 */
void
kill_user_windows(p)
	struct proc *p;
{

	write_user_windows();
	p->p_addr->u_pcb.pcb_nsaved = 0;
}

/*
 * Called from locore.s trap handling, for synchronous memory faults.
 *
 * This duplicates a lot of logic in trap() and perhaps should be
 * moved there; but the bus-error-register parameters are unique to
 * this routine.
 *
 * Since synchronous errors accumulate during prefetch, we can have
 * more than one `cause'.  But we do not care what the cause, here;
 * we just want to page in the page and try again.
 */
void
mem_access_fault(type, ser, v, pc, psr, tf)
	unsigned type;
	int ser;
	u_int v;
	int pc, psr;
	struct trapframe *tf;
{
#if defined(SUN4) || defined(SUN4C)
	struct proc *p;
	struct vmspace *vm;
	vaddr_t va;
	int rv;
	vm_prot_t atype;
	int onfault;
	u_quad_t sticks;
	char bits[64];

	uvmexp.traps++;
	if ((p = curproc) == NULL)	/* safety check */
		p = &proc0;
	sticks = p->p_sticks;

	if ((psr & PSR_PS) == 0)
		KERNEL_PROC_LOCK(p);

#ifdef FPU_DEBUG
	if ((tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fpproc != p)
			panic("FPU enabled but wrong proc (1)");
		savefpstate(p->p_md.md_fpstate);
		p->p_md.md_fpumid = -1;
		cpuinfo.fpproc = NULL;
		tf->tf_psr &= ~PSR_EF;
		setpsr(getpsr() & ~PSR_EF);
	}
#endif

	/*
	 * Figure out what to pass the VM code, and ignore the sva register
	 * value in v on text faults (text faults are always at pc).
	 * Kernel faults are somewhat different: text faults are always
	 * illegal, and data faults are extra complex.  User faults must
	 * set p->p_md.md_tf, in case we decide to deliver a signal.  Check
	 * for illegal virtual addresses early since those can induce more
	 * faults.
	 */
	if (type == T_TEXTFAULT)
		v = pc;
	if (VA_INHOLE(v)) {
		rv = KERN_PROTECTION_FAILURE;
		goto fault;
	}
	atype = ser & SER_WRITE ? VM_PROT_WRITE : VM_PROT_READ;
	va = trunc_page(v);
	if (psr & PSR_PS) {
		extern char Lfsbail[];
		if (type == T_TEXTFAULT) {
			(void) splhigh();
			printf("text fault: pc=0x%x ser=%s\n", pc,
			  bitmask_snprintf(ser, SER_BITS, bits, sizeof(bits)));
			panic("kernel fault");
			/* NOTREACHED */
		}
		/*
		 * If this was an access that we shouldn't try to page in,
		 * resume at the fault handler without any action.
		 */
		if (p->p_addr && p->p_addr->u_pcb.pcb_onfault == Lfsbail)
			goto kfault;

		/*
		 * During autoconfiguration, faults are never OK unless
		 * pcb_onfault is set.  Once running normally we must allow
		 * exec() to cause copy-on-write faults to kernel addresses.
		 */
		if (cold)
			goto kfault;
		if (va >= KERNBASE) {
			if (uvm_fault(kernel_map, va, 0, atype) == KERN_SUCCESS)
				return;
			goto kfault;
		}
	} else
		p->p_md.md_tf = tf;

	/*
	 * mmu_pagein returns -1 if the page is already valid, in which
	 * case we have a hard fault; it returns 1 if it loads a segment
	 * that got bumped out via LRU replacement.
	 */
	vm = p->p_vmspace;
	rv = mmu_pagein(vm->vm_map.pmap, va,
			ser & SER_WRITE ? VM_PROT_WRITE : VM_PROT_READ);
	if (rv < 0) {
		rv = KERN_PROTECTION_FAILURE;
		goto fault;
	}
	if (rv > 0)
		goto out;

	/* alas! must call the horrible vm code */
	rv = uvm_fault(&vm->vm_map, (vaddr_t)va, 0, atype);

	/*
	 * If this was a stack access we keep track of the maximum
	 * accessed stack size.  Also, if vm_fault gets a protection
	 * failure it is due to accessing the stack region outside
	 * the current limit and we need to reflect that as an access
	 * error.
	 */
	if ((caddr_t)va >= vm->vm_maxsaddr
#ifdef COMPAT_SUNOS
	    && !(p->p_emul == &emul_sunos && va < USRSTACK -
		 (vaddr_t)p->p_limit->pl_rlimit[RLIMIT_STACK].rlim_cur +
		 SUNOS_MAXSADDR_SLOP)
#endif
	    ) {
		if (rv == KERN_SUCCESS) {
			unsigned nss = btoc(USRSTACK - va);
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
		} else if (rv == KERN_PROTECTION_FAILURE)
			rv = KERN_INVALID_ADDRESS;
	}
	if (rv == KERN_SUCCESS) {
		/*
		 * pmap_enter() does not enter all requests made from
		 * vm_fault into the MMU (as that causes unnecessary
		 * entries for `wired' pages).  Instead, we call
		 * mmu_pagein here to make sure the new PTE gets installed.
		 */
		(void) mmu_pagein(vm->vm_map.pmap, va, VM_PROT_NONE);
	} else {
		/*
		 * Pagein failed.  If doing copyin/out, return to onfault
		 * address.  Any other page fault in kernel, die; if user
		 * fault, deliver SIGSEGV.
		 */
fault:
		if (psr & PSR_PS) {
kfault:
			onfault = p->p_addr ?
			    (int)p->p_addr->u_pcb.pcb_onfault : 0;
			if (!onfault) {
				(void) splhigh();
				printf("data fault: pc=0x%x addr=0x%x ser=%s\n",
				    pc, v, bitmask_snprintf(ser, SER_BITS,
				    bits, sizeof(bits)));
				panic("kernel fault");
				/* NOTREACHED */
			}
			tf->tf_pc = onfault;
			tf->tf_npc = onfault + 4;
			return;
		}
		if (rv == KERN_RESOURCE_SHORTAGE) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(p, SIGKILL, (u_int)v);
		} else
			trapsignal(p, SIGSEGV, (u_int)v);
	}
out:
	if ((psr & PSR_PS) == 0) {
		KERNEL_PROC_UNLOCK(p);
		userret(p, pc, sticks);
		share_fpu(p, tf);
	}
#endif /* Sun4/Sun4C */
}

#if defined(SUN4M)	/* 4m version of mem_access_fault() follows */
static int tfaultaddr = (int) 0xdeadbeef;

void
mem_access_fault4m(type, sfsr, sfva, tf)
	unsigned type;
	u_int sfsr;
	u_int sfva;
	struct trapframe *tf;
{
	int pc, psr;
	struct proc *p;
	struct vmspace *vm;
	vaddr_t va=0;
	int rv;
	vm_prot_t atype;
	int onfault;
	u_quad_t sticks;
	char bits[64];

	uvmexp.traps++;	/* XXXSMP */

	if ((p = curproc) == NULL)	/* safety check */
		p = &proc0;
	sticks = p->p_sticks;

#ifdef FPU_DEBUG
	if ((tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fpproc != p)
			panic("FPU enabled but wrong proc (2)");
		savefpstate(p->p_md.md_fpstate);
		p->p_md.md_fpumid = -1;
		cpuinfo.fpproc = NULL;
		tf->tf_psr &= ~PSR_EF;
		setpsr(getpsr() & ~PSR_EF);
	}
#endif

	pc = tf->tf_pc;			/* These are needed below */
	psr = tf->tf_psr;

	if ((psr & PSR_PS) == 0)
		KERNEL_PROC_LOCK(p);

	/*
	 * Our first priority is handling serious faults, such as
	 * parity errors or async faults that might have come through here.
	 * If afsr & AFSR_AFO != 0, then we're on a HyperSPARC and we
	 * got an async fault. We pass it on to memerr4m. Similarly, if
	 * the trap was T_STOREBUFFAULT, we pass it on to memerr4m.
	 * If we have a data fault, but SFSR_FAV is not set in the sfsr,
	 * then things are really bizarre, and we treat it as a hard
	 * error and pass it on to memerr4m. See section 8.12.4 in the
	 * SuperSPARC user's guide for more info, and for a possible
	 * solution which we don't implement here.
	 * Note: store buffer faults may also lead to a level 15 interrupt
	 * being posted to the module (see sun4m system architecture,
	 * section B.I.9).
	 */
	if (type == T_STOREBUFFAULT ||
	    (type == T_DATAFAULT && (sfsr & SFSR_FAV) == 0)) {
		if ((psr & PSR_PS) == 0)
			KERNEL_PROC_UNLOCK(p);
		(*cpuinfo.memerr)(type, sfsr, sfva, tf);
		/*
		 * If we get here, exit the trap handler and wait for the
		 * trap to re-occur.
		 */
		if ((psr & PSR_PS) == 0)
			KERNEL_PROC_LOCK(p);
		goto out;
	}

	/*
	 * Figure out what to pass the VM code. We cannot ignore the sfva
	 * register on text faults, since this might be a trap on an
	 * alternate-ASI access to code space. However, if we're on a
	 * supersparc, we can't help using PC, since we don't get a VA in
	 * sfva.
	 * Kernel faults are somewhat different: text faults are always
	 * illegal, and data faults are extra complex.  User faults must
	 * set p->p_md.md_tf, in case we decide to deliver a signal.  Check
	 * for illegal virtual addresses early since those can induce more
	 * faults.
	 * All translation faults are illegal, and result in a SIGSEGV
	 * being delivered to the running process (or a kernel panic, for
	 * a kernel fault). We check the translation first to make sure
	 * it is not spurious.
	 * Also, note that in the case where we have an overwritten
	 * text fault (OW==1, AT==2,3), we attempt to service the
	 * second (overwriting) fault, then restart the instruction
	 * (which is from the first fault) and allow the first trap
	 * to reappear. XXX is this right? It will probably change...
	 */
	if ((sfsr & SFSR_FT) == SFSR_FT_NONE)
		goto out;	/* No fault. Why were we called? */

	/*
	 * NOTE: the per-CPU fault status register readers (in locore)
	 * may already have decided to pass `pc' in `sfva', so we avoid
	 * testing CPU types here.
	 * Q: test SFSR_FAV in the locore stubs too?
	 */
	if ((sfsr & SFSR_FAV) == 0) {
		if (type == T_TEXTFAULT)
			sfva = pc;
		else {
			rv = KERN_PROTECTION_FAILURE;
			goto fault;
		}
	}

	if ((sfsr & SFSR_FT) == SFSR_FT_TRANSERR) {
		/*
		 * Translation errors are always fatal, as they indicate
		 * a corrupt translation (page) table hierarchy.
		 */
		rv = KERN_PROTECTION_FAILURE;

		/* XXXSMP - why bother with this anyway? */
		if (tfaultaddr == sfva)	/* Prevent infinite loops w/a static */
			goto fault;
		tfaultaddr = sfva;
		if ((lda((sfva & 0xFFFFF000) | ASI_SRMMUFP_LN, ASI_SRMMUFP) &
		    SRMMU_TETYPE) != SRMMU_TEPTE)
			goto fault;	/* Translation bad */
		lda(SRMMU_SFSR, ASI_SRMMU);
#ifdef DEBUG
		printf("mem_access_fault4m: SFSR_FT_TRANSERR: "
			"pid %d, va 0x%x: retrying\n", p->p_pid, sfva);
#endif
		goto out;	/* Translation OK, retry operation */
	}

	va = trunc_page(sfva);

	if (((sfsr & SFSR_AT_TEXT) || type == T_TEXTFAULT) &&
	    !(sfsr & SFSR_AT_STORE) && (sfsr & SFSR_OW)) {
		if (psr & PSR_PS)	/* never allow in kernel */
			goto kfault;
#if 0
		/*
		 * Double text fault. The evil "case 5" from the HS manual...
		 * Attempt to handle early fault. Ignores ASI 8,9 issue...may
		 * do a useless VM read.
		 * XXX: Is this really necessary?
		 */
		if (cpuinfo.cpu_type == CPUTYP_HS_MBUS) {
			/* On HS, we have va for both */
			vm = p->p_vmspace;
			if (uvm_fault(&vm->vm_map, trunc_page(pc),
				     0, VM_PROT_READ) != KERN_SUCCESS)
#ifdef DEBUG
				printf("mem_access_fault: "
					"can't pagein 1st text fault.\n")
#endif
				;
		}
#endif
	}

	/* Now munch on protections... */
	atype = sfsr & SFSR_AT_STORE ? VM_PROT_WRITE : VM_PROT_READ;
	if (psr & PSR_PS) {
		extern char Lfsbail[];
		if (sfsr & SFSR_AT_TEXT || type == T_TEXTFAULT) {
			(void) splhigh();
			printf("text fault: pc=0x%x sfsr=%s sfva=0x%x\n", pc,
			    bitmask_snprintf(sfsr, SFSR_BITS, bits,
			    sizeof(bits)), sfva);
			panic("kernel fault");
			/* NOTREACHED */
		}
		/*
		 * If this was an access that we shouldn't try to page in,
		 * resume at the fault handler without any action.
		 */
		if (p->p_addr && p->p_addr->u_pcb.pcb_onfault == Lfsbail)
			goto kfault;

		/*
		 * During autoconfiguration, faults are never OK unless
		 * pcb_onfault is set.  Once running normally we must allow
		 * exec() to cause copy-on-write faults to kernel addresses.
		 */
		if (cold)
			goto kfault;
		if (va >= KERNBASE) {
			if (uvm_fault(kernel_map, va, 0, atype) == KERN_SUCCESS)
				return;
			goto kfault;
		}
	} else
		p->p_md.md_tf = tf;

	vm = p->p_vmspace;

	/* alas! must call the horrible vm code */
	rv = uvm_fault(&vm->vm_map, (vaddr_t)va, 0, atype);

	/*
	 * If this was a stack access we keep track of the maximum
	 * accessed stack size.  Also, if vm_fault gets a protection
	 * failure it is due to accessing the stack region outside
	 * the current limit and we need to reflect that as an access
	 * error.
	 */
	if ((caddr_t)va >= vm->vm_maxsaddr) {
		if (rv == KERN_SUCCESS) {
			unsigned nss = btoc(USRSTACK - va);
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
		} else if (rv == KERN_PROTECTION_FAILURE)
			rv = KERN_INVALID_ADDRESS;
	}
	if (rv != KERN_SUCCESS) {
		/*
		 * Pagein failed.  If doing copyin/out, return to onfault
		 * address.  Any other page fault in kernel, die; if user
		 * fault, deliver SIGSEGV.
		 */
fault:
		if (psr & PSR_PS) {
kfault:
			onfault = p->p_addr ?
			    (int)p->p_addr->u_pcb.pcb_onfault : 0;
			if (!onfault) {
				(void) splhigh();
				printf("data fault: pc=0x%x addr=0x%x sfsr=%s\n",
				    pc, sfva, bitmask_snprintf(sfsr, SFSR_BITS,
				    bits, sizeof(bits)));
				panic("kernel fault");
				/* NOTREACHED */
			}
			tf->tf_pc = onfault;
			tf->tf_npc = onfault + 4;
			return;
		}
		if (rv == KERN_RESOURCE_SHORTAGE) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(p, SIGKILL, (u_int)sfva);
		} else
			trapsignal(p, SIGSEGV, (u_int)sfva);
	}
out:
	if ((psr & PSR_PS) == 0) {
		KERNEL_PROC_UNLOCK(p);
		userret(p, pc, sticks);
		share_fpu(p, tf);
	}
}
#endif

/*
 * System calls.  `pc' is just a copy of tf->tf_pc.
 *
 * Note that the things labelled `out' registers in the trapframe were the
 * `in' registers within the syscall trap code (because of the automatic
 * `save' effect of each trap).  They are, however, the %o registers of the
 * thing that made the system call, and are named that way here.
 */
void
syscall(code, tf, pc)
	register_t code;
	struct trapframe *tf;
	register_t pc;
{
	int i, nsys, *ap, nap;
	const struct sysent *callp;
	struct proc *p;
	int error, new;
	struct args {
		register_t i[8];
	} args;
	register_t rval[2];
	u_quad_t sticks;

	uvmexp.syscalls++;	/* XXXSMP */
	p = curproc;

	KERNEL_PROC_LOCK(p);

#ifdef DIAGNOSTIC
	if (tf->tf_psr & PSR_PS)
		panic("syscall");
	if (cpuinfo.curpcb != &p->p_addr->u_pcb)
		panic("syscall cpcb/ppcb");
	if (tf != (struct trapframe *)((caddr_t)cpuinfo.curpcb + USPACE) - 1)
		panic("syscall trapframe");
#endif
	sticks = p->p_sticks;
	p->p_md.md_tf = tf;
	new = code & (SYSCALL_G7RFLAG | SYSCALL_G2RFLAG);
	code &= ~(SYSCALL_G7RFLAG | SYSCALL_G2RFLAG);

#ifdef FPU_DEBUG
	if ((tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fpproc != p)
			panic("FPU enabled but wrong proc (3)");
		savefpstate(p->p_md.md_fpstate);
		p->p_md.md_fpumid = -1;
		cpuinfo.fpproc = NULL;
		tf->tf_psr &= ~PSR_EF;
		setpsr(getpsr() & ~PSR_EF);
	}
#endif

	callp = p->p_emul->e_sysent;
	nsys = p->p_emul->e_nsysent;

	/*
	 * The first six system call arguments are in the six %o registers.
	 * Any arguments beyond that are in the `argument extension' area
	 * of the user's stack frame (see <machine/frame.h>).
	 *
	 * Check for ``special'' codes that alter this, namely syscall and
	 * __syscall.  The latter takes a quad syscall number, so that other
	 * arguments are at their natural alignments.  Adjust the number
	 * of ``easy'' arguments as appropriate; we will copy the hard
	 * ones later as needed.
	 */
	ap = &tf->tf_out[0];
	nap = 6;

	switch (code) {
	case SYS_syscall:
		code = *ap++;
		nap--;
		break;
	case SYS___syscall:
		if (!(p->p_emul->e_flags & EMUL_HAS_SYS___syscall))
			break;
		code = ap[_QUAD_LOWWORD];
		ap += 2;
		nap -= 2;
		break;
	}

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;
	else {
		callp += code;
		i = callp->sy_argsize / sizeof(register_t);
		if (i > nap) {	/* usually false */
			if (i > 8)
				panic("syscall nargs");
			error = copyin((caddr_t)tf->tf_out[6] +
			    offsetof(struct frame, fr_argx),
			    (caddr_t)&args.i[nap], (i - nap) * sizeof(register_t));
			if (error) {
#ifdef KTRACE
				if (KTRPOINT(p, KTR_SYSCALL))
					ktrsyscall(p, code,
					    callp->sy_argsize, args.i);
#endif
				goto bad;
			}
			i = nap;
		}
		copywords(ap, args.i, i * sizeof(register_t));
	}
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p, code, callp->sy_argsize, args.i);
#endif
	rval[0] = 0;
	rval[1] = tf->tf_out[1];
	error = (*callp->sy_call)(p, &args, rval);

	switch (error) {
	case 0:
		/* Note: fork() does not return here in the child */
		tf->tf_out[0] = rval[0];
		tf->tf_out[1] = rval[1];
		if (new) {
			/* jmp %g2 (or %g7, deprecated) on success */
			i = tf->tf_global[new & SYSCALL_G2RFLAG ? 2 : 7];
			if (i & 3) {
				error = EINVAL;
				goto bad;
			}
		} else {
			/* old system call convention: clear C on success */
			tf->tf_psr &= ~PSR_C;	/* success */
			i = tf->tf_npc;
		}
		tf->tf_pc = i;
		tf->tf_npc = i + 4;
		break;

	case ERESTART:
	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_out[0] = error;
		tf->tf_psr |= PSR_C;	/* fail */
		i = tf->tf_npc;
		tf->tf_pc = i;
		tf->tf_npc = i + 4;
		break;
	}

	KERNEL_PROC_UNLOCK(p);
	userret(p, pc, sticks);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(p);
		ktrsysret(p, code, error, rval[0]);
		KERNEL_PROC_UNLOCK(p);
	}
#endif
	share_fpu(p, tf);
}

/*
 * Process the tail end of a fork() for the child.
 */
void
child_return(arg)
	void *arg;
{
	struct proc *p = arg;

	/*
	 * Return values in the frame set by cpu_fork().
	 */
	KERNEL_PROC_UNLOCK(p);
	userret(p, p->p_md.md_tf->tf_pc, 0);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(p);
		ktrsysret(p,
			  (p->p_flag & P_PPWAIT) ? SYS_vfork : SYS_fork, 0, 0);
		KERNEL_PROC_UNLOCK(p);
	}
#endif
}
