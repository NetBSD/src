/*	$NetBSD: svr4_machdep.c,v 1.67.4.1 2009/05/13 17:18:37 jym Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_machdep.c,v 1.67.4.1 2009/05/13 17:18:37 jym Exp $");

#if defined(_KERNEL_OPT)
#include "opt_kgdb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/user.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/exec_elf.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/vmparam.h>
#include <machine/svr4_machdep.h>

static void svr4_getsiginfo(union svr4_siginfo *, int, u_long, void *);

void
svr4_setregs(struct lwp *l, struct exec_package *epp, u_long stack)
{

	setregs(l, epp, stack);
}

#ifdef DEBUG
extern int sigdebug;
extern int sigpid;
#define SDB_FOLLOW	0x01	/* XXX: dup from machdep.c */
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

#ifdef DEBUG_SVR4
static void svr4_printmcontext(const char *, struct svr4_mcontext *);

static void
svr4_printmcontext(const char *fun, struct svr4_mcontext *mc)
{
	svr4_greg_t *r = mc->greg;

	printf("%s at %p\n", fun, mc);

	printf("Regs: ");
	printf("PSR = 0x%x ", r[SVR4_SPARC_PSR]);
	printf("PC = 0x%x ",  r[SVR4_SPARC_PC]);
	printf("nPC = 0x%x ", r[SVR4_SPARC_nPC]);
	printf("Y = 0x%x ",   r[SVR4_SPARC_Y]);
	printf("G1 = 0x%x ",  r[SVR4_SPARC_G1]);
	printf("G2 = 0x%x ",  r[SVR4_SPARC_G2]);
	printf("G3 = 0x%x ",  r[SVR4_SPARC_G3]);
	printf("G4 = 0x%x ",  r[SVR4_SPARC_G4]);
	printf("G5 = 0x%x ",  r[SVR4_SPARC_G5]);
	printf("G6 = 0x%x ",  r[SVR4_SPARC_G6]);
	printf("G7 = 0x%x ",  r[SVR4_SPARC_G7]);
	printf("O0 = 0x%x ",  r[SVR4_SPARC_O0]);
	printf("O1 = 0x%x ",  r[SVR4_SPARC_O1]);
	printf("O2 = 0x%x ",  r[SVR4_SPARC_O2]);
	printf("O3 = 0x%x ",  r[SVR4_SPARC_O3]);
	printf("O4 = 0x%x ",  r[SVR4_SPARC_O4]);
	printf("O5 = 0x%x ",  r[SVR4_SPARC_O5]);
	printf("O6 = 0x%x ",  r[SVR4_SPARC_O6]);
	printf("O7 = 0x%x ",  r[SVR4_SPARC_O7]);
	printf("\n");
}
#endif

void *
svr4_getmcontext(struct lwp *l, struct svr4_mcontext *mc, u_long *flags)
{
	struct trapframe *tf = (struct trapframe *)l->l_md.md_tf;
	svr4_greg_t *r = mc->greg;
#ifdef FPU_CONTEXT
	svr4_fregset_t *f = &mc->freg;
	struct fpstate *fps = l->l_md.md_fpstate;
#endif

	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(l->l_proc->p_lock);
		sigexit(l, SIGILL);
	}

	/*
	 * Get the general purpose registers
	 */
	r[SVR4_SPARC_PSR] = tf->tf_psr;
	r[SVR4_SPARC_PC] = tf->tf_pc;
	r[SVR4_SPARC_nPC] = tf->tf_npc;
	r[SVR4_SPARC_Y] = tf->tf_y;
	r[SVR4_SPARC_G1] = tf->tf_global[1];
	r[SVR4_SPARC_G2] = tf->tf_global[2];
	r[SVR4_SPARC_G3] = tf->tf_global[3];
	r[SVR4_SPARC_G4] = tf->tf_global[4];
	r[SVR4_SPARC_G5] = tf->tf_global[5];
	r[SVR4_SPARC_G6] = tf->tf_global[6];
	r[SVR4_SPARC_G7] = tf->tf_global[7];
	r[SVR4_SPARC_O0] = tf->tf_out[0];
	r[SVR4_SPARC_O1] = tf->tf_out[1];
	r[SVR4_SPARC_O2] = tf->tf_out[2];
	r[SVR4_SPARC_O3] = tf->tf_out[3];
	r[SVR4_SPARC_O4] = tf->tf_out[4];
	r[SVR4_SPARC_O5] = tf->tf_out[5];
	r[SVR4_SPARC_O6] = tf->tf_out[6];
	r[SVR4_SPARC_O7] = tf->tf_out[7];

	*flags |= SVR4_UC_CPU;

