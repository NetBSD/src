/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 */
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ident "$Id: machdep.c,v 1.1.1.1 1993/09/29 06:09:17 briggs Exp $"
/*
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "kernel.h"
#include "map.h"
#include "proc.h"
#include "exec.h"
#include "buf.h"
#include "exec.h"
#include "vnode.h"
#include "reboot.h"
#include "conf.h"
#include "file.h"
#include "clist.h"
#include "callout.h"
#include "malloc.h"
#include "mbuf.h"
#include "msgbuf.h"
#include "user.h"
#include "myframe.h"
#ifdef SYSVSHM
#include "shm.h"
#endif

#include "../include/cpu.h"
#include "../include/reg.h"
#include "../include/psl.h"
#include "isr.h"
#include "machine/pte.h"
#include "net/netisr.h"

#define	MAXMEM	64*1024*CLSIZE	/* XXX - from cmap.h */
#include "vm/vm_param.h"
#include "vm/pmap.h"
#include "vm/vm_map.h"
#include "vm/vm_object.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"

vm_map_t buffer_map;
extern vm_offset_t avail_end;

int dbg_flg = 0;

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	msgbufmapped;		/* set when safe to use msgbuf */
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;
extern int	freebufspace;

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	cninit();	/* this is the dumb console; no NuBus intelligence. */
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	register unsigned i;
	register caddr_t v, firstaddr;
	int base, residual;
	extern long Usrptsize;
	extern struct map *useriomap;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;

	/*
	 * Initialize error message buffer (at end of core).
	 */
#ifdef DEBUG
	pmapdebug = 0;
#endif
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		pmap_enter(pmap_kernel(), msgbufp, avail_end + i * NBPG,
			   VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	printf("real mem = %d\n", ctob(physmem));

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */
	/*
	 * Make two passes.  The first pass calculates how much memory is
	 * needed and allocates it.  The second pass assigns virtual
	 * addresses to the various data structures.
	 */
	firstaddr = 0;
again:
	v = (caddr_t)firstaddr;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
	/* valloc(cfree, struct cblock, nclist); */
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif

	if(dbg_flg)printf ("\n*** After shitloads of valloc()'s ***\n\n");
	
	/*
	 * Determine how many buffers to allocate.
	 * We allocate the BSD standard of
	 * use 10% of memory for the first 2 Meg, 5% of remaining.
	 * We just allocate a flat 10%.  Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		if (physmem < btoc(2 * 1024 * 1024))
			bufpages = physmem / 10 / CLSIZE;
		else
			bufpages = (btoc(2 * 1024 * 1024) + physmem) / 20 / CLSIZE;

	bufpages = min(NKMEMCLUSTERS*2/5,bufpages);

	if (nbuf == 0) {
		nbuf = bufpages/2;
		if (nbuf < 16) {
			nbuf = 16;
			bufpages = 32;
		}
	}
	/* freebufspace = bufpages * NBPG; */
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if (firstaddr == 0) {
		size = (vm_size_t)(v - firstaddr);
		firstaddr = (caddr_t) kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}
	if(dbg_flg)printf ("\n*** End of second pass ***\n\n");
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
		panic("startup: table size inconsistency");
#if 0
	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t)&buffers,
				   &maxaddr, size, FALSE);
	if(dbg_flg)printf ("\n*** Breakpoint Number One ***\n\n");
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	if(dbg_flg)printf ("\n*** Breakpoint Number Two ***\n\n");
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		if(dbg_flg)printf ("\n*** Breakpoint Number Three (pass %d of %d) ***\n\n",i+1,nbuf);
		vm_map_simplify(buffer_map, curbuf);
		if(dbg_flg)printf ("\n*** Breakpoint Number Four (pass %d of %d) ***\n\n",i+1,nbuf);
	}
#else
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t)&minaddr,
				   &maxaddr, bufpages*CLBYTES, TRUE);
#endif
	if(dbg_flg)printf ("\n*** After allocating buffers proper ***\n\n");
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */

#if 0	/* in 386BSD, execve() doesn't use an exec_map. */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);
#endif 
	if(dbg_flg)printf ("\n*** After allocating submap of exec args ***\n\n");
	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);

	if(dbg_flg)printf ("\n*** After allocating submap for physio ***\n\n");
	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
				   M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);
	if(dbg_flg)printf ("\n*** After allocating mbuf pool ***\n\n");
	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %d\n", ptoa(vm_page_free_count));
	if(dbg_flg)printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);
	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();
}

/*
 * Set registers on exec.
 * XXX Should clear registers except sp, pc,
 * but would break init; should be fixed soon.
 */
void
#if 1
setregs(p, entry, sp, retval)
#else
setregs(p, entry, retval)
#endif
	register struct proc *p;
	u_long entry;
#if 1
	u_long sp;
#endif
	int retval[2];
{
	p->p_regs[PC] = entry & ~1;
#if 1
	p->p_regs[SP] = sp;
#endif
#ifdef FPCOPROC
	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
#endif
}

identifycpu()
{
/* MF Just a little interesting tidbit about what machine we are
   running on.  In the future magic may happen here based on
   machine, i.e. different via, scsi, iop, serial stuff
*/
	/* machine id is 
	bit 0				bit 32
	ppMMMMMMxxxxxxxxxSxxxxxxxxx..xx currently
	where pp is processor
	MM is machine 
	S is serial boot
	*/
	printf("Apple Macintosh ");
	switch ((machineid >>2) & 0x3f) {
	case MACH_MAC2:
		printf("II ");
		break;
	case MACH_MAC2X:
		printf("IIx ");
		break;
	case MACH_MAC2SI:
		printf("IIsi ");
		break;
	case MACH_MAC2CI:
		printf("IIci ");
		break;
	case MACH_MAC2CX:
		printf("IIcx ");
		break;
	case MACH_MAC2FX:
		printf("IIfx ");
		break;
	case MACH_MACSE30:
		printf("SE30 ");
		break;
	default:
		printf("Pentium ");
		break;
	}

	switch(machineid & 3) {
	case MACH_68020:
		printf("(68020)");
		break;	
	case MACH_68030:
		printf("(68030)");
		break;	
	case MACH_68040:
		printf("(68040)");
		break;	
	case MACH_PENTIUM:
	default:
		printf("(PENTIUM)");
		break;	
	}
	printf ("\n");
}

