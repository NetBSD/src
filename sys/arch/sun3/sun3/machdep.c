/* Copyright (c) 1993 Adam Glass
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
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	from: @(#)machdep.c	7.16 (Berkeley) 6/3/91
 *	machdep.c,v 1.3 1993/07/07 07:20:03 cgd Exp
 */

#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "kernel.h"
#include "map.h"
#include "proc.h"
#include "buf.h"
#include "reboot.h"
#include "conf.h"
#include "file.h"
#include "clist.h"
#include "callout.h"
#include "malloc.h"
#include "mbuf.h"
#include "msgbuf.h"
#include "user.h"
#ifdef SYSVSHM
#include "shm.h"
#endif
#ifdef HPUXCOMPAT
#include "../hpux/hpux.h"
#endif

#include "vm/vm.h"
#include "vm/vm_map.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"

#include "machine/cpu.h"
#include "machine/reg.h"
#include "machine/psl.h"

#include "machine/pte.h"

#include "machine/mon.h" 
#include "machine/isr.h"
#include "net/netisr.h"



extern char *cpu_string;
int physmem;
int cold;
extern char kstack[];
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

void identifycpu()
{
    /*
     * actual identification done earlier because i felt like it,
     * and i believe i will need the info to deal with some VAC, and awful
     * framebuffer placement problems.  could be moved later.
     */

    printf("Model:\tSun 3/%s\n", cpu_string);
    /* should eventually include whether it has a VAC, mc6888x version, etc */
}

void save_u_area(pcbp, va)
     struct pcb *pcbp;
     vm_offset_t va;
{
    pcbp->pcb_upte[0] = get_pte(va);
    pcbp->pcb_upte[1] = get_pte(va+NBPG);
    pcbp->pcb_upte[2] = get_pte(va+NBPG+NBPG);
}
void load_u_area(pcbp)
     struct pcb *pcbp;
{

    set_pte(u_area_va, pcbp->pcb_upte[0]);
    set_pte(u_area_va+NBPG, pcbp->pcb_upte[1]);
    set_pte(u_area_va+NBPG+NBPG, pcbp->pcb_upte[2]);
}


void cpu_startup()
{
    caddr_t v;
    int firstaddr, i;
    vm_size_t size;    
    vm_offset_t minaddr, maxaddr, uarea_pages;


    /* msgbuf mapped earlier, should figure out why? */
    printf(version);
    identifycpu();
		/* compute physmem? */
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
	    /*	valloc(cfree, struct cblock, nclist);  no clists any more!!! - cgd */
	    valloc(callout, struct callout, ncallout);
    valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
    valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
    if (bufpages == 0)
	if (physmem < (2 * 1024 * 1024))
	    bufpages = physmem / 10 / CLSIZE;
	else
	    bufpages = ((2 * 1024 * 1024 + physmem) / 20) / CLSIZE;
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
	firstaddr = (int) kmem_alloc(kernel_map, round_page(size));
	if (firstaddr == 0)
	    panic("cpu_startup: no room for tables");
	goto again;
    }
    /*
     * End of second pass, addresses have been assigned
     */
    if ((vm_size_t)(v - firstaddr) != size)
	panic("cpu_startup: table size inconsistency");
    /* buffer_map stuff but not used */
    /*
     * Allocate a submap for exec arguments.  This map effectively
     * limits the number of processes exec'ing at any time.
     */
    /*	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
     *				16*NCARGS, TRUE);
     *	NOT CURRENTLY USED -- cgd
     */
    /*
     * Allocate a map for physio and DVMA
     */
    phys_map = vm_map_create(kernel_pmap, DVMA_SPACE_START, DVMA_SPACE_END, 0);
    if (!phys_map)
	panic("cpu_startup: unable to create physmap");
    
    /*
     * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
     * we use the more space efficient malloc in place of kmem_alloc.
     */
    mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
			       M_MBUF, M_NOWAIT);
    bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
    mb_map = kmem_suballoc(kernel_map, (vm_offset_t)&mbutl, &maxaddr,
			   VM_MBUF_SIZE, FALSE);
    /*
     * Initialize callouts
     */
    callfree = callout;
    for (i = 1; i < ncallout; i++)
	callout[i-1].c_next = &callout[i];
    
    printf("avail mem = %d\n", ptoa(vm_page_free_count));
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
}

