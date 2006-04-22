/*	$NetBSD: sh3_machdep.c,v 1.58.4.1 2006/04/22 11:37:56 simonb Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sh3_machdep.c,v 1.58.4.1 2006/04/22 11:37:56 simonb Exp $");

#include "opt_kgdb.h"
#include "opt_memsize.h"
#include "opt_compat_netbsd.h"
#include "opt_kstack_debug.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>
#include <sys/ucontext.h>
#include <sys/user.h>

#ifdef KGDB
#include <sys/kgdb.h>
#ifndef KGDB_DEVNAME
#define	KGDB_DEVNAME "nodev"
#endif
const char kgdb_devname[] = KGDB_DEVNAME;
#endif /* KGDB */

#include <uvm/uvm_extern.h>

#include <sh3/cache.h>
#include <sh3/clock.h>
#include <sh3/exception.h>
#include <sh3/locore.h>
#include <sh3/mmu.h>
#include <sh3/intr.h>

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;
int cpu_arch;
int cpu_product;
char cpu_model[120];

struct vm_map *exec_map;
struct vm_map *mb_map;
struct vm_map *phys_map;

int physmem;
struct user *proc0paddr;	/* init_main.c use this. */
struct pcb *curpcb;
struct md_upte *curupte;	/* SH3 wired u-area hack */

#if !defined(IOM_RAM_BEGIN)
#error "define IOM_RAM_BEGIN"
#elif (IOM_RAM_BEGIN & SH3_P1SEG_BASE) != 0
#error "IOM_RAM_BEGIN is physical address. not P1 address."
#endif

#define	VBR	(uint8_t *)SH3_PHYS_TO_P1SEG(IOM_RAM_BEGIN)
vaddr_t ram_start = SH3_PHYS_TO_P1SEG(IOM_RAM_BEGIN);
/* exception handler holder (sh3/sh3/exception_vector.S) */
extern char sh_vector_generic[], sh_vector_generic_end[];
extern char sh_vector_interrupt[], sh_vector_interrupt_end[];
#ifdef SH3
extern char sh3_vector_tlbmiss[], sh3_vector_tlbmiss_end[];
#endif
#ifdef SH4
extern char sh4_vector_tlbmiss[], sh4_vector_tlbmiss_end[];
#endif
/*
 * These variables are needed by /sbin/savecore
 */
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int dumpsize;			/* pages */
long dumplo;	 		/* blocks */

void
sh_cpu_init(int arch, int product)
{
	/* CPU type */
	cpu_arch = arch;
	cpu_product = product;

#if defined(SH3) && defined(SH4)
	/* Set register addresses */
	sh_devreg_init();
#endif
	/* Cache access ops. */
	sh_cache_init();

	/* MMU access ops. */
	sh_mmu_init();

	/* Hardclock, RTC initialize. */
	machine_clock_init();

	/* ICU initiailze. */
	intc_init();

	/* Exception vector. */
	memcpy(VBR + 0x100, sh_vector_generic,
	    sh_vector_generic_end - sh_vector_generic);
#ifdef SH3
	if (CPU_IS_SH3)
		memcpy(VBR + 0x400, sh3_vector_tlbmiss,
		    sh3_vector_tlbmiss_end - sh3_vector_tlbmiss);
#endif
#ifdef SH4
	if (CPU_IS_SH4)
		memcpy(VBR + 0x400, sh4_vector_tlbmiss,
		    sh4_vector_tlbmiss_end - sh4_vector_tlbmiss);
#endif
	memcpy(VBR + 0x600, sh_vector_interrupt,
	    sh_vector_interrupt_end - sh_vector_interrupt);

	if (!SH_HAS_UNIFIED_CACHE)
		sh_icache_sync_all();

	__asm volatile("ldc %0, vbr" :: "r"(VBR));

	/* kernel stack setup */
	__sh_switch_resume = CPU_IS_SH3 ? sh3_switch_resume : sh4_switch_resume;

	/* Set page size (4KB) */
	uvm_setpagesize();
}

/*
 * void sh_proc0_init(void):
 *	Setup proc0 u-area.
 */
