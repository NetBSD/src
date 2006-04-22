/*	$NetBSD: trap.c,v 1.163.8.1 2006/04/22 11:37:59 simonb Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.163.8.1 2006/04/22 11:37:59 simonb Exp $");

#include "opt_ddb.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"
#include "opt_sparc_arch.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscall.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <sparc/sparc/asm.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/trap.h>
#include <machine/instr.h>
#include <machine/pmap.h>
#include <machine/userret.h>

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
	  ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0 },
	0, 0,
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
	"watchpoint",		/* 0b */
	T, T, T, T, T,		/* 0c..10 */
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
	"register access error",/* 20 */
	"instruction access error",/* 21 */
	T, T,			/* 22..23 */
	"cp disabled",		/* 24 */
	"unimplemented flush",	/* 25 */
	T, T,			/* 26..27 */
	"cp exception",		/* 28 */
	"data access error",	/* 29 */
	"hw zero divide",	/* 2a */
	"data store error",	/* 2b */
	"data access MMU miss",	/* 2c */
	T, T, T,		/* 2d..2f */
	T, T, T, T, T, T, T, T,	/* 30..37 */
	T, T, T, T,		/* 38..3b */
	"insn access MMU miss",	/* 3c */
	T, T, T,		/* 3d..3f */
	T, T, T, T, T, T, T, T,	/* 40..47 */
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

void trap(unsigned, int, int, struct trapframe *);
void mem_access_fault(unsigned, int, u_int, int, int, struct trapframe *);
void mem_access_fault4m(unsigned, u_int, u_int, struct trapframe *);

int ignore_bogus_traps = 1;

/*
 * Called from locore.s trap handling, for non-MMU-related traps.
 * (MMU-related traps go through mem_access_fault, below.)
 */
void
trap(unsigned type, int psr, int pc, struct trapframe *tf)
{
	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;
	int n, s;
	char bits[64];
	u_quad_t sticks;
	ksiginfo_t ksi;
	int code, sig;

	/* This steps the PC over the trap. */
#define	ADVANCE (n = tf->tf_npc, tf->tf_pc = n, tf->tf_npc = n + 4)

	uvmexp.traps++;	/* XXXSMP */
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
#if defined(MULTIPROCESSOR)
		if (type == T_DBPAUSE) {
			/* XXX - deal with kgdb too */
			extern void ddb_suspend(struct trapframe *);
			write_all_windows();
			ddb_suspend(tf);
			ADVANCE;
			return;
		}
#endif
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
		if (type == T_UNIMPLFLUSH) {
			/*
			 * This should happen only on hypersparc.
			 * It also is a rare event to get this trap
			 * from kernel space. For now, just flush the
			 * entire I-cache.
			 */
#if defined(MULTIPROCESSOR)
			/* Broadcast to all CPUs */
			XCALL0(*cpuinfo.pure_vcache_flush, CPUSET_ALL);
#else
			(*cpuinfo.pure_vcache_flush)();
#endif
			ADVANCE;
			return;
		}

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
		write_all_windows();
		(void) kdb_trap(type, tf);
#endif
		panic(type < N_TRAP_TYPES ? trap_type[type] : T);
		/* NOTREACHED */
	}
	if ((l = curlwp) == NULL)
		l = &lwp0;
	p = l->l_proc;
	sticks = p->p_sticks;
	pcb = &l->l_addr->u_pcb;
	l->l_md.md_tf = tf;	/* for ptrace/signals */

#ifdef FPU_DEBUG
	if (type != T_FPDISABLED && (tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fplwp != l)
			panic("FPU enabled but wrong proc (0) [l=%p, fwlp=%p]",
				l, cpuinfo.fplwp);
		savefpstate(l->l_md.md_fpstate);
		l->l_md.md_fpu = NULL;
		cpuinfo.fplwp = NULL;
		tf->tf_psr &= ~PSR_EF;
		setpsr(getpsr() & ~PSR_EF);
	}
