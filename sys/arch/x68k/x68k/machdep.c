/*	$NetBSD: machdep.c,v 1.9 1997/03/27 21:01:58 thorpej Exp $	*/

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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
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
#include <sys/sysctl.h>
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

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <net/netisr.h>
#include <dev/cons.h>

#define	MAXMEM	64*1024*CLSIZE	/* XXX - from cmap.h */
#include <vm/vm_kern.h>

#include <x68k/x68k/iodevice.h>

void dumpmem __P((int *, int, int));
void initcpu __P((void));
void identifycpu __P((void));
void doboot __P((void))
    __attribute__((__noreturn__));

/* the following is used externally (sysctl_hw) */
char machine[] = "x68k";		/* cpu "architecture" */

vm_map_t buffer_map;
#ifdef MACHINE_NONCONTIG
extern int numranges;
extern u_long low[8];
extern u_long high[8];
#else
extern vm_offset_t avail_end;
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
int	msgbufmapped;		/* set when safe to use msgbuf */
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

extern	u_int lowram;
extern	short exframesize[];

#ifdef COMPAT_HPUX
extern struct emul emul_hpux;
#endif

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{

	/*
	 * Set cpuspeed immediately since cninit() called routines
	 * might use delay.  Note that we only set it if a custom value
	 * has not already been specified.
	 */

	cpuspeed = MHZ_25; /* XXX */

	if (mmutype == MMU_68040)
		cpuspeed *= 2;	/* XXX */

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#ifdef KGDB
	zs_kgdb_init();			/* XXX */
#endif
#ifdef DDB
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	register unsigned i;
	register caddr_t v, firstaddr;
	int base, residual;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif
	rtclockinit(); /* XXX */

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
#ifdef MACHINE_NONCONTIG
		pmap_enter(pmap_kernel(), (vm_offset_t)msgbufp,
		    high[numranges - 1] + i * NBPG, VM_PROT_ALL, TRUE);
#else
		pmap_enter(pmap_kernel(), (vm_offset_t)msgbufp,
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
#endif
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	printf("real mem  = %d\n", ctob(physmem));

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
	 * Determine how many buffers to allocate.
	 * Since HPs tend to be long on memory and short on disk speed,
	 * we allocate more buffer space than the BSD standard of
	 * use 10% of memory for the first 2 Meg, 5% of remaining.
	 * We just allocate a flat 10%.  Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		bufpages = physmem / 10 / CLSIZE;
#if 0
		if (physmem < btoc(2 * 1024 * 1024))
			bufpages = physmem / 10 / CLSIZE;
		else
			bufpages = (btoc(2 * 1024 * 1024) + physmem) /
			    (20 * CLSIZE);
#endif
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
#if 0
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
#endif
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
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);
	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
	callout[i-1].c_next = NULL;

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %ld\n", ptoa(cnt.v_free_count));
	printf("using %d buffers containing %d bytes of memory\n",
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
setregs(p, pack, stack, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[SP] = stack;
	frame->f_regs[A2] = (int)PS_STRINGS;
#ifdef FPCOPROC
	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
#endif
#ifdef COMPAT_HPUX
	p->p_md.md_flags &= ~MDP_HPUXMMAP;
	if (p->p_emul == &emul_hpux) {
		frame->f_regs[A0] = 0; /* not 68010 (bit 31), no FPA (30) */
		retval[0] = 0;		/* no float card */
#ifdef FPCOPROC
		retval[1] = 1;		/* yes 68881 */
#else
		retval[1] = 0;		/* no 68881 */
#endif
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

		if ((p->p_pptr->p_emul == &emul_hpux) &&
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

/*
 * Info for CTL_HW
 */
char	cpu_model[120];
extern	char version[];
static char *fpu_descr[] = {
#ifdef	FPU_EMULATE
	"emulator FPU", 	/* 0 */
#else
	"no math support",	/* 0 */
#endif
	" mc68881 FPU",		/* 1 */
	" mc68882 FPU",		/* 2 */
	"/FPU",			/* 3 */
	"/FPU",			/* 4 */
	};

void
identifycpu()
{
        /* there's alot of XXX in here... */
	char *cpu_type, *mach, *mmu, *fpu;

	/*
	 * check machine type constant
	 */
	switch (IODEVbase->io_sysport.mpustat) {
	case 0xdc:
		/*
		 * CPU Type == 68030, Clock == 25MHz
		 */
		mach = "030";
		break;
	case 0xfe:
		/*
		 * CPU Type == 68000, Clock == 16MHz
		 */
		mach = "000XVI";
		break;
	case 0xff:
		/*
		 * CPU Type == 68000, Clock == 10MHz
		 */
		mach = "000/ACE/PRO/EXPERT/SUPER";
		break;
	default:
		/*
		 * unknown type
		 */
		mach = "000?(unknown model)";
		break;
	}

	switch (cputype) {
	case CPU_68060:
		cpu_type = "m68060";
		mmu = "/MMU";
		break;
	case CPU_68040:
		cpu_type = "m68040";
		mmu = "/MMU";
		break;
	case CPU_68030:
		cpu_type = "m68030";
		mmu = "/MMU";
		break;
	case CPU_68020:
		cpu_type = "m68020";
		mmu = " m68851 MMU";
		break;
	default:
		cpu_type = "unknown";
		mmu = " unknown MMU";
		break;
	}
	fputype = fpu_probe();
	if (fputype >= 0 && fputype < sizeof(fpu_descr)/sizeof(fpu_descr[0]))
		fpu = fpu_descr[fputype];
	else
		fpu = " unknown FPU";
	sprintf(cpu_model, "X68%s (%s CPU%s%s)", mach, cpu_type, mmu, fpu);
	printf("%s\n", cpu_model);
}

/*
 * machine dependent system variables.
 */
int
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
	register struct sigacts *psp = p->p_sigacts;
	register short ft;
	int oonstack, fsize;
	extern char sigcode[], esigcode[];

	frame = (struct frame *)p->p_md.md_regs;
	ft = frame->f_format;
	oonstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
#ifdef COMPAT_HPUX
	if (p->p_emul == &emul_hpux)
		fsize = sizeof(struct sigframe) + sizeof(struct hpuxsigframe);
	else
#endif
	fsize = sizeof(struct sigframe);
	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp +
					 psp->ps_sigstk.ss_size - fsize);
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else
		fp = (struct sigframe *)(frame->f_regs[SP] - fsize);
	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d ssp %p usp %p scp %p ft %d\n",
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
		printf("sendsig(%d): copy out FP state (%x) to %p\n",
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
	if (p->p_emul == &emul_hpux) {
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
		printf("sendsig(%d): sig %d scp %p fp %p sc_sp %x sc_ap %x\n",
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
int
sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sigreturn_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	register struct sigcontext *scp;
	register struct frame *frame;
	register int rf;
	struct sigcontext tsigc;
	struct sigstate tstate;
	int flags;

	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
#endif
	if ((int)scp & 1)
		return (EINVAL);
#ifdef COMPAT_HPUX
	/*
	 * Grab context as an HP-UX style context and determine if it
	 * was one that we contructed in sendsig.
	 */
	if (p->p_emul == &emul_hpux) {
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
				p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
			else
				p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
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
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
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
		printf("sigreturn(%d): ssp %p usp %x scp %p ft %d\n",
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
#ifdef FPCOPROC
	/*
	 * Finally we restore the original FP context
	 */
	if (flags & SS_FPSTATE)
		m68881_restore(&tstate.ss_fpstate);
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&tstate.ss_fpstate)
		printf("sigreturn(%d): copied in FP state (%x) at %p\n",
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
cpu_reboot(howto, bootstr)
	register int howto;
	char *bootstr;
{
	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(&curproc->p_addr->u_pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/*resettodr();*/
	}

	/* Disable interrputs. */
	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

	/* Run any shutdown hooks. */
	doshutdownhooks();

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		(void)cngetc();
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("System halted.  Hit any key to reboot.\n\n");
		(void)cngetc();
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

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
	int nblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpsize = physmem;

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP NBPG	/* Must be a multiple of pagesize XXX small */
static vm_offset_t dumpspace;

vm_offset_t
reserve_dumppages(p)
	vm_offset_t p;
{
	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

#ifdef MACHINE_NONCONTIG
static int
find_range(pa)
	vm_offset_t pa;
{
	int i;

	for (i = 0; i < numranges; i++) {
		if (low[i] <= pa && pa < high[i])
			return i;
	}
	return -1;
}

static int
find_next_range(pa)
	vm_offset_t pa;
{
	int     i, near, best, t;

	near = -1;
	best = 0x7FFFFFFF;
	for (i = 0; i < numranges; i++) {
		if (low[i] <= pa && pa < high[i])
			return i;
		t = low[i] - pa;
		if (t > 0) {
			if (t < best) {
				near = i;
				best = t;
			}
		}
	}
	return near;
}
#endif

/*
 * Write a crash dump.
 */
void
dumpsys()
{
	unsigned bytes, i, n;
	int range;
	register int maddr, psize;
	register daddr_t blkno;
	register int (*dump)	__P((dev_t, daddr_t, caddr_t, size_t));
	int error = 0;

	/* Don't put dump messages in msgbuf. */
	msgbufmapped = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0) {
		cpu_dumpconf();
	}
	if (dumplo < 0)
		return;
	printf("\ndumping to dev 0x%x, offset %ld\n", dumpdev, dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;
	for (i = 0; i < bytes; i += n) {
#ifdef MACHINE_NONCONTIG
		/*
		 * Avoid dumping "holes."
		 */
		if ((range == -1) || (i >= high[range])) {
			range = find_next_range(i);
			if (range == -1) {
				error = EIO;
				break;
			}
			n = low[range] - i;
			maddr += n;
			blkno += btodb(n);
			continue;
		}
#endif
		/* Print out how many MBs we have to go. */
		n = bytes - i;
		if (n && (n % (1024 * 1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > BYTES_PER_DUMP)
			n = BYTES_PER_DUMP;

		(void) pmap_map(dumpspace, maddr, maddr + n, VM_PROT_READ);
		error = (*dump)(dumpdev, blkno, (caddr_t) dumpspace, n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);	/* XXX? */
	}
	switch(error) {

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

	case EINTR:
		printf("aborted from console\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
}

void
initcpu()
{
	/* XXX should init '40 vecs here, too */
#if defined(M68060)
	extern caddr_t vectab[256];
#if defined(M060SP)
	extern u_int8_t I_CALL_TOP[];
	extern u_int8_t FP_CALL_TOP[];
#else
	extern u_int8_t illinst;
#endif
	extern u_int8_t fpfault;
#endif

#ifdef MAPPEDCOPY
	extern u_int mappedcopysize;

	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if (mappedcopysize == 0) {
		mappedcopysize = NBPG;
	}
#endif

#if defined(M68060)
	if (cputype == CPU_68060) {
#if defined(M060SP)
		/* integer support */
		vectab[61] = &I_CALL_TOP[128 + 0x00];

		/* floating point support */
		vectab[11] = &FP_CALL_TOP[128 + 0x30];
		vectab[55] = &FP_CALL_TOP[128 + 0x38];
		vectab[60] = &FP_CALL_TOP[128 + 0x40];

		vectab[54] = &FP_CALL_TOP[128 + 0x00];
		vectab[52] = &FP_CALL_TOP[128 + 0x08];
		vectab[53] = &FP_CALL_TOP[128 + 0x10];
		vectab[51] = &FP_CALL_TOP[128 + 0x18];
		vectab[50] = &FP_CALL_TOP[128 + 0x20];
		vectab[49] = &FP_CALL_TOP[128 + 0x28];
#else
		vectab[61] = &illinst;
#endif
		vectab[48] = &fpfault;
	}
	DCIS();
#endif
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
#if defined(DDB)
	Debugger();
#endif
}

int	*nofault;

int
badaddr(addr)
	register caddr_t addr;
{
	register int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = (int *) 0;
	return(0);
}

int
badbaddr(addr)
	register caddr_t addr;
{
	register int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return(0);
}

/*
 * XXX Why on earth isn't this in a common file?!
 */
void
netintr()
{
#ifdef INET
#include "ether.h"
#if NETHER > 0
	if (netisr & (1 << NETISR_ARP)) {
		netisr &= ~(1 << NETISR_ARP);
		arpintr();
	}
#endif
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
#ifdef CCITT
	if (netisr & (1 << NETISR_CCITT)) {
		netisr &= ~(1 << NETISR_CCITT);
		ccittintr();
	}
#endif
#include "ppp.h"
#if NPPP > 0
	if (netisr & (1 << NETISR_PPP)) {
		netisr &= ~(1 << NETISR_PPP);
		pppintr();
	}
#endif
}

/* this is a handy package to have asynchronously executed
   function calls executed at very low interrupt priority.
   Example for use is keyboard repeat, where the repeat 
   handler running at splclock() triggers such a (hardware
   aided) software interrupt.

   Note: the installed functions are currently called in a
         LIFO fashion, might want to change this to FIFO
	 later. */

struct si_callback {
	struct si_callback *next;
	void (*function) __P((void *rock1, void *rock2));
	void *rock1, *rock2;
};

static struct si_callback *si_callbacks = 0;

void
add_sicallback (function, rock1, rock2)
	void (*function) __P((void *rock1, void *rock2));
	void *rock1, *rock2;
{
	struct si_callback *si;
	int s;

	/* Note: this function may be called from high-priority
	 * interrupt handlers. We may NOT block for 
	 * memory-allocation in here!.
	 */
	si = (struct si_callback *)malloc(sizeof(*si), M_TEMP, M_NOWAIT);

	/* bad luck really.. */
	if (! si)
		return;
	si->function = function;
	si->rock1    = rock1;
	si->rock2    = rock2;

	s = splhigh();
	si->next     = si_callbacks;
	si_callbacks = si;
	splx(s);
}

void
rem_sicallback (function)
	void (*function) __P((void *rock1, void *rock2));
{
	struct si_callback *si, *psi;
	int s;

	s = splhigh();
	for (psi = 0, si = si_callbacks; si; ) {
		struct si_callback *nsi = si->next;
		if (si->function == function) {
			free (si, M_TEMP);
			if (psi)
				psi->next = nsi;
			else
				si_callbacks = nsi;
		} else
			psi = si;
		si = nsi;
	}
	splx(s);
}


void
intrhand(sr)
	int sr;
{
	printf("intrhand: unexpected sr 0x%x\n", sr);
}

#if (defined(DDB) || defined(DEBUG)) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */

void
candbtimer(arg)
	void *arg;
{

	crashandburn = 0;
}
#endif

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void
nmihand(frame)
	struct frame frame;
{
	sysport.keyctrl = sysport.keyctrl | 0x04;
	if (1) {
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
#ifdef DDB
		Debugger();
#else
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				panic(panicstr ?
				      "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			timeout(candbtimer, (void *)0, candbdelay);
		}
#endif /* DDB */
#endif /* PANICBUTTON */
		return;
	}
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");
}

void
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

#define KSADDR	((int *)((u_int)curproc->p_addr + USPACE - NBPG))

void
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
		if (x > 9)
			nbuf[i] = x - 10 + 'A';
		else
			nbuf[i] = x + '0';
		val >>= 4;
	}
	return(nbuf);
}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * XXX what are the special cases for the hp300?
 * XXX why is this COMPAT_NOMID?  was something generating
 *	hp300 binaries with an a_mid of 0?  i thought that was only
 *	done on little-endian machines...  -- cgd
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
#if defined(COMPAT_NOMID) || defined(COMPAT_44)
	u_long midmag, magic;
	u_short mid;
	int error;
	struct exec *execp = epp->ep_hdr;

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
#ifdef COMPAT_NOMID
	case (MID_ZERO << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		break;
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		break;
#endif
	default:
		error = ENOEXEC;
	}

	return error;
#else /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */
	return ENOEXEC;
#endif
}