void
sh_proc0_init()
{
	struct switchframe *sf;
	vaddr_t u;

	/* Steal process0 u-area */
	u = uvm_pageboot_alloc(USPACE);
	memset((void *)u, 0, USPACE);

	/* Setup proc0 */
	proc0paddr = (struct user *)u;
	lwp0.l_addr = proc0paddr;
	/*
	 * u-area map:
	 * |user| .... | .................. |
	 * | PAGE_SIZE | USPACE - PAGE_SIZE |
         *        frame top        stack top
	 * current frame ... r6_bank
	 * stack top     ... r7_bank
	 * current stack ... r15
	 */
	curpcb = lwp0.l_md.md_pcb = &lwp0.l_addr->u_pcb;
	curupte = lwp0.l_md.md_upte;

	sf = &curpcb->pcb_sf;
	sf->sf_r6_bank = u + PAGE_SIZE;
	sf->sf_r7_bank = sf->sf_r15	= u + USPACE;
	__asm volatile("ldc %0, r6_bank" :: "r"(sf->sf_r6_bank));
	__asm volatile("ldc %0, r7_bank" :: "r"(sf->sf_r7_bank));

	lwp0.l_md.md_regs = (struct trapframe *)sf->sf_r6_bank - 1;
#ifdef KSTACK_DEBUG
	memset((char *)(u + sizeof(struct user)), 0x5a,
	    PAGE_SIZE - sizeof(struct user));
	memset((char *)(u + PAGE_SIZE), 0xa5, USPACE - PAGE_SIZE);
#endif /* KSTACK_DEBUG */
}

void
sh_startup()
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	printf("%s%s", copyright, version);
	if (*cpu_model != '\0')
		printf("%s", cpu_model);
#ifdef DEBUG
	printf("general exception handler:\t%d byte\n",
	    sh_vector_generic_end - sh_vector_generic);
	printf("TLB miss exception handler:\t%d byte\n",
#if defined(SH3) && defined(SH4)
	    CPU_IS_SH3 ? sh3_vector_tlbmiss_end - sh3_vector_tlbmiss :
	    sh4_vector_tlbmiss_end - sh4_vector_tlbmiss
#elif defined(SH3)
	    sh3_vector_tlbmiss_end - sh3_vector_tlbmiss
#elif defined(SH4)
	    sh4_vector_tlbmiss_end - sh4_vector_tlbmiss
#endif
	    );
	printf("interrupt exception handler:\t%d byte\n",
	    sh_vector_interrupt_end - sh_vector_interrupt);
#endif /* DEBUG */

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
}

void
dumpsys()
{
}

/*
 * void cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
 *     void *sas, void *ap, void *sp, sa_upcall_t upcall):
 *
 * Send an upcall to userland.
 */