#endif

	sig = 0;

	switch (type) {

	default:
		if (type < 0x80) {
			if (!ignore_bogus_traps)
				goto dopanic;
			printf("trap type 0x%x: pc=0x%x npc=0x%x psr=%s\n",
			       type, pc, tf->tf_npc, bitmask_snprintf(psr,
			       PSR_BITS, bits, sizeof(bits)));
			sig = SIGILL;
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_trap = type;
			ksi.ksi_code = ILL_ILLTRP;
			ksi.ksi_addr = (void *)pc;
			break;
		}
#if defined(COMPAT_SVR4)
badtrap:
#endif
#ifdef DIAGNOSTIC
		if (type < 0x90 || type > 0x9f) {
			/* the following message is gratuitous */
			/* ... but leave it in until we find anything */
			uprintf("%s[%d]: unimplemented software trap 0x%x\n",
				p->p_comm, p->p_pid, type);
		}
#endif
		sig = SIGILL;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_ILLTRP;
		ksi.ksi_addr = (void *)pc;
		break;

#ifdef COMPAT_SVR4
	case T_SVR4_GETCC:
	case T_SVR4_SETCC:
	case T_SVR4_GETPSR:
	case T_SVR4_SETPSR:
	case T_SVR4_GETHRTIME:
	case T_SVR4_GETHRVTIME:
	case T_SVR4_GETHRESTIME:
		if (!svr4_trap(type, l))
			goto badtrap;
		break;
#endif

	case T_AST:
		break;	/* the work is all in userret() */

	case T_UNIMPLFLUSH:
		/* Invalidate the entire I-cache */
#if defined(MULTIPROCESSOR)
		/* Broadcast to all CPUs */
		XCALL0(*cpuinfo.pure_vcache_flush, CPUSET_ALL);
#else
		(*cpuinfo.pure_vcache_flush)();
#endif
		ADVANCE;
		break;

	case T_ILLINST:
		/* Note: Cypress generates a T_ILLINST on FLUSH instructions */
		if ((sig = emulinstr(pc, tf)) == 0) {
			ADVANCE;
			break;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_PRIVINST:
		sig = SIGILL;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_PRVOPC;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_FPDISABLED: {
		struct fpstate *fs = l->l_md.md_fpstate;

#ifdef FPU_DEBUG
		if ((tf->tf_psr & PSR_PS) != 0) {
			printf("FPU fault from kernel mode, pc=%x\n", pc);
#ifdef DDB
			Debugger();
#endif
		}
#endif

		if (fs == NULL) {
			KERNEL_PROC_LOCK(l);
			fs = malloc(sizeof *fs, M_SUBPROC, M_WAITOK);
			KERNEL_PROC_UNLOCK(l);
			*fs = initfpstate;
			l->l_md.md_fpstate = fs;
		}
		/*
		 * If we have not found an FPU, we have to emulate it.
		 */
		if (!cpuinfo.fpupresent) {
#ifdef notyet
			fpu_emulate(l, tf, fs);
#else
			sig = SIGFPE;
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_trap = type;
			ksi.ksi_code = SI_NOINFO;
			ksi.ksi_addr = (void *)pc;
#endif
			break;
		}
		/*
		 * We may have more FPEs stored up and/or ops queued.
		 * If they exist, handle them and get out.  Otherwise,
		 * resolve the FPU state, turn it on, and try again.
		 */
		if (fs->fs_qsize) {
			if ((code = fpu_cleanup(l, fs)) != 0) {
				sig = SIGFPE;
				KSI_INIT_TRAP(&ksi);
				ksi.ksi_trap = type;
				ksi.ksi_code = code;
				ksi.ksi_addr = (void *)pc;
			}
			break;
		}

		/*
		 * If we do not own the FPU state on this CPU, we must
		 * now acquire it.
		 */
		if (cpuinfo.fplwp != l) {
			struct cpu_info *cpi;

			FPU_LOCK(s);
			if (cpuinfo.fplwp != NULL) {
				/* someone else had it*/
				savefpstate(cpuinfo.fplwp->l_md.md_fpstate);
				cpuinfo.fplwp->l_md.md_fpu = NULL;
			}

			/*
			 * On MP machines, some of the other FPUs might
			 * still have our state. Tell the owning processor
			 * to save the process' FPU state.
			 */
			if ((cpi = l->l_md.md_fpu) != NULL) {
				if (cpi->ci_cpuid == cpuinfo.ci_cpuid)
					panic("FPU(%d): state for %p",
							cpi->ci_cpuid, l);
#if defined(MULTIPROCESSOR)
				XCALL1(savefpstate, fs, 1 << cpi->ci_cpuid);
#endif
				cpi->fplwp = NULL;
			}
			loadfpstate(fs);

			/* now we do have it */
			cpuinfo.fplwp = l;
			l->l_md.md_fpu = curcpu();
			FPU_UNLOCK(s);
		}

		tf->tf_psr |= PSR_EF;
		break;
	}

	case T_WINOF:
		KERNEL_PROC_LOCK(l);
		if (rwindow_save(l))
			sigexit(l, SIGILL);
		KERNEL_PROC_UNLOCK(l);
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
		KERNEL_PROC_LOCK(l);
		if (pcb->pcb_uw || pcb->pcb_nsaved)
			panic("trap T_RWRET 1");
#ifdef DEBUG
		if (rwindow_debug)
			printf("cpu%d:%s[%d]: rwindow: pcb<-stack: 0x%x\n",
				cpuinfo.ci_cpuid, p->p_comm, p->p_pid,
				tf->tf_out[6]);
#endif
		if (read_rw(tf->tf_out[6], &pcb->pcb_rw[0]))
			sigexit(l, SIGILL);
		if (pcb->pcb_nsaved)
			panic("trap T_RWRET 2");
		pcb->pcb_nsaved = -1;		/* mark success */
		KERNEL_PROC_UNLOCK(l);
		break;

	case T_WINUF:
		/*
		 * T_WINUF is a real window underflow, from a restore
		 * instruction.  It needs to have the contents of two
		 * windows---the one belonging to the restore instruction
		 * itself, which is at its %sp, and the one belonging to
		 * the window above, which is at its %fp or %i6---both
		 * in the pcb.  The restore's window may still be in
		 * the CPU; we need to force it out to the stack.
		 */
		KERNEL_PROC_LOCK(l);
#ifdef DEBUG
		if (rwindow_debug)
			printf("cpu%d:%s[%d]: rwindow: T_WINUF 0: pcb<-stack: 0x%x\n",
				cpuinfo.ci_cpuid, p->p_comm, p->p_pid,
				tf->tf_out[6]);
#endif
		write_user_windows();
		if (rwindow_save(l) || read_rw(tf->tf_out[6], &pcb->pcb_rw[0]))
			sigexit(l, SIGILL);
#ifdef DEBUG
		if (rwindow_debug)
			printf("cpu%d:%s[%d]: rwindow: T_WINUF 1: pcb<-stack: 0x%x\n",
				cpuinfo.ci_cpuid, p->p_comm, p->p_pid,
				pcb->pcb_rw[0].rw_in[6]);
#endif
		if (read_rw(pcb->pcb_rw[0].rw_in[6], &pcb->pcb_rw[1]))
			sigexit(l, SIGILL);
		if (pcb->pcb_nsaved)
			panic("trap T_WINUF");
		pcb->pcb_nsaved = -1;		/* mark success */
		KERNEL_PROC_UNLOCK(l);
		break;

	case T_ALIGN:
		if ((p->p_md.md_flags & MDP_FIXALIGN) != 0) {
			KERNEL_PROC_LOCK(l);
			n = fixalign(l, tf);
			KERNEL_PROC_UNLOCK(l);
			if (n == 0) {
				ADVANCE;
				break;
			}
		}
		sig = SIGBUS;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = BUS_ADRALN;
		ksi.ksi_addr = (void *)pc;
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
		KERNEL_PROC_LOCK(l);
		if (l != cpuinfo.fplwp)
			panic("fpe without being the FP user");
		FPU_LOCK(s);
		savefpstate(l->l_md.md_fpstate);
		cpuinfo.fplwp = NULL;
		l->l_md.md_fpu = NULL;
		FPU_UNLOCK(s);
		KERNEL_PROC_UNLOCK(l);
		/* tf->tf_psr &= ~PSR_EF; */	/* share_fpu will do this */
		if ((code = fpu_cleanup(l, l->l_md.md_fpstate)) != 0) {
			sig = SIGFPE;
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_trap = type;
			ksi.ksi_code = code;
			ksi.ksi_addr = (void *)pc;
		}
#if 0		/* ??? really never??? */
		ADVANCE;
#endif
		break;

	case T_TAGOF:
		sig = SIGEMT;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = SI_NOINFO;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_CPDISABLED:
		uprintf("coprocessor instruction\n");	/* XXX */
		sig = SIGILL;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_COPROC;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_BREAKPOINT:
		sig = SIGTRAP;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = TRAP_BRKPT;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_DIV0:
	case T_IDIV0:
		ADVANCE;
		sig = SIGFPE;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = FPE_INTDIV;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_FLUSHWIN:
		write_user_windows();
#ifdef probably_slower_since_this_is_usually_false
		KERNEL_PROC_LOCK(l);
		if (pcb->pcb_nsaved && rwindow_save(p))
			sigexit(l, SIGILL);
		KERNEL_PROC_UNLOCK(l);
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
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_ILLOPN;
		ksi.ksi_addr = (void *)pc;
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
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = FPE_INTOVF;
		ksi.ksi_addr = (void *)pc;
		break;
	}
	if (sig != 0) {
		KERNEL_PROC_LOCK(l);
		ksi.ksi_signo = sig;
		trapsignal(l, &ksi);
		KERNEL_PROC_UNLOCK(l);
	}
	userret(l, pc, sticks);
	share_fpu(l, tf);
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
rwindow_save(struct lwp *l)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
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
		printf("cpu%d:%s[%d]: rwindow: pcb->stack:",
			cpuinfo.ci_cpuid, l->l_proc->p_comm, l->l_proc->p_pid);
#endif
	do {
#ifdef DEBUG
		if (rwindow_debug)
			printf(" [%d]0x%x", cpuinfo.ci_cpuid, rw[1].rw_in[6]);
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
kill_user_windows(struct lwp *l)
{

	write_user_windows();
	l->l_addr->u_pcb.pcb_nsaved = 0;
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
mem_access_fault(unsigned type, int ser, u_int v, int pc, int psr,
		 struct trapframe *tf)
{
#if defined(SUN4) || defined(SUN4C)
	struct proc *p;
	struct lwp *l;
	struct vmspace *vm;
	vaddr_t va;
	int rv = EFAULT;
	vm_prot_t atype;
	int onfault;
	u_quad_t sticks;
	char bits[64];
	ksiginfo_t ksi;

	uvmexp.traps++;
	if ((l = curlwp) == NULL)	/* safety check */
		l = &lwp0;
	p = l->l_proc;
	sticks = p->p_sticks;

	if ((psr & PSR_PS) == 0)
		KERNEL_PROC_LOCK(l);

#ifdef FPU_DEBUG
	if ((tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fplwp != l)
			panic("FPU enabled but wrong proc (1) [l=%p, fwlp=%p]",
				l, cpuinfo.fplwp);
		savefpstate(l->l_md.md_fpstate);
		l->l_md.md_fpu = NULL;
		cpuinfo.fplwp = NULL;
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
		rv = EACCES;
		goto fault;
	}
	atype = ser & SER_WRITE ? VM_PROT_WRITE : VM_PROT_READ;
	if ((ser & SER_PROT) && atype == VM_PROT_READ && type != T_TEXTFAULT) {

		/*
		 * The hardware reports faults by the atomic load/store
		 * instructions as read faults, so if the faulting instruction
		 * is one of those, relabel this fault as both read and write.
		 */
		if ((fuword((void *)pc) & 0xc1680000) == 0xc0680000) {
			atype = VM_PROT_READ | VM_PROT_WRITE;
		}
	}
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
		if (l->l_addr && l->l_addr->u_pcb.pcb_onfault == Lfsbail)
			goto kfault;

		/*
		 * During autoconfiguration, faults are never OK unless
		 * pcb_onfault is set.  Once running normally we must allow
		 * exec() to cause copy-on-write faults to kernel addresses.
		 */
		if (cold)
			goto kfault;
		if (va >= KERNBASE) {
			rv = mmu_pagein(pmap_kernel(), va, atype);
			if (rv < 0) {
				rv = EACCES;
				goto kfault;
			}
			if (rv > 0)
				return;
			rv = uvm_fault(kernel_map, va, atype);
			if (rv == 0)
				return;
			goto kfault;
		}
	} else {
		l->l_md.md_tf = tf;
		if (l->l_flag & L_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)v;
			l->l_flag |= L_SA_PAGEFAULT;
		}
	}

	/*
	 * mmu_pagein returns -1 if the page is already valid, in which
	 * case we have a hard fault; it returns 1 if it loads a segment
	 * that got bumped out via LRU replacement.
	 */
	vm = p->p_vmspace;
	rv = mmu_pagein(vm->vm_map.pmap, va, atype);
	if (rv < 0) {
		rv = EACCES;
		goto fault;
	}
	if (rv > 0)
		goto out;

	/* alas! must call the horrible vm code */
	rv = uvm_fault(&vm->vm_map, (vaddr_t)va, atype);

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
	    && rv == 0)
		uvm_grow(p, va);

	if (rv == 0) {
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
			onfault = l->l_addr ?
			    (int)l->l_addr->u_pcb.pcb_onfault : 0;
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
			tf->tf_out[0] = (rv == EACCES) ? EFAULT : rv;
			return;
		}
		KSI_INIT_TRAP(&ksi);
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = SI_NOINFO;
		} else {
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = (rv == EACCES
				? SEGV_ACCERR : SEGV_MAPERR);
		}
		ksi.ksi_errno = rv;
		ksi.ksi_trap = type;
		ksi.ksi_addr = (void *)v;
		trapsignal(l, &ksi);
	}