#define SS_RTEFRAME	1
#define SS_FPSTATE	2
#define SS_USERREGS	4

struct sigstate {
	int	ss_flags;		/* which of the following are valid */
	struct	frame ss_frame;		/* original exception frame */
	struct	fpframe ss_fpstate;	/* 68881/68882 state info */
};

/*
 * WARNING: code in locore.s assumes the layout shown for sf_signum
 * thru sf_handler so... don't screw with them!
 */
struct sigframe {
	int	sf_signum;		/* signo for handler */
	int	sf_code;		/* additional info for handler */
	struct	sigcontext *sf_scp;	/* context ptr for handler */
	sig_t	sf_handler;		/* handler addr for u_sigc */
	struct	sigstate sf_state;	/* state of the hardware */
	struct	sigcontext sf_sc;	/* actual context */
};

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send an interrupt to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	unsigned code;
{
	register struct proc *p = curproc;
	register struct sigframe *fp, *kfp;
	register struct frame *frame;
	register struct sigacts *ps = p->p_sigacts;
	register short ft;
	int oonstack, fsize;
	extern short exframesize[];
	extern char sigcode[], esigcode[];

	frame = (struct frame *)p->p_regs;
	ft = frame->f_format;
	oonstack = ps->ps_onstack;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	fsize = sizeof(struct sigframe);
	if (!ps->ps_onstack && (ps->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(ps->ps_sigsp - fsize);
		ps->ps_onstack = 1;
	} else
		fp = (struct sigframe *)(frame->f_regs[SP] - fsize);
	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d ssp %x usp %x scp %x ft %d\n",
		       p->p_pid, sig, &oonstack, fp, &fp->sf_sc, ft);
#endif
	if (useracc((caddr_t)fp, fsize, B_WRITE) == 0) {
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig(%d): useracc failed on sig %d\n",
			       p->p_pid, sig);
#endif
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		SIGACTION(p, SIGILL) = SIG_DFL;
		sig = sigmask(SIGILL);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, SIGILL);
		return;
	}
	kfp = (struct sigframe *)malloc((u_long)fsize, M_TEMP, M_WAITOK);
	/* 
	 * Build the argument list for the signal handler.
	 */
	kfp->sf_signum = sig;
	kfp->sf_code = code;
	kfp->sf_scp = &fp->sf_sc;
	kfp->sf_handler = catcher;
	/*
	 * Save necessary hardware state.  Currently this includes:
	 *	- general registers
	 *	- original exception frame (if not a "normal" frame)
	 *	- FP coprocessor state
	 */
	kfp->sf_state.ss_flags = SS_USERREGS;
	bcopy((caddr_t)frame->f_regs,
	      (caddr_t)kfp->sf_state.ss_frame.f_regs, sizeof frame->f_regs);
	if (ft >= FMT9) {
#ifdef DEBUG
		if (ft != FMT9 && ft != FMTA && ft != FMTB)
			panic("sendsig: bogus frame type");
#endif
		kfp->sf_state.ss_flags |= SS_RTEFRAME;
		kfp->sf_state.ss_frame.f_format = frame->f_format;
		kfp->sf_state.ss_frame.f_vector = frame->f_vector;
		bcopy((caddr_t)&frame->F_u,
		      (caddr_t)&kfp->sf_state.ss_frame.F_u, exframesize[ft]);
		/*
		 * Leave an indicator that we need to clean up the kernel
		 * stack.  We do this by setting the "pad word" above the
		 * hardware stack frame to the amount the stack must be
		 * adjusted by.
		 *
		 * N.B. we increment rather than just set f_stackadj in
		 * case we are called from syscall when processing a
		 * sigreturn.  In that case, f_stackadj may be non-zero.
		 */
		frame->f_stackadj += exframesize[ft];
		frame->f_format = frame->f_vector = 0;
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("sendsig(%d): copy out %d of frame %d\n",
			       p->p_pid, exframesize[ft], ft);
#endif
	}
#ifdef FPCOPROC
	kfp->sf_state.ss_flags |= SS_FPSTATE;
	m68881_save(&kfp->sf_state.ss_fpstate);
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&kfp->sf_state.ss_fpstate)
		printf("sendsig(%d): copy out FP state (%x) to %x\n",
		       p->p_pid, *(u_int *)&kfp->sf_state.ss_fpstate,
		       &kfp->sf_state.ss_fpstate);
#endif
#endif
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	kfp->sf_sc.sc_onstack = oonstack;
	kfp->sf_sc.sc_mask = mask;
	kfp->sf_sc.sc_sp = frame->f_regs[SP];
	kfp->sf_sc.sc_fp = frame->f_regs[A6];
	kfp->sf_sc.sc_ap = (int)&fp->sf_state;
	kfp->sf_sc.sc_pc = frame->f_pc;
	kfp->sf_sc.sc_ps = frame->f_sr;
	(void) copyout((caddr_t)kfp, (caddr_t)fp, fsize);
	frame->f_regs[SP] = (int)fp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig(%d): sig %d scp %x fp %x sc_sp %x sc_ap %x\n",
		       p->p_pid, sig, kfp->sf_scp, fp,
		       kfp->sf_sc.sc_sp, kfp->sf_sc.sc_ap);