void
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted, void *sas,
    void *ap, void *sp, sa_upcall_t upcall)
{
	struct trapframe *tf;
	struct saframe *sf, frame;

	tf = l->l_md.md_regs;

	/* Build the stack frame. */
#if 0 /* First 4 args in regs (see below). */
	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
#endif
	frame.sa_arg = ap;

	sf = (struct saframe *)sp - 1;
	if (copyout(&frame, sf, sizeof(frame)) != 0) {
		/* Copying onto the stack didn't work.  Die. */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_r4 = type;
	tf->tf_r5 = (int) sas;
	tf->tf_r6 = nevents;
	tf->tf_r7 = ninterrupted;

	tf->tf_spc = (int) upcall;
	tf->tf_pr = 0;		/* no return */
	tf->tf_r15 = (int) sf;
}

/*
 * Get the base address of the signal frame either on the lwp's stack
 * or on the signal stack and set *onstack accordingly.  Caller then
 * just subtracts the size of appropriate struct sigframe_foo.
 */
static void *
getframe(struct lwp *l, int sig, int *onstack)
{
	struct proc *p = l->l_proc;
	struct sigaltstack *sigstk= &p->p_sigctx.ps_sigstk;

	/* Do we need to jump onto the signal stack? */
	*onstack = (sigstk->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
		&& (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (*onstack)
		return ((char *)sigstk->ss_sp + sigstk->ss_size);
	else
		return ((void *)l->l_md.md_regs->tf_r15);
}

#ifdef COMPAT_16
/*
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
static void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf = l->l_md.md_regs;
	int sig = ksi->ksi_info._signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigframe_sigcontext *fp, frame;
	int onstack;

	fp = getframe(l, sig, &onstack);
	--fp;

	/* Save register context. */
	frame.sf_sc.sc_ssr = tf->tf_ssr;
	frame.sf_sc.sc_spc = tf->tf_spc;
	frame.sf_sc.sc_pr = tf->tf_pr;
	frame.sf_sc.sc_r15 = tf->tf_r15;
	frame.sf_sc.sc_r14 = tf->tf_r14;
	frame.sf_sc.sc_r13 = tf->tf_r13;
	frame.sf_sc.sc_r12 = tf->tf_r12;
	frame.sf_sc.sc_r11 = tf->tf_r11;
	frame.sf_sc.sc_r10 = tf->tf_r10;
	frame.sf_sc.sc_r9 = tf->tf_r9;
	frame.sf_sc.sc_r8 = tf->tf_r8;
	frame.sf_sc.sc_r7 = tf->tf_r7;
	frame.sf_sc.sc_r6 = tf->tf_r6;
	frame.sf_sc.sc_r5 = tf->tf_r5;
	frame.sf_sc.sc_r4 = tf->tf_r4;
	frame.sf_sc.sc_r3 = tf->tf_r3;
	frame.sf_sc.sc_r2 = tf->tf_r2;
	frame.sf_sc.sc_r1 = tf->tf_r1;
	frame.sf_sc.sc_r0 = tf->tf_r0;
	frame.sf_sc.sc_expevt = tf->tf_expevt;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.  We invoke the handler
	 * directly, only returning via the trampoline.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		tf->tf_pr = (int)p->p_sigctx.ps_sigcode;
		break;

	case 1:
		tf->tf_pr = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		printf("sendsig_sigcontext: bad version %d\n",
		       ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	}

	tf->tf_r4 = sig;
	tf->tf_r5 = ksi->ksi_code;
	tf->tf_r6 = (int)&fp->sf_sc;
 	tf->tf_spc = (int)catcher;
	tf->tf_r15 = (int)fp;

	/* Remember if we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}
#endif /* COMPAT_16 */

static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf = l->l_md.md_regs;
	int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigframe_siginfo *fp, frame;
	int onstack;

	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* FALLTHROUGH */ /* handled by sendsig_sigcontext */
	case 1:		/* FALLTHROUGH */ /* handled by sendsig_sigcontext */
	default:	/* unknown version */
		printf("sendsig_siginfo: bad version %d\n",
		       ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
		/* NOTREACHED */
	case 2:
		break;
	}

	fp = getframe(l, sig, &onstack);
	--fp;

	frame.sf_si._info = ksi->ksi_info;
	frame.sf_uc.uc_link = NULL;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_flags = _UC_SIGMASK;
	frame.sf_uc.uc_flags |= (p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
		? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));
	cpu_getmcontext(l, &frame.sf_uc.uc_mcontext, &frame.sf_uc.uc_flags);

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_r4 = sig;		/* "signum" argument for handler */
	tf->tf_r5 = (int)&fp->sf_si;	/* "sip" argument for handler */
	tf->tf_r6 = (int)&fp->sf_uc;	/* "ucp" argument for handler */
 	tf->tf_spc = (int)catcher;
	tf->tf_r15 = (int)fp;
	tf->tf_pr = (int)ps->sa_sigdesc[sig].sd_tramp;

	/* Remember if we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * Send an interrupt to process.
 */
void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
#ifdef COMPAT_16
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else
#endif
		sendsig_siginfo(ksi, mask);
}

#ifdef COMPAT_16
/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
compat_16_sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
	struct trapframe *tf;
	struct proc *p = l->l_proc;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore signal context. */
	tf = l->l_md.md_regs;

	/* Check for security violations. */
	if (((context.sc_ssr ^ tf->tf_ssr) & PSL_USERSTATIC) != 0)
		return (EINVAL);

	tf->tf_ssr = context.sc_ssr;

	tf->tf_r0 = context.sc_r0;
	tf->tf_r1 = context.sc_r1;
	tf->tf_r2 = context.sc_r2;
	tf->tf_r3 = context.sc_r3;
	tf->tf_r4 = context.sc_r4;
	tf->tf_r5 = context.sc_r5;
	tf->tf_r6 = context.sc_r6;
	tf->tf_r7 = context.sc_r7;
	tf->tf_r8 = context.sc_r8;
	tf->tf_r9 = context.sc_r9;
	tf->tf_r10 = context.sc_r10;
	tf->tf_r11 = context.sc_r11;
	tf->tf_r12 = context.sc_r12;
	tf->tf_r13 = context.sc_r13;
	tf->tf_r14 = context.sc_r14;
	tf->tf_spc = context.sc_spc;
	tf->tf_r15 = context.sc_r15;
	tf->tf_pr = context.sc_pr;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return (EJUSTRETURN);
}
#endif /* COMPAT_16 */

