/*	$NetBSD: mips_machdep.c,v 1.18 1997/07/19 09:54:37 jonathan Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/exec.h>
#include <sys/reboot.h>
#include <sys/mount.h>			/* fsid_t for syscallargs */
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/user.h>

#include <mips/cpu.h>			/* declaration of of cpu_id */
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/locore.h>
#include <mips/vmparam.h>			/* USRSTACK */
#include <mips/psl.h>
#include <machine/cpu.h>		/* declaration of of cpu_id */

mips_locore_jumpvec_t mips_locore_jumpvec = {
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL
};

/*
 * Forward declarations
 * XXX should be in a header file so each mips port can include it.
 */
extern void cpu_identify __P((void));
extern void mips_vector_init __P((void));
extern void savefpregs __P((struct proc *));		/* lcoore */

void mips1_vector_init __P((void));
void mips3_vector_init __P((void));

/* the following is used externally (sysctl_hw) */
char	machine_arch[] = MACHINE_ARCH;	/* cpu "architecture" */

#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
/*
 * MIPS-I (r2000) locore-function vector.
 */
mips_locore_jumpvec_t mips1_locore_vec =
{
	mips1_ConfigCache,
	mips1_FlushCache,
	mips1_FlushDCache,
	mips1_FlushICache,
	/*mips1_FlushICache*/ mips1_FlushCache,
	mips1_SetPID,
	mips1_TLBFlush,
	mips1_TLBFlushAddr,
	mips1_TLBUpdate,
	mips1_wbflush,
	mips1_proc_trampoline,
	mips1_switch_exit,
	mips1_cpu_switch_resume
};

void
mips1_vector_init()
{
	extern char mips1_UTLBMiss[], mips1_UTLBMissEnd[];
	extern char mips1_exception[], mips1_exceptionEnd[];

	/*
	 * Copy down exception vector code.
	 */
	if (mips1_UTLBMissEnd - mips1_UTLBMiss > 0x80)
		panic("startup: UTLB code too large");
	bcopy(mips1_UTLBMiss, (char *)MIPS_UTLB_MISS_EXC_VEC,
		mips1_UTLBMissEnd - mips1_UTLBMiss);
	bcopy(mips1_exception, (char *)MIPS1_GEN_EXC_VEC,
	      mips1_exceptionEnd - mips1_exception);

	/*
	 * Copy locore-function vector.
	 */
	bcopy(&mips1_locore_vec, &mips_locore_jumpvec,
	      sizeof(mips_locore_jumpvec_t));

	/*
	 * Clear out the I and D caches.
	 */
	mips1_ConfigCache();
	mips1_FlushCache();
}
#endif /* MIPS1 */


#ifdef MIPS3		/* r4000 family (mips-III cpu) */
/*
 * MIPS-III (r4000) locore-function vector.
 */
mips_locore_jumpvec_t mips3_locore_vec =
{
	mips3_ConfigCache,
	mips3_FlushCache,
	mips3_FlushDCache,
	mips3_FlushICache,
#if 0
	 /*
	  * No such vector exists, perhaps it was meant to be HitFlushDCache?
	  */
	mips3_ForceCacheUpdate,
#else
	mips3_FlushCache,
#endif
	mips3_SetPID,
	mips3_TLBFlush,
	mips3_TLBFlushAddr,
	mips3_TLBUpdate,
	mips3_wbflush,
	mips3_proc_trampoline,
	mips3_switch_exit,
	mips3_cpu_switch_resume
};

void
mips3_vector_init()
{

	/* r4000 exception handler address and end */
	extern char mips3_exception[], mips3_exceptionEnd[];

	/* TLB miss handler address and end */
	extern char mips3_TLBMiss[], mips3_TLBMissEnd[];

	/*
	 * Copy down exception vector code.
	 */
#if 0  /* XXX not restricted? */
	if (mips3_TLBMissEnd - mips3_TLBMiss > 0x80)
		panic("startup: UTLB code too large");
#endif
	bcopy(mips3_TLBMiss, (char *)MIPS_UTLB_MISS_EXC_VEC,
	      mips3_TLBMissEnd - mips3_TLBMiss);

	bcopy(mips3_exception, (char *)MIPS3_GEN_EXC_VEC,
	      mips3_exceptionEnd - mips3_exception);

	/*
	 * Copy locore-function vector.
	 */
	bcopy(&mips3_locore_vec, &mips_locore_jumpvec,
	      sizeof(mips_locore_jumpvec_t));

	/*
	 * Clear out the I and D caches.
	 */
	mips3_ConfigCache();
	mips3_FlushCache();
}
#endif	/* MIPS3 */


