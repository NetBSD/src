/*	$NetBSD: machdep.c,v 1.246 2004/04/03 12:38:14 pk Exp $ */

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
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
 *	@(#)machdep.c	8.6 (Berkeley) 1/14/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.246 2004/04/03 12:38:14 pk Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_sparc_arch.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/extent.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/savar.h>
#include <sys/ucontext.h>

#include <uvm/uvm.h>		/* we use uvm.kernel_object */

#include <sys/sysctl.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/oldmon.h>
#include <machine/bsd_openprom.h>
#include <machine/bootinfo.h>

#include <sparc/sparc/asm.h>
#include <sparc/sparc/cache.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>

#include "fb.h"
#include "power.h"

#if NPOWER > 0
#include <sparc/dev/power.h>
#endif

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
extern paddr_t avail_end;

int	physmem;

struct simplelock fpulock = SIMPLELOCK_INITIALIZER;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int   safepri = 0;

/*
 * dvmamap24 is used to manage DVMA memory for devices that have the upper
 * eight address bits wired to all-ones (e.g. `le' and `ie')
 */
struct extent *dvmamap24;

void	dumpsys __P((void));
void	stackdump __P((void));

/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	paddr_t pa;
	char pbuf[9];

#ifdef DEBUG
	pmapdebug = 0;
#endif

	/*
	 * Re-map the message buffer from its temporary address
	 * at KERNBASE to MSGBUF_VA.
	 */
#if !defined(MSGBUFSIZE) || MSGBUFSIZE <= 8192
	/*
	 * We use the free page(s) in front of the kernel load address.
	 */
	size = 8192;

	/* Get physical address of the message buffer */
	pmap_extract(pmap_kernel(), (vaddr_t)KERNBASE, &pa);

	/* Invalidate the current mapping at KERNBASE. */
	pmap_kremove((vaddr_t)KERNBASE, size);
	pmap_update(pmap_kernel());

	/* Enter the new mapping */
	pmap_map(MSGBUF_VA, pa, pa + size, VM_PROT_READ|VM_PROT_WRITE);

	/*
	 * Re-initialize the message buffer.
	 */
	initmsgbuf((caddr_t)MSGBUF_VA, size);
#else /* MSGBUFSIZE */
	{
	struct pglist mlist;
	struct vm_page *m;
	vaddr_t va0, va;

	/*
	 * We use the free page(s) in front of the kernel load address,
	 * and then allocate some more.
	 */
	size = round_page(MSGBUFSIZE);

	/* Get physical address of first 8192 chunk of the message buffer */
	pmap_extract(pmap_kernel(), (vaddr_t)KERNBASE, &pa);

	/* Allocate additional physical pages */
	if (uvm_pglistalloc(size - 8192,
			    vm_first_phys, vm_first_phys+vm_num_phys,
			    0, 0, &mlist, 1, 0) != 0)
		panic("cpu_start: no memory for message buffer");

	/* Invalidate the current mapping at KERNBASE. */
	pmap_kremove((vaddr_t)KERNBASE, 8192);
	pmap_update(pmap_kernel());

	/* Allocate virtual memory space */
	va0 = va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		panic("cpu_start: no virtual memory for message buffer");

	/* Map first 8192 */
	while (va < va0 + 8192) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		pa += PAGE_SIZE;
		va += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	/* Map the rest of the pages */
	TAILQ_FOREACH(m, &mlist ,pageq) {
		if (va >= va0 + size)
			panic("cpu_start: memory buffer size botch");
		pa = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		va += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	/*
	 * Re-initialize the message buffer.
	 */
	initmsgbuf((caddr_t)va0, size);
	}
#endif /* MSGBUFSIZE */

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	/*identifycpu();*/
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Tune buffer cache variables based on the capabilities of the MMU
	 * to cut down on VM space allocated for the buffer caches that
	 * would lead to MMU resource shortage.
	 */
	if (CPU_ISSUN4C) {
		/* Clip UBC windows */
		if (cpuinfo.mmu_nsegment <= 128) {
			ubc_nwins = 256;
			/*ubc_winshift = 12; tune this too? */
		}

		/* Clip max data & stack to avoid running into the MMU hole */
#if MAXDSIZ > 256*1024*1024
		{ extern rlim_t maxdmap; maxdmap = 256*1024*1024; }
#endif
#if MAXSSIZ > 256*1024*1024
		{ extern rlim_t maxsmap; maxsmap = 256*1024*1024; }
#endif
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	minaddr = 0;
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		/*
		 * Allocate DMA map for 24-bit devices (le, ie)
		 * [dvma_base - dvma_end] is for VME devices..
		 */
		dvmamap24 = extent_create("dvmamap24",
					  D24_DVMA_BASE, D24_DVMA_END,
					  M_DEVBUF, 0, 0, EX_NOWAIT);
		if (dvmamap24 == NULL)
			panic("unable to allocate DVMA map");
	}

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
        mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, FALSE, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	pmap_redzone();
}

/*
 * Set up registers on exec.
 *
 * XXX this entire mess must be fixed
 */
/* ARGSUSED */
void
setregs(l, pack, stack)
	struct lwp *l;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = l->l_md.md_tf;
	struct fpstate *fs;
	int psr;

	/* Don't allow misaligned code by default */
	l->l_md.md_flags &= ~MDP_FIXALIGN;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%psr: (retain CWP and PSR_S bits)
	 *	%g1: address of p->p_psstr (used by crt0)
	 *	%pc,%npc: entry point of program
	 */
	psr = tf->tf_psr & (PSR_S | PSR_CWP);
	if ((fs = l->l_md.md_fpstate) != NULL) {
		struct cpu_info *cpi;
		int s;
		/*
		 * We hold an FPU state.  If we own *some* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		FPU_LOCK(s);
		if ((cpi = l->l_md.md_fpu) != NULL) {
			if (cpi->fplwp != l)
				panic("FPU(%d): fplwp %p",
					cpi->ci_cpuid, cpi->fplwp);
			if (l == cpuinfo.fplwp)
				savefpstate(fs);
#if defined(MULTIPROCESSOR)
			else
				XCALL1(savefpstate, fs, 1 << cpi->ci_cpuid);
#endif
			cpi->fplwp = NULL;
		}
		l->l_md.md_fpu = NULL;
		FPU_UNLOCK(s);
		free((void *)fs, M_SUBPROC);
		l->l_md.md_fpstate = NULL;
	}
	bzero((caddr_t)tf, sizeof *tf);
	tf->tf_psr = psr;
	tf->tf_global[1] = (int)l->l_proc->p_psstr;
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;
	stack -= sizeof(struct rwindow);
	tf->tf_out[6] = stack;
}

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_boot(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct btinfo_kernelfile *bi_file;
	char *cp;


	switch (node.sysctl_num) {
	case CPU_BOOTED_KERNEL:
		if ((bi_file = lookup_bootinfo(BTINFO_KERNELFILE)) != NULL)
			cp = bi_file->name;
		else
			cp = prom_getbootfile();
		if (cp != NULL && cp[0] == '\0')
			cp = "netbsd";
		break;
	case CPU_BOOTED_DEVICE:
		cp = prom_getbootpath();
		break;
	case CPU_BOOT_ARGS:
		cp = prom_getbootargs();
		break;
	default:
		return (EINVAL);
	}

	if (cp == NULL || cp[0] == '\0')
		return (ENOENT);

	node.sysctl_data = cp;
	node.sysctl_size = strlen(cp) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_device", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_DEVICE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "boot_args", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOT_ARGS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "cpu_arch", NULL,
		       NULL, 0, &cpu_arch, 0,
		       CTL_MACHDEP, CPU_ARCH, CTL_EOL);
}

/*
 * Send an interrupt to process.
 */
#ifdef COMPAT_16
struct sigframe_sigcontext {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	struct	sigcontext *sf_scp;	/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sigcontext sf_sc;	/* actual sigcontext */
};

static void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct sigframe_sigcontext *fp;
	struct trapframe *tf;
	int addr, onstack, oldsp, newsp;
	struct sigframe_sigcontext sf;
	int sig = ksi->ksi_signo;
	u_long code = KSI_TRAPCODE(ksi);
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = l->l_md.md_tf;
	oldsp = tf->tf_out[6];

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (onstack)
		fp = (struct sigframe_sigcontext *)
			((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
			 p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe_sigcontext *)oldsp;

	fp = (struct sigframe_sigcontext *)((int)(fp - 1) & ~7);

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: %s[%d] sig %d newusp %p scp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc);
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = code;
	sf.sf_scp = 0;
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;
	sf.sf_sc.sc_mask = *mask;
#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &sf.sf_sc.__sc_mask13);
#endif
	sf.sf_sc.sc_sp = oldsp;
	sf.sf_sc.sc_pc = tf->tf_pc;
	sf.sf_sc.sc_npc = tf->tf_npc;
	sf.sf_sc.sc_psr = tf->tf_psr;
	sf.sf_sc.sc_g1 = tf->tf_global[1];
	sf.sf_sc.sc_o0 = tf->tf_out[0];

	/*
	 * Put the stack in a consistent state before we whack away
	 * at it.  Note that write_user_windows may just dump the
	 * registers into the pcb; we need them in the process's memory.
	 * We also need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 */
	newsp = (int)fp - sizeof(struct rwindow);
	write_user_windows();
	if (rwindow_save(l) || copyout((caddr_t)&sf, (caddr_t)fp, sizeof sf) ||
	    suword(&((struct rwindow *)newsp)->rw_in[6], oldsp)) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig: window save or copyout error\n");
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		addr = (int)p->p_sigctx.ps_sigcode;
		break;

	case 1:
		addr = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		addr = 0;
		sigexit(l, SIGILL);
	}

	tf->tf_global[1] = (int)catcher;
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: about to return to catcher\n");
#endif
}
#endif /* COMPAT_16 */