out:
	if ((psr & PSR_PS) == 0) {
		l->l_flag &= ~L_SA_PAGEFAULT;
		KERNEL_PROC_UNLOCK(l);
		userret(l, pc, sticks);
		share_fpu(l, tf);
	}
#endif /* SUN4 || SUN4C */
}

#if defined(SUN4M)	/* 4m version of mem_access_fault() follows */
static int tfaultaddr = (int) 0xdeadbeef;

void
mem_access_fault4m(unsigned type, u_int sfsr, u_int sfva, struct trapframe *tf)
{
	int pc, psr;
	struct proc *p;
	struct lwp *l;
	struct vmspace *vm;
	vaddr_t va;
	int rv = EFAULT;
	vm_prot_t atype;
	int onfault;
	u_quad_t sticks;
	char bits[64];
	ksiginfo_t ksi;

	uvmexp.traps++;	/* XXXSMP */

	if ((l = curlwp) == NULL)	/* safety check */
		l = &lwp0;
	p = l->l_proc;
	sticks = p->p_sticks;

#ifdef FPU_DEBUG
	if ((tf->tf_psr & PSR_EF) != 0) {
		if (cpuinfo.fplwp != l)
			panic("FPU enabled but wrong proc (2) [l=%p, fwlp=%p]",
				l, cpuinfo.fplwp);
		savefpstate(l->l_md.md_fpstate);
		l->l_md.md_fpu = NULL;
		cpuinfo.fplwp = NULL;
		tf->tf_psr &= ~PSR_EF;
		setpsr(getpsr() & ~PSR_EF);
	}
#endif

	pc = tf->tf_pc;			/* These are needed below */
	psr = tf->tf_psr;

#if /*DIAGNOSTICS*/1
	if (type == T_DATAERROR || type == T_TEXTERROR)
		printf("%s[%d]: trap 0x%x: pc=0x%x sfsr=0x%x sfva=0x%x\n",
			p->p_comm, p->p_pid, type, pc, sfsr, sfva);
#endif

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
		(*cpuinfo.memerr)(type, sfsr, sfva, tf);
		/*
		 * If we get here, exit the trap handler and wait for the
		 * trap to re-occur.
		 */
		goto out_nounlock;
	}

	if ((psr & PSR_PS) == 0)
		KERNEL_PROC_LOCK(l);
	else
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);

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
		/* note: T_TEXTERROR == T_TEXTFAULT | 0x20 */
		if ((type & ~0x20) == T_TEXTFAULT)
			sfva = pc;
		else {
			rv = EACCES;
			goto fault;
		}
	}

	if ((sfsr & SFSR_FT) == SFSR_FT_TRANSERR) {
		/*
		 * Translation errors are always fatal, as they indicate
		 * a corrupt translation (page) table hierarchy.
		 */
		rv = EACCES;

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
		 * XXX: If it's necessary, add SA_PAGEFAULT handling
		 */
		if (cpuinfo.cpu_type == CPUTYP_HS_MBUS) {
			/* On HS, we have va for both */
			vm = p->p_vmspace;
			if (uvm_fault(&vm->vm_map, trunc_page(pc),
				      VM_PROT_READ) != 0)
#ifdef DEBUG
				printf("mem_access_fault: "
					"can't pagein 1st text fault.\n")
#endif
				;
		}
#endif
	}

	/* Now munch on protections... */
	if (sfsr & SFSR_AT_STORE) {
		/* stores are never text faults. */
		atype = VM_PROT_WRITE;
	} else {
		if ((sfsr & SFSR_AT_TEXT) || (type & ~0x20) == T_TEXTFAULT) {
			atype = VM_PROT_EXECUTE;
		} else {
			atype = VM_PROT_READ;
		}
	}

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
		if (l->l_addr && l->l_addr->u_pcb.pcb_onfault == Lfsbail)
			goto kfault;

		/*
		 * During autoconfiguration, faults are never OK unless
		 * pcb_onfault is set.  Once running normally we must allow
		 * exec() to cause copy-on-write faults to kernel addresses.
		 */
		if (cold)
			goto kfault;
		if (va >= KERNBASE) {
			rv = uvm_fault(kernel_map, va, atype);
			if (rv == 0) {
				KERNEL_UNLOCK();
				return;
			}
			goto kfault;
		}
	} else {
		l->l_md.md_tf = tf;
		if (l->l_flag & L_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)sfva;
			l->l_flag |= L_SA_PAGEFAULT;
		}
	}

	vm = p->p_vmspace;

	/* alas! must call the horrible vm code */
	rv = uvm_fault(&vm->vm_map, (vaddr_t)va, atype);

	/*
	 * If this was a stack access we keep track of the maximum
	 * accessed stack size.  Also, if vm_fault gets a protection
	 * failure it is due to accessing the stack region outside
	 * the current limit and we need to reflect that as an access
	 * error.
	 */
	if (rv == 0 && (caddr_t)va >= vm->vm_maxsaddr)
		uvm_grow(p, va);
	if (rv != 0) {
		/*
		 * Pagein failed.  If doing copyin/out, return to onfault
		 * address.  Any other page fault in kernel, die; if user
		 * fault, deliver SIGSEGV.
		 */
fault:
		if (psr & PSR_PS) {
kfault:
			onfault = l->l_addr ?
			    (int)l->l_addr->u_pcb.pcb_onfault : 0;
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
			tf->tf_out[0] = (rv == EACCES) ? EFAULT : rv;
			KERNEL_UNLOCK();
			return;
		}
		KSI_INIT_TRAP(&ksi);
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = SI_NOINFO;
		} else {
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = (rv == EACCES)
				? SEGV_ACCERR : SEGV_MAPERR;
		}
		ksi.ksi_errno = rv;
		ksi.ksi_trap = type;
		ksi.ksi_addr = (void *)sfva;
		trapsignal(l, &ksi);
	}
out:
	if ((psr & PSR_PS) == 0) {
		l->l_flag &= ~L_SA_PAGEFAULT;
		KERNEL_PROC_UNLOCK(l);
out_nounlock:
		userret(l, pc, sticks);
		share_fpu(l, tf);
	}
	else
		KERNEL_UNLOCK();
}
#endif /* SUN4M */
