/*	$NetBSD: machdep.c,v 1.48 1995/03/24 17:27:41 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
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
 *
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 *	from: @(#)machdep.c	7.16 (Berkeley) 6/3/91
 *	machdep.c,v 1.3 1993/07/07 07:20:03 cgd Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef COMPAT_HPUX
#include <compat/hpux/hpux.h>
#endif

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/mon.h> 
#include <machine/isr.h>

#include <dev/cons.h>

#include <vm/vm.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <net/netisr.h>

#include <setjmp.h>

extern char *cpu_string;

int physmem;
int cold;
int fpu_type;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

extern char kstack[];
extern short exframesize[];

#ifdef COMPAT_SUNOS
void sun_sendsig();
static void hack_sun_reboot();	/* XXX - Temporary hack... */
#endif

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
int *nofault;

extern vm_offset_t u_area_va;
caddr_t allocsys __P((caddr_t));
  
/*
 * Info for CTL_HW
 */
char	machine[] = "sun3";		/* cpu "architecture" */
char	cpu_model[120];
extern	char version[];


void
identifycpu()
{
    /*
     * actual identification done earlier because i felt like it,
     * and i believe i will need the info to deal with some VAC, and awful
     * framebuffer placement problems.  could be moved later.
     */
	strcpy(cpu_model, "Sun 3/");

    /* should eventually include whether it has a VAC, mc6888x version, etc */
	strcat(cpu_model, cpu_string);

	printf("Model: %s\n", cpu_model);
}

void
save_u_area(procp, va)
	struct proc *procp;
	register vm_offset_t va;
{
	register int pte, *ptep, *limit;

	ptep = &procp->p_md.md_upte[0];
	limit = &ptep[UPAGES];

	do {
		pte = get_pte(va);
		pte &= ~(PG_NC | PG_MODREF);
		*ptep = pte;
		va += NBPG;
		ptep++;
	} while (ptep < limit);
}

void
load_u_area()
{
	register vm_offset_t va;
	register int pte, *ptep, *limit;

	va = (vm_offset_t) UADDR;
	ptep = &curproc->p_md.md_upte[0];
	limit = &ptep[UPAGES];

	do {
#ifdef	HAVECACHE
		cache_flush_page(va);
#endif
		pte = *ptep;
		set_pte(va, pte);
		va += NBPG;
		ptep++;
	} while (ptep < limit);
}

/*
 * This function exists just so stack traces show the right caller
 * when the first interrupts come in (else ddb gets confused).
 */
static void
enable_interrupts()
{
	(void)spl0();
}

/*
 * This is called early in init_main.c:main(), after the
 * kernel memory allocator is ready for use, but before
 * the creation of process 0,1,2 and mountroot, etc.
 */
void cpu_startup()
{
    caddr_t v;
    int sz, i;
    vm_size_t size;    
    int base, residual;
    vm_offset_t minaddr, maxaddr, uarea_pages;

    /* msgbuf mapped earlier, should figure out why? */
    printf(version);
    identifycpu();
    printf("real mem  = %d\n", ctob(physmem));
    
    /*
     * Find out how much space we need, allocate it,
     * and then give everything true virtual addresses.
     */
    sz = (int)allocsys((caddr_t)0);
    if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(sz))) == 0)
	panic("startup: no room for tables");
    if (allocsys(v) - v != sz)
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
	/* don't want to alloc more physical mem than needed */
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
    /*
     * Allocate a map for physio and DVMA
     */
    phys_map = vm_map_create(kernel_pmap, DVMA_SPACE_START,
			     DVMA_SPACE_END, TRUE);
    if (phys_map == NULL)
	panic("cpu_startup: unable to create physmap");

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
    callout[i-1].c_next = NULL;

    printf("avail mem = %d\n", ptoa(cnt.v_free_count));
    printf("using %d buffers containing %d bytes of memory\n",
	   nbuf, bufpages * CLBYTES);

    /*    initcpu();*/
    /*
     * Set up buffers, so they can be used to read disk labels.
     */
    bufinit();

    /*
     * Configure the system.
     */
    nofault = NULL;
    configure();

	cold = 0;
	enable_interrupts();