struct sigframe {
	siginfo_t sf_si;
	ucontext_t sf_uc;
};

void sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf;
	ucontext_t uc;
	struct sigframe *fp;
	u_int onstack, oldsp, newsp;
	u_int catcher;
	int sig;
	size_t ucsz;

	sig = ksi->ksi_signo;

#ifdef COMPAT_16
	if (ps->sa_sigdesc[sig].sd_vers < 2) {
		sendsig_sigcontext(ksi, mask);
		return;
	}
#endif /* COMPAT_16 */

	tf = l->l_md.md_tf;
	oldsp = tf->tf_out[6];

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (onstack)
		fp = (struct sigframe *)
			((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
				  p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)oldsp;

	fp = (struct sigframe *)((int)(fp - 1) & ~7);

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: %s[%d] sig %d newusp %p si %p uc %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_si, &fp->sf_uc);
#endif

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	uc.uc_flags = _UC_SIGMASK |
		((p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
			? _UC_SETSTACK : _UC_CLRSTACK);
	uc.uc_sigmask = *mask;
	uc.uc_link = NULL;
	memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));
	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);
	ucsz = (int)&uc.__uc_pad - (int)&uc;

	/*
	 * Now copy the stack contents out to user space.
	 * We need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 * Since we're calling the handler directly, allocate a full size
	 * C stack frame.
	 */
	newsp = (int)fp - sizeof(struct frame);
	if (copyout(&ksi->ksi_info, &fp->sf_si, sizeof ksi->ksi_info) ||
	    copyout(&uc, &fp->sf_uc, ucsz) ||
	    suword(&((struct rwindow *)newsp)->rw_in[6], oldsp)) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig: window save or copyout error\n");
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	switch (ps->sa_sigdesc[sig].sd_vers) {
	default:
		/* Unsupported trampoline version; kill the process. */
		sigexit(l, SIGILL);
	case 2:
		/*
		 * Arrange to continue execution at the user's handler.
		 * It needs a new stack pointer, a return address and
		 * three arguments: (signo, siginfo *, ucontext *).
		 */
		catcher = (u_int)SIGACTION(p, sig).sa_handler;
		tf->tf_pc = catcher;
		tf->tf_npc = catcher + 4;
		tf->tf_out[0] = sig;
		tf->tf_out[1] = (int)&fp->sf_si;
		tf->tf_out[2] = (int)&fp->sf_uc;
		tf->tf_out[6] = newsp;
		tf->tf_out[7] = (int)ps->sa_sigdesc[sig].sd_tramp - 8;
		break;
	}

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: about to return to catcher\n");
#endif
}