#endif
	/*
	 * Signal trampoline code is at base of user stack.
	 */
	/* frame->f_pc = USRSTACK - (esigcode - sigcode); */
	frame->f_pc = (int) (((u_char *)PS_STRINGS) - (esigcode - sigcode));
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
	free((caddr_t)kfp, M_TEMP);
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */
/* ARGSUSED */
sigreturn(p, uap, retval)
	struct proc *p;
	struct args {
		struct sigcontext *sigcntxp;
	} *uap;
	int *retval;
{
	register struct sigcontext *scp;
	register struct frame *frame;
	register int rf;
	struct sigcontext tsigc;
	struct sigstate tstate;
	int flags;
	extern short exframesize[];

	scp = uap->sigcntxp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %x\n", p->p_pid, scp);
#endif
	if ((int)scp & 1)
		return (EINVAL);
	/*
	 * Test and fetch the context structure.
	 * We grab it all at once for speed.
	 */
	if (useracc((caddr_t)scp, sizeof (*scp), B_WRITE) == 0 ||
	    copyin((caddr_t)scp, (caddr_t)&tsigc, sizeof tsigc))
		return (EINVAL);
	scp = &tsigc;
	if ((scp->sc_ps & (PSL_MBZ|PSL_IPL|PSL_S)) != 0)
		return (EINVAL);
	/*
	 * Restore the user supplied information
	 */
	p->p_sigacts->ps_onstack = scp->sc_onstack & 01;
	p->p_sigmask = scp->sc_mask &~ sigcantmask;
	frame = (struct frame *) p->p_regs;
	frame->f_regs[SP] = scp->sc_sp;
	frame->f_regs[A6] = scp->sc_fp;
	frame->f_pc = scp->sc_pc;
	frame->f_sr = scp->sc_ps;
	/*
	 * Grab pointer to hardware state information.
	 * If zero, the user is probably doing a longjmp.
	 */
	if ((rf = scp->sc_ap) == 0)
		return (EJUSTRETURN);
	/*
	 * See if there is anything to do before we go to the
	 * expense of copying in close to 1/2K of data
	 */
	flags = fuword((caddr_t)rf);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn(%d): sc_ap %x flags %x\n",
		       p->p_pid, rf, flags);
#endif
	/*
	 * fuword failed (bogus sc_ap value).
	 */
	if (flags == -1)
		return (EINVAL);
	if (flags == 0 || copyin((caddr_t)rf, (caddr_t)&tstate, sizeof tstate))
		return (EJUSTRETURN);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sigreturn(%d): ssp %x usp %x scp %x ft %d\n",
		       p->p_pid, &flags, scp->sc_sp, uap->sigcntxp,
		       (flags&SS_RTEFRAME) ? tstate.ss_frame.f_format : -1);
#endif
	/*
	 * Restore most of the users registers except for A6 and SP
	 * which were handled above.
	 */
	if (flags & SS_USERREGS)
		bcopy((caddr_t)tstate.ss_frame.f_regs,
		      (caddr_t)frame->f_regs, sizeof(frame->f_regs)-2*NBPW);
	/*
	 * Restore long stack frames.  Note that we do not copy
	 * back the saved SR or PC, they were picked up above from
	 * the sigcontext structure.
	 */
	if (flags & SS_RTEFRAME) {
		register int sz;
		
		/* grab frame type and validate */
		sz = tstate.ss_frame.f_format;
		if (sz > 15 || (sz = exframesize[sz]) < 0)
			return (EINVAL);
		frame->f_stackadj -= sz;
		frame->f_format = tstate.ss_frame.f_format;
		frame->f_vector = tstate.ss_frame.f_vector;
		bcopy((caddr_t)&tstate.ss_frame.F_u, (caddr_t)&frame->F_u, sz);
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("sigreturn(%d): copy in %d of frame type %d\n",
			       p->p_pid, sz, tstate.ss_frame.f_format);
#endif
	}
#ifdef FPCOPROC
	/*
	 * Finally we restore the original FP context
	 */
	if (flags & SS_FPSTATE)
		m68881_restore(&tstate.ss_fpstate);
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&tstate.ss_fpstate)
		printf("sigreturn(%d): copied in FP state (%x) at %x\n",
		       p->p_pid, *(u_int *)&tstate.ss_fpstate,
		       &tstate.ss_fpstate);
#endif
#endif
#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}

int	waittime = -1;

void
boot(howto)
	register int howto;
{
	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx(curproc->p_addr, 0);

	boothowto = howto;
	if ((howto&RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
		register struct buf *bp;
		int iter, nbusy;

		waittime = 0;
		(void) spl0();
		printf("syncing disks... ");
		/*
		 * Release vnodes held by texts before sync.
		 */
		if (panicstr == 0)
			vnode_pager_umount(NULL);
#ifdef notdef
#include "fd.h"
#if NFD > 0
		fdshutdown();
#endif
#endif
		sync(&proc0, (void *)NULL, (int *)NULL);

		for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			DELAY(40000 * iter);
		}
		if (nbusy)
			printf("giving up\n");
		else
			printf("done\n");
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
	splhigh();			/* extreme priority */
	if (howto&RB_HALT) {
		/* LAK: Actually shut down machine */
#if 1
		via_shutdown();  /* in via.c */
#else
		printf("halted\n\n");
		asm("	stop	#0x2700");
#endif
	} else {
		if (howto & RB_DUMP)
			dumpsys();
		doboot();
		/*NOTREACHED*/
	}
	/*NOTREACHED*/
}

int	dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

dumpconf()
{
	int nblks;

	dumpsize = physmem;
	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(physmem));
	}
	/*
	 * Don't dump on the first CLBYTES (why CLBYTES?)
	 * in case the dump device includes a disk label.
	 */
	if (dumplo < btodb(CLBYTES))
		dumplo = btodb(CLBYTES);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