/*
 * Do all the stuff that locore normally does before calling main(),
 * that is common to all mips-CPU NetBSD ports.
 *
 * The principal purpose of this function is to examine the
 * variable cpu_id, into which the kernel locore start code
 * writes the cpu ID register, and to then copy appropriate
 * cod into the CPU exception-vector entries and the jump tables
 * used to  hide the differences in cache and TLB handling in
 * different MIPS  CPUs.
 * 
 * This should be the very first thing called by each port's
 * init_main() function.
 */

/*
 * Initialize the hardware exception vectors, and the jump table used to
 * call locore cache and TLB management functions, based on the kind
 * of CPU the kernel is running on.
 */
void
mips_vector_init()
{


#ifdef notyet
	register caddr_t v;
	extern char edata[], end[];

	/* clear the BSS segment */
	v = (caddr_t)mips_round_page(end);
	bzero(edata, v - edata);
#endif
	int i;

	(void) &i;		/* shut off gcc unused-variable warnings */

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */

	switch (cpu_id.cpu.cp_imp) {
#ifdef MIPS1
	case MIPS_R2000:
	case MIPS_R3000:
		cpu_arch = 1;
		mips1_TLBFlush();
		for (i = 0; i < MIPS1_TLB_FIRST_RAND_ENTRY; ++i)
			mips1_TLBWriteIndexed(i, MIPS_KSEG0_START, 0);
		mips1_vector_init();
		break;
#endif
#ifdef MIPS3
	case MIPS_R4000:
		cpu_arch = 3;
		mips3_SetWIRED(0);
		mips3_TLBFlush();
		mips3_SetWIRED(MIPS3_TLB_WIRED_ENTRIES);
		mips3_vector_init();
		break;
#endif

	default:
		printf("CPU type (%d) not supported\n", cpu_id.cpu.cp_imp);
		cpu_reboot(RB_HALT, NULL);
	}
}


/*
 * Identify cpu and fpu type and revision.
 *
 * XXX Should be moved to mips_cpu.c but that doesn't exist
 */