#ifdef COMPAT_16
/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above),
 * and return to the given trap frame (if there is one).
 * Check carefully to make sure that the user has not
 * modified the state to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
compat_16_sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext sc, *scp;
	struct trapframe *tf;
	struct proc *p;
	int error;

	p = l->l_proc;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l))
		sigexit(l, SIGILL);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
#endif
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);
	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((scp->sc_pc | scp->sc_npc) & 3) != 0)
		return (EINVAL);

	/* take only psr ICC field */
	tf->tf_psr = (tf->tf_psr & ~PSR_ICC) | (scp->sc_psr & PSR_ICC);
	tf->tf_pc = scp->sc_pc;
	tf->tf_npc = scp->sc_npc;
	tf->tf_global[1] = scp->sc_g1;
	tf->tf_out[0] = scp->sc_o0;
	tf->tf_out[6] = scp->sc_sp;

	if (scp->sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask */
	(void) sigprocmask1(p, SIG_SETMASK, &scp->sc_mask, 0);

	return (EJUSTRETURN);
}
#endif /* COMPAT_16 */

/*
 * cpu_upcall:
 *
 *	Send an an upcall to userland.
 */
void
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
	   void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct trapframe *tf;
	vaddr_t addr;

	tf = l->l_md.md_tf;
	addr = (vaddr_t) upcall;

	/* Arguments to the upcall... */
	tf->tf_out[0] = type;
	tf->tf_out[1] = (vaddr_t) sas;
	tf->tf_out[2] = nevents;
	tf->tf_out[3] = ninterrupted;
	tf->tf_out[4] = (vaddr_t) ap;

	/*
	 * Ensure the stack is double-word aligned, and provide a
	 * C call frame.
	 */
	sp = (void *)(((vaddr_t)sp & ~0x7) - CCFSZ);

	/* Arrange to begin execution at the upcall handler. */
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (vaddr_t) sp;
	tf->tf_out[7] = -1;		/* "you lose" if upcall returns */
}

void
cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flags;
{
	struct trapframe *tf = (struct trapframe *)l->l_md.md_tf;
	__greg_t *r = mcp->__gregs;
#ifdef FPU_CONTEXT
	__fpregset_t *f = &mcp->__fpregs;
	struct fpstate *fps = l->l_md.md_fpstate;
#endif

	/*
	 * Put the stack in a consistent state before we whack away
	 * at it.  Note that write_user_windows may just dump the
	 * registers into the pcb; we need them in the process's memory.
	 */
	write_user_windows();
	if (rwindow_save(l))
		sigexit(l, SIGILL);

	/*
	 * Get the general purpose registers
	 */
	r[_REG_PSR] = tf->tf_psr;
	r[_REG_PC] = tf->tf_pc;
	r[_REG_nPC] = tf->tf_npc;
	r[_REG_Y] = tf->tf_y;
	r[_REG_G1] = tf->tf_global[1];
	r[_REG_G2] = tf->tf_global[2];
	r[_REG_G3] = tf->tf_global[3];
	r[_REG_G4] = tf->tf_global[4];
	r[_REG_G5] = tf->tf_global[5];
	r[_REG_G6] = tf->tf_global[6];
	r[_REG_G7] = tf->tf_global[7];
	r[_REG_O0] = tf->tf_out[0];
	r[_REG_O1] = tf->tf_out[1];
	r[_REG_O2] = tf->tf_out[2];
	r[_REG_O3] = tf->tf_out[3];
	r[_REG_O4] = tf->tf_out[4];
	r[_REG_O5] = tf->tf_out[5];
	r[_REG_O6] = tf->tf_out[6];
	r[_REG_O7] = tf->tf_out[7];

	*flags |= _UC_CPU;

#ifdef FPU_CONTEXT
	/*
	 * Get the floating point registers
	 */
	bcopy(fps->fs_regs, f->__fpu_regs, sizeof(fps->fs_regs));
	f->__fp_nqsize = sizeof(struct fp_qentry);
	f->__fp_nqel = fps->fs_qsize;
	f->__fp_fsr = fps->fs_fsr;
	if (f->__fp_q != NULL) {
		size_t sz = f->__fp_nqel * f->__fp_nqsize;
		if (sz > sizeof(fps->fs_queue)) {
#ifdef DIAGNOSTIC
			printf("getcontext: fp_queue too large\n");
#endif
			return;
		}
		if (copyout(fps->fs_queue, f->__fp_q, sz) != 0) {
#ifdef DIAGNOSTIC
			printf("getcontext: copy of fp_queue failed %d\n",
			    error);
#endif
			return;
		}
	}
	f->fp_busy = 0;	/* XXX: How do we determine that? */
	*flags |= _UC_FPU;
#endif

	return;
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
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct trapframe *tf;
	__greg_t *r = mcp->__gregs;
#ifdef FPU_CONTEXT
	__fpregset_t *f = &mcp->__fpregs;
	struct fpstate *fps = l->l_md.md_fpstate;
#endif

	write_user_windows();
	if (rwindow_save(l))
		sigexit(l, SIGILL);

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("__setmcontext: %s[%d], __mcontext %p\n",
		    l->l_proc->p_comm, l->l_proc->p_pid, mcp);
#endif

	if (flags & _UC_CPU) {
		/* Restore register context. */
		tf = (struct trapframe *)l->l_md.md_tf;

		/*
		 * Only the icc bits in the psr are used, so it need not be
		 * verified.  pc and npc must be multiples of 4.  This is all
		 * that is required; if it holds, just do it.
		 */
		if (((r[_REG_PC] | r[_REG_nPC]) & 3) != 0) {
			printf("pc or npc are not multiples of 4!\n");
			return (EINVAL);
		}

		/* take only psr ICC field */
		tf->tf_psr = (tf->tf_psr & ~PSR_ICC) |
		    (r[_REG_PSR] & PSR_ICC);
		tf->tf_pc = r[_REG_PC];
		tf->tf_npc = r[_REG_nPC];
		tf->tf_y = r[_REG_Y];

		/* Restore everything */
		tf->tf_global[1] = r[_REG_G1];
		tf->tf_global[2] = r[_REG_G2];
		tf->tf_global[3] = r[_REG_G3];
		tf->tf_global[4] = r[_REG_G4];
		tf->tf_global[5] = r[_REG_G5];
		tf->tf_global[6] = r[_REG_G6];
		tf->tf_global[7] = r[_REG_G7];

		tf->tf_out[0] = r[_REG_O0];
		tf->tf_out[1] = r[_REG_O1];
		tf->tf_out[2] = r[_REG_O2];
		tf->tf_out[3] = r[_REG_O3];
		tf->tf_out[4] = r[_REG_O4];
		tf->tf_out[5] = r[_REG_O5];
		tf->tf_out[6] = r[_REG_O6];
		tf->tf_out[7] = r[_REG_O7];
	}

#ifdef FPU_CONTEXT
	if (flags & _UC_FPU) {
		/*
		 * Set the floating point registers
		 */
		int error;
		size_t sz = f->__fp_nqel * f->__fp_nqsize;
		if (sz > sizeof(fps->fs_queue)) {
#ifdef DIAGNOSTIC
			printf("setmcontext: fp_queue too large\n");
#endif
			return (EINVAL);
		}
		bcopy(f->__fpu_regs, fps->fs_regs, sizeof(fps->fs_regs));
		fps->fs_qsize = f->__fp_nqel;
		fps->fs_fsr = f->__fp_fsr;
		if (f->__fp_q != NULL) {
			if ((error = copyin(f->__fp_q, fps->fs_queue, sz)) != 0) {
#ifdef DIAGNOSTIC
				printf("setmcontext: fp_queue copy failed\n");
#endif
				return (error);
			}
		}
	}
#endif

	if (flags & _UC_SETSTACK)  
		l->l_proc->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK; 

	return (0);
}