dumpsys()
{

	msgbufmapped = 0;
	if (dumpdev == NODEV)
		return;
	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
	printf("dump ");
	switch ((*bdevsw[major(dumpdev)].d_dump)(dumpdev)) {

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

	default:
		printf("succeeded\n");
		break;
	}
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

initcpu()
{
	/* 06/22/92,22:49:49 BG */
	/* I'm not sure we have to do anything here. */
	/* printf ("Kernel booting; please have lunch, maybe a pizza.\n"); */
}

straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

int	*nofault;

badaddr(addr)
	register caddr_t addr;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = (int *) 0;
	return(0);
}

badbaddr(addr)
	register caddr_t addr;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return(0);
}

netintr()
{
#ifdef INET
	if (netisr & (1 << NETISR_IP)) {
		netisr &= ~(1 << NETISR_IP);
		ipintr();
	}
#endif
#ifdef NS
	if (netisr & (1 << NETISR_NS)) {
		netisr &= ~(1 << NETISR_NS);
		nsintr();
	}
#endif
#ifdef ISO
	if (netisr & (1 << NETISR_ISO)) {
		netisr &= ~(1 << NETISR_ISO);
		clnlintr();
	}
#endif
}

#if defined(USING_FLEXIBLE_INTERRUPTS)
intrhand(sr)
	int sr;
{
	register struct isr *isr;
	register int found = 0;
	register int ipl;
	extern struct isr isrqueue[];

	ipl = (sr >> 8) & 7;
	switch (ipl) {

	case 3:
	case 4:
	case 5:
		ipl = ISRIPL(ipl);
		isr = isrqueue[ipl].isr_forw;
		for (; isr != &isrqueue[ipl]; isr = isr->isr_forw) {
			if ((isr->isr_intr)(isr->isr_arg)) {
				found++;
				break;
			}
		}
		if (found == 0)
			printf("stray interrupt, sr 0x%x\n", sr);
		break;

	case 0:
	case 1:
	case 2:
	case 6:
	case 7:
		printf("intrhand: unexpected sr 0x%x\n", sr);
		break;
	}
}
#endif

#if defined(DEBUG) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */

candbtimer()
{
	crashandburn = 0;
}
#endif

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */

extern long via1_spent[2][7];

nmihand(struct frame frame)
{
   int i;
  /* LAK: Should call debugger */
	printf("VIA1 interrupt timings:\n");
	for(i = 0; i < 7; i++)
		if(via1_spent[0][i] != 0)
			printf("# %d: %d usec inside, %d invocations.\n",
			    via1_spent[1][i], via1_spent[0][i]);

  
  panic("debugger switch\n");
}

#if defined(PARITY)
nmihand(frame)
	struct frame frame;
{
	if (kbdnmi()) {
#ifdef PANICBUTTON
		static int innmihand = 0;

		/*
		 * Attempt to reduce the window of vulnerability for recursive
		 * NMIs (e.g. someone holding down the keyboard reset button).
		 */
		if (innmihand == 0) {
			innmihand = 1;
			printf("Got a keyboard NMI\n");
			innmihand = 0;
		}
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				panic(panicstr ?
				      "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			timeout(candbtimer, (caddr_t)0, candbdelay);
		}
#endif
		return;
	}
	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");
}

/*
 * Parity error section.  Contains magic.
 */
#define PARREG		((volatile short *)IIOV(0x5B0000))
static int gotparmem = 0;
#ifdef DEBUG
int ignorekperr = 0;	/* ignore kernel parity errors */
#endif


/*
 * Determine if level 7 interrupt was caused by a parity error
 * and deal with it if it was.  Returns 1 if it was a parity error.
 */
parityerror(fp)
	struct frame *fp;
{
	if (!gotparmem)
		return(0);
	*PARREG = 0;
	DELAY(10);
	*PARREG = 1;
	if (panicstr) {
		printf("parity error after panic ignored\n");
		return(1);
	}
	if (!findparerror())
		printf("WARNING: transient parity error ignored\n");
	else if (USERMODE(fp->f_sr)) {
		printf("pid %d: parity error\n", curproc->p_pid);
		uprintf("sorry, pid %d killed due to memory parity error\n",
			curproc->p_pid);
		psignal(curproc, SIGKILL);
#ifdef DEBUG
	} else if (ignorekperr) {
		printf("WARNING: kernel parity error ignored\n");
#endif
	} else {
		regdump(fp->f_regs, 128);
		panic("kernel parity error");
	}
	return(1);
}

/*
 * Yuk!  There has got to be a better way to do this!
 * Searching all of memory with interrupts blocked can lead to disaster.
 */