#ifdef	COMPAT_SUNOS
	hack_sun_reboot();	/* XXX - Temporary hack... */
#endif
}

/*
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */
caddr_t
allocsys(v)
	register caddr_t v;
{

#define	valloc(name, type, num) \
	    v = (caddr_t)(((name) = (type *)v) + (num))

#ifdef REAL_CLISTS
	valloc(cfree, struct cblock, nclist);
#endif
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
	 * Determine how many buffers to allocate (enough to
	 * hold 5% of total physical memory, but at least 16).
	 * Allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
	    if (physmem < btoc(2 * 1024 * 1024))
		bufpages = (physmem / 10) / CLSIZE;
	    else
		bufpages = (physmem / 20) / CLSIZE;
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
	return v;
}

/*
 * This is called at the beginning of init_main.c:main()
 * to get the console device ready for kernel printf calls.
 */
void consinit()
{
    extern void cninit();
    cninit();

#ifdef DDB
	/* Now that we have a console, we can stop in DDB. */
	db_machine_init();
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif DDB
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

/*
 * Set registers on exec.
 * XXX Should clear registers except sp, pc,
 * but would break init; should be fixed soon.
 */
void
setregs(p, entry, stack, retval)
	register struct proc *p;
	u_long entry;
	u_long stack;
	int retval[2];
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_pc = entry & ~1;
	frame->f_regs[SP] = stack;
#ifdef FPCOPROC
	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fpu_type) {
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
	}
#endif
#ifdef COMPAT_HPUX
	if (p->p_flag & SHPUX) {
		frame->f_regs[A0] = 0; /* not 68010 (bit 31), no FPA (30) */
		retval[0] = 0;		/* no float card */
		retval[1] = fpu_type;		/* 0: none, 1: 68881 */
	}
	/*
	 * XXX This doesn't have much to do with setting registers but
	 * I didn't want to muck up kern_exec.c with this code, so I
	 * stuck it here.
	 *
	 * Ensure we perform the right action on traps type 1 and 2:
	 * If our parent is an HPUX process and we are being traced, turn
	 * on HPUX style interpretation.  Else if we were using the HPUX
	 * style interpretation, revert to the BSD interpretation.
	 *
	 * Note that we do this by changing the trap instruction in the
	 * global "sigcode" array which then gets copied out to the user's
	 * sigcode in the stack.  Since we are changing it in the global
	 * array we must always reset it, even for non-HPUX processes.
	 *
	 * Note also that implementing it in this way creates a potential
	 * race where we could have tweaked it for process A which then
	 * blocks in the copyout to the stack and process B comes along
	 * and untweaks it causing A to wind up with the wrong setting
	 * when the copyout continues.  However, since we have already
	 * copied something out to this user stack page (thereby faulting
	 * it in), this scenerio is extremely unlikely.
	 */
	{
		extern short sigcodetrap[];

		if ((p->p_pptr->p_emul == EMUL_HPUX) &&
		    (p->p_flag & P_TRACED)) {
			p->p_md.md_flags |= MDP_HPUXTRACE;
			*sigcodetrap = 0x4E42;
		} else {
			p->p_md.md_flags &= ~MDP_HPUXTRACE;
			*sigcodetrap = 0x4E41;
		}
	}
#endif
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

#ifdef COMPAT_SUNOS
/* sigh.. I guess it's too late to change now, but "our" sigcontext
   is plain vax, not very 68000 (ap, for example..) */
struct sun_sigcontext {
	int 	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
	int	sc_sp;			/* sp to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_ps;			/* psl to restore */
};
struct sun_sigframe {
	int	ssf_signum;		/* signo for handler */
	int	ssf_code;		/* additional info for handler */
	struct sun_sigcontext *ssf_scp;	/* context pointer for handler */
	u_int	ssf_addr;		/* even more info for handler */
	struct sun_sigcontext ssf_sc;	/* I don't know if that's what 
					   comes here */
};
#endif	

#ifdef COMPAT_HPUX
struct	hpuxsigcontext {
	int	hsc_syscall;
	char	hsc_action;
	char	hsc_pad1;
	char	hsc_pad2;
	char	hsc_onstack;
	int	hsc_mask;
	int	hsc_sp;
	short	hsc_ps;
	int	hsc_pc;
/* the rest aren't part of the context but are included for our convenience */
	short	hsc_pad;
	u_int	hsc_magic;		/* XXX sigreturn: cookie */
	struct	sigcontext *hsc_realsc;	/* XXX sigreturn: ptr to BSD context */
};