void internal_configure()
{
    obio_internal_configure();
}

void consinit()
{
    extern void cninit();
    cninit();
}

void cpu_reset()
{
    mon_reboot("");		/* or do i have to get the value out of
				 * the eeprom...bleh
				 */
}

#ifdef HPUXCOMPAT
tweaksigcode(ishpux)
{
	static short *sigtrap = NULL;
	extern short sigcode[], esigcode[];

	/* locate trap instruction in pcb_sigc */
	if (sigtrap == NULL) {
		sigtrap = esigcode;
		while (--sigtrap >= sigcode)
			if ((*sigtrap & 0xFFF0) == 0x4E40)
				break;
		if (sigtrap < sigcode)
			panic("bogus sigcode\n");
	}
	*sigtrap = ishpux ? 0x4E42 : 0x4E41;
}
#endif
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
	p->p_regs[PC] = entry & ~1;
	p->p_regs[SP] = stack;
#ifdef FPCOPROC
	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
#endif
#ifdef HPUXCOMPAT
	if (p->p_flag & SHPUX) {

		p->p_regs[A0] = 0;	/* not 68010 (bit 31), no FPA (30) */
		retval[0] = 0;		/* no float card */
#ifdef FPCOPROC
		retval[1] = 1;		/* yes 68881 */
#else
		retval[1] = 0;		/* no 68881 */
#endif
	}
	/*
	 * Ensure we perform the right action on traps type 1 and 2:
	 * If our parent is an HPUX process and we are being traced, turn
	 * on HPUX style interpretation.  Else if we were using the HPUX
	 * style interpretation, revert to the BSD interpretation.
	 *
	 * XXX This doesn't have much to do with setting registers but
	 * I didn't want to muck up kern_exec.c with this code, so I
	 * stuck it here.
	 */
	if ((p->p_pptr->p_flag & SHPUX) &&
	    (p->p_flag & STRC)) {
		tweaksigcode(1);
		p->p_addr->u_pcb.pcb_flags |= PCB_HPUXTRACE;
	} else if (p->p_addr->u_pcb.pcb_flags & PCB_HPUXTRACE) {
		tweaksigcode(0);
		p->p_addr->u_pcb.pcb_flags &= ~PCB_HPUXTRACE;
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

#ifdef HPUXCOMPAT
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
#ifdef HPUXCOMPAT
	if (p->p_flag & SHPUX)
		fsize = sizeof(struct sigframe) + sizeof(struct hpuxsigframe);
	else
#endif
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
#ifdef HPUXCOMPAT
	/*
	 * Create an HP-UX style sigcontext structure and associated goo
	 */
	if (p->p_flag & SHPUX) {
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
	frame->f_pc = USRSTACK - (esigcode - sigcode);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
	free((caddr_t)kfp, M_TEMP);
}

struct sigreturn_args {
    struct sigcontext *sigcntxp;
};

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
	struct sigreturn_args *uap;
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
#ifdef HPUXCOMPAT
	/*
	 * Grab context as an HP-UX style context and determine if it
	 * was one that we contructed in sendsig.
	 */
	if (p->p_flag & SHPUX) {
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
			p->p_sigacts->ps_onstack = hscp->hsc_onstack & 01;
			p->p_sigmask = hscp->hsc_mask &~ sigcantmask;
			frame = (struct frame *) p->p_regs;
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

int waittime = -1;

void boot(howto)
     int howto;
{

    mon_printf("booting....\n");
    sun3_stop();
    if ((howto&RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
	struct buf *bp;
	int iter, nbusy;
	extern struct proc proc0;

	
	waittime = 0;
	(void) splnet();
	printf("syncing disks... ");
		/*
		 * Release inodes held by texts before update.
		 */
	if (panicstr == 0)
	    vnode_pager_umount(NULL);
	sync(&proc0, (void *) NULL, (int *) NULL);
	
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
    resettodr();
    splhigh();
    if (howto&RB_HALT) {
	printf("\n");
	printf("The operating system has halted.\n");
	sun3_stop();
    } else {
	if (howto & RB_DUMP) {
	    printf("dumping not supported yet :)\n");
	}
    }
    cpu_reset();
    for(;;) ;
    /*NOTREACHED*/
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

straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}