findparerror()
{
	static label_t parcatch;
	static int looking = 0;
	volatile struct pte opte;
	volatile int pg, o, s;
	register volatile int *ip;
	register int i;
	int found;

#ifdef lint
	ip = &found;
	i = o = pg = 0; if (i) return(0);
#endif
	/*
	 * If looking is true we are searching for a known parity error
	 * and it has just occured.  All we do is return to the higher
	 * level invocation.
	 */
	if (looking)
		longjmp(&parcatch);
	s = splhigh();
	/*
	 * If setjmp returns true, the parity error we were searching
	 * for has just occured (longjmp above) at the current pg+o
	 */
	if (setjmp(&parcatch)) {
		printf("Parity error at 0x%x\n", ctob(pg)|o);
		found = 1;
		goto done;
	}
	/*
	 * If we get here, a parity error has occured for the first time
	 * and we need to find it.  We turn off any external caches and
	 * loop thru memory, testing every longword til a fault occurs and
	 * we regain control at setjmp above.  Note that because of the
	 * setjmp, pg and o need to be volatile or their values will be lost.
	 */
	looking = 1;
	ecacheoff();
	/* LAK: the "pg = 0" below was "pg = lowram" */
	for (pg = 0; pg < physmem; pg++) {
		pmap_enter(pmap_kernel(), vmmap, ctob(pg), VM_PROT_READ, TRUE);
		for (o = 0; o < NBPG; o += sizeof(int))
			i = *(int *)(&vmmap[o]);
	}
	/*
	 * Getting here implies no fault was found.  Should never happen.
	 */
	printf("Couldn't locate parity error\n");
	found = 0;
done:
	looking = 0;
	pmap_remove(pmap_kernel(), vmmap, &vmmap[NBPG]);
	ecacheon();
	splx(s);
	return(found);
}