void
cpu_identify()
{
	/* Work out what kind of CPU and FPU are present. */

	switch(cpu_id.cpu.cp_imp) {

	case MIPS_R2000:
		printf("MIPS R2000 CPU");
		break;
	case MIPS_R3000:

	  	/*
		 * XXX
		 * R2000A silicion has an r3000 core and shows up here.
		 *  The caller should indicate that by setting a flag
		 * indicating the baseboard is  socketed for an r2000.
		 * Needs more thought.
		 */
#ifdef notyet
	  	if (SYSTEM_HAS_R2000_CPU_SOCKET())
			printf("MIPS R2000A CPU");
		else
#endif
		printf("MIPS R3000 CPU");
		break;
	case MIPS_R6000:
		printf("MIPS R6000 CPU");
		break;

	case MIPS_R4000:
		if(mips_L1InstCacheSize == 16384)
			printf("MIPS R4400 CPU");
		else
			printf("MIPS R4000 CPU");
		break;
	case MIPS_R3LSI:
		printf("LSI Logic R3000 derivate");
		break;
	case MIPS_R6000A:
		printf("MIPS R6000A CPU");
		break;
	case MIPS_R3IDT:
		printf("IDT R3000 derivate");
		break;
	case MIPS_R10000:
		printf("MIPS R10000/T5 CPU");
		break;
	case MIPS_R4200:
		printf("MIPS R4200 CPU (ICE)");
		break;
	case MIPS_R8000:
		printf("MIPS R8000 Blackbird/TFP CPU");
		break;
	case MIPS_R4600:
		printf("QED R4600 Orion CPU");
		break;
	case MIPS_R3SONY:
		printf("Sony R3000 based CPU");
		break;
	case MIPS_R3TOSH:
		printf("Toshiba R3000 based CPU");
		break;
	case MIPS_R3NKK:
		printf("NKK R3000 based CPU");
		break;
	case MIPS_UNKC1:
	case MIPS_UNKC2:
	default:
		printf("Unknown CPU type (0x%x)",cpu_id.cpu.cp_imp);
		break;
	}
	printf(" Rev. %d.%d with ", cpu_id.cpu.cp_majrev, cpu_id.cpu.cp_minrev);


	switch(fpu_id.cpu.cp_imp) {

	case MIPS_SOFT:
		printf("Software emulation float");
		break;
	case MIPS_R2360:
		printf("MIPS R2360 FPC");
		break;
	case MIPS_R2010:
		printf("MIPS R2010 FPC");
		break;
	case MIPS_R3010:
	  	/*
		 * XXX FPUs  for R2000A(?) silicion has an r3010 core and
		 *  shows up here.
		 */
#ifdef notyet
	  	if (SYSTEM_HAS_R2000_CPU_SOCKET())
			printf("MIPS R2010A CPU");
		else
#endif
		printf("MIPS R3010 FPC");
		break;
	case MIPS_R6010:
		printf("MIPS R6010 FPC");
		break;
	case MIPS_R4010:
		printf("MIPS R4010 FPC");
		break;
	case MIPS_R31LSI:
		printf("FPC");
		break;
	case MIPS_R10010:
		printf("MIPS R10000/T5 FPU");
		break;
	case MIPS_R4210:
		printf("MIPS R4200 FPC (ICE)");
	case MIPS_R8000:
		printf("MIPS R8000 Blackbird/TFP");
		break;
	case MIPS_R4600:
		printf("QED R4600 Orion FPC");
		break;
	case MIPS_R3SONY:
		printf("Sony R3000 based FPC");
		break;
	case MIPS_R3TOSH:
		printf("Toshiba R3000 based FPC");
		break;
	case MIPS_R3NKK:
		printf("NKK R3000 based FPC");
		break;
	case MIPS_UNKF1:
	default:
		printf("Unknown FPU type (0x%x)", fpu_id.cpu.cp_imp);
		break;
	}
	printf(" Rev. %d.%d", fpu_id.cpu.cp_majrev, fpu_id.cpu.cp_minrev);
	printf("\n");

	printf("        L1 cache: %dkb Instruction, %dkb Data.",
	    mips_L1InstCacheSize / 1024,
	    mips_L1DataCacheSize / 1024);
	if (mips_L2CacheSize)
		printf(" L2 cache: %dkb mixed.\n", mips_L2CacheSize / 1024);
	else
		printf("\n");
/* XXX cache sizes for MIPS1? */
}


/*
 * Set registers on exec.
 * Clear all registers except sp, pc, and t9.
 * $sp is set to the stack pointer passed in.  $pc is set to the entry
 * point given by the exec_package passed in, as is $t9 (used for PIC
 * code by the MIPS elf abi).
 */
void
setregs(p, pack, stack, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	extern struct proc *fpcurproc;

	bzero((caddr_t)p->p_md.md_regs, sizeof(struct frame));
	bzero((caddr_t)&p->p_addr->u_pcb.pcb_fpregs, sizeof(struct fpreg));
	p->p_md.md_regs[SP] = stack;
	p->p_md.md_regs[PC] = pack->ep_entry & ~3;
	p->p_md.md_regs[T9] = pack->ep_entry & ~3; /* abicall requirement */
	p->p_md.md_regs[PS] = PSL_USERSET;
	p->p_md.md_flags &= ~MDP_FPUSED;
	if (fpcurproc == p)
		fpcurproc = (struct proc *)0;
	p->p_md.md_ss_addr = 0;

	/*
	 * Set up arguments for the dld-capable crt0:
	 *
	 *	a0	stack pointer
	 *	a1	rtld cleanup (filled in by dynamic loader)
	 *	a2	rtld object (filled in by dynamic loader)
	 *	a3	ps_strings
	 */
	p->p_md.md_regs[A0] = stack;
	p->p_md.md_regs[A1] = 0;
	p->p_md.md_regs[A2] = 0;
	p->p_md.md_regs[A3] = (u_long)PS_STRINGS;
}

