/*	$NetBSD: machdep.c,v 1.42 1995/04/22 20:27:33 christos Exp $	*/

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
/*
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <net/netisr.h>

#define	MAXMEM	64*1024*CLSIZE	/* XXX - from cmap.h */
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <dev/cons.h>

#include "via.h"
#include "macrom.h"

/* The following is used externally (sysctl_hw) */
char machine[] = "mac68k";	/* cpu "architecture" */

vm_map_t buffer_map;
extern vm_offset_t avail_end;

int dbg_flg = 0;
struct mac68k_machine_S	mac68k_machine;

volatile unsigned char	*Via1Base;
unsigned long		NuBusBase = NBBASE;
unsigned long		IOBase;

extern unsigned long	videoaddr;
extern unsigned long	videorowbytes;
u_int			cache_copyback = PG_CCB;
unsigned long		int_video_start, int_video_length;

extern vm_size_t	Sysptsize; /* in pmap.c */

/* These are used to map kernel space: */
extern int		numranges;
extern unsigned long	low[8];
extern unsigned long	high[8];

/* These are used to map NuBus space: */
#define		NBMAXRANGES	16
int		nbnumranges;  /* = 0 == don't use the ranges */
unsigned long	nbphys[NBMAXRANGES];    /* Start physical addr of this range */
unsigned long	nblog[NBMAXRANGES];     /* Start logical addr of this range */
/* If the length is negative, the all physical addresses are the same: */
long		nblen[NBMAXRANGES];     /* Length of this range */

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
 * For the fpu emulation and fpu driver.
 */
int	fpu_type;

static void	identifycpu(void);

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	/*
	 * Generic console: sys/dev/cons.c
	 *	Initializes either ite or ser as console.
	 */

	/* Cause called from locore sometimes: */
	static int	init; /* = 0 */

	if (!init) {
		cninit();

#ifdef  DDB
		/*
		 * Initialize kernel debugger, if compiled in.
		 */
		ddb_init();
#endif
		init = 1;
	}
}

#define CURRENTBOOTERVER	106

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	int	vers;
	register unsigned i;
	register caddr_t v, firstaddr;
	int base, residual;
	extern long Usrptsize;
	extern struct map *useriomap;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size = 0; /* To avoid compiler warning */

	/*
	 * Initialize error message buffer (at end of core).
	 */
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
#ifdef MACHINE_NONCONTIG
		pmap_enter(pmap_kernel(), (vm_offset_t) msgbufp,
			   avail_end + i * NBPG, VM_PROT_ALL, TRUE);
#else  /* MACHINE_NONCONTIG */
		pmap_enter(pmap_kernel(), (vm_offset_t) msgbufp,
			   avail_end + i * NBPG, VM_PROT_ALL, TRUE);
#endif /* MACHINE_NONCONTIG */
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();

	vers = mac68k_machine.booter_version;
	if (vers < CURRENTBOOTERVER) {
		int	delay;

		/* fix older booters with indicies, not versions */
		if (vers < 100) vers += 99;

		printf("\nYou booted with booter version %d.%d.\n",
			vers / 100, vers % 100);
		printf("Booter version %d.%d is necessary to fully support\n",
			CURRENTBOOTERVER / 100, CURRENTBOOTERVER % 100);
		printf("this kernel.\n\n");
		for (delay=0 ; delay < 1000000 ; delay++);
	}
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
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif
	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory for the first 2 Meg, 5% of the remaining
	 * memory. Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		if (physmem < btoc(2 * 1024 * 1024))
			bufpages = physmem / 10 / CLSIZE;
		else
			bufpages = (btoc(2 * 1024 * 1024) + physmem) / 20 / CLSIZE;

	bufpages = min(NKMEMCLUSTERS*2/5, bufpages);

	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
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

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* Don't want to alloc more physical mem than needed. */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
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
		vm_map_simplify(buffer_map, curbuf);
	}
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
				   M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	printf("avail mem = %d\n", ptoa(cnt.v_free_count));
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);

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
setregs(p, pack, sp, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long sp;
	register_t *retval;
{
	struct frame	*frame;

	frame = (struct frame *) p->p_md.md_regs;
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[SP] = sp;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;

	if (fpu_type) {
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
	}
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
int sigdebug = 0x0;
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
	u_long code;
{
	register struct proc *p = curproc;
	register struct sigframe *fp, *kfp;
	register struct frame *frame;
	register struct sigacts *psp = p->p_sigacts;
	register short ft;
	int oonstack;
	extern short exframesize[];
	extern char sigcode[], esigcode[];

/*printf("sendsig %d %d %x %x %x\n", p->p_pid, sig, mask, code, catcher);*/

	frame = (struct frame *)p->p_md.md_regs;
	ft = frame->f_format;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;


		/*
		 * build the short SunOS frame instead
		 */
		sunos_sendsig(catcher, sig, mask, code);
		return;
	}
#endif
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK) && oonstack == 0 &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_base +
		    psp->ps_sigstk.ss_size - sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else
		fp = (struct sigframe *)frame->f_regs[SP] - 1;
	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d ssp %x usp %x scp %x ft %d\n",
		       p->p_pid, sig, &oonstack, fp, &fp->sf_sc, ft);