int	waittime = -1;

void
cpu_reboot(howto, user_boot_string)
	int howto;
	char *user_boot_string;
{
	int i;
	char opts[4];
	static char str[128];

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

#if NFB > 0
	fb_unblank();
#endif
	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		extern struct lwp lwp0;
		extern int sparc_clock_time_is_ok;

		/* XXX protect against curlwp->p_stats.foo refs in sync() */
		if (curlwp == NULL)
			curlwp = &lwp0;
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 * Do this only if the TOD clock has already been read out
		 * successfully by inittodr() or set by an explicit call
		 * to resettodr() (e.g. from settimeofday()).
		 */
		if (sparc_clock_time_is_ok)
			resettodr();
	}

	/* Disable interrupts. But still allow IPI on MP systems */
	if (ncpu > 1)
		(void)splsched();
	else
		(void)splhigh();

#if defined(MULTIPROCESSOR)
	/* Direct system interrupts to this CPU, since dump uses polled I/O */
	if (CPU_ISSUN4M)
		*((u_int *)ICR_ITR) = cpuinfo.mid - 8;
#endif

	/* If rebooting and a dump is requested, do it. */
#if 0
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
#else
	if (howto & RB_DUMP)
#endif
		dumpsys();

 haltsys:

	/* Run any shutdown hooks. */
	doshutdownhooks();

	/* If powerdown was requested, do it. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		prom_interpret("power-off");
#if NPOWER > 0
		/* Fall back on `power' device if the PROM can't do it */ 
		powerdown();
#endif
		printf("WARNING: powerdown not supported\n");
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
	}

	if (howto & RB_HALT) {
#if defined(MULTIPROCESSOR)
		mp_halt_cpus();
		printf("cpu%d halted\n\n", cpu_number());
#else
		printf("halted\n\n");
#endif
		prom_halt();
	}

	printf("rebooting\n\n");

	i = 1;
	if (howto & RB_SINGLE)
		opts[i++] = 's';
	if (howto & RB_KDB)
		opts[i++] = 'd';
	opts[i] = '\0';
	opts[0] = (i > 1) ? '-' : '\0';

	if (user_boot_string && *user_boot_string) {
		i = strlen(user_boot_string);
		if (i > sizeof(str) - sizeof(opts) - 1)
			prom_boot(user_boot_string);	/* XXX */
		bcopy(user_boot_string, str, i);
		if (opts[0] != '\0')
			str[i] = ' ';
	}
	strcat(str, opts);
	prom_boot(str);
	/*NOTREACHED*/
}

u_int32_t dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

void
cpu_dumpconf()
{
	const struct bdevsw *bdev;
	int nblks, dumpblks;

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;

	nblks = (*bdev->d_psize)(dumpdev);

	dumpblks = ctod(physmem) + pmap_dumpsize();
	if (dumpblks > (nblks - ctod(1)))
		/*
		 * dump size is too big for the partition.
		 * Note, we safeguard a click at the front for a
		 * possible disk label.
		 */
		return;

	/* Put the dump at the end of the partition */
	dumplo = nblks - dumpblks;

	/*
	 * savecore(8) expects dumpsize to be the number of pages
	 * of actual core dumped (i.e. excluding the MMU stuff).
	 */
	dumpsize = physmem;
}