void
cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flags;
{
	const struct trapframe *tf = l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;

	/* Save register context. */
	gr[_REG_EXPEVT] = tf->tf_expevt;
	gr[_REG_PC]     = tf->tf_spc;
	gr[_REG_SR]     = tf->tf_ssr;
	gr[_REG_MACL]   = tf->tf_macl;
	gr[_REG_MACH]   = tf->tf_mach;
	gr[_REG_PR]     = tf->tf_pr;
	gr[_REG_R14]    = tf->tf_r14;
	gr[_REG_R13]    = tf->tf_r13;
	gr[_REG_R12]    = tf->tf_r12;
	gr[_REG_R11]    = tf->tf_r11;
	gr[_REG_R10]    = tf->tf_r10;
	gr[_REG_R9]     = tf->tf_r9;
	gr[_REG_R8]     = tf->tf_r8;
	gr[_REG_R7]     = tf->tf_r7;
	gr[_REG_R6]     = tf->tf_r6;
	gr[_REG_R5]     = tf->tf_r5;
	gr[_REG_R4]     = tf->tf_r4;
	gr[_REG_R3]     = tf->tf_r3;
	gr[_REG_R2]     = tf->tf_r2;
	gr[_REG_R1]     = tf->tf_r1;
	gr[_REG_R0]     = tf->tf_r0;
	gr[_REG_R15]    = tf->tf_r15;

	if ((ras_pc = (__greg_t)ras_lookup(l->l_proc,
	    (caddr_t) gr[_REG_PC])) != -1)
		gr[_REG_PC] = ras_pc;

	*flags |= _UC_CPU;

	/* FPU context is currently not handled by the kernel. */
	memset(&mcp->__fpregs, 0, sizeof (mcp->__fpregs));
}

int
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct trapframe *tf = l->l_md.md_regs;
	const __greg_t *gr = mcp->__gregs;

	/* Restore register context, if any. */
	if ((flags & _UC_CPU) != 0) {
		/* Check for security violations. */
		if (((tf->tf_ssr ^ gr[_REG_SR]) & PSL_USERSTATIC) != 0)
			return (EINVAL);
	
		/* _REG_EXPEVT not restored */
		tf->tf_spc    = gr[_REG_PC];
		tf->tf_ssr    = gr[_REG_SR];
		tf->tf_macl   = gr[_REG_MACL];
		tf->tf_mach   = gr[_REG_MACH];
		tf->tf_pr     = gr[_REG_PR];
		tf->tf_r14    = gr[_REG_R14];
		tf->tf_r13    = gr[_REG_R13];
		tf->tf_r12    = gr[_REG_R12];
		tf->tf_r11    = gr[_REG_R11];
		tf->tf_r10    = gr[_REG_R10];
		tf->tf_r9     = gr[_REG_R9];
		tf->tf_r8     = gr[_REG_R8];
		tf->tf_r7     = gr[_REG_R7];
		tf->tf_r6     = gr[_REG_R6];
		tf->tf_r5     = gr[_REG_R5];
		tf->tf_r4     = gr[_REG_R4];
		tf->tf_r3     = gr[_REG_R3];
		tf->tf_r2     = gr[_REG_R2];
		tf->tf_r1     = gr[_REG_R1];
		tf->tf_r0     = gr[_REG_R0];
		tf->tf_r15    = gr[_REG_R15];
	}

#if 0
	/* XXX: FPU context is currently not handled by the kernel. */
	if (flags & _UC_FPU) {
		/* TODO */;
	}
#endif

	if (flags & _UC_SETSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	return (0);
}

/*
 * Clear registers on exec
 */
void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf;

	l->l_md.md_flags &= ~MDP_USEDFPU;

	tf = l->l_md.md_regs;

	tf->tf_r0 = 0;
	tf->tf_r1 = 0;
	tf->tf_r2 = 0;
	tf->tf_r3 = 0;
	tf->tf_r4 = fuword((caddr_t)stack);	/* argc */
	tf->tf_r5 = stack + 4;			/* argv */
	tf->tf_r6 = stack + 4 * tf->tf_r4 + 8;	/* envp */
	tf->tf_r7 = 0;
	tf->tf_r8 = 0;
	tf->tf_r9 = (int)l->l_proc->p_psstr;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;
	tf->tf_r12 = 0;
	tf->tf_r13 = 0;
	tf->tf_r14 = 0;
	tf->tf_spc = pack->ep_entry;
	tf->tf_ssr = PSL_USERSET;
	tf->tf_r15 = stack;
}

/*
 * Jump to reset vector.
 */
void
cpu_reset()
{

	_cpu_exception_suspend();
	_reg_write_4(SH_(EXPEVT), EXPEVT_RESET_MANUAL);

#ifndef __lint__
	goto *(void *)0xa0000000;
#endif
	/* NOTREACHED */
}