#ifdef FPU_CONTEXT
	/*
	 * Get the floating point registers
	 */
	memcpy( f->fpu_regs, fps->fs_regs, sizeof(fps->fs_regs));
	f->fp_nqsize = sizeof(struct fp_qentry);
	f->fp_nqel = fps->fs_qsize;
	f->fp_fsr = fps->fs_fsr;
	if (f->fp_q != NULL) {
		size_t sz = f->fp_nqel * f->fp_nqsize;
		if (sz > sizeof(fps->fs_queue)) {
#ifdef DIAGNOSTIC
			printf("getcontext: fp_queue too large\n");
#endif
			return;
		}
		if (copyout(fps->fs_queue, f->fp_q, sz) != 0) {
#ifdef DIAGNOSTIC
			printf("getcontext: copy of fp_queue failed %d\n",
			    error);
#endif
			return;
		}
	}
	f->fp_busy = 0;	/* XXX: How do we determine that? */
	*flags |= SVR4_UC_FPU;
#endif


#ifdef DEBUG_SVR4
	svr4_printmcontext("getmcontext", mc);
#endif
	return (void *)tf->tf_out[6];
}


/*
 * Set to mcontext specified.
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 * This is almost like sigreturn() and it shows.
 */
int
svr4_setmcontext(struct lwp *l, struct svr4_mcontext *mc, u_long flags)
{
	register struct trapframe *tf;
	svr4_greg_t *r = mc->greg;
#ifdef FPU_CONTEXT
	svr4_fregset_t *f = &mc->freg;
	struct fpstate *fps = l->l_md.md_fpstate;
#endif

#ifdef DEBUG_SVR4
	svr4_printmcontext("setmcontext", mc);
#endif

	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(l->l_proc->p_lock);
		sigexit(l, SIGILL);
	}

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("svr4_setmcontext: %s[%d], svr4_mcontext %p\n",
		    l->l_proc->p_comm, l->l_proc->p_pid, mc);
#endif

	if (flags & SVR4_UC_CPU) {
		/* Restore register context. */
		tf = (struct trapframe *)l->l_md.md_tf;

		/*
		 * Only the icc bits in the psr are used, so it need not be
		 * verified.  pc and npc must be multiples of 4.  This is all
		 * that is required; if it holds, just do it.
		 */
		if (((r[SVR4_SPARC_PC] | r[SVR4_SPARC_nPC]) & 3) != 0) {
			printf("pc or npc are not multiples of 4!\n");
			return EINVAL;
		}

		/* take only psr ICC field */
		tf->tf_psr = (tf->tf_psr & ~PSR_ICC) |
		    (r[SVR4_SPARC_PSR] & PSR_ICC);
		tf->tf_pc = r[SVR4_SPARC_PC];
		tf->tf_npc = r[SVR4_SPARC_nPC];
		tf->tf_y = r[SVR4_SPARC_Y];

		/* Restore everything */
		tf->tf_global[1] = r[SVR4_SPARC_G1];
		tf->tf_global[2] = r[SVR4_SPARC_G2];
		tf->tf_global[3] = r[SVR4_SPARC_G3];
		tf->tf_global[4] = r[SVR4_SPARC_G4];
		tf->tf_global[5] = r[SVR4_SPARC_G5];
		tf->tf_global[6] = r[SVR4_SPARC_G6];
		tf->tf_global[7] = r[SVR4_SPARC_G7];

		tf->tf_out[0] = r[SVR4_SPARC_O0];
		tf->tf_out[1] = r[SVR4_SPARC_O1];
		tf->tf_out[2] = r[SVR4_SPARC_O2];
		tf->tf_out[3] = r[SVR4_SPARC_O3];
		tf->tf_out[4] = r[SVR4_SPARC_O4];
		tf->tf_out[5] = r[SVR4_SPARC_O5];
		tf->tf_out[6] = r[SVR4_SPARC_O6];
		tf->tf_out[7] = r[SVR4_SPARC_O7];
	}


#ifdef FPU_CONTEXT
	if (flags & SVR4_UC_FPU) {
		/*
		 * Set the floating point registers
		 */
		int error;
		size_t sz = f->fp_nqel * f->fp_nqsize;
		if (sz > sizeof(fps->fs_queue)) {
#ifdef DIAGNOSTIC
			printf("setmcontext: fp_queue too large\n");
#endif
			return EINVAL;
		}
		memcpy( fps->fs_regs, f->fpu_regs, sizeof(fps->fs_regs));
		fps->fs_qsize = f->fp_nqel;
		fps->fs_fsr = f->fp_fsr;
		if (f->fp_q != NULL) {
			if ((error = copyin(f->fp_q, fps->fs_queue,
					    f->fp_nqel * f->fp_nqsize)) != 0) {
#ifdef DIAGNOSTIC
				printf("setmcontext: fp_queue copy failed\n");
#endif
				return error;
			}
		}
	}