/*
 * For an HP-UX process, a partial hpuxsigframe follows the normal sigframe.
 * Tremendous waste of space, but some HP-UX applications (e.g. LCL) need it.
 */
struct hpuxsigframe {
	int	hsf_signum;
	int	hsf_code;
	struct	sigcontext *hsf_scp;
	struct	hpuxsigcontext hsf_sc;
	int	hsf_regs[15];
};
#endif

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
	u_long code;
{
	register struct proc *p = curproc;
	register struct sigframe *fp, *kfp;
	register struct frame *frame;
	register struct sigacts *ps = p->p_sigacts;
	register short ft;
	int oonstack, fsize;
	extern char sigcode[], esigcode[];

	frame = (struct frame *)p->p_md.md_regs;
	ft = frame->f_format;
	oonstack = ps->ps_sigstk.ss_flags & SA_ONSTACK;

#ifdef COMPAT_SUNOS
	if (p->p_emul == EMUL_SUNOS)
	  {
	    /* if this is a hardware fault (ft >= FMT9), sun_sendsig
	       can't currently handle it. Reset signal actions and
	       have the process die unconditionally. */
	    if (ft >= FMT9)
	      {
		SIGACTION(p, sig) = SIG_DFL;
		mask = sigmask(sig);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, sig);
		return;
	      }

	    /* else build the short SunOS frame instead */
	    sun_sendsig (catcher, sig, mask, code);
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
#ifdef COMPAT_HPUX
	if (p->p_emul == EMUL_HPUX)
		fsize = sizeof(struct sigframe) + sizeof(struct hpuxsigframe);
	else
#endif
	fsize = sizeof(struct sigframe);
	if ((ps->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (ps->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(ps->ps_sigstk.ss_base +
					 ps->ps_sigstk.ss_size - fsize);
		ps->ps_sigstk.ss_flags |= SA_ONSTACK;
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
	if (ft >= FMT7) {
#ifdef DEBUG
		if (ft > 15 || exframesize[ft] < 0)
			panic("sendsig: bogus frame type");
#endif
		kfp->sf_state.ss_flags |= SS_RTEFRAME;
		kfp->sf_state.ss_frame.f_format = frame->f_format;
		kfp->sf_state.ss_frame.f_vector = frame->f_vector;
		bcopy((caddr_t)&frame->F_u,
		      (caddr_t)&kfp->sf_state.ss_frame.F_u,
			  (size_t) exframesize[ft]);
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
#ifdef COMPAT_HPUX
	/*
	 * Create an HP-UX style sigcontext structure and associated goo
	 */
	if (p->p_emul == EMUL_HPUX) {
		register struct hpuxsigframe *hkfp;

		hkfp = (struct hpuxsigframe *)&kfp[1];
		hkfp->hsf_signum = bsdtohpuxsig(kfp->sf_signum);
		hkfp->hsf_code = kfp->sf_code;
		hkfp->hsf_scp = (struct sigcontext *)
			&((struct hpuxsigframe *)(&fp[1]))->hsf_sc;
		hkfp->hsf_sc.hsc_syscall = 0;		/* XXX */
		hkfp->hsf_sc.hsc_action = 0;		/* XXX */
		hkfp->hsf_sc.hsc_pad1 = hkfp->hsf_sc.hsc_pad2 = 0;
		hkfp->hsf_sc.hsc_onstack = kfp->sf_sc.sc_onstack;
		hkfp->hsf_sc.hsc_mask = kfp->sf_sc.sc_mask;
		hkfp->hsf_sc.hsc_sp = kfp->sf_sc.sc_sp;
		hkfp->hsf_sc.hsc_ps = kfp->sf_sc.sc_ps;
		hkfp->hsf_sc.hsc_pc = kfp->sf_sc.sc_pc;
		hkfp->hsf_sc.hsc_pad = 0;
		hkfp->hsf_sc.hsc_magic = 0xdeadbeef;
		hkfp->hsf_sc.hsc_realsc = kfp->sf_scp;
		bcopy((caddr_t)frame->f_regs, (caddr_t)hkfp->hsf_regs,
		      sizeof (hkfp->hsf_regs));

		kfp->sf_signum = hkfp->hsf_signum;
		kfp->sf_scp = hkfp->hsf_scp;
	}
#endif
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
	frame->f_pc = (int)PS_STRINGS - (esigcode - sigcode);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
	free((caddr_t)kfp, M_TEMP);
}

#ifdef COMPAT_SUNOS
/* much simpler sendsig() for SunOS processes, as SunOS does the whole
   context-saving in usermode. For now, no hardware information (ie.
   frames for buserror etc) is saved. This could be fatal, so I take 
   SIG_DFL for "dangerous" signals. */

void
sun_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct sun_sigframe *fp;
	struct sun_sigframe kfp;
	register struct frame *frame;
	register struct sigacts *ps = p->p_sigacts;
	register short ft;
	int oonstack, fsize;

	frame = (struct frame *)p->p_md.md_regs;
	ft = frame->f_format;
	oonstack = ps->ps_sigstk.ss_flags & SA_ONSTACK;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	fsize = sizeof(struct sun_sigframe);
	if ((ps->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (ps->ps_sigonstack & sigmask(sig))) {
		fp = (struct sun_sigframe *)(ps->ps_sigstk.ss_base +
					 ps->ps_sigstk.ss_size - fsize);
		ps->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else
		fp = (struct sun_sigframe *)(frame->f_regs[SP] - fsize);
	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sun_sendsig(%d): sig %d ssp %x usp %x scp %x ft %d\n",
		       p->p_pid, sig, &oonstack, fp, &fp->ssf_sc, ft);
#endif
	if (useracc((caddr_t)fp, fsize, B_WRITE) == 0) {
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sun_sendsig(%d): useracc failed on sig %d\n",
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
	/* 
	 * Build the argument list for the signal handler.
	 */
	kfp.ssf_signum = sig;
	kfp.ssf_code = code;
	kfp.ssf_scp = &fp->ssf_sc;
	kfp.ssf_addr = ~0;		/* means: not computable */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	kfp.ssf_sc.sc_onstack = oonstack;
	kfp.ssf_sc.sc_mask = mask;
	kfp.ssf_sc.sc_sp = frame->f_regs[SP];
	kfp.ssf_sc.sc_pc = frame->f_pc;
	kfp.ssf_sc.sc_ps = frame->f_sr;
	if (copyout((caddr_t)&kfp, (caddr_t)fp, fsize))
	    panic("sendsig: copying out signal context\n");
	frame->f_regs[SP] = (int)fp;

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sun_sendsig(%d): sig %d scp %x sc_sp %x\n",
		       p->p_pid, sig, kfp.ssf_sc.sc_sp);
#endif

	/* have the user-level trampoline code sort out what registers it
	   has to preserve. */
	frame->f_pc = (u_int) catcher;
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sun_sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
}

#endif	/* COMPAT_SUNOS */

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

sigreturn(p, uap, retval)
	struct proc *p;
	struct sigreturn_args *uap;
	int *retval;
{
	register struct sigcontext *scp;
	register struct frame *frame;
	register int rf;
	struct sigcontext tsigc;
	struct sigstate tstate;
	int flags;

	scp = SCARG(uap,sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %x\n", p->p_pid, scp);
#endif
	if ((int)scp & 1)
		return (EINVAL);
#ifdef COMPAT_HPUX
	/*
	 * Grab context as an HP-UX style context and determine if it
	 * was one that we contructed in sendsig.
	 */
	if (p->p_emul == EMUL_HPUX) {
		struct hpuxsigcontext *hscp = (struct hpuxsigcontext *)scp;
		struct hpuxsigcontext htsigc;

		if (useracc((caddr_t)hscp, sizeof (*hscp), B_WRITE) == 0 ||
		    copyin((caddr_t)hscp, (caddr_t)&htsigc, sizeof htsigc))
			return (EINVAL);
		/*
		 * If not generated by sendsig or we cannot restore the
		 * BSD-style sigcontext, just restore what we can -- state
		 * will be lost, but them's the breaks.
		 */
		hscp = &htsigc;
		if (hscp->hsc_magic != 0xdeadbeef ||
		    (scp = hscp->hsc_realsc) == 0 ||
		    useracc((caddr_t)scp, sizeof (*scp), B_WRITE) == 0 ||
		    copyin((caddr_t)scp, (caddr_t)&tsigc, sizeof tsigc)) {
			if (hscp->hsc_onstack & 01)
				p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
			else
				p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
			p->p_sigmask = hscp->hsc_mask &~ sigcantmask;
			frame = (struct frame *) p->p_md.md_regs;
			frame->f_regs[SP] = hscp->hsc_sp;
			frame->f_pc = hscp->hsc_pc;
			frame->f_sr = hscp->hsc_ps &~ PSL_USERCLR;
			return (EJUSTRETURN);
		}
		/*
		 * Otherwise, overlay BSD context with possibly modified
		 * HP-UX values.
		 */
		tsigc.sc_onstack = hscp->hsc_onstack;
		tsigc.sc_mask = hscp->hsc_mask;
		tsigc.sc_sp = hscp->hsc_sp;
		tsigc.sc_ps = hscp->hsc_ps;
		tsigc.sc_pc = hscp->hsc_pc;
	} else
#endif
	/*
	 * Test and fetch the context structure.
	 * We grab it all at once for speed.
	 */
#ifdef	COMPAT_SUNOS
	if (p->p_emul == EMUL_SUNOS) {
	    struct sunos_sigcontext {
		int ssc_onstack;
		int ssc_mask;
		int ssc_sp;
		int ssc_pc;
		int ssc_ps;
	    } *sscp, stsigc;

	    sscp = (struct sunos_sigcontext *) scp;
	    if (useracc((caddr_t)sscp, sizeof (*sscp), B_WRITE) == 0 ||
		copyin((caddr_t)sscp, (caddr_t)&stsigc, sizeof stsigc))
		    return (EINVAL);
	    sscp = &stsigc;
	    tsigc.sc_onstack = sscp->ssc_onstack;
	    tsigc.sc_mask = sscp->ssc_mask;
	    tsigc.sc_sp = sscp->ssc_sp;
	    tsigc.sc_ps = sscp->ssc_ps;
	    tsigc.sc_pc = sscp->ssc_pc;
	}
	else
#endif

	if (useracc((caddr_t)scp, sizeof (*scp), B_WRITE) == 0 ||
	    copyin((caddr_t)scp, (caddr_t)&tsigc, sizeof tsigc))
		return (EINVAL);
	scp = &tsigc;
	if ((scp->sc_ps & (PSL_MBZ|PSL_IPL|PSL_S)) != 0)
		return (EINVAL);
	/*
	 * Restore the user supplied information
	 */
	if (scp->sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = scp->sc_mask &~ sigcantmask;
	frame = (struct frame *) p->p_md.md_regs;
	frame->f_regs[SP] = scp->sc_sp;
#ifdef COMPAT_SUNOS
	if (p->p_emul != EMUL_SUNOS)
#endif
	    frame->f_regs[A6] = scp->sc_fp;
	frame->f_pc = scp->sc_pc;
	frame->f_sr = scp->sc_ps;

#ifdef COMPAT_SUNOS
	if (p->p_emul == EMUL_SUNOS)
		return EJUSTRETURN;
#endif

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


/*
 * Do a sync in preparation for a reboot.
 * XXX - This could probably be common code. -gwr
 */
int waittime = -1;	/* XXX - Who else looks at this? -gwr */
static void reboot_sync()
{
	struct buf *bp;
	int iter, nbusy;

	if (waittime >= 0)
		return;
	waittime = 0;

	/* XXX - Should this be spl0() like hp300? -gwr */
	(void) splnet();

	printf("syncing disks... ");
	/*
	 * Release vnodes held by texts before sync.
	 */
	if (panicstr == 0)
		vnode_pager_umount(NULL);

	sync(&proc0, (void *)0, (int *)0);
	
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
	DELAY(10000);			/* wait for printf to finish */
}

/*
 * Common part of the BSD and SunOS reboot system calls.
 */
static int reboot2(howto, user_boot_string)
	int howto;
	char *user_boot_string;
{
	char *bs, *p;
	char default_boot_string[16];

	if ((howto & RB_NOSYNC) == 0) {
		reboot_sync();
	}

	/*
	 * If we've been adjusting the clock, the todr
	 * will be out of synch; adjust it now.
	 *
	 * XXX - However, if the kernel has been sitting in ddb,
	 * the time will be way off, so don't set the HW clock!
	 * XXX - Should do sanity check against HW clock. -gwr
	 */
	/* resettodr(); */

	/* Write out a crash dump if asked. */
	splhigh();
	if (howto & RB_DUMP)
		dumpsys();

	if (howto & RB_HALT) {
		printf("Kernel halted.\n");
		sun3_mon_halt();
	}

	/*
	 * Automatic reboot.
	 */
	bs = user_boot_string;
	if (bs == NULL) {
		/*
		 * Build our own boot string with an empty
		 * boot device/file and (maybe) some flags.
		 * The PROM will supply the device/file name.
		 */
		bs = default_boot_string;
		*bs = '\0';
		if (howto & (RB_KDB|RB_ASKNAME|RB_SINGLE)) {
			/* Append the boot flags. */
			p = bs;
			*p++ = ' ';
			*p++ = '-';
			if (howto & RB_KDB)
				*p++ = 'd';
			if (howto & RB_ASKNAME)
				*p++ = 'a';
			if (howto & RB_SINGLE)
				*p++ = 's';
			*p = '\0';
		}
	}
	printf("Kernel rebooting...\n");
	sun3_mon_reboot(bs);
	/*NOTREACHED*/
}

/*
 * BSD reboot system call
 * XXX - Should be named: cpu_reboot maybe? -gwr
 * XXX - It would be nice to allow a second argument
 * that specifies a machine-dependent boot string that
 * is passed to the boot program if RB_STRING is set.
 */
void boot(howto)
	int howto;
{
	(void) reboot2(howto, NULL);
}

#ifdef	COMPAT_SUNOS
/*
 * SunOS reboot system call (for compatibility).
 * Sun lets you pass in a boot string which the PROM
 * saves and provides to the next boot program.
 * XXX - Stuff this into sunos_sysent at boot time?
 */
static struct sun_howto_conv {
	int sun_howto;
	int bsd_howto;
} sun_howto_conv[] = {
	0x001,	RB_ASKNAME,
	0x002,	RB_SINGLE,
	0x004,	RB_NOSYNC,
	0x008,	RB_HALT,
	0x080,	RB_DUMP,
};
struct sun_reboot_args {
	int howto;
	char *bootstr;
};
int sun_reboot(p, uap, retval)
	struct proc *p;
	struct sun_reboot_args *uap;
	int *retval;
{
	struct sun_howto_conv *convp;
	int error, bsd_howto, sun_howto;
	char bs[128];
	char *bsd_bootstr = NULL;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);

	/*
	 * Convert howto bits to BSD format.
	 */
	sun_howto = uap->howto;
	bsd_howto = 0;
	convp = sun_howto_conv;
	while (convp->sun_howto) {
		if (sun_howto &  convp->sun_howto)
			bsd_howto |= convp->bsd_howto;
		convp++;
	}

	/*
	 * Sun RB_STRING (Get user supplied bootstring.)
	 */
	bsd_bootstr = NULL;
	if (sun_howto & 0x200) {
		error = copyinstr(uap->bootstr, bs, sizeof(bs), 0);
		if (error) return error;
		bsd_bootstr = bs;
	}

	return (reboot2(bsd_howto, bsd_bootstr));
}
/*
 * XXX - Temporary hack:  Install sun_reboot() syscall.
 * Fix compat/sunos/sun_sysent instead.
 */
static void hack_sun_reboot()
{
	extern struct sysent sunos_sysent[];
	sunos_sysent[55].sy_narg = 2;
	sunos_sysent[55].sy_call = sun_reboot;
}
#endif	/* COMPAT_SUNOS */

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * This is called by cpu_startup to set dumplo, dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end.
 */
void
dumpconf()
{
	int nblks;	/* size of dump area */
	int maj;
	int (*getsize)();

	if (dumpdev == NODEV)
		return;

	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	getsize = bdevsw[maj].d_psize;
	if (getsize == NULL)
		return;
	nblks = (*getsize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	/* Position dump image near end of space, page aligned. */
	dumpsize = physmem; /* pages */
	dumplo = nblks - ctod(dumpsize);
	dumplo &= ~(ctod(1)-1);

	/* If it does not fit, truncate it by moving dumplo. */
	if (dumplo < ctod(1)) {
		dumplo = ctod(1);
		dumpsize = dtoc(nblks - dumplo);
	}
}

/*
 * Write a crash dump.
 */
dumpsys()
{
    printf("dumping not supported yet :)\n");
}

/*
 * Print a register and stack dump.
 */
regdump(fp, sbytes)
	struct frame *fp; /* must not be register */
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
	printf("pid = %d, pc = %s, ",
	       curproc ? curproc->p_pid : -1, hexstr(fp->f_pc, 8));
	printf("ps = %s, ", hexstr(fp->f_sr, 4));
	printf("sfc = %s, ", hexstr(getsfc(), 4));
	printf("dfc = %s\n", hexstr(getdfc(), 4));
	printf("Registers:\n     ");
	for (i = 0; i < 8; i++)
		printf("        %d", i);
	printf("\ndreg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i], 8));
	printf("\nareg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i+8], 8));
	if (sbytes > 0) {
		if (fp->f_sr & PSL_S) {
			printf("\n\nKernel stack (%s):",
			       hexstr((int)(((int *)&fp)-1), 8));
			dumpmem(((int *)&fp)-1, sbytes, 0);
		} else {
			printf("\n\nUser stack (%s):", hexstr(fp->f_regs[SP], 8));
			dumpmem((int *)fp->f_regs[SP], sbytes, 1);
		}
	}
	doingdump = 0;
	splx(s);
}

#define KSADDR	((int *)&(kstack[(UPAGES-1)*NBPG]))

dumpmem(ptr, sz, ustack)
	register int *ptr;
	int sz, ustack;
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
	int len;
{
	static char nbuf[9];
	register int x, i;

	if (len > 8)
		return("");
	nbuf[len] = '\0';
	for (i = len-1; i >= 0; --i) {
		x = val & 0xF;
		/* Isn't this a cool trick? */
		nbuf[i] = "0123456789ABCDEF"[x];
		val >>= 4;
	}
	return(nbuf);
}

straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

int
peek_word(addr)
	register caddr_t addr;
{
	jmp_buf		faultbuf;
	register int x;

	nofault = (int *) &faultbuf;
	if (setjmp(nofault)) {
		nofault = (int *) 0;
		return(-1);
	}
	x = *(volatile u_short *)addr;
	nofault = (int *) 0;
	return(x);
}

peek_byte(addr)
	register caddr_t addr;
{
	jmp_buf 	faultbuf;
	register int x;

	nofault = (int *) &faultbuf;
	if (setjmp(nofault)) {
		nofault = (int *) 0;
		return(-1);
	}
	x = *(volatile u_char *)addr;
	nofault = (int *) 0;
	return(x);
}

int
sysarch(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	return ENOSYS;
}

/* XXX should be in an include file somewhere */
#define CC_PURGE	1
#define CC_FLUSH	2
#define CC_IPURGE	4
#define CC_EXTPURGE	0x80000000
/* XXX end should be */

/*ARGSUSED1*/
cachectl(req, addr, len)
	int req;
	caddr_t	addr;
	int len;
{
	int error = 0;

	switch (req) {
	case CC_EXTPURGE|CC_PURGE:
	case CC_EXTPURGE|CC_FLUSH:
	case CC_PURGE:
	case CC_FLUSH:
		DCIU();
		break;
	case CC_EXTPURGE|CC_IPURGE:
		DCIU();
		/* fall into... */
	case CC_IPURGE:
		ICIA();
		break;
	default:
		error = EINVAL;
		break;
	}
	return(error);
}

cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error = ENOEXEC;

#ifdef COMPAT_SUNOS
	extern sunos_exec_aout_makecmds
		__P((struct proc *, struct exec_package *));
	if ((error = sunos_exec_aout_makecmds(p, epp)) == 0)
		return 0;
#endif
	return error;
}