/*
 * WARNING: code in locore.s assumes the layout shown for sf_signum
 * thru sf_handler so... don't screw with them!
 */
struct sigframe {
	int	sf_signum;		/* signo for handler */
	int	sf_code;		/* additional info for handler */
	struct	sigcontext *sf_scp;	/* context ptr for handler */
	sig_t	sf_handler;		/* handler addr for u_sigc */
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
	u_long code;
{
	register struct proc *p = curproc;
	register struct sigframe *fp;
	register int *regs;
	register struct sigacts *psp = p->p_sigacts;
	int oonstack, fsize;
	struct sigcontext ksc;
	extern char sigcode[], esigcode[];

	regs = p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in data space, the
	 * call to grow() is a nop, and the copyout()
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	fsize = sizeof(struct sigframe);
	if ((psp->ps_flags & SAS_ALTSTACK) &&
	    (psp->ps_sigstk.ss_flags & SS_ONSTACK) == 0 &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp +
					 psp->ps_sigstk.ss_size - fsize);
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else
		fp = (struct sigframe *)(regs[SP] - fsize);
	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);
#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sendsig(%d): sig %d ssp %p usp %p scp %p\n",
		       p->p_pid, sig, &oonstack, fp, &fp->sf_sc);
#endif
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	ksc.sc_onstack = oonstack;
	ksc.sc_mask = mask;
	ksc.sc_pc = regs[PC];
	ksc.mullo = regs[MULLO];
	ksc.mulhi = regs[MULHI];
	ksc.sc_regs[ZERO] = 0xACEDBADE;		/* magic number */
	bcopy((caddr_t)&regs[1], (caddr_t)&ksc.sc_regs[1],
		sizeof(ksc.sc_regs) - sizeof(ksc.sc_regs[0]));
	ksc.sc_fpused = p->p_md.md_flags & MDP_FPUSED;
	if (ksc.sc_fpused) {
		extern struct proc *fpcurproc;

		/* if FPU has current state, save it first */
		if (p == fpcurproc)
			savefpregs(p);
		*(struct fpreg *)ksc.sc_fpregs = p->p_addr->u_pcb.pcb_fpregs;
	}
	if (copyout((caddr_t)&ksc, (caddr_t)&fp->sf_sc, sizeof(ksc))) {
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
	regs[A0] = sig;
	regs[A1] = code;
	regs[A2] = (int)&fp->sf_sc;
	regs[A3] = (int)catcher;

	regs[PC] = (int)catcher;
	regs[T9] = (int)catcher;
	regs[SP] = (int)fp;
	/*
	 * Signal trampoline code is at base of user stack.
	 */
	regs[RA] = (int)PS_STRINGS - (esigcode - sigcode);
#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
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
	register int *regs;
	struct sigcontext ksc;
	int error;

	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
#endif
	regs = p->p_md.md_regs;
	/*
	 * Test and fetch the context structure.
	 * We grab it all at once for speed.
	 */
	error = copyin((caddr_t)scp, (caddr_t)&ksc, sizeof(ksc));
	if (error || ksc.sc_regs[ZERO] != 0xACEDBADE) {
#ifdef DEBUG
		if (!(sigdebug & SDB_FOLLOW))
			printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
		printf("  old sp %x ra %x pc %x\n",
			regs[SP], regs[RA], regs[PC]);
		printf("  new sp %x ra %x pc %x err %d z %x\n",
			ksc.sc_regs[SP], ksc.sc_regs[RA], ksc.sc_regs[PC],
			error, ksc.sc_regs[ZERO]);
#endif
		return (EINVAL);
	}
	scp = &ksc;
	/*
	 * Restore the user supplied information
	 */
	if (scp->sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = scp->sc_mask &~ sigcantmask;
	regs[PC] = scp->sc_pc;
	regs[MULLO] = scp->mullo;
	regs[MULHI] = scp->mulhi;
	bcopy((caddr_t)&scp->sc_regs[1], (caddr_t)&regs[1],
		sizeof(scp->sc_regs) - sizeof(scp->sc_regs[0]));
	if (scp->sc_fpused)
		p->p_addr->u_pcb.pcb_fpregs = *(struct fpreg *)scp->sc_fpregs;
	return (EJUSTRETURN);
}

