/*	$NetBSD: intr.c,v 1.54 2001/07/10 15:09:04 mrg Exp $ */

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
 *	@(#)intr.c	8.3 (Berkeley) 11/11/93
 */

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <net/netisr.h>

#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/instr.h>
#include <machine/trap.h>
#include <machine/promlib.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>

#if defined(MULTIPROCESSOR) && defined(DDB)
#include <machine/db_machdep.h>
#endif

#include "com.h"
#if NCOM > 0
extern void comsoft __P((void));
#endif

union sir	sir;

void	strayintr __P((struct clockframe *));
int	soft01intr __P((void *));

/*
 * Stray interrupt handler.  Clear it if possible.
 * If not, and if we get 10 interrupts in 10 seconds, panic.
 * XXXSMP: We are holding the kernel lock at entry & exit.
 */
void
strayintr(fp)
	struct clockframe *fp;
{
	static int straytime, nstray;
	char bits[64];
	int timesince;

	printf("stray interrupt ipl 0x%x pc=0x%x npc=0x%x psr=%s\n",
		fp->ipl, fp->pc, fp->npc, bitmask_snprintf(fp->psr,
		       PSR_BITS, bits, sizeof(bits)));

	timesince = time.tv_sec - straytime;
	if (timesince <= 10) {
		if (++nstray > 9)
			panic("crazy interrupts");
	} else {
		straytime = time.tv_sec;
		nstray = 1;
	}
}

/*
 * Level 1 software interrupt (could also be Sbus level 1 interrupt).
 * Three possible reasons:
 *	ROM console input needed
 *	Network software interrupt
 *	Soft clock interrupt
 */
int
soft01intr(fp)
	void *fp;
{

	KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
	if (sir.sir_any) {
		/*
		 * XXX	this is bogus: should just have a list of
		 *	routines to call, a la timeouts.  Mods to
		 *	netisr are not atomic and must be protected (gah).
		 */
		if (sir.sir_which[SIR_NET]) {
			int n, s;

			s = splhigh();
			n = netisr;
			netisr = 0;
			splx(s);
			sir.sir_which[SIR_NET] = 0;

#define DONETISR(bit, fn) do {		\
	if (n & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR

		}
		if (sir.sir_which[SIR_CLOCK]) {
			sir.sir_which[SIR_CLOCK] = 0;
			softclock(NULL);
		}
#if NCOM > 0
		/*
		 * XXX - consider using __GENERIC_SOFT_INTERRUPTS instead
		 */
		if (sir.sir_which[SIR_SERIAL]) {
			sir.sir_which[SIR_SERIAL] = 0;
			comsoft();
		}
#endif
	}
	KERNEL_UNLOCK();
	return (1);
}

#if defined(SUN4M)
void	nmi_hard __P((void));
void	nmi_soft __P((struct trapframe *));

int	(*memerr_handler) __P((void));
int	(*sbuserr_handler) __P((void));
int	(*vmeerr_handler) __P((void));
int	(*moduleerr_handler) __P((void));

#if defined(MULTIPROCESSOR)
volatile int nmi_hard_wait = 0;
struct simplelock nmihard_lock = SIMPLELOCK_INITIALIZER;
#endif

void
nmi_hard()
{
	/*
	 * A level 15 hard interrupt.
	 */
	int fatal = 0;
	u_int32_t si;
	char bits[64];
	u_int afsr, afva;

	afsr = afva = 0;
	if ((*cpuinfo.get_asyncflt)(&afsr, &afva) == 0) {
		printf("Async registers (mid %d): afsr=%s; afva=0x%x%x\n",
			cpuinfo.mid,
			bitmask_snprintf(afsr, AFSR_BITS, bits, sizeof(bits)),
			(afsr & AFSR_AFA) >> AFSR_AFA_RSHIFT, afva);
	}

#if defined(MULTIPROCESSOR)
	/*
	 * Increase nmi_hard_wait.  If we aren't the master, loop while this
	 * variable is non-zero.  If we are the master, loop while this
	 * variable is less then the number of cpus.
	 */
	simple_lock(&nmihard_lock);
	nmi_hard_wait++;
	simple_unlock(&nmihard_lock);

	if (cpuinfo.master == 0) {
		while (nmi_hard_wait)
			;
		return;
	} else {
		int n = 0;

		while (nmi_hard_wait < ncpu)
			if (n++ > 100000)
				panic("nmi_hard: SMP botch.");
	}
#endif

	/*
	 * Examine pending system interrupts.
	 */
	si = *((u_int32_t *)ICR_SI_PEND);
	printf("cpu%d: NMI: system interrupts: %s\n", cpu_number(),
		bitmask_snprintf(si, SINTR_BITS, bits, sizeof(bits)));

	if ((si & SINTR_M) != 0) {
		/* ECC memory error */
		if (memerr_handler != NULL)
			fatal |= (*memerr_handler)();
	}
	if ((si & SINTR_I) != 0) {
		/* MBus/SBus async error */
		if (sbuserr_handler != NULL)
			fatal |= (*sbuserr_handler)();
	}
	if ((si & SINTR_V) != 0) {
		/* VME async error */
		if (vmeerr_handler != NULL)
			fatal |= (*vmeerr_handler)();
	}
	if ((si & SINTR_ME) != 0) {
		/* Module async error */
		if (moduleerr_handler != NULL)
			fatal |= (*moduleerr_handler)();
	}

#if defined(MULTIPROCESSOR)
	/*
	 * Tell everyone else we've finished dealing with the hard NMI.
	 */
	simple_lock(&nmihard_lock);
	nmi_hard_wait = 0;
	simple_unlock(&nmihard_lock);
#endif

	if (fatal)
		panic("nmi");
}

void
nmi_soft(tf)
	struct trapframe *tf;
{

#ifdef MULTIPROCESSOR
	switch (cpuinfo.msg.tag) {
	case XPMSG_SAVEFPU:
		savefpstate(cpuinfo.fpproc->p_md.md_fpstate);
		cpuinfo.fpproc->p_md.md_fpumid = -1;
		cpuinfo.fpproc = NULL;
		break;
	case XPMSG_PAUSECPU:
	    {
#if defined(DDB)
		db_regs_t regs;

		regs.db_tf = *tf;
		regs.db_fr = *(struct frame *)tf->tf_out[6];
		cpuinfo.ci_ddb_regs = &regs;
#endif
		cpuinfo.flags |= CPUFLG_PAUSED|CPUFLG_GOTMSG;
		while (cpuinfo.flags & CPUFLG_PAUSED)
			cpuinfo.cache_flush((caddr_t)&cpuinfo.flags,
			    sizeof(cpuinfo.flags));
#if defined(DDB)
		cpuinfo.ci_ddb_regs = 0;
#endif
		return;
	    }
	case XPMSG_FUNC:
	    {
		struct xpmsg_func *p = &cpuinfo.msg.u.xpmsg_func;

		p->retval = (*p->func)(p->arg0, p->arg1, p->arg2, p->arg3); 
		break;
	    }
	case XPMSG_VCACHE_FLUSH_PAGE:
	    {
		struct xpmsg_flush_page *p = &cpuinfo.msg.u.xpmsg_flush_page;
		int ctx = getcontext();

		setcontext(p->ctx);
		cpuinfo.sp_vcache_flush_page(p->va);
		setcontext(ctx);
		break;
	    }
	case XPMSG_VCACHE_FLUSH_SEGMENT:
	    {
		struct xpmsg_flush_segment *p = &cpuinfo.msg.u.xpmsg_flush_segment;
		int ctx = getcontext();

		setcontext(p->ctx);
		cpuinfo.sp_vcache_flush_segment(p->vr, p->vs);
		setcontext(ctx);
		break;
	    }
	case XPMSG_VCACHE_FLUSH_REGION:
	    {
		struct xpmsg_flush_region *p = &cpuinfo.msg.u.xpmsg_flush_region;
		int ctx = getcontext();

		setcontext(p->ctx);
		cpuinfo.sp_vcache_flush_region(p->vr);
		setcontext(ctx);
		break;
	    }
	case XPMSG_VCACHE_FLUSH_CONTEXT:
	    {
		struct xpmsg_flush_context *p = &cpuinfo.msg.u.xpmsg_flush_context;
		int ctx = getcontext();

		setcontext(p->ctx);
		cpuinfo.sp_vcache_flush_context();
		setcontext(ctx);
		break;
	    }
	case XPMSG_VCACHE_FLUSH_RANGE:
	    {
		struct xpmsg_flush_range *p = &cpuinfo.msg.u.xpmsg_flush_range;
		int ctx = getcontext();

		setcontext(p->ctx);
		cpuinfo.sp_cache_flush(p->va, p->size);
		setcontext(ctx);
		break;
	    }
	case XPMSG_DEMAP_TLB_PAGE:
	    {
		struct xpmsg_flush_page *p = &cpuinfo.msg.u.xpmsg_flush_page;
		int ctx = getcontext();

		setcontext(p->ctx);
		tlb_flush_page_real(p->va);
		setcontext(ctx);
		break;
	    }
	case XPMSG_DEMAP_TLB_SEGMENT:
	    {
		struct xpmsg_flush_segment *p = &cpuinfo.msg.u.xpmsg_flush_segment;
		int ctx = getcontext();

		setcontext(p->ctx);
		tlb_flush_segment_real(p->vr, p->vs);
		setcontext(ctx);
		break;
	    }
	case XPMSG_DEMAP_TLB_REGION:
	    {
		struct xpmsg_flush_region *p = &cpuinfo.msg.u.xpmsg_flush_region;
		int ctx = getcontext();

		setcontext(p->ctx);
		tlb_flush_region_real(p->vr);
		setcontext(ctx);
		break;
	    }
	case XPMSG_DEMAP_TLB_CONTEXT:
	    {
		struct xpmsg_flush_context *p = &cpuinfo.msg.u.xpmsg_flush_context;
		int ctx = getcontext();

		setcontext(p->ctx);
		tlb_flush_context_real();
		setcontext(ctx);
		break;
	    }
	case XPMSG_DEMAP_TLB_ALL:
		tlb_flush_all_real();
		break;
	}
	cpuinfo.flags |= CPUFLG_GOTMSG;
#endif
}
#endif

static struct intrhand level01 = { soft01intr };

/*
 * Level 15 interrupts are special, and not vectored here.
 * Only `prewired' interrupts appear here; boot-time configured devices
 * are attached via intr_establish() below.
 */
struct intrhand *intrhand[15] = {
	NULL,			/*  0 = error */
	&level01,		/*  1 = software level 1 + Sbus */
	NULL,	 		/*  2 = Sbus level 2 (4m: Sbus L1) */
	NULL,			/*  3 = SCSI + DMA + Sbus level 3 (4m: L2,lpt)*/
	NULL,			/*  4 = software level 4 (tty softint) (scsi) */
	NULL,			/*  5 = Ethernet + Sbus level 4 (4m: Sbus L3) */
	NULL,			/*  6 = software level 6 (not used) (4m: enet)*/
	NULL,			/*  7 = video + Sbus level 5 */
	NULL,			/*  8 = Sbus level 6 */
	NULL,			/*  9 = Sbus level 7 */
	NULL, 			/* 10 = counter 0 = clock */
	NULL,			/* 11 = floppy */
	NULL,			/* 12 = zs hardware interrupt */
	NULL,			/* 13 = audio chip */
	NULL, 			/* 14 = counter 1 = profiling timer */
};

static int fastvec;		/* marks fast vectors (see below) */
#ifdef DIAGNOSTIC
extern int sparc_interrupt4m[];
extern int sparc_interrupt44c[];
#endif

/*
 * Attach an interrupt handler to the vector chain for the given level.
 * This is not possible if it has been taken away as a fast vector.
 */
void
intr_establish(level, ih)
	int level;
	struct intrhand *ih;
{
	struct intrhand **p, *q;
#ifdef DIAGNOSTIC
	struct trapvec *tv;
	int displ;
#endif
	int s;

	s = splhigh();
	if (fastvec & (1 << level))
		panic("intr_establish: level %d interrupt tied to fast vector",
		    level);
#ifdef DIAGNOSTIC
	/* double check for legal hardware interrupt */
	if ((level != 1 && level != 4 && level != 6) || CPU_ISSUN4M ) {
		tv = &trapbase[T_L1INT - 1 + level];
		displ = (CPU_ISSUN4M)
			? &sparc_interrupt4m[0] - &tv->tv_instr[1]
			: &sparc_interrupt44c[0] - &tv->tv_instr[1];

		/* has to be `mov level,%l3; ba _sparc_interrupt; rdpsr %l0' */
		if (tv->tv_instr[0] != I_MOVi(I_L3, level) ||
		    tv->tv_instr[1] != I_BA(0, displ) ||
		    tv->tv_instr[2] != I_RDPSR(I_L0))
			panic("intr_establish(%d, %p)\n0x%x 0x%x 0x%x != 0x%x 0x%x 0x%x",
			    level, ih,
			    tv->tv_instr[0], tv->tv_instr[1], tv->tv_instr[2],
			    I_MOVi(I_L3, level), I_BA(0, displ), I_RDPSR(I_L0));
	}
#endif
	/*
	 * This is O(N^2) for long chains, but chains are never long
	 * and we do want to preserve order.
	 */
	for (p = &intrhand[level]; (q = *p) != NULL; p = &q->ih_next)
		continue;
	*p = ih;
	ih->ih_next = NULL;
	splx(s);
}

/*
 * Like intr_establish, but wires a fast trap vector.  Only one such fast
 * trap is legal for any interrupt, and it must be a hardware interrupt.
 */
void
intr_fasttrap(level, vec)
	int level;
	void (*vec) __P((void));
{
	struct trapvec *tv;
	u_long hi22, lo10;
#ifdef DIAGNOSTIC
	int displ;	/* suspenders, belt, and buttons too */
#endif
	int s;

	tv = &trapbase[T_L1INT - 1 + level];
	hi22 = ((u_long)vec) >> 10;
	lo10 = ((u_long)vec) & 0x3ff;
	s = splhigh();
	if ((fastvec & (1 << level)) != 0 || intrhand[level] != NULL)
		panic("intr_fasttrap: already handling level %d interrupts",
		    level);
#ifdef DIAGNOSTIC
	displ = (CPU_ISSUN4M)
		? &sparc_interrupt4m[0] - &tv->tv_instr[1]
		: &sparc_interrupt44c[0] - &tv->tv_instr[1];

	/* has to be `mov level,%l3; ba _sparc_interrupt; rdpsr %l0' */
	if (tv->tv_instr[0] != I_MOVi(I_L3, level) ||
	    tv->tv_instr[1] != I_BA(0, displ) ||
	    tv->tv_instr[2] != I_RDPSR(I_L0))
		panic("intr_fasttrap(%d, %p)\n0x%x 0x%x 0x%x != 0x%x 0x%x 0x%x",
		    level, vec,
		    tv->tv_instr[0], tv->tv_instr[1], tv->tv_instr[2],
		    I_MOVi(I_L3, level), I_BA(0, displ), I_RDPSR(I_L0));
#endif
	/* kernel text is write protected -- let us in for a moment */
	pmap_changeprot(pmap_kernel(), (vaddr_t)tv,
	    VM_PROT_READ|VM_PROT_WRITE, 1);
	cpuinfo.cache_flush_all();
	tv->tv_instr[0] = I_SETHI(I_L3, hi22);	/* sethi %hi(vec),%l3 */
	tv->tv_instr[1] = I_JMPLri(I_G0, I_L3, lo10);/* jmpl %l3+%lo(vec),%g0 */
	tv->tv_instr[2] = I_RDPSR(I_L0);	/* mov %psr, %l0 */
	pmap_changeprot(pmap_kernel(), (vaddr_t)tv, VM_PROT_READ, 1);
	cpuinfo.cache_flush_all();
	fastvec |= 1 << level;
	splx(s);
}

#ifdef MULTIPROCESSOR
/*
 * Called by interrupt stubs, etc., to lock/unlock the kernel.
 */
void
intr_lock_kernel()
{

	KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
}

void
intr_unlock_kernel()
{

	KERNEL_UNLOCK();
}
#endif