#define	BYTES_PER_DUMP	(32 * 1024)	/* must be a multiple of pagesize */
static vaddr_t dumpspace;

caddr_t
reserve_dumppages(p)
	caddr_t p;
{

	dumpspace = (vaddr_t)p;
	return (p + BYTES_PER_DUMP);
}

/*
 * Write a crash dump.
 */
void
dumpsys()
{
	const struct bdevsw *bdev;
	int psize;
	daddr_t blkno;
	int (*dump)	__P((dev_t, daddr_t, caddr_t, size_t));
	int error = 0;
	struct memarr *mp;
	int nmem;
	extern struct memarr pmemarr[];
	extern int npmemarr;

	/* copy registers to memory */
	snapshot(cpuinfo.curpcb);
	stackdump();

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}
	blkno = dumplo;
	dump = bdev->d_dump;

	error = pmap_dumpmmu(dump, blkno);
	blkno += pmap_dumpsize();

	for (mp = pmemarr, nmem = npmemarr; --nmem >= 0 && error == 0; mp++) {
		unsigned i = 0, n;
		int maddr = mp->addr;

		if (maddr == 0) {
			/* Skip first page at physical address 0 */
			maddr += PAGE_SIZE;
			i += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
		}

		for (; i < mp->len; i += n) {
			n = mp->len - i;
			if (n > BYTES_PER_DUMP)
				 n = BYTES_PER_DUMP;

			/* print out how many MBs we have dumped */
			if (i && (i % (1024*1024)) == 0)
				printf("%d ", i / (1024*1024));

			(void) pmap_map(dumpspace, maddr, maddr + n,
					VM_PROT_READ);
			error = (*dump)(dumpdev, blkno,
					(caddr_t)dumpspace, (int)n);
			pmap_kremove(dumpspace, n);
			pmap_update(pmap_kernel());
			if (error)
				break;
			maddr += n;
			blkno += btodb(n);
		}
	}

	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
}

/*
 * get the fp and dump the stack as best we can.  don't leave the
 * current stack page
 */
void
stackdump()
{
	struct frame *fp = getfp(), *sfp;

	sfp = fp;
	printf("Frame pointer is at %p\n", fp);
	printf("Call traceback:\n");
	while (fp && ((u_long)fp >> PGSHIFT) == ((u_long)sfp >> PGSHIFT)) {
		printf("  pc = 0x%x  args = (0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) fp = %p\n",
		    fp->fr_pc, fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2],
		    fp->fr_arg[3], fp->fr_arg[4], fp->fr_arg[5], fp->fr_arg[6],
		    fp->fr_fp);
		fp = fp->fr_fp;
	}
}

int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	return (ENOEXEC);
}

#if defined(SUN4)
void
oldmon_w_trace(va)
	u_long va;
{
	u_long stop;
	struct frame *fp;

	if (curlwp)
		printf("curlwp = %p, pid %d\n",
			curlwp, curproc->p_pid);
	else
		printf("no curlwp\n");

	printf("uvm: swtch %d, trap %d, sys %d, intr %d, soft %d, faults %d\n",
	    uvmexp.swtch, uvmexp.traps, uvmexp.syscalls, uvmexp.intrs,
		uvmexp.softs, uvmexp.faults);
	write_user_windows();

#define round_up(x) (( (x) + (PAGE_SIZE-1) ) & (~(PAGE_SIZE-1)) )

	printf("\nstack trace with sp = 0x%lx\n", va);
	stop = round_up(va);
	printf("stop at 0x%lx\n", stop);
	fp = (struct frame *) va;
	while (round_up((u_long) fp) == stop) {
		printf("  0x%x(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) fp %p\n", fp->fr_pc,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3],
		    fp->fr_arg[4], fp->fr_arg[5], fp->fr_arg[6], fp->fr_fp);
		fp = fp->fr_fp;
		if (fp == NULL)
			break;
	}
	printf("end of stack trace\n");
}

void
oldmon_w_cmd(va, ar)
	u_long va;
	char *ar;
{
	switch (*ar) {
	case '\0':
		switch (va) {
		case 0:
			panic("g0 panic");
		case 4:
			printf("w: case 4\n");
			break;
		default:
			printf("w: unknown case %ld\n", va);
			break;
		}
		break;
	case 't':
		oldmon_w_trace(va);
		break;
	default:
		printf("w: arg not allowed\n");
	}
}
#endif /* SUN4 */

int
ldcontrolb(addr)
caddr_t addr;
{
	struct pcb *xpcb;
	extern struct user *proc0paddr;
	u_long saveonfault;
	int res;
	int s;

	if (CPU_ISSUN4M || CPU_ISSUN4D) {
		printf("warning: ldcontrolb called on sun4m/sun4d\n");
		return 0;
	}

	s = splhigh();
	if (curlwp == NULL)
		xpcb = (struct pcb *)proc0paddr;
	else
		xpcb = &curlwp->l_addr->u_pcb;

	saveonfault = (u_long)xpcb->pcb_onfault;
        res = xldcontrolb(addr, xpcb);
	xpcb->pcb_onfault = (caddr_t)saveonfault;

	splx(s);
	return (res);
}

void
wzero(vb, l)
	void *vb;
	u_int l;
{
	u_char *b = vb;
	u_char *be = b + l;
	u_short *sp;

	if (l == 0)
		return;

	/* front, */
	if ((u_long)b & 1)
		*b++ = 0;

	/* back, */
	if (b != be && ((u_long)be & 1) != 0) {
		be--;
		*be = 0;
	}

	/* and middle. */
	sp = (u_short *)b;
	while (sp != (u_short *)be)
		*sp++ = 0;
}