#endif

	return 0;
}

/*
 * map the trap code into the svr4 siginfo as best we can
 */
static void
svr4_getsiginfo(union svr4_siginfo *si, int sig, u_long code, void *addr)
{

	si->si_signo = native_to_svr4_signo[sig];
	si->si_errno = 0;
	si->si_addr  = addr;
	/*
	 * we can do this direct map as they are the same as all sparc
	 * architectures.
	 */
	si->si_trap = code;
	switch (code) {
	case T_RESET:
		si->si_code = 0;
		break;

	case T_TEXTFAULT:
		si->si_code = SVR4_BUS_ADRALN;
		break;

	case T_ILLINST:
		si->si_code = SVR4_ILL_ILLOPC;
		break;

	case T_PRIVINST:
		si->si_code = SVR4_ILL_PRVOPC;
		break;

	case T_FPDISABLED:
		si->si_code = SVR4_FPE_FLTINV;
		break;

	case T_ALIGN:
		si->si_code = SVR4_BUS_ADRALN;
		break;

	case T_FPE:
		si->si_code = SVR4_FPE_FLTINV;
		break;

	case T_DATAFAULT:
		si->si_code = SVR4_BUS_ADRALN;
		break;

	case T_TAGOF:
		si->si_code = SVR4_EMT_TAGOVF;
		break;

	case T_CPDISABLED:
		si->si_code = SVR4_FPE_FLTINV;
		break;

	case T_CPEXCEPTION:
		si->si_code = SVR4_FPE_FLTINV;
		break;

	case T_DIV0:
		si->si_code = SVR4_FPE_INTDIV;
		break;

	case T_INTOF:
		si->si_code = SVR4_FPE_INTOVF;
		break;

	case T_BREAKPOINT:
		si->si_code = SVR4_TRAP_BRKPT;
		break;

	/*
	 * XXX - hardware traps with unknown code
	 */
	case T_WINOF:
	case T_WINUF:
	case T_L1INT:
	case T_L2INT:
	case T_L3INT:
	case T_L4INT:
	case T_L5INT:
	case T_L6INT:
	case T_L7INT:
	case T_L8INT:
	case T_L9INT:
	case T_L10INT:
	case T_L11INT:
	case T_L12INT:
	case T_L13INT:
	case T_L14INT:
	case T_L15INT:
		si->si_code = 0;
		break;

	/*
	 * XXX - software traps with unknown code
	 */
	case T_SUN_SYSCALL:
	case T_FLUSHWIN:
	case T_CLEANWIN:
	case T_RANGECHECK:
	case T_FIXALIGN:
	case T_SVR4_SYSCALL:
	case T_BSD_SYSCALL:
	case T_KGDB_EXEC:
		si->si_code = 0;
		break;

	default:
		si->si_code = 0;
#ifdef notyet
		/*
		 * XXX: in trap.c, code gets passed the address
		 * of the fault! not the trap code on SEGV!
		 */
#ifdef DIAGNOSTIC
		printf("sig %d code %ld\n", sig, code);
		panic("svr4_getsiginfo");
#endif
#endif
		break;
	}
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine. After the handler is
 * done svr4 will call setcontext for us
 * with the user context we just set up, and we
 * will return to the user pc, psl.
 */
void
svr4_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	register struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	register struct trapframe *tf;
	struct svr4_sigframe *fp, frame;
	int onstack, oldsp, newsp, addr, error;
	int sig = ksi->ksi_signo;
	u_long code = ksi->ksi_code;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = (struct trapframe *)l->l_md.md_tf;
	oldsp = tf->tf_out[6];

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/*
	 * Allocate space for the signal handler context.
	 */
	if (onstack)
		fp = (struct svr4_sigframe *)((char *)l->l_sigstk.ss_sp +
						l->l_sigstk.ss_size);
	else
		fp = (struct svr4_sigframe *)oldsp;
	fp = (struct svr4_sigframe *) ((int) (fp - 1) & ~7);

	/*
	 * Build the argument list for the signal handler.
	 */
	svr4_getcontext(l, &frame.sf_uc);
	svr4_getsiginfo(&frame.sf_si, sig, code, (void *) tf->tf_pc);

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = frame.sf_si.si_signo;
	frame.sf_sip = &fp->sf_si;
	frame.sf_ucp = &fp->sf_uc;
	frame.sf_handler = catcher;

	DPRINTF(("svr4_sendsig signum=%d si = %p uc = %p handler = %p\n",
	         frame.sf_signum, frame.sf_sip,
		 frame.sf_ucp, frame.sf_handler));
	/*
	 * Modify the signal context to be used by sigreturn.
	 */
	sendsig_reset(l, sig);
	mutex_exit(p->p_lock);
	frame.sf_uc.uc_mcontext.greg[SVR4_SPARC_SP] = oldsp;
	newsp = (int)fp - sizeof(struct rwindow);
	write_user_windows();
	error = (rwindow_save(l) || copyout(&frame, fp, sizeof(frame)) != 0 ||
	    suword(&((struct rwindow *)newsp)->rw_in[6], oldsp));
	mutex_enter(p->p_lock);

	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("svr4_sendsig: window save or copyout error\n");
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	addr = (int)p->p_sigctx.ps_sigcode;
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_global[1] = (int)catcher;
	tf->tf_out[6] = newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}