regdump(rp, sbytes)
  int *rp; /* must not be register */
  int sbytes;
{
	static int doingdump = 0;
	register int i;
	int s;
	extern char *hexstr();

	if (doingdump)
		return;
	s = splhigh();
	doingdump = 1;
	printf("pid = %d, pc = %s, ", curproc->p_pid, hexstr(rp[PC], 8));
	printf("ps = %s, ", hexstr(rp[PS], 4));
	printf("sfc = %s, ", hexstr(getsfc(), 4));
	printf("dfc = %s\n", hexstr(getdfc(), 4));
	printf("Registers:\n     ");
	for (i = 0; i < 8; i++)
		printf("        %d", i);
	printf("\ndreg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(rp[i], 8));
	printf("\nareg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(rp[i+8], 8));
	if (sbytes > 0) {
		if (rp[PS] & PSL_S) {
			printf("\n\nKernel stack (%s):",
			       hexstr((int)(((int *)&rp)-1), 8));
			dumpmem(((int *)&rp)-1, sbytes, 0);
		} else {
			printf("\n\nUser stack (%s):", hexstr(rp[SP], 8));
			dumpmem((int *)rp[SP], sbytes, 1);
		}
	}
	doingdump = 0;
	splx(s);
}


extern char kstack[];
#define KSADDR	((int *)&(kstack[(UPAGES-1)*NBPG]))

dumpmem(ptr, sz, ustack)
	register int *ptr;
	int sz;
{
	register int i, val;
	extern char *hexstr();

	for (i = 0; i < sz; i++) {
		if ((i & 7) == 0)
			printf("\n%s: ", hexstr((int)ptr, 6));
		else
			printf(" ");
		if (ustack == 1) {
			if ((val = fuword(ptr++)) == -1)
				break;
		} else {
			if (ustack == 0 &&
			    (ptr < KSADDR || ptr > KSADDR+(NBPG/4-1)))
				break;
			val = *ptr++;
		}
		printf("%s", hexstr(val, 8));
	}
	printf("\n");
}


char *
hexstr(val, len)
	register int val;
{
	static char nbuf[9];
	register int x, i;

	if (len > 8)
		return("");
	nbuf[len] = '\0';
	for (i = len-1; i >= 0; --i) {
		x = val & 0xF;
		if (x > 9)
			nbuf[i] = x - 10 + 'A';
		else
			nbuf[i] = x + '0';
		val >>= 4;
	}
	return(nbuf);
}

#endif

/* LAK: The following function was taken from the i386 machdep.c file,
   probably written by Bill Jolitz. */

physstrat(bp, strat, prio)
	struct buf *bp;
	int (*strat)(), prio;
{
	register int s;
	caddr_t baddr;

	/*
	 * vmapbuf clobbers b_addr so we must remember it so that it
	 * can be restored after vunmapbuf.  This is truely rude, we
	 * should really be storing this in a field in the buf struct
	 * but none are available and I didn't want to add one at
	 * this time.  Note that b_addr for dirty page pushes is 
	 * restored in vunmapbuf. (ugh!)
	 */
	baddr = bp->b_un.b_addr;
	vmapbuf(bp);
	(*strat)(bp);
	/* pageout daemon doesn't wait for pushed pages */
	if (bp->b_flags & B_DIRTY)
		return;
	s = splbio();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, prio);
	splx(s);
	vunmapbuf(bp);
	bp->b_un.b_addr = baddr;
}


extern unsigned long videoaddr;
extern unsigned long videorowbytes;
static unsigned long *gray_nextaddr = 0;


/* MF A little note to casual passers by, this code will crash you if
 your machine has a larger rowbytes than 128...*/

void bar_flash()
{
   static int count = 0;
   static int i;

   asm("movl a0, sp@-");
   asm("movl a1, sp@-");
   asm("movl d0, sp@-");
   asm("movl d1, sp@-");
   if (count++ == 100)
   {
     for(i = 0;i < 32; i++)
        gray_nextaddr[i] ^= 0xffffffff;
     for(i = 0;i < 32; i++)
        gray_nextaddr[i] ^= 0xffffffff;
     count=0;
   }
   asm("movl sp@+, d1");
   asm("movl sp@+, d0");
   asm("movl sp@+, a1");
   asm("movl sp@+, a0");
}


void gray_bar()
{
   static int i=0;
   static int flag=0;
/* MF basic premise as I see it, save the scratch regs as they are not
   saved by the compilier.
   Set the video base address that was passed from the booter.
   Check to see if we want gray bars, if so display 40 lines
   of gray, a couple of lines of white(about 8), and loop to slow this
   down.
   then restore regs

REALLY BIG GREY BARS!! REALLY!!
*/

   asm("movl a0, sp@-");
   asm("movl a1, sp@-");
   asm("movl d0, sp@-");
   asm("movl d1, sp@-");


/* set video addr once ! */

   if (!flag)
   {
	flag++;
	gray_nextaddr = (unsigned long *)videoaddr;
   }

/* check to see if gray bars are turned off */
   if ((machineid & 0x20000)) 
   {
   /* MF the 10*rowbytes is done lots, but we want this to be slow, so its ok */
   for(i = 0; i < 10*videorowbytes; i++)
      *gray_nextaddr++ = 0xaaaaaaaa;
   for(i = 0; i < 2*videorowbytes; i++)
      *gray_nextaddr++ = 0x00000000;
	
   for(i=0;i<100000;i++);
   }


   asm("movl sp@+, d1");
   asm("movl sp@+, d0");
   asm("movl sp@+, a1");
   asm("movl sp@+, a0");
}


void dprintf(unsigned long value)
{
   static int count = 1, i;
   static char hex[] = "0123456789ABCDEF";
   void macputchar(dev_t,int);

   macputchar((dev_t)0,(count/10)+'0');
   macputchar((dev_t)0,(count%10)+'0');
   count++;
   macputchar((dev_t)0,':');
   macputchar((dev_t)0,' ');
   macputchar((dev_t)0,'0');
   macputchar((dev_t)0,'x');
   for (i = 7; i >= 0; i--)
     macputchar((dev_t)0,hex[(value >> (i*4)) & 0xF]);
   macputchar((dev_t)0,'\n');
   macputchar((dev_t)0,'\r');
}

void strprintf(char *str, unsigned long value)
{
   static int i;
   void macputchar(dev_t,int);

   while (*str)
     macputchar((dev_t)0,*str++); 
   macputchar((dev_t)0,':');
   macputchar((dev_t)0,' ');
   dprintf(value);

}

void hex_dump(int addr, int len)
{
  int i,j;
  char p;
  static long prev = 0;

  if (addr == -1)
    addr=prev;
  for (i=0;i<len;i+=16)
  {
    printf("0x%08x: ",addr+i);
    for (j=0;j<16;j++)
      printf("%02x ",(int)(*(unsigned char *)(addr+i+j)));
    printf("  ");
    for (j=0;j<16;j++)
    {
      p= *(char *)(addr+i+j);
      if (p >= ' ' && p < 127)
        printf("%c",p);
      else
        printf(".");
    }
    printf("\n");
  }
  prev=addr+len;
}

void stack_trace(struct my_frame *fp)
{
  unsigned long *a6;
  int i;

  printf("D: ");
  for(i=0;i<8;i++)
     printf("%08x ", fp->dregs[i]);
  printf("\nA:");
  for(i=0;i<8;i++)
     printf("%08x ", fp->aregs[i]);
  printf("\n");
  printf("FP:%08x ", fp->aregs[6]);
  printf("SP:%08x\n", fp->aregs[7]);

  printf ("Stack trace:\n");

  a6 = (unsigned long *)fp -> aregs[6];

  while (a6) {
    printf ("  Return addr = 0x%08x\n",(unsigned long)a6[1]);
    a6 = (unsigned long *)*a6;
  }
}

void stack_list(unsigned long *a6)
{
  int i;

  printf ("Stack trace:\n");

  while (a6) {
    printf ("  (a6 == 0x%08x)", a6);
    printf ("  Return addr = 0x%08x\n",(unsigned long)a6[1]);
    a6 = (unsigned long *)*a6;
  }
}

void print_bus(struct my_frame *fp)
{
  int format;

  printf("\n\nKernel Panic -- Bus Error\n\n");
  format = fp -> frame >> 12;
  switch (format)
  {
    case 0: printf ("Normal Stack Frame\n\n"); break;
    case 1: printf ("Throwaway Stack Frame\n\n"); break;
    case 10: printf ("Short Bus Cycle Stack Frame\n\n"); break;
    case 11: printf ("Long Bus Cycle Stack Frame\n\n"); break;
    default: printf ("Unknown stack frame format: %d\n\n",(int)format); break;
  }
  if (format == 10 || format == 11)
  {
    printf ("Data cycle fault address: 0x%08x\n",fp -> Address);
    printf ("Data output buffer 0x%08x\n",fp -> DataOutBuf);
  }
  printf ("Status word: 0x%04x\n",(long)fp -> sr);
  printf ("Program counter: 0x%08x\n",fp -> pc);
  printf ("Stack pointer: 0x%08x\n",fp -> aregs[7]);
  printf ("Frame: 0x%04x\n",fp -> frame);
  printf ("MMU status register: %04x\n", get_mmusr());
  stack_trace(fp);
#ifdef NO_MY_CORE_DUMP
  my_core_dump(fp);
#endif
}


/* Rest of file by Brad */


#define PMapPTE(v)	(&Sysmap[(vm_offset_t)(v) >> PG_SHIFT])
#define brad_kvtoste(va) (&kmem_map->pmap->pm_stab[va>>SG_ISHIFT])


force_pte_invalid(
	int addr)
{
	PMapPTE(addr)->pg_v = 0;
	PMapPTE(addr)->pg_prot = 1;
	TBIA();
}


force_pte_valid(
	int addr)
{
	int valid;

	valid = PMapPTE(addr)->pg_v;
	PMapPTE(addr)->pg_v = PG_V;
	TBIA();
	return(valid);
}


int md_phys(
	int vaddr)
{
	int pa;

	return(*((int *)PMapPTE(vaddr)) & PG_FRAME);
}


int md_virt(
	int paddr)
{
	int va, pa;

	for(va = NBPG; va != 0; va += NBPG)
	   if(brad_kvtoste(va)->sg_v)
	      if(kvtopte(va)->pg_v)
	         if(paddr == kvtopte(va)->pg_pfnum << PG_SHIFT)
                    return(va + (paddr & (~ PG_FRAME)));
/*	for(va = - 10 * 1024 * 1024; va != 0; va += NBPG)
	   if(brad_kvtoste(va)->sg_v)
	      if(kvtopte(va)->pg_v)
	         if(paddr == kvtopte(va)->pg_pfnum << PG_SHIFT)
                    return(va + (paddr & (~ PG_FRAME))); */
	return(0xffffffff);
}


int get_crp_pa(register long crp[2])
{
	asm __volatile ("pmove crp, %0@" : : "a" (crp));
}
	

int get_srp_pa(register long srp[2])
{
	asm __volatile ("pmove srp, %0@" : : "a" (srp));
}


int clr_mmusr()
{
   	int q=0;

	asm __volatile ("pmove %0, psr" : : "d" (q));
}


int get_mmusr()
{
	int q;

	asm __volatile ("pmove psr, %0" : : "d" (q));
	return(q);
}


#define MEGABYTE 1048576


static int
tmpbadaddr(caddr_t addr)
{
	unsigned long int	k = 0xdeadbee0; /* we search mem for this */
	unsigned long int	tk;

	k = k | 0xf;
	tk = *(unsigned long int *)addr;
	*(unsigned long int *)addr = k;

	if (*(unsigned long int *) addr != k && (unsigned long int *)addr != &k)
		return 1;

	*(unsigned long int *)addr = tk;
	return 0;
}


/* This next function used to use badaddr() to step through memory
    looking for the end of physical memory, but it wouldn't work
    because someone at Apple was drunk when they designed the memory
    interface... */

char mem_store[4096], *mem_storep;

int get_top_of_ram(void)
{
	unsigned long	search=0xb00bfade;
	unsigned long	i, found, store;
	char		*p, *zero;

	return((((machineid >> 7) & 0x1f) * MEGABYTE) - 4096);

#if TESTING 	/* Why doesn't any of this code work? */
	found = 0;
	zero = p = 0;
	while(!tmpbadaddr(p) && ((unsigned long)p) < 0x40000000)
		p += 4096;

	sprintf(mem_store, "mem store test - %x?\n", ((unsigned long)p) -
	    4096);

	/* This should be interesting: */
	store = (*(unsigned long *) zero);
	(*(unsigned long *) zero) = search;
	for(p = (char *)0; p < (char *)0x1000000; p += 4)
		if((*(unsigned long *)p) == search){
			sprintf(mem_store + strlen(mem_store),
			 "Ooo! I found repeat at 0x%x!\n", p);
		}
	return(p);
	(*(unsigned long *) zero) = store;

	p = 0x40000000;

	if(((unsigned long)p) == 0x40000000){

		p = 0;

		store = *((unsigned long *) p);
		*((unsigned long *) p) = search;

		p += MEGABYTE;
		while (*((unsigned long *) p) != search && (unsigned long) p < 0x0a000000) {
			p += MEGABYTE;
		}

		*((unsigned long *) zero) = store;
		if ((unsigned long) p >= 0x0a000000) {
  			return 0x400000-4096;
		}
	}
	return (((unsigned long)p) - 4096);
#endif
}

#if THIS_FUNCTION_IS_READY
dump_mem_map(
	int sva,
	int eva,
	int use_srp)
{
	sva &= PG_FRAME;
	eva &= PG_FRAME;
	while(sva != eva){
	}
}
#endif /* this function is NOT READY */


print_rp(
	int use_srp)
{
	long rp[2];

	if(use_srp)
		get_srp_pa(rp);
	else
		get_crp_pa(rp);

	printf("%s: %x,%x\n", use_srp ? "SRP" : "CRP", rp[0], rp[1]);
}


#if TRYING_TO_MAKE_KERNEL_FAIL


print_pte_dups(
	int addr)
{
	int pa, ad;
	unsigned int va;
	int stnum, ptnum;

	ad = kvtopte(addr)->pg_pfnum << PG_SHIFT;
	printf("Segment table entries:\n");
	for(va = NBPG; va != 0; va += NBPG){
	   if(brad_kvtoste(va)->sg_v)
              if (kvtopte(va)->pg_v){
	      pa = kvtopte(va)->pg_pfnum << PG_SHIFT;
	      if((pa >= ad - NBPG) && (pa <= ad + NBPG))
	         printf("print_pte_dups: VA 0x%x maps to PA 0x%x\n", va, pa);
	   }
	}
}


dump_ptes()
{
	int va, pa;

	for(va = NBPG; va < 20 * 1024 * 1024; va += NBPG)
	   if(brad_kvtoste(va)->sg_v)
	      if(kvtopte(va)->pg_v)
	         printf("VA %d maps to PA %d\n", va,
	          kvtopte(va)->pg_pfnum << PG_SHIFT);
	for(va = - 10 * 1024 * 1024; va != 0; va += NBPG)
	   if(brad_kvtoste(va)->sg_v)
	      if(kvtopte(va)->pg_v)
	         printf("VA %d maps to PA %d\n", va,
                  kvtopte(va)->pg_pfnum << PG_SHIFT);
}


#endif


int alice_debug(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
   printf("*AHEM* -- process %d says hello.\n", p->p_pid);
   return(0);
}


/* BARF MF this if from i386 machdep.c, locore.s version commented out */
/*
 * Below written in C to allow access to debugging code
copyinstr(fromaddr, toaddr, maxlength, lencopied) u_int *lencopied, maxlength;
	void *toaddr, *fromaddr; {
	int c,tally;
printf("copyinstr: entered maxlen=%u\n",maxlength);

	tally = 0;
	while (maxlength--) {
		c = fubyte(fromaddr++);
		if (c == -1) {
			if(lencopied) *lencopied = tally;
printf("copyinstr: returing efault tally=%d\n",tally);
			return(EFAULT);
		}
		tally++;
		*(char *)toaddr++ = (char) c;
		if (c == 0){
			if(lencopied) *lencopied = (u_int)tally;
printf("copyinstr: returing 0 tally=%d\n",tally);
			return(0);
		}
	}
	if(lencopied) *lencopied = (u_int)tally;
printf("copyinstr: exited nametoolong tally=%d\n",tally);
	return(ENAMETOOLONG);
}
 */

#define COMPAT_NOMID	1
cpu_exec_makecmds(p, epp)
struct proc *p;
struct exec_package *epp;
{
#ifdef COMPAT_NOMID
  u_long midmag, magic;
  u_short mid;
  int error;

  midmag = ntohl(epp->ep_execp->a_midmag);
  mid = (midmag >> 16 ) & 0xffff;
  magic = midmag & 0xffff;

  switch (mid << 16 | magic) {
  case (MID_ZERO << 16) | ZMAGIC:
    error = cpu_exec_prep_oldzmagic(p, epp);
    break;
  default:
    error = ENOEXEC;
  }

  return error;
#else /* ! COMPAT_NOMID */
  return ENOEXEC;
#endif
}

#ifdef COMPAT_NOMID
int
cpu_exec_prep_oldzmagic(p, epp)
     struct proc *p;
     struct exec_package *epp;
{
  struct exec *execp = epp->ep_execp;
  struct exec_vmcmd *ccmdp;

#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up size fields in epp\n");
#endif
  epp->ep_taddr = 0;
  epp->ep_tsize = execp->a_text;
  epp->ep_daddr = epp->ep_taddr + execp->a_text;
  epp->ep_dsize = execp->a_data + execp->a_bss;
  epp->ep_maxsaddr = USRSTACK - MAXSSIZ;
  epp->ep_minsaddr = USRSTACK;
  epp->ep_ssize = p->p_rlimit[RLIMIT_STACK].rlim_cur;
  epp->ep_entry = execp->a_entry;

  /* check if vnode is in open for writing, because we want to demand-page
   * out of it.  if it is, don't do it, for various reasons
   */
  if ((execp->a_text != 0 || execp->a_data != 0) &&
      (epp->ep_vp->v_flag & VTEXT) == 0 && epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
    if (epp->ep_vp->v_flag & VTEXT)
      panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
    epp->ep_vcp = NULL;
#ifdef EXEC_DEBUG
    printf("exec_prep_oldzmagic: returning with ETXTBSY\n");
#endif
    return ETXTBSY;
  }
  epp->ep_vp->v_flag |= VTEXT;

  /* set up command for text segment */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up text segment commands\n");
#endif
  epp->ep_vcp = new_vmcmd(vmcmd_map_pagedvn,
			  execp->a_text,
			  epp->ep_taddr,
			  epp->ep_vp,
			  NBPG,                  /* should this be CLBYTES? */
			  VM_PROT_READ|VM_PROT_EXECUTE);
  ccmdp = epp->ep_vcp;

  /* set up command for data segment */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up data segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_pagedvn,
			     execp->a_data,
			     epp->ep_daddr,
			     epp->ep_vp,
			     execp->a_text + NBPG, /* should be CLBYTES? */
			     VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
  ccmdp = ccmdp->ev_next;

  /* set up command for bss segment */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up bss segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
			     execp->a_bss,
			     epp->ep_daddr + execp->a_data,
			     0,
			     0,
			     VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
  ccmdp = ccmdp->ev_next;

  /* set up commands for stack.  note that this takes *two*, one
   * to map the part of the stack which we can access, and one
   * to map the part which we can't.
   *
   * arguably, it could be made into one, but that would require
   * the addition of another mapping proc, which is unnecessary
   *
   * note that in memory, thigns assumed to be:
   *    0 ....... ep_maxsaddr <stack> ep_minsaddr
   */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up unmapped stack segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
			     ((epp->ep_minsaddr - epp->ep_ssize) -
			      epp->ep_maxsaddr),
			     epp->ep_maxsaddr,
			     0,
			     0,
			     VM_PROT_NONE);
  ccmdp = ccmdp->ev_next;
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up mapped stack segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
			     epp->ep_ssize,
			     (epp->ep_minsaddr - epp->ep_ssize),
			     0,
			     0,
			     VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: returning with no error\n");
#endif
  return 0;
}
#endif /* COMPAT_NOMID */

void
likeohmigod(void)
{
	register struct proc *p = curproc;
	if (p)
		printf("-%d,0x%x,0x%x\n", p->p_pid, p->p_regs[PC], p->p_regs[SP]);
	return;
	if (p)
	  printf("swtch %d (%s) out.\n", p->p_pid, p->p_comm);
	else
	  printf("proc NULL out.\n", p->p_pid, p->p_comm);
/*	printf("proc %d (%s, pc=0x%x, sp=0x%x being switched out.\n",
		p->p_pid, p->p_comm, p->p_regs[PC], p->p_regs[SP]); */
}

void
likeyuhknow(void)
{
	register struct proc *p = curproc;
	if (p)
		printf("+%d,0x%x,0x%x\n", p->p_pid, p->p_regs[PC], p->p_regs[SP]);
	return;
	if (p)
	  printf("proc %d (%s) in.\n", p->p_pid, p->p_comm);
	else
	  printf("proc NULL in.\n", p->p_pid, p->p_comm);
/*	printf("proc %d (%s, pc=0x%x, sp=0x%x being switched in.\n",
		p->p_pid, p->p_comm, p->p_regs[PC], p->p_regs[SP]);*/
}