#endif
	if (useracc((caddr_t)fp, sizeof(struct sigframe), B_WRITE) == 0) {
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
	kfp = malloc(sizeof(struct sigframe), M_TEMP, M_WAITOK);
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

	if (fpu_type) {
		kfp->sf_state.ss_flags |= SS_FPSTATE;
		m68881_save(&kfp->sf_state.ss_fpstate);
	}
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&kfp->sf_state.ss_fpstate)
		printf("sendsig(%d): copy out FP state (%x) to %x\n",
		       p->p_pid, *(u_int *)&kfp->sf_state.ss_fpstate,
		       &kfp->sf_state.ss_fpstate);
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
	(void) copyout((caddr_t)kfp, (caddr_t)fp, sizeof(struct sigframe));
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
	frame->f_pc = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));
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
	struct sigreturn_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap;
	register_t *retval;
{
	struct sigcontext *scp, context;
	struct frame *frame;
	int rf, flags;
	struct sigstate tstate;
	extern short exframesize[];

	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %x\n", p->p_pid, scp);
#endif
	if ((int)scp & 1)
		return(EINVAL);
	/*
	 * Test and fetch the context structure.
	 * We grab it all at once for speed.
	 */
	if (useracc((caddr_t)scp, sizeof(*scp), B_WRITE) == 0 ||
	    copyin(scp, &context, sizeof(context)))
		return(EINVAL);
	scp = &context;
	if ((scp->sc_ps & (PSL_MBZ|PSL_IPL|PSL_S)) != 0)
		return(EINVAL);
	/*
	 * Restore the user supplied information
	 */
	if (scp->sc_onstack & 1)
		p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else 
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = scp->sc_mask &~ sigcantmask;
	frame = (struct frame *) p->p_md.md_regs;
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
		       p->p_pid, &flags, scp->sc_sp, SCARG(uap, sigcntxp),
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
	if ((howto&RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;

		/*
		 * Release inodes, sync and unmount the filesystems.
		 */
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
	splhigh();			/* extreme priority */
	if (howto&RB_HALT) {
		/* LAK: Actually shut down machine */
		printf("halted\n\n");
#if 1
		via_shutdown();  /* in via.c */
#else
		asm("	stop	#0x2700");
#endif
	} else {
		if (howto & RB_DUMP)
			dumpsys();
		doboot();
		/*NOTREACHED*/
	}
	printf("            The system is down.\n");
	printf("You may reboot or turn the machine off, now.\n");
	for (;;) ; /* Foil the compiler... */
	/*NOTREACHED*/
}

unsigned int	dumpmag = 0x8fca0101;	/* magic number for savecore */
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
void
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

straytrap(frame)
	struct frame frame;
{
	printf("unexpected trap; vector offset 0x%x from 0x%x.\n",
		frame.f_vector, frame.f_pc);
	regdump(&frame, 128);
	dumptrace();
	Debugger();
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
	if (netisr & (1 << NETISR_ARP)) {
		netisr &= ~(1 << NETISR_ARP);
		arpintr();
	}
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

nmihand(struct frame frame)
{
	static int	nmihanddeep = 0;

	if (nmihanddeep++ >= 4) /* then this is the fifth */
		asm("stop #2700");
	printf("PC is 0x%x.\n", frame.f_pc);
/*	regdump(&frame, 128);
	dumptrace(); */
	Debugger();
	nmihanddeep--;
}

regdump(frame, sbytes)
  struct frame *frame;
  int sbytes;
{
	static int doingdump = 0;
	register int i;
	int s;

	if (doingdump)
		return;
	s = splhigh();
	doingdump = 1;
	printf("pid = %d, pc = 0x%08x, ", curproc->p_pid, frame->f_pc);
	printf("ps = 0x%08x, ", frame->f_sr);
	printf("sfc = 0x%08x, ", getsfc());
	printf("dfc = 0x%08x\n", getdfc());
	printf("Registers:\n     ");
	for (i = 0; i < 8; i++)
		printf("        %d", i);
	printf("\ndreg:");
	for (i = 0; i < 8; i++)
		printf(" %08x", frame->f_regs[i]);
	printf("\nareg:");
	for (i = 0; i < 8; i++)
		printf(" %08x", frame->f_regs[i+8]);
	if (sbytes > 0) {
		if (1) /* (frame->f_sr & PSL_S) */ { /* BARF - BG */
			printf("\n\nKernel stack (%08x):",
			       (int)(((int *)frame) - 1));
			dumpmem(((int *)frame)-1, sbytes);
		} else {
			printf("\n\nUser stack (%08x):", frame->f_regs[15]);
			dumpmem((int *)frame->f_regs[15], sbytes);
		}
	}
	doingdump = 0;
	splx(s);
}


extern char kstack[];
#define KSADDR	((int *)&(kstack[(UPAGES-1)*NBPG]))

dumpmem(ptr, sz)
	register unsigned int *ptr;
	int sz;
{
	register int i, val, same;

	sz /= 4;
	for (i = 0; i < sz; i++) {
		if ((i & 7) == 0)
			printf("\n%08x: ", (unsigned int) ptr);
		else
			printf(" ");
		printf("%08x ", *ptr++);
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

#if 0
extern void	macserputchar(unsigned char c);
#endif

void dprintf(unsigned long value)
{
   static int count = 1, i;
   static char hex[] = "0123456789ABCDEF";
   void itecnputc(dev_t,int);

   itecnputc((dev_t)0,(count/10)+'0');
   itecnputc((dev_t)0,(count%10)+'0');
   count++;
   itecnputc((dev_t)0,':');
   itecnputc((dev_t)0,' ');
   itecnputc((dev_t)0,'0');
   itecnputc((dev_t)0,'x');
   for (i = 7; i >= 0; i--)
     itecnputc((dev_t)0,hex[(value >> (i*4)) & 0xF]);
   itecnputc((dev_t)0,'\n');
   itecnputc((dev_t)0,'\r');
}

void strprintf(char *str, unsigned long value)
{
   static int i;
   void itecnputc(dev_t,int);

   while (*str)
     itecnputc((dev_t)0,*str++); 
   itecnputc((dev_t)0,':');
   itecnputc((dev_t)0,' ');
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

void stack_trace(struct frame *fp)
{
  unsigned long *a6;
  int i;

  printf("D: ");
  for(i=0;i<8;i++)
     printf("%08x ", fp->f_regs[i]);
  printf("\nA:");
  for(i=0;i<8;i++)
     printf("%08x ", fp->f_regs[i+8]);
  printf("\n");
  printf("FP:%08x ", fp->f_regs[A6]);
  printf("SP:%08x\n", fp->f_regs[SP]);

  printf ("Stack trace:\n");

  a6 = (unsigned long *)fp -> f_regs[A6];

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

void print_bus(struct frame *fp)
{
  int format;

  printf("\n\nKernel Panic -- Bus Error\n\n");
  format = fp -> f_format;
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
    printf ("Data cycle fault address: 0x%08x\n",fp -> F_u.F_fmtA.f_dcfa);
    printf ("Data output buffer 0x%08x\n",fp -> F_u.F_fmtA.f_dob);
  }
  printf ("Status word: 0x%04x\n",(long)fp -> f_sr);
  printf ("Program counter: 0x%08x\n",fp -> f_pc);
  printf ("Stack pointer: 0x%08x\n",fp -> f_regs[SP]);
  printf ("Frame: 0x%04x\n",fp -> f_vector + format << 12);
  printf ("MMU status register: %04x\n", get_mmusr());
  stack_trace(fp);
#ifdef NO_MY_CORE_DUMP
  my_core_dump(fp);
#endif
}

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

	return((mac68k_machine.mach_memsize * MEGABYTE) - 4096);

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
	extern dddprintf(char *, int, int);

	for(va = NBPG; va < 20 * 1024 * 1024; va += NBPG)
	   if(brad_kvtoste(va)->sg_v)
	      if(kvtopte(va)->pg_v)
	         dddprintf("VA %d maps to PA %d\n", va,
	          kvtopte(va)->pg_pfnum << PG_SHIFT);
	for(va = - 10 * 1024 * 1024; va != 0; va += NBPG)
	   if(brad_kvtoste(va)->sg_v)
	      if(kvtopte(va)->pg_v)
	         dddprintf("VA %d maps to PA %d\n", va,
                  kvtopte(va)->pg_pfnum << PG_SHIFT);
}
#endif

int alice_debug(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{
   printf("*AHEM* -- process %d says hello.\n", p->p_pid);
   return(0);
}

/*
 * machine dependent system variables.
 */
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error = ENOEXEC;
	struct exec *execp = epp->ep_hdr;

#ifdef COMPAT_NOMID
	if (execp->a_midmag == ZMAGIC) /* i.e., MID == 0. */
		return cpu_exec_prep_oldzmagic(p, epp);
#endif

#ifdef COMPAT_SUNOS
	{
		extern sunos_exec_aout_makecmds __P((struct proc *,
						   struct exec_package *));
		if ((error = sunos_exec_aout_makecmds(p, epp)) == 0)
			return 0;
	}
#endif
	return error;
}

#ifdef COMPAT_NOMID
int
cpu_exec_prep_oldzmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_hdr;
	struct exec_vmcmd *ccmdp;

	epp->ep_taddr = 0;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* check if vnode is in open for writing, because we want to demand-page
	 * out of it.  if it is, don't do it, for various reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
		epp->ep_taddr, epp->ep_vp, NBPG, /* should NBPG be CLBYTES? */
		VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
		epp->ep_daddr, epp->ep_vp,
		execp->a_text + NBPG, /* should NBPG be CLBYTES? */
	 	VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
		epp->ep_daddr + execp->a_data, NULLVP, 0,
		VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}
#endif /* COMPAT_NOMID */

void ddprintf (char *fmt, int val)
{
#if 0
  char buf[128], *s;

  if (!mac68k_machine.serial_boot_echo) return;
  sprintf (buf, fmt, val);
  for (s = buf; *s; s++) {
    macserputchar (*s);
    if (*s == '\n') {
      macserputchar ('\r');
    }
  }
#endif
}

void dddprintf (char *fmt, int val1, int val2)
{
#if 0
  char buf[128], *s;

  if (!mac68k_machine.serial_boot_echo) return;
  sprintf (buf, fmt, val1, val2);
  for (s = buf; *s; s++) {
    macserputchar (*s);
    if (*s == '\n') {
      macserputchar ('\r');
    }
  }
#endif
}

static char *envbuf = NULL;

void initenv (unsigned long flag, char *buf)
{
  /*
   * If flag & 0x80000000 == 0, then we're booting with the old booter
   * and we should freak out.
   */

  if ((flag & 0x80000000) == 0) {
    /* Freak out; print something if that becomes available */
  } else {
    envbuf = buf;
  }
}

static char toupper (char c)
{
  if (c >= 'a' && c <= 'z') {
    return c - 'a'+ 'A';
  } else {
    return c;
  }
}

static long getenv (char *str)
{
  /*
   * Returns the value of the environment variable "str".
   *
   * Format of the buffer is "var=val\0var=val\0...\0var=val\0\0".
   *
   * Returns 0 if the variable is not there, and 1 if the variable is there
   * without an "=val".
   */

  char *s, *s1, *s2;
  int val, base;

  s = envbuf;
  while (1) {
    for (s1 = str; *s1 && *s && *s != '='; s1++, s++) {
      if (toupper (*s1) != toupper (*s)) {
        break;
      }
    }
    if (*s1) {  /* No match */
      while (*s) {
        s++;
      }
      s++;
      if (*s == '\0') {  /* Not found */
        /* Boolean flags are FALSE (0) if not there */
        return 0;
      }
      continue;
    }
    if (*s == '=') { /* Has a value */
      s++;
      val = 0;
      base = 10;
      if (*s == '0' && (*(s+1) == 'x' || *(s+1) == 'X')) {
        base = 16;
        s += 2;
      } else if (*s == '0') {
        base = 8;
      }
      while (*s) {
        if (toupper (*s) >= 'A' && toupper (*s) <= 'F') {
          val = val * base + toupper (*s) - 'A' + 10;
        } else {
          val = val * base + (*s - '0');
        }
        s++;
      }
      return val;
    } else {  /* TRUE (1) */
      return 1;
    }
  }
}

/*
 *ROM Vector information for calling drivers in ROMs 
 */
static romvec_t romvecs[]=
{
	/* 
	 * Vectors verified for II, IIx, IIcx, SE/30
	 */
	{ /* 0 */
		"Mac II class ROMs", 
		(caddr_t)0x40807002,	/* where does ADB interrupt */
		0,			/* PM interrupt (?) */
		(caddr_t)0x4080a4d8,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x40807778,	/* CountADBs */
		(caddr_t)0x40807792,	/* GetIndADB */
		(caddr_t)0x408077be,	/* GetADBInfo */
		(caddr_t)0x408077c4,	/* SetADBInfo */
		(caddr_t)0x40807704,	/* ADBReInit */
		(caddr_t)0x408072fa,	/* ADBOp */
		0,			/* PMgrOp */
		(caddr_t)0x4080dd78,	/* ReadXPRam */
	},
	/*
	 * Vectors verified for PB 140, PB 170
	 * (PB 100?)
	 */
	{ /* 1 */
		"Powerbook class ROMs",
		(caddr_t)0x4088ae5e,	/* ADB interrupt */
		(caddr_t)0x408885ec,	/* PB ADB interrupt */
		(caddr_t)0x4088ae0e,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x408888ec,	/* PMgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Vectors verified for IIsi, IIvx, IIvi
	 */
	{ /* 2 */
		"Mac IIsi class ROMs",
		(caddr_t)0x40814912,	/* ADB interrupt */
		(caddr_t)0,		/* PM ADB interrupt */
		(caddr_t)0x408150f0,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0,		/* PMgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Vectors verified for Mac Classic II and LC II
	 * (LC III?  Other LC's?  680x0 Performas?)
	 */
	{ /* 3 */
		"Mac Classic II ROMs",
		(caddr_t)0x40a14912,	/* ADB interrupt */
		(caddr_t)0,		/* PM ADB interrupt */
		(caddr_t)0x40a150f0,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x40a0a360,	/* CountADBs */
		(caddr_t)0x40a0a37a,	/* GetIndADB */
		(caddr_t)0x40a0a3a6,	/* GetADBInfo */
		(caddr_t)0x40a0a3ac,	/* SetADBInfo */
		(caddr_t)0x40a0a752,	/* ADBReInit */
		(caddr_t)0x40a0a3dc,	/* ADBOp */
		(caddr_t)0,		/* PMgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Vectors verified for IIci
	 */
	{ /* 4 */
		"Mac IIci ROMs",
		(caddr_t)0x4080a700,	/* ADB interrupt */
		(caddr_t)0,		/* PM ADB interrupt */
		(caddr_t)0x4080a5aa,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0,		/* PMgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Vectors verified for Duo 230, PB 180, PB 165
	 * (Duo 210?  PB 165?  PB 160?  Duo 250?  Duo 270?)
	 */
	{ /* 5 */
		"2nd Powerbook class ROMs",
		(caddr_t)0x408b2eec,	/* ADB interrupt */
		(caddr_t)0x408885ec,	/* PB ADB interrupt */
		(caddr_t)0x408b2e76,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x408888ec,	/* PMgrOp */
		(caddr_t)0x4080B186,	/* ReadXPRam */
	},
	/*
	 * Quadra, Centris merged table (650, 610, Q800)
	 * (BG - this is a mish-mash of various sources, none complete,
	 *  so this may not work.)
	 */
	{ /* 6 */
		"Quadra/Centris ROMs",
		(caddr_t)0x408b2dea,	/* ADB int */
		0,			/* PM intr */
		(caddr_t)0x408b2c72,	/* ADBBase + 130 */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		0,			/* PMgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Quadra 840AV (but ADBBase + 130 intr is unknown)
	 * (PM intr is known to be 0, PMgrOp is guessed to be 0)
	 */
	{ /* 7 */
		"Quadra AV ROMs",
		(caddr_t)0x4080cac6,	/* ADB int */
		0,			/* PM int */
		/* !?! */ 0,		/* ADBBase + 130 */
		(caddr_t)0x40839600,	/* CountADBs */
		(caddr_t)0x4083961a,	/* GetIndADB */
		(caddr_t)0x40839646,	/* GetADBInfo */
		(caddr_t)0x4083964c,	/* SetADBInfo */
		(caddr_t)0x408397b8,	/* ADBReInit */
		(caddr_t)0x4083967c,	/* ADBOp */
		0,			/* PMgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * PB 540 (but ADBBase + 130 intr and PMgrOp is unknown)
	 * (PB 520?  Duo 280?)
	 */
	{ /* 8 */
		"68040 PowerBook ROMs",
		(caddr_t)0x400b2efc,	/* ADB int */
		(caddr_t)0x400d8e66,	/* PM int */
		/* !?! */ 0,		/* ADBBase + 130 */
		(caddr_t)0x4000a360,	/* CountADBs */
		(caddr_t)0x4000a37a,	/* GetIndADB */
		(caddr_t)0x4000a3a6,	/* GetADBInfo */
		(caddr_t)0x4000a3ac,	/* SetADBInfo */
		(caddr_t)0x4000a752,	/* ADBReInit */
		(caddr_t)0x4000a3dc,	/* ADBOp */
		/* !?! */ 0,		/* PmgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Q 605 (but guessing at ADBBase + 130, based on Q 650)
	 */
	{ /* 9 */
		"Quadra/Centris 605 ROMs",
		(caddr_t)0x408a9b56,	/* ADB int */
		0,			/* PM int */
		(caddr_t)0x408a99de,	/* ADBBase + 130 */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		0,			/* PmgrOp */
		0,	/* ReadXParam */
	},
	/*
	 * Vectors verified for Duo 270c
	 */
	{ /* 10 */
		"Duo 270C ROMs",
		(caddr_t)0x408b2efc,	/* ADB interrupt */
		(caddr_t)0x408885ec,	/* PB ADB interrupt */
		(caddr_t)0x408b2e86,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x408888ec,	/* PMgrOp */
		0,	/* ReadXParam */
	},
	/* Please fill these in! -BG */
};


struct cpu_model_info cpu_models[] = {

/* The first four. */
{ MACH_MACII,         "II ",       "",        MACH_CLASSII,	&romvecs[0] },
{ MACH_MACIIX,        "IIx ",      "",        MACH_CLASSII,	&romvecs[0] },
{ MACH_MACIICX,       "IIcx ",     "",        MACH_CLASSII,	&romvecs[0] },
{ MACH_MACSE30,       "SE/30 ",    "",        MACH_CLASSII,	&romvecs[0] },

/* The rest of the II series... */
{ MACH_MACIICI,       "IIci ",     "",        MACH_CLASSIIci,	&romvecs[4] },
{ MACH_MACIISI,       "IIsi ",     "",        MACH_CLASSIIsi,	&romvecs[2] },
{ MACH_MACIIVI,       "IIvi ",     "",        MACH_CLASSIIsi,	&romvecs[2] },
{ MACH_MACIIVX,       "IIvx ",     "",        MACH_CLASSIIsi,	&romvecs[2] },
{ MACH_MACIIFX,       "IIfx ",     "",        MACH_CLASSIIfx,	NULL },

/* The Centris/Quadra series. */
{ MACH_MACQ700,       "Quadra",    " 700 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACQ900,       "Quadra",    " 900 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACQ950,       "Quadra",    " 950 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACQ800,       "Quadra",    " 800 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACQ650,	      "Quadra",    " 650 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACC650,       "Centris",   " 650 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACQ605,       "Quadra",    " 605",    MACH_CLASSQ,	&romvecs[9] },
{ MACH_MACC610,	      "Centris",   " 610 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACQ610,       "Quadra",    " 610 ",   MACH_CLASSQ,	&romvecs[6] },
{ MACH_MACC660AV,     "Centris",   " 660AV ", MACH_CLASSQ,	&romvecs[7] },
{ MACH_MACQ840AV,     "Quadra",    " 840AV ", MACH_CLASSQ,	&romvecs[7] },

/* The Powerbooks/Duos... */
{ MACH_MACPB100,      "PowerBook", " 100 ",   MACH_CLASSPB,	&romvecs[1] },
	/* PB 100 has no MMU! */
{ MACH_MACPB140,      "PowerBook", " 140 ",   MACH_CLASSPB,	&romvecs[1] },
{ MACH_MACPB145,      "PowerBook", " 145 ",   MACH_CLASSPB,	&romvecs[1] },
{ MACH_MACPB160,      "PowerBook", " 160 ",   MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB165,      "PowerBook", " 165 ",   MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB165C,     "PowerBook", " 165c ",  MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB170,      "PowerBook", " 170 ",   MACH_CLASSPB,	&romvecs[1] },
{ MACH_MACPB180,      "PowerBook", " 180 ",   MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB180C,     "PowerBook", " 180c ",  MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB210,      "PowerBook", " 210 ",   MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB230,      "PowerBook", " 230 ",   MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB250,      "PowerBook", " 250 ",   MACH_CLASSPB,	&romvecs[5] },
{ MACH_MACPB270,      "PowerBook", " 270 ",   MACH_CLASSPB,	&romvecs[10] },

/* The Performas... */
{ MACH_MACP600,       "Performa",  " 600 ",   MACH_CLASSLC,	&romvecs[3] },
{ MACH_MACP460,       "Performa",  " 460 ",   MACH_CLASSLC,	&romvecs[3] },
{ MACH_MACP550,       "Performa",  " 550 ",   MACH_CLASSLC,	&romvecs[3] },

/* The LCs... */
{ MACH_MACLCII,       "LC",        " II ",    MACH_CLASSLC,	&romvecs[3] },
{ MACH_MACLCIII,      "LC",        " III ",   MACH_CLASSLC,	&romvecs[3] },
{ MACH_MACLC475,      "LC",        " 475 ",   MACH_CLASSLC,	&romvecs[3] },
{ MACH_MACLC520,      "LC",        " 520 ",   MACH_CLASSLC,	&romvecs[3] },
{ MACH_MACLC575,      "LC",        " 575 ",   MACH_CLASSLC,	&romvecs[3] },
/* Does this belong here? */
{ MACH_MACCLASSICII,  "Classic",   " II ",    MACH_CLASSLC,	&romvecs[3] },

/* The hopeless ones... */
{ MACH_MACCCLASSIC,   "Classic ",  "",        MACH_CLASSH,	NULL },
{ MACH_MACTV,         "TV ",       "",        MACH_CLASSH,	NULL },

/* The unknown one and the end... */
{ 0,                 "Unknown",    "",        MACH_CLASSII,	NULL },
{ 0,                 NULL,         NULL,      0,		NULL },
}; /* End of cpu_models[] initialization. */

/*
 * Missing Mac Models:
 *	PowerMac 6100
 *	PowerMac 7100
 *	PowerMac 8100
 *	PowerBook 540
 *	PowerBook 520
 *	PowerBook 150
 *	Duo 280
 *	Quadra	630
 *	Performa 6000s
 * 	...?
 */

char	cpu_model[120];	/* for sysctl() */

int
mach_cputype(void)
{
	return(mac68k_machine.mach_processor);
}

static void
identifycpu(void)
{
	char	*proc;

	switch(mac68k_machine.mach_processor) {
		case MACH_68020:
			proc = ("(68020)");
			break;	
		case MACH_68030:
			proc = ("(68030)");
			break;	
		case MACH_68040:
			proc = ("(68040)");
			break;	
		case MACH_PENTIUM:
		default:
			proc = ("(unknown processor)");
			break;	
	}
	sprintf(cpu_model, "Apple Macintosh %s%s %s",
		cpu_models[mac68k_machine.cpu_model_index].model_major,
		cpu_models[mac68k_machine.cpu_model_index].model_minor,
		proc);
	printf("%s\n", cpu_model);
}

static void
get_machine_info(void)
{
	char	*proc;
	int	i;

	for (i=0 ; cpu_models[i].model_major ; i++) {
		if (mac68k_machine.machineid == cpu_models[i].machineid)
			break;
	}

	if (cpu_models[i].model_major == NULL)
		i--;

	switch(mac68k_machine.mach_processor) {
		case MACH_68040:
			cpu040 = 1;
			break;	
		case MACH_68020:
		case MACH_68030:
		case MACH_PENTIUM:
		default:
			cpu040 = 0;
			break;	
	}

	mac68k_machine.cpu_model_index = i;
}

/*
 * getenvvars: Grab a few useful variables
 */
extern void
getenvvars (void)
{
  extern unsigned long	locore_dodebugmarks;
  extern unsigned long	bootdev, videobitdepth, videosize;
  extern unsigned long	end, esym;
  int			root_scsi_id;
  extern unsigned long	macos_boottime;
  extern long		macos_gmtbias;

  root_scsi_id = getenv ("ROOT_SCSI_ID");
  /*
   * For now, we assume that the boot device is off the first controller.
   */
  bootdev = (root_scsi_id << 16) | 4;

  boothowto = getenv ("SINGLE_USER");

	/* These next two should give us mapped video & serial */
	/* We need these for pre-mapping graybars & echo, but probably */
	/* only on MacII or LC.  --  XXX */
  /* videoaddr = getenv("MACOS_VIDEO"); */
  /* sccaddr = getenv("MACOS_SCC"); */

  /*
   * The following are not in a structure so that they can be
   * accessed more quickly.
   */
  videoaddr = getenv ("VIDEO_ADDR");
  videorowbytes = getenv ("ROW_BYTES");
  videobitdepth = getenv ("SCREEN_DEPTH");
  videosize = getenv ("DIMENSIONS");

  /*
   * More misc stuff from booter.
   */
  mac68k_machine.machineid = getenv("MACHINEID");
  mac68k_machine.mach_processor = getenv("PROCESSOR");
  mac68k_machine.mach_memsize = getenv("MEMSIZE");
  mac68k_machine.do_graybars = getenv("GRAYBARS");
  locore_dodebugmarks = mac68k_machine.do_graybars;
  mac68k_machine.serial_boot_echo = getenv("SERIALECHO");
  mac68k_machine.serial_console = getenv("SERIALCONSOLE");
		/* Should probably check this and fail if old */
  mac68k_machine.booter_version = getenv("BOOTERVER");

  /*
   * Get end of symbols for kernel debugging
   */
  esym = getenv("END_SYM");
  if (esym == 0) esym = (long) &end;
  
  /* Get MacOS time */
  macos_boottime = getenv("BOOTTIME");

  /* Save GMT BIAS saved in Booter parameters dialog box */
  macos_gmtbias = getenv("GMTBIAS");

  /*
   * Save globals stolen from MacOS
   */

  ROMBase = (caddr_t) getenv("ROMBASE");
  TimeDBRA = getenv("TIMEDBRA");
  ADBDelay = (u_short) getenv("ADBDELAY");
}

void printenvvars (void)
{
  extern unsigned long bootdev, videobitdepth, videosize;

  ddprintf ("bootdev = %u\n\r", (int)bootdev);
  ddprintf ("boothowto = %u\n\r", (int)boothowto);
  ddprintf ("videoaddr = %u\n\r", (int)videoaddr);
  ddprintf ("videorowbytes = %u\n\r", (int)videorowbytes);
  ddprintf ("videobitdepth = %u\n\r", (int)videobitdepth);
  ddprintf ("videosize = %u\n\r", (int)videosize);
  ddprintf ("machineid = %u\n\r", (int)mac68k_machine.machineid);
  ddprintf ("processor = %u\n\r", (int)mac68k_machine.mach_processor);
  ddprintf ("memsize = %u\n\r", (int)mac68k_machine.mach_memsize);
  ddprintf ("graybars = %u\n\r", (int)mac68k_machine.do_graybars);
  ddprintf ("serial echo = %u\n\r", (int)mac68k_machine.serial_boot_echo);
}

extern volatile unsigned char	*sccA;
extern volatile unsigned char	*ASCBase;

struct cpu_model_info *current_mac_model;

/*
 * Sets a bunch of machine-specific variables
 */
void
setmachdep(void)
{
static	int			firstpass = 1;
	struct cpu_model_info	*cpui;

	/*
	 * First, set things that need to be set on the first pass only
	 * Ideally, we'd only call this once, but for some reason, the
	 * VIAs need interrupts turned off twice !?
	 */
	if (firstpass) {
		get_machine_info();

		load_addr = 0;
	}

	cpui = &(cpu_models[mac68k_machine.cpu_model_index]);
	current_mac_model = cpui;

	/*
	 * Set up current ROM Glue vectors
	 */
	mrg_setvectors(cpui->rom_vectors);

	/*
	 * Set up any machine specific stuff that we have to before
	 * ANYTHING else happens
	 */
	switch(cpui->class){	/* Base this on class of machine... */
		case MACH_CLASSII:
			if (firstpass) {
				VIA2 = 1;
				IOBase = 0x50f00000;
				Via1Base = (volatile u_char *) IOBase;
				sccA = (volatile u_char *) 0x4000;
				ASCBase = (volatile u_char *) 0x14000;
				mac68k_machine.scsi80 = 1;
				mac68k_machine.sccClkConst = 115200;
			}
			via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
			via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */
			break;
		case MACH_CLASSPB:
			if (firstpass) {
				VIA2 = 1;
				IOBase = 0x50f00000;
				Via1Base = (volatile u_char *) IOBase;
				sccA = (volatile u_char *) 0x4000;
				ASCBase = (volatile u_char *) 0x14000;
				mac68k_machine.scsi80 = 1;
				mac68k_machine.sccClkConst = 115200;
			}
			/* Disable everything but PM; we need it. */
			via_reg(VIA1, vIER) = 0x6f;	/* disable VIA1 int */
			/* Are we disabling something important? */
			via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */
			break;
		case MACH_CLASSQ:
			if (firstpass) {
				VIA2 = 1;
				IOBase = 0x50f00000;
				Via1Base = (volatile u_char *) IOBase;
				sccA = (volatile u_char *) 0xc000;
				ASCBase = (volatile u_char *) 0x14000;
				mac68k_machine.scsi96 = 1;
				mac68k_machine.sccClkConst = 249600;
			}
			via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
			via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */
			break;
		case MACH_CLASSIIci:
			if (firstpass) {
				VIA2 = 0x13;
				IOBase = 0x50f00000;
				Via1Base = (volatile u_char *) IOBase;
				sccA = (volatile u_char *) 0x4000;
				ASCBase = (volatile u_char *) 0x14000;
				mac68k_machine.scsi80 = 1;
				mac68k_machine.sccClkConst = 122400;
			/*
			 * LAK: Find out if internal video is on.  If yes, then
			 * we loaded in bank B.  We need a better way to
			 * determine this, like use the TT0 register.
			 */
				if (rbv_vidstatus ()) {
					load_addr = 0x04000000;
				}
			}
			via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
			via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
			break;
		case MACH_CLASSIIsi:
			if (firstpass) {
				VIA2 = 0x13;
				IOBase = 0x50f00000;
				Via1Base = (volatile u_char *) IOBase;
				sccA = (volatile u_char *) 0x4000;
				ASCBase = (volatile u_char *) 0x14000;
				mac68k_machine.scsi80 = 1;
				mac68k_machine.sccClkConst = 122400;
			/*
			 * LAK: Find out if internal video is on.  If yes, then
			 * we loaded in bank B.  We need a better way to
			 * determine this, like use the TT0 register.
			 */
				if (rbv_vidstatus ()) {
					load_addr = 0x04000000;
				}
			}
			via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
			via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
			break;
		case MACH_CLASSLC:
			if (firstpass) {
				VIA2 = 0x13;
				IOBase = 0x50f00000;
				Via1Base = (volatile u_char *) IOBase;
				sccA = (volatile u_char *) 0x4000;
				ASCBase = (volatile u_char *) 0x14000;
				mac68k_machine.scsi80 = 1;
				mac68k_machine.sccClkConst = 122400;
			/*
			 * LAK: Find out if internal video is on.  If yes, then
			 * we loaded in bank B.  We need a better way to
			 * determine this, like use the TT0 register.
			 */
				if (rbv_vidstatus ()) {
					load_addr = 0x04000000;
				}
			}
			via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
			via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
			break;
		default:
		case MACH_CLASSH:
		case MACH_CLASSIIfx:
			break;
	}
	firstpass = 0;
}

void mmudebug (long phys2, long phys1, long logical)
{
  ddprintf ("logical = 0x%x\n", logical);
  ddprintf ("phys1 = 0x%x\n", phys1);
  ddprintf ("phys2 = 0x%x\n", phys2);
}

void gothere (long i)
{
  dddprintf ("Got here #%d (0x%x)\n", i, i);
}

void dump_pmaps (void)
{
  /* LAK: Dumps all of the page tables to serial */

  unsigned long *s, *p;
  extern unsigned long *Sysseg;
  int i, j;

  s = (unsigned long *)Sysseg;

  ddprintf ("About to dump the pmaps (%x):\n", (unsigned int)s);
  for (i = 0; i < 1000; i++) {
    if (s[i] & SG_V) {
      p = (unsigned long *)((s[i] & SG_FRAME) - load_addr);
      for (j = 0; j < 1000; j++) {
        if (p[j] & PG_V) {
          dddprintf ("%x --> %x\n", i*1024*4096 + j*4096, p[j] & PG_FRAME);
        }
      }
    }
  }
  ddprintf ("Just dumped the pmaps\n", 0);
}

unsigned long getphysical (unsigned long tc, unsigned long pte,
                           unsigned long psr, unsigned int kva)
{
  /*
   * LAK: (1/2/94) This function should be called right after a
   *  ptestr instruction.  tc is the current TC, pte is the
   *  one returned by ptestr, and psr is the PSR right after
   *  the ptestr.  This function returns the physical address
   *  of kva.
   */

  /*
   * Here is the general idea.  We do a ptestr, and the MMU looks
   * up "kva" as if it were looking up the address normally.  It
   * returns a pointer to the last PTE which it accesses (pte),
   * and puts a number in psr which says how many levels it
   * had to go through to get there.  This number of levels may
   * not be the number of levels that TC says it has because this
   * particular leaf may have been early-terminated.  Now we must
   * know the page size for that PTE to get the right offset into
   * that page.
   */

  unsigned int pagesize, pagebits, levels, i;

  pagebits = 32;  /* Start with 32-bit addressing */

  pagebits -= (tc >> 16) & 0xF; /* Subtract Initial Shift */

  /* Subtract each level of the table tree: */
  levels = psr & 0x7;
  for (i = 0; i < levels; i++) {
    pagebits -= (tc >> (12 - i * 4)) & 0xF;
  }

  /* Number of bits left must be the size of that page: */
  pagesize = 1 << pagebits;

  /* Mask off info bits: */
  pte &= 0xFFFFFF00;

  /* Add offset into page: */
  pte += kva & (pagesize-1);

  /* And return that sucker: */
  return pte;
}

static unsigned long gray_nextaddr = 0;

void
gray_bar2(void)
{
   static int i=0;
   static int flag=0;

/* Same premise as gray_bar, but bigger.  Gives a quicker check of
   where we are while debugging. */

   asm("movl a0, sp@-");
   asm("movl a1, sp@-");
   asm("movl d0, sp@-");
   asm("movl d1, sp@-");

/* check to see if gray bars are turned off */
   if (mac68k_machine.do_graybars) {
   	/* MF the 10*rowbytes is done lots, but we want this to be slow */
   	for(i = 0; i < 10*videorowbytes; i++)
      		((unsigned long *)videoaddr)[gray_nextaddr++] = 0xaaaaaaaa;
   	for(i = 0; i < 2*videorowbytes; i++)
      		((unsigned long *)videoaddr)[gray_nextaddr++] = 0x00000000;
   }

   asm("movl sp@+, d1");
   asm("movl sp@+, d0");
   asm("movl sp@+, a1");
   asm("movl sp@+, a0");
}

void
gray_bar(void)
{
   static int i=0;
   static int flag=0;

/* MF basic premise as I see it:
	1) Save the scratch regs as they are not saved by the compilier.
   	2) Check to see if we want gray bars, if so,
		display some lines of gray,
		a couple of lines of white(about 8),
		and loop to slow this down.
   	3) restore regs
*/

   asm("movl a0, sp@-");
   asm("movl a1, sp@-");
   asm("movl d0, sp@-");
   asm("movl d1, sp@-");

/* check to see if gray bars are turned off */
   if (mac68k_machine.do_graybars) {
   	/* MF the 10*rowbytes/4 is done lots, but we want this to be slow */
   	for(i = 0; i < 10*videorowbytes/4; i++)
      		((unsigned long *)videoaddr)[gray_nextaddr++] = 0xaaaaaaaa;
   	for(i = 0; i < 2*videorowbytes/4; i++)
      		((unsigned long *)videoaddr)[gray_nextaddr++] = 0x00000000;
   }

   asm("movl sp@+, d1");
   asm("movl sp@+, d0");
   asm("movl sp@+, a1");
   asm("movl sp@+, a0");
}

extern int	get_pte (unsigned int addr, unsigned long pte[2],
			unsigned short *psr); /* in locore */

/*
 * LAK (7/24/94): given a logical address, puts the physical address
 *  in *phys and return 1, or returns 0 on failure.  This is intended
 *  to look through MacOS page tables.
 */

unsigned long get_physical (unsigned int addr, unsigned long *phys)
{
	unsigned long		pte[2], ph;
	unsigned short		psr;
	int			i, numbits;
	extern unsigned int	macos_tc;

	i = get_pte (addr, pte, &psr);

	switch (i) {
		case -1:	return 0;
		case 0:		ph = pte[0] & 0xFFFFFF00; break;
		case 1:		ph = pte[1] & 0xFFFFFF00; break;
		default:	panic ("get_physical(): bad get_pte()");
	}

	/*
	 * We must now figure out how many levels down we went and
	 * mask the bits appropriately -- the returned value may only
	 * be the upper n bits, and we've got to take the rest from addr.
	 */

	numbits = 0;
	psr &= 0x0007;	/* Number of levels we went */
	for (i = 0; i < psr; i++) {
		numbits += (macos_tc >> (12 - i * 4)) & 0x0f;
	}

	/*
	 * We have to take the most significant "numbits" from
	 * the returned value "ph", and the rest from our addr.
	 * Assume that the lower (32-numbits) bits of ph are
	 * already zero.  Also assume numbits != 0.  Also, notice
	 * that this is an addition, not an "or".
	 */

	*phys = ph + (addr & ((1 << (32 - numbits)) - 1));

	return 1;
}

void printstar (void)
{
	/*
	 * Be careful calling this from assembly, it doesn't seem to
	 * save these registers properly.
	 */

	asm("movl a0, sp@-");
	asm("movl a1, sp@-");
	asm("movl d0, sp@-");
	asm("movl d1, sp@-");

	/* printf ("*"); */

	asm("movl sp@+, d1");
	asm("movl sp@+, d0");
	asm("movl sp@+, a1");
	asm("movl sp@+, a0");
}

/*
 * Find out how MacOS has mapped itself so we can do the same thing.
 * Returns the address of logical 0 so that locore can map the kernel
 * properly.
 */
unsigned int get_mapping (void)
{
	int			i, same;
	unsigned long		addr, lastpage, phys;

	numranges = 0;
	for (i = 0; i < 8; i++) {
		low[i] = 0;
		high[i] = 0;
	}

	lastpage = get_top_of_ram ();

	for (addr = 0; addr <= lastpage && get_physical (addr, &phys);
		addr += NBPG) {
		/* printf ("0x%x --> 0x%x\n", addr, phys); */
		if (numranges > 0 && phys == high[numranges - 1]) {
			high[numranges - 1] += NBPG;
		} else {
			numranges++;
			low[numranges - 1] = phys;
			high[numranges - 1] = phys + NBPG;
		}
	}
#if 1
	for (i = 0; i < numranges; i++) {
		printf ("Low = 0x%x, high = 0x%x\n", low[i], high[i]);
	}
	printf ("%d bytes available (%d pages)\n", addr, addr / NBPG);
#endif

	/*
	 * We should now look through all of NuBus space to find where
	 * the internal video is being mapped.  Just to be sure we handle
	 * all the cases, we simply map our NuBus space exactly how
	 * MacOS did it.  As above, we find a bunch of ranges that are
	 * contiguously mapped.  Since there are a lot of pages that
	 * are all mapped to 0, we handle that as a special case where
	 * the length is negative.  We search in increments of 32768
	 * because that's the page size that MacOS uses.
	 */

	int_video_start = 0;	/* Logical address */
	int_video_length = 0;	/* Length in bytes */

#if 0
	for (addr = 0xF9000000; addr < 0xFF000000; addr += 32768) {
		/*
		 * If this address is not mapped, skip it, cause we're
		 * not interested.  I don't think this happens in
		 * NuBus space.
		 */
		if (get_physical (addr, &phys) && addr != phys) {
			break;
		}
	}

	/*
	 * We must do some guessing here because the pages map
	 * like to these physical locations: 0, 0, 32768, 65536, ..., 
	 * 0, 0, 0, ....  Hence the bizarre checks below.
	 */

	if (addr < 0xFF000000) {
		/* Assume only one such block */
		do {
			printf ("0x%x --> 0x%x\n", addr, phys);
			if (phys == 32768) {
				int_video_start = addr - 32768;
			}
			addr += 32768;
		} while ((addr & 0x00FFFFFF) && get_physical (addr, &phys) &&
			addr != phys && !(int_video_start != 0 && phys == 0));
		int_video_length = addr - int_video_start;
	}
#else
	nbnumranges = 0;
	for (i = 0; i < NBMAXRANGES; i++) {
		nbphys[i] = 0;
		nblog[i] = 0;
		nblen[i] = 0;
	}

	same = 0;
	for (addr = 0xF9000000; addr < 0xFF000000; addr += 32768) {
		if (!get_physical (addr, &phys)) {
			continue;
		}
		/* printf ("0x%x --> 0x%x\n", addr, phys); */
		if (nbnumranges > 0 &&
			addr == nblog[nbnumranges-1] + nblen[nbnumranges-1] &&
			phys == nbphys[nbnumranges-1]) { /* Same as last one */
			nblen[nbnumranges-1] += 32768;
			same = 1;
		} else if (nbnumranges > 0 && !same &&
			addr == nblog[nbnumranges-1] + nblen[nbnumranges-1] &&
			phys == nbphys[nbnumranges-1] + nblen[nbnumranges-1]) {
			nblen[nbnumranges-1] += 32768;
		} else {
			if (same) {
				nblen[nbnumranges-1] = -nblen[nbnumranges-1];
				same = 0;
			}
			if (nbnumranges == NBMAXRANGES) {
				printf ("get_mapping(): Too many NuBus "
					"ranges.\n");
				break;
			}
			nbnumranges++;
			nblog[nbnumranges-1] = addr;
			nbphys[nbnumranges-1] = phys;
			nblen[nbnumranges-1] = 32768;
		}
	}
	if (same) {
		nblen[nbnumranges-1] = -nblen[nbnumranges-1];
		same = 0;
	}
#if 1
	for (i = 0; i < nbnumranges; i++) {
		printf ("Log = 0x%lx, Phys = 0x%lx, Len = 0x%lx (%ld)\n",
			nblog[i], nbphys[i], nblen[i], nblen[i]);
	}
#endif

	/*
	 * We must now find the logical address of internal video in the
	 * ranges we made above.  Internal video is at physical 0, but
	 * a lot of pages map there.  Instead, we look for the logical
	 * page that maps to 32768 and go back one page.
	 */

	for (i = 0; i < nbnumranges; i++) {
		if (nblen[i] > 0 && nbphys[i] <= 32768 &&
			32768 <= nbphys[i] + nblen[i]) {
			int_video_start = nblog[i] - nbphys[i];
			/* XXX Guess: */
			int_video_length = nblen[i] + nbphys[i];
			break;
		}
	}
	if (i == nbnumranges) {
		printf ("get_mapping(): no internal video.\n");
	}
#endif

	printf ("  Video address = 0x%x\n", videoaddr);
	printf ("  Weird mapping starts at 0x%x\n", int_video_start);
	printf ("  Length = 0x%x (%d) bytes\n", int_video_length, int_video_length);

	return low[0];	 /* Return physical address of logical 0 */
}

/*
 * remap_kernel()
 *
 *   The booter might have loaded the kernel across a bank break.  Since
 *   locore maps the kernel as if it was load contiguously, we've got
 *   to go back here and remap it properly according to the bank mapping
 *   that we found in get_mapping().
 */

static void remap_kernel (unsigned long *pt)
{
	int		i, len, numleft;
	unsigned long	pte;

	numleft = Sysptsize * NPTEPG;
	for (i = 0; i < numranges; i++) {
		pte = low[i] & PG_FRAME;
		len = (high[i] - low[i]) >> PGSHIFT;
		while (len--) {
			if ((*pt & PG_V) == 0 || numleft == 0) {
				/* End of kernel mapping */
				return;
			}
			*pt = (*pt & ~PG_FRAME) | pte;
			pt++;
			numleft--;
			pte += NBPG;
		}
	}
	/* Not likely to get here */
}

/*
 * remap_nubus()
 *
 *   Locore maps all of NuBus space linearly.  Some systems that have
 *   internal video at physical 0 map the screen into NuBus space.
 *   Here we go back and remap NuBus the way the MacOS had it so that
 *   we can use their address for video.
 */

static void remap_nubus (unsigned long *pt)
{
	int		i, len;
	unsigned long	*pteptr, pte, offset;

	for (i = 0; i < nbnumranges; i++) {
		pteptr = pt + ((nblog[i] - NBBASE) >> PGSHIFT);
		pte = (nbphys[i] & PG_FRAME) | PG_RW | PG_CI | PG_V;
		if (nblen[i] < 0) {
			len = -nblen[i] >> PGSHIFT;
			offset = 0;
			while (len--) {
				*pteptr++ = pte + offset;
				/* Wrap around every 32k: */
				offset = (offset + NBPG) & 0x7fff;
			}
		} else {
			len = nblen[i] >> PGSHIFT;
			while (len--) {
				*pteptr++ = pte;
				pte += NBPG;
			}
		}
	}
}

/*
 * remap_rom()
 *
 *   Remaps the first 8 megs of ROM.  Uses early-termination pages.
 */

static void remap_rom (unsigned long *st)
{
        unsigned long   addr, index;

	addr = ROMBASE;

	while (addr < ROMTOP) {	/* ROMSIZE must be a multiple of 4 MB! */
		index = addr / 0x400000;
		st[index] = addr | 0x01;
		addr += 0x400000;
	}
}

/*
 * remap_MMU()
 *
 *   This function remaps kernel and NuBus pages the way they were done
 *   in MacOS.  "st" is the address of the segment table, and "pt" is
 *   the address of the first kernel page table.
 */

void remap_MMU (unsigned long st, unsigned long pt)
{
	remap_kernel ((unsigned long *)pt);
	remap_nubus ((unsigned long *)(pt +
		((Sysptsize + (IIOMAPSIZE+NPTEPG-1)/NPTEPG) << PGSHIFT)));
	remap_rom ((unsigned long *)st);
}