void
wcopy(vb1, vb2, l)
	const void *vb1;
	void *vb2;
	u_int l;
{
	const u_char *b1e, *b1 = vb1;
	u_char *b2 = vb2;
	u_short *sp;
	int bstore = 0;

	if (l == 0)
		return;

	/* front, */
	if ((u_long)b1 & 1) {
		*b2++ = *b1++;
		l--;
	}

	/* middle, */
	sp = (u_short *)b1;
	b1e = b1 + l;
	if (l & 1)
		b1e--;
	bstore = (u_long)b2 & 1;

	while (sp < (u_short *)b1e) {
		if (bstore) {
			b2[1] = *sp & 0xff;
			b2[0] = *sp >> 8;
		} else
			*((short *)b2) = *sp;
		sp++;
		b2 += 2;
	}

	/* and back. */
	if (l & 1)
		*b2 = *b1e;
}


/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct sparc_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct sparc_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct sparc_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_align = PAGE_SIZE;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	free(map, M_DMAMAP);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(t, map, m, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
{

	panic("_bus_dmamap_load_mbuf: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{

	panic("_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	vaddr_t low, high;
	struct pglist *mlist;
	int error;

	/* Always round the size. */
	size = round_page(size);
	low = vm_first_phys;
	high = vm_first_phys + vm_num_phys - PAGE_SIZE;

	if ((mlist = malloc(sizeof(*mlist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * Allocate pages from the VM system.
	 */
	error = uvm_pglistalloc(size, low, high, 0, 0,
				mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = mlist;

	/*
	 * We now have physical pages, but no DVMA addresses yet. These
	 * will be allocated in bus_dmamap_load*() routines. Hence we
	 * save any alignment and boundary requirements in this DMA
	 * segment.
	 */
	segs[0].ds_addr = 0;
	segs[0].ds_len = 0;
	segs[0]._ds_va = 0;
	*rsegs = 1;
	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{

	if (nsegs != 1)
		panic("bus_dmamem_free: nsegs = %d", nsegs);

	/*
	 * Return the list of pages back to the VM system.
	 */
	uvm_pglistfree(segs[0]._ds_mlist);
	free(segs[0]._ds_mlist, M_DEVBUF);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("_bus_dmamem_unmap");
#endif

	size = round_page(size);
	uvm_unmap(kernel_map, (vaddr_t)kva, (vaddr_t)kva + size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	off_t off;
	int prot, flags;
{

	panic("_bus_dmamem_mmap: not implemented");
}

/*
 * Utility to allocate an aligned kernel virtual address range
 */
vaddr_t
_bus_dma_valloc_skewed(size, boundary, align, skew)
	size_t size;
	u_long boundary;
	u_long align;
	u_long skew;
{
	size_t oversize;
	vaddr_t va, sva;

	/*
	 * Find a region of kernel virtual addresses that is aligned
	 * to the given address modulo the requested alignment, i.e.
	 *
	 *	(va - skew) == 0 mod align
	 *
	 * The following conditions apply to the arguments:
	 *
	 *	- `size' must be a multiple of the VM page size
	 *	- `align' must be a power of two
	 *	   and greater than or equal to the VM page size
	 *	- `skew' must be smaller than `align'
	 *	- `size' must be smaller than `boundary'
	 */

#ifdef DIAGNOSTIC
	if ((size & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid size %lx", size);
	if ((align & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid alignment %lx", align);
	if (align < skew)
		panic("_bus_dma_valloc_skewed: align %lx < skew %lx",
			align, skew);
#endif

	/* XXX - Implement this! */
	if (boundary) {
		printf("_bus_dma_valloc_skewed: "
			"boundary check not implemented");
		return (0);
	}

	/*
	 * First, find a region large enough to contain any aligned chunk
	 */
	oversize = size + align - PAGE_SIZE;
	sva = uvm_km_valloc(kernel_map, oversize);
	if (sva == 0)
		return (0);

	/*
	 * Compute start of aligned region
	 */
	va = sva;
	va += (skew + align - va) & (align - 1);

	/*
	 * Return excess virtual addresses
	 */
	if (va != sva)
		(void)uvm_unmap(kernel_map, sva, va);
	if (va + size != sva + oversize)
		(void)uvm_unmap(kernel_map, va + size, sva + oversize);

	return (va);
}

/* sun4/sun4c DMA map functions */
int	sun4_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
				bus_size_t, struct proc *, int));
int	sun4_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
				bus_dma_segment_t *, int, bus_size_t, int));
void	sun4_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
int	sun4_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
				int nsegs, size_t size, caddr_t *kvap,
				int flags));

/*
 * sun4/sun4c: load DMA map with a linear buffer.
 */
int
sun4_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_size_t sgsize;
	vaddr_t va = (vaddr_t)buf;
	int pagesz = PAGE_SIZE;
	vaddr_t dva;
	pmap_t pmap;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	cache_flush(buf, buflen);

	if ((map->_dm_flags & BUS_DMA_24BIT) == 0) {
		/*
		 * XXX Need to implement "don't DMA across this boundry".
		 */
		if (map->_dm_boundary != 0) {
			bus_addr_t baddr;

			/* Calculate first boundary line after `buf' */
			baddr = ((bus_addr_t)va + map->_dm_boundary) &
					-map->_dm_boundary;

			/*
			 * If the requested segment crosses the boundary,
			 * we can't grant a direct map. For now, steal some
			 * space from the `24BIT' map instead.
			 *
			 * (XXX - no overflow detection here)
			 */
			if (buflen > (baddr - (bus_addr_t)va))
				goto no_fit;
		}
		map->dm_mapsize = buflen;
		map->dm_nsegs = 1;
		map->dm_segs[0].ds_addr = (bus_addr_t)va;
		map->dm_segs[0].ds_len = buflen;
		map->_dm_flags |= _BUS_DMA_DIRECTMAP;
		return (0);
	}

no_fit:
	sgsize = round_page(buflen + (va & (pagesz - 1)));

	if (extent_alloc(dvmamap24, sgsize, pagesz, map->_dm_boundary,
			 (flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT,
			 &dva) != 0) {
		return (ENOMEM);
	}

	/*
	 * We always use just one segment.
	 */
	map->dm_mapsize = buflen;
	map->dm_segs[0].ds_addr = dva + (va & (pagesz - 1));
	map->dm_segs[0].ds_len = buflen;
	map->dm_segs[0]._ds_sgsize = sgsize;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; buflen > 0; ) {
		paddr_t pa;

		/*
		 * Get the physical address for this page.
		 */
		(void) pmap_extract(pmap, va, &pa);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = pagesz - (va & (pagesz - 1));
		if (buflen < sgsize)
			sgsize = buflen;

#ifdef notyet
#if defined(SUN4)
		if (have_iocache)
			pa |= PG_IOC;
#endif
#endif
		pmap_kenter_pa(dva, (pa & -pagesz) | PMAP_NC,
		    VM_PROT_READ | VM_PROT_WRITE);

		dva += pagesz;
		va += sgsize;
		buflen -= sgsize;
	}
	pmap_update(pmap_kernel());

	map->dm_nsegs = 1;
	return (0);
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
sun4_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	struct vm_page *m;
	paddr_t pa;
	vaddr_t dva;
	bus_size_t sgsize;
	struct pglist *mlist;
	int pagesz = PAGE_SIZE;
	int error;

	map->dm_nsegs = 0;
	sgsize = (size + pagesz - 1) & -pagesz;

	/* Allocate DVMA addresses */
	if ((map->_dm_flags & BUS_DMA_24BIT) != 0) {
		error = extent_alloc(dvmamap24, sgsize, pagesz,
					map->_dm_boundary,
					(flags & BUS_DMA_NOWAIT) == 0
						? EX_WAITOK : EX_NOWAIT,
					&dva);
		if (error)
			return (error);
	} else {
		/* Any properly aligned virtual address will do */
		dva = _bus_dma_valloc_skewed(sgsize, map->_dm_boundary,
					     pagesz, 0);
		if (dva == 0)
			return (ENOMEM);
	}

	map->dm_segs[0].ds_addr = dva;
	map->dm_segs[0].ds_len = size;
	map->dm_segs[0]._ds_sgsize = sgsize;

	/* Map physical pages into IOMMU */
	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		if (sgsize == 0)
			panic("sun4_dmamap_load_raw: size botch");
		pa = VM_PAGE_TO_PHYS(m);
#ifdef notyet
#if defined(SUN4)
		if (have_iocache)
			pa |= PG_IOC;
#endif
#endif
		pmap_kenter_pa(dva, (pa & -pagesz) | PMAP_NC,
		    VM_PROT_READ | VM_PROT_WRITE);

		dva += pagesz;
		sgsize -= pagesz;
	}
	pmap_update(pmap_kernel());

	map->dm_nsegs = 1;
	map->dm_mapsize = size;

	return (0);
}

/*
 * sun4/sun4c function for unloading a DMA map.
 */
void
sun4_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	bus_dma_segment_t *segs = map->dm_segs;
	int nsegs = map->dm_nsegs;
	int flags = map->_dm_flags;
	vaddr_t dva;
	bus_size_t len;
	int i, s, error;

	if ((flags & _BUS_DMA_DIRECTMAP) != 0) {
		/* Nothing to release */
		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
		map->_dm_flags &= ~_BUS_DMA_DIRECTMAP;
		return;
	}

	for (i = 0; i < nsegs; i++) {
		dva = segs[i].ds_addr & -PAGE_SIZE;
		len = segs[i]._ds_sgsize;

		pmap_kremove(dva, len);

		if ((flags & BUS_DMA_24BIT) != 0) {
			s = splhigh();
			error = extent_free(dvmamap24, dva, len, EX_NOWAIT);
			splx(s);
			if (error != 0)
				printf("warning: %ld of DVMA space lost\n", len);
		} else {
			uvm_unmap(kernel_map, dva, dva + len);
		}
	}
	pmap_update(pmap_kernel());

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
sun4_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct vm_page *m;
	vaddr_t va;
	struct pglist *mlist;

	if (nsegs != 1)
		panic("sun4_dmamem_map: nsegs = %d", nsegs);

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (caddr_t)va;

	mlist = segs[0]._ds_mlist;
	TAILQ_FOREACH(m, mlist, pageq) {
		paddr_t pa;

		if (size == 0)
			panic("sun4_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_kenter_pa(va, pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE);

		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	return (0);
}


struct sparc_bus_dma_tag mainbus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	sun4_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	sun4_dmamap_load_raw,
	sun4_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	sun4_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};


/*
 * Base bus space handlers.
 */
static int	sparc_bus_map __P(( bus_space_tag_t, bus_addr_t,
				    bus_size_t, int, vaddr_t,
				    bus_space_handle_t *));
static int	sparc_bus_unmap __P((bus_space_tag_t, bus_space_handle_t,
				     bus_size_t));
static int	sparc_bus_subregion __P((bus_space_tag_t, bus_space_handle_t,
					 bus_size_t, bus_size_t,
					 bus_space_handle_t *));
static paddr_t	sparc_bus_mmap __P((bus_space_tag_t, bus_addr_t, off_t,
				    int, int));
static void	*sparc_mainbus_intr_establish __P((bus_space_tag_t, int, int,
						   int (*) __P((void *)),
						   void *,
						   void (*) __P((void)) ));
static void     sparc_bus_barrier __P(( bus_space_tag_t, bus_space_handle_t,
					bus_size_t, bus_size_t, int));

/*
 * Generic routine to translate an address using OpenPROM `ranges'.
 */
int
bus_translate_address_generic(struct openprom_range *ranges, int nranges,
    bus_addr_t addr, bus_addr_t *addrp)
{
	int i, space = BUS_ADDR_IOSPACE(addr);

	for (i = 0; i < nranges; i++) {
		struct openprom_range *rp = &ranges[i];

		if (rp->or_child_space != space)
			continue;

		/* We've found the connection to the parent bus. */
		*addrp = BUS_ADDR(rp->or_parent_space,
		    rp->or_parent_base + BUS_ADDR_PADDR(addr));
		return (0);
	}

	return (EINVAL);
}

int
sparc_bus_map(t, ba, size, flags, va, hp)
	bus_space_tag_t t;
	bus_addr_t	ba;
	bus_size_t	size;
	vaddr_t		va;
	bus_space_handle_t *hp;
{
	vaddr_t v;
	paddr_t pa;
	unsigned int pmtype;
static	vaddr_t iobase;

	if (t->ranges != NULL) {
		bus_addr_t addr;
		int error;

		error = bus_translate_address_generic(t->ranges, t->nranges,
		    ba, &addr);
		if (error)
			return (error);
		return (bus_space_map2(t->parent, addr, size, flags, va, hp));
	}

	if (iobase == 0)
		iobase = IODEV_BASE;

	size = round_page(size);
	if (size == 0) {
		printf("sparc_bus_map: zero size\n");
		return (EINVAL);
	}

	if (va)
		v = trunc_page(va);
	else {
		v = iobase;
		iobase += size;
		if (iobase > IODEV_END)	/* unlikely */
			panic("sparc_bus_map: iobase=0x%lx", iobase);
	}

	pmtype = PMAP_IOENC(BUS_ADDR_IOSPACE(ba));
	pa = BUS_ADDR_PADDR(ba);

	/* note: preserve page offset */
	*hp = (bus_space_handle_t)(v | ((u_long)pa & PGOFSET));

	pa = trunc_page(pa);
	do {
		pmap_kenter_pa(v, pa | pmtype | PMAP_NC,
		    VM_PROT_READ | VM_PROT_WRITE);
		v += PAGE_SIZE;
		pa += PAGE_SIZE;
	} while ((size -= PAGE_SIZE) > 0);

	pmap_update(pmap_kernel());
	return (0);
}

int
sparc_bus_unmap(t, bh, size)
	bus_space_tag_t t;
	bus_size_t	size;
	bus_space_handle_t bh;
{
	vaddr_t va = trunc_page((vaddr_t)bh);

	pmap_kremove(va, round_page(size));
	pmap_update(pmap_kernel());
	return (0);
}

int
sparc_bus_subregion(tag, handle, offset, size, nhandlep)
	bus_space_tag_t		tag;
	bus_space_handle_t	handle;
	bus_size_t		offset;
	bus_size_t		size;
	bus_space_handle_t	*nhandlep;
{
	*nhandlep = handle + offset;
	return (0);
}

paddr_t
sparc_bus_mmap(t, ba, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t	ba;
	off_t		off;
	int		prot;
	int		flags;
{
	u_int pmtype;
	paddr_t pa;

	if (t->ranges != NULL) {
		bus_addr_t addr;
		int error;

		error = bus_translate_address_generic(t->ranges, t->nranges,
		    ba, &addr);
		if (error)
			return (-1);
		return (bus_space_mmap(t->parent, addr, off, prot, flags));
	}

	pmtype = PMAP_IOENC(BUS_ADDR_IOSPACE(ba));
	pa = trunc_page(BUS_ADDR_PADDR(ba) + off);

	return (paddr_t)(pa | pmtype | PMAP_NC);
}

/*
 * Establish a temporary bus mapping for device probing.
 */
int
bus_space_probe(tag, paddr, size, offset, flags, callback, arg)
	bus_space_tag_t tag;
	bus_addr_t	paddr;
	bus_size_t	size;
	size_t		offset;
	int		flags;
	int		(*callback) __P((void *, void *));
	void		*arg;
{
	bus_space_handle_t bh;
	caddr_t tmp;
	int result;

	if (bus_space_map2(tag, paddr, size, flags, TMPMAP_VA, &bh) != 0)
		return (0);

	tmp = (caddr_t)bh;
	result = (probeget(tmp + offset, size) != -1);
	if (result && callback != NULL)
		result = (*callback)(tmp, arg);
	bus_space_unmap(tag, bh, size);
	return (result);
}


void *
sparc_mainbus_intr_establish(t, pil, level, handler, arg, fastvec)
	bus_space_tag_t t;
	int	pil;
	int	level;
	int	(*handler)__P((void *));
	void	*arg;
	void	(*fastvec)__P((void));
{
	struct intrhand *ih;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(pil, level, ih, fastvec);
	return (ih);
}

void sparc_bus_barrier (t, h, offset, size, flags)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t	offset;
	bus_size_t	size;
	int		flags;
{
	/* No default barrier action defined */
	return;
}

struct sparc_bus_space_tag mainbus_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	NULL,				/* ranges */
	0,				/* nranges */
	sparc_bus_map,			/* bus_space_map */
	sparc_bus_unmap,		/* bus_space_unmap */
	sparc_bus_subregion,		/* bus_space_subregion */
	sparc_bus_barrier,		/* bus_space_barrier */
	sparc_bus_mmap,			/* bus_space_mmap */
	sparc_mainbus_intr_establish,	/* bus_intr_establish */
#if __FULL_SPARC_BUS_SPACE
	NULL,				/* read_1 */
	NULL,				/* read_2 */
	NULL,				/* read_4 */
	NULL,				/* read_8 */
	NULL,				/* write_1 */
	NULL,				/* write_2 */
	NULL,				/* write_4 */
	NULL				/* write_8 */
#endif
};