#define	ADVANCE (n = tf->tf_npc, tf->tf_pc = n, tf->tf_npc = n + 4)

int
svr4_trap(int type, struct lwp *l)
{
	int n;
	struct trapframe *tf = l->l_md.md_tf;
	struct timespec ts;
	struct timeval tv;
	struct timeval rtime, stime;
	uint64_t tm;

	if (l->l_proc->p_emul != &emul_svr4)
		return 0;

	switch (type) {
	case T_SVR4_GETCC:
		uprintf("T_SVR4_GETCC\n");
		break;

	case T_SVR4_SETCC:
		uprintf("T_SVR4_SETCC\n");
		break;

	case T_SVR4_GETPSR:
		tf->tf_out[0] = tf->tf_psr;
		break;

	case T_SVR4_SETPSR:
		/* Disable for now; it makes things worse */
#if 0
		/* I have no clue if this is right!  */
#define PRESERVE_PSR	(PSR_IMPL|PSR_VER|PSR_PIL|PSR_S|PSR_PS|PSR_ET|PSR_CWP)
		tf->tf_psr = (tf->tf_psr & ~PRESERVE_PSR) |
		    (tf->tf_out[0] & PRESERVE_PSR);
#endif
		break;

	case T_SVR4_GETHRTIME:
		/*
		 * This is like gethrtime(3), returning the time expressed
		 * in nanoseconds since an arbitrary time in the past and
		 * guaranteed to be monotonically increasing, which we
		 * obtain from nanotime(9).
		 */
		nanouptime(&ts);

		tm = ts.tv_nsec;
		tm += ts.tv_sec * (uint64_t)1000000000u;
		tf->tf_out[0] = (tm >> 32) & 0x00000000ffffffffUL;
		tf->tf_out[1] = tm & 0x00000000ffffffffUL;
		break;

	case T_SVR4_GETHRVTIME:
		/*
		 * This is like gethrvtime(3). returning the LWP's (now:
		 * proc's) virtual time expressed in nanoseconds. It is
		 * supposedly guaranteed to be monotonically increasing, but
		 * for now using the process's real time augmented with its
		 * current runtime is the best we can do.
		 */
		microtime(&tv); /* XXX should move on to struct bintime */
		bintime2timeval(&l->l_rtime, &rtime);
		bintime2timeval(&l->l_stime, &stime);

		tm = (rtime.tv_sec + tv.tv_sec - stime.tv_sec) * 1000000ull;
		tm += rtime.tv_usec + tv.tv_usec;
		tm -= stime.tv_usec;
		tm *= 1000u;
		tf->tf_out[0] = (tm >> 32) & 0x00000000ffffffffUL;
		tf->tf_out[1] = tm & 0x00000000ffffffffUL;
		break;

	case T_SVR4_GETHRESTIME:
		/*
		 * This is used by gettimeofday(3), among other things.
		 */
		nanotime(&ts);
		tf->tf_out[0] = ts.tv_sec;
		tf->tf_out[1] = ts.tv_nsec;
		break;

	default:
		return 0;
	}

	ADVANCE;
	return 1;
}

/*
 */
int
svr4_sys_sysarch(struct lwp *l, const struct svr4_sys_sysarch_args *uap, register_t *retval)
{

	switch (SCARG(uap, op)) {
	default:
		printf("(sparc) svr4_sysarch(%d)\n", SCARG(uap, op));
		return EINVAL;
	}
}

void
svr4_md_init(void)
{

}

void
svr4_md_fini(void)
{

}
