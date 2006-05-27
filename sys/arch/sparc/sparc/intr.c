/*	$NetBSD: intr.c,v 1.94.6.1 2006/05/27 22:49:52 kardel Exp $ */

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
 *	@(#)intr.c	8.3 (Berkeley) 11/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.94.6.1 2006/05/27 22:49:52 kardel Exp $");

#include "opt_multiprocessor.h"
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <net/netisr.h>

#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/instr.h>
#include <machine/intr.h>
#include <machine/trap.h>
#include <machine/promlib.h>

#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>

#if defined(MULTIPROCESSOR) && defined(DDB)
#include <machine/db_machdep.h>
#endif

void *softnet_cookie;
#if defined(MULTIPROCESSOR)
void *xcall_cookie;

/* Stats */
struct evcnt lev13_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,0,"xcall","std");
struct evcnt lev14_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,0,"xcall","fast");
EVCNT_ATTACH_STATIC(lev13_evcnt);
EVCNT_ATTACH_STATIC(lev14_evcnt);
#endif


void	strayintr(struct clockframe *);
#ifdef DIAGNOSTIC
void	bogusintr(struct clockframe *);
#endif
void	softnet(void *);

/*
 * Stray interrupt handler.  Clear it if possible.
 * If not, and if we get 10 interrupts in 10 seconds, panic.
 * XXXSMP: We are holding the kernel lock at entry & exit.
 */
void
strayintr(struct clockframe *fp)
{
	static int straytime, nstray;
	char bits[64];
	int timesince;

	printf("stray interrupt ipl 0x%x pc=0x%x npc=0x%x psr=%s\n",
		fp->ipl, fp->pc, fp->npc, bitmask_snprintf(fp->psr,
		       PSR_BITS, bits, sizeof(bits)));

	timesince = time_uptime - straytime;
	if (timesince <= 10) {
		if (++nstray > 10)
			panic("crazy interrupts");
	} else {
		straytime = time_uptime;
		nstray = 1;
	}
}


#ifdef DIAGNOSTIC
/*
 * Bogus interrupt for which neither hard nor soft interrupt bit in
 * the IPR was set.
 */
void
bogusintr(struct clockframe *fp)
{
	char bits[64];

	printf("cpu%d: bogus interrupt ipl 0x%x pc=0x%x npc=0x%x psr=%s\n",
		cpu_number(),
		fp->ipl, fp->pc, fp->npc, bitmask_snprintf(fp->psr,
		       PSR_BITS, bits, sizeof(bits)));
}
#endif /* DIAGNOSTIC */

/*
 * Get module ID of interrupt target.
 */
u_int
getitr(void)
{
#if defined(MULTIPROCESSOR)
	u_int v;

	if (!CPU_ISSUN4M || sparc_ncpus <= 1)
		return (0);

	v = *((u_int *)ICR_ITR);
	return (v + 8);
#else
	return (0);
#endif
}

/*
 * Set interrupt target.
 * Return previous value.
 */
u_int
setitr(u_int mid)
{
#if defined(MULTIPROCESSOR)
	u_int v;

	if (!CPU_ISSUN4M || sparc_ncpus <= 1)
		return (0);

	v = *((u_int *)ICR_ITR);
	*((u_int *)ICR_ITR) = CPU_MID2CPUNO(mid);
	return (v + 8);
#else
	return (0);
#endif
}

/*
 * Process software network interrupts.
 */
void
softnet(void *fp)
{
	int n, s;

	s = splhigh();
	n = netisr;
	netisr = 0;
	splx(s);

	if (n == 0)
		return;

#define DONETISR(bit, fn) do {		\
	if (n & (1 << bit))		\
		fn();			\
	} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

#if (defined(SUN4M) && !defined(MSIIEP)) || defined(SUN4D)
void	nmi_hard(void);
void	nmi_soft(struct trapframe *);

int	(*memerr_handler)(void);
int	(*sbuserr_handler)(void);
int	(*vmeerr_handler)(void);
int	(*moduleerr_handler)(void);

#if defined(MULTIPROCESSOR)
volatile int nmi_hard_wait = 0;
struct simplelock nmihard_lock = SIMPLELOCK_INITIALIZER;
int drop_into_rom_on_fatal = 1;
#endif

void
nmi_hard(void)
{
	/*
	 * A level 15 hard interrupt.
	 */
	int fatal = 0;
	uint32_t si;
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
	 * variable is less than the number of cpus.
	 */
	simple_lock(&nmihard_lock);
	nmi_hard_wait++;
	simple_unlock(&nmihard_lock);

	if (cpuinfo.master == 0) {
		while (nmi_hard_wait)
			;
		return;
	} else {
		int n = 100000;

		while (nmi_hard_wait < sparc_ncpus) {
			DELAY(1);
			if (n-- > 0)
				continue;
			printf("nmi_hard: SMP botch.");
			break;
		}
	}
#endif

	/*
	 * Examine pending system interrupts.
	 */
	si = *((uint32_t *)ICR_SI_PEND);
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
	if (fatal && drop_into_rom_on_fatal) {
		prom_abort();
		return;
	}
#endif

	if (fatal)
		panic("nmi");
}

/*
 * Non-maskable soft interrupt level 15 handler
 */
void
nmi_soft(struct trapframe *tf)
{
	if (cpuinfo.mailbox) {
		/* Check PROM messages */
		uint8_t msg = *(uint8_t *)cpuinfo.mailbox;
		switch (msg) {
		case OPENPROM_MBX_STOP:
		case OPENPROM_MBX_WD:
			/* In case there's an xcall in progress (unlikely) */
			spl0();
			cpuinfo.flags &= ~CPUFLG_READY;
			cpu_ready_mask &= ~(1 << cpu_number());
			prom_cpustop(0);
			break;
		case OPENPROM_MBX_ABORT:
		case OPENPROM_MBX_BPT:
			prom_cpuidle(0);
			/*
			 * We emerge here after someone does a
			 * prom_resumecpu(ournode).
			 */
			return;
		default:
			break;
		}
	}

#if defined(MULTIPROCESSOR)
	switch (cpuinfo.msg_lev15.tag) {
	case XPMSG15_PAUSECPU:
		/* XXX - assumes DDB is the only user of mp_pause_cpu() */
		cpuinfo.flags |= CPUFLG_PAUSED;
#if defined(DDB)
		/* trap(T_DBPAUSE) */
		__asm("ta 0x8b");
#else
		while (cpuinfo.flags & CPUFLG_PAUSED)
			/* spin */;
#endif /* DDB */
	}
	cpuinfo.msg_lev15.tag = 0;
#endif /* MULTIPROCESSOR */
}

#if defined(MULTIPROCESSOR)
/*
 * Respond to an xcall() request from another CPU.
 */
static void
xcallintr(void *v)
{

	/* Tally */
	lev13_evcnt.ev_count++;

	/* notyet - cpuinfo.msg.received = 1; */
	switch (cpuinfo.msg.tag) {
	case XPMSG_FUNC:
	    {
		volatile struct xpmsg_func *p = &cpuinfo.msg.u.xpmsg_func;

		if (p->func)
			p->retval = (*p->func)(p->arg0, p->arg1, p->arg2);
		break;
	    }
	}
	cpuinfo.msg.tag = 0;
	cpuinfo.msg.complete = 1;
}
#endif /* MULTIPROCESSOR */
#endif /* SUN4M || SUN4D */


#ifdef MSIIEP
/*
 * It's easier to make this separate so that not to further obscure
 * SUN4M case with more ifdefs.  There's no common functionality
 * anyway.
 */

#include <sparc/sparc/msiiepreg.h>

void	nmi_hard_msiiep(void);
void	nmi_soft_msiiep(void);


void
nmi_hard_msiiep(void)
{
	uint32_t si;
	char bits[128];
	int fatal = 0;

	si = mspcic_read_4(pcic_sys_ipr);
	printf("NMI: system interrupts: %s\n",
	       bitmask_snprintf(si, MSIIEP_SYS_IPR_BITS, bits, sizeof(bits)));

	if (si & MSIIEP_SYS_IPR_MEM_FAULT) {
		uint32_t afsr, afar, mfsr, mfar;

		afar = *(volatile uint32_t *)MSIIEP_AFAR;
		afsr = *(volatile uint32_t *)MSIIEP_AFSR;

		mfar = *(volatile uint32_t *)MSIIEP_MFAR;
		mfsr = *(volatile uint32_t *)MSIIEP_MFSR;

		if (afsr & MSIIEP_AFSR_ERR)
			printf("async fault: afsr=%s; afar=%08x\n",
			       bitmask_snprintf(afsr, MSIIEP_AFSR_BITS,
						bits, sizeof(bits)),
			       afar);

		if (mfsr & MSIIEP_MFSR_ERR)
			printf("mem fault: mfsr=%s; mfar=%08x\n",
			       bitmask_snprintf(mfsr, MSIIEP_MFSR_BITS,
						bits, sizeof(bits)),
			       mfar);

		fatal = 0;
	}

	if (si & MSIIEP_SYS_IPR_SERR) {	/* XXX */
		printf("serr#\n");
		fatal = 0;
	}

	if (si & MSIIEP_SYS_IPR_DMA_ERR) {
		printf("dma: %08x\n",
		       mspcic_read_stream_4(pcic_iotlb_err_addr));
		fatal = 0;
	}

	if (si & MSIIEP_SYS_IPR_PIO_ERR) {
		printf("pio: addr=%08x, cmd=%x\n",
		       mspcic_read_stream_4(pcic_pio_err_addr),
		       mspcic_read_stream_1(pcic_pio_err_cmd));
		fatal = 0;
	}

	if (fatal)
		panic("nmi");

	/* Clear the NMI if it was PCIC related */
	mspcic_write_1(pcic_sys_ipr_clr, MSIIEP_SYS_IPR_CLR_ALL);
}


void
nmi_soft_msiiep(void)
{

	panic("soft nmi");
}

#endif /* MSIIEP */


/*
 * Level 15 interrupts are special, and not vectored here.
 * Only `prewired' interrupts appear here; boot-time configured devices
 * are attached via intr_establish() below.
 */
struct intrhand *intrhand[15] = {
	NULL,			/*  0 = error */
	NULL,			/*  1 = software level 1 + Sbus */
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

/*
 * Soft interrupts use a separate set of handler chains.
 * This is necessary since soft interrupt handlers do not return a value
 * and therefore cannot be mixed with hardware interrupt handlers on a
 * shared handler chain.
 */
struct intrhand *sintrhand[15] = { NULL };

static void
ih_insert(struct intrhand **head, struct intrhand *ih)
{
	struct intrhand **p, *q;
	/*
	 * This is O(N^2) for long chains, but chains are never long
	 * and we do want to preserve order.
	 */
	for (p = head; (q = *p) != NULL; p = &q->ih_next)
		continue;
	*p = ih;
	ih->ih_next = NULL;
}

static void
ih_remove(struct intrhand **head, struct intrhand *ih)
{
	struct intrhand **p, *q;

	for (p = head; (q = *p) != ih; p = &q->ih_next)
		continue;
	if (q == NULL)
		panic("intr_remove: intrhand %p fun %p arg %p",
			ih, ih->ih_fun, ih->ih_arg);

	*p = q->ih_next;
	q->ih_next = NULL;
}

static int fastvec;		/* marks fast vectors (see below) */
extern int sparc_interrupt4m[];
extern int sparc_interrupt44c[];

#ifdef DIAGNOSTIC
static void
check_tv(int level)
{
	struct trapvec *tv;
	int displ;

	/* double check for legal hardware interrupt */
	tv = &trapbase[T_L1INT - 1 + level];
	displ = (CPU_ISSUN4M || CPU_ISSUN4D)
		? &sparc_interrupt4m[0] - &tv->tv_instr[1]
		: &sparc_interrupt44c[0] - &tv->tv_instr[1];

	/* has to be `mov level,%l3; ba _sparc_interrupt; rdpsr %l0' */
	if (tv->tv_instr[0] != I_MOVi(I_L3, level) ||
	    tv->tv_instr[1] != I_BA(0, displ) ||
	    tv->tv_instr[2] != I_RDPSR(I_L0))
		panic("intr_establish(%d)\n0x%x 0x%x 0x%x != 0x%x 0x%x 0x%x",
		    level,
		    tv->tv_instr[0], tv->tv_instr[1], tv->tv_instr[2],
		    I_MOVi(I_L3, level), I_BA(0, displ), I_RDPSR(I_L0));
}
#endif

/*
 * Wire a fast trap vector.  Only one such fast trap is legal for any
 * interrupt, and it must be a hardware interrupt.
 */
static void
inst_fasttrap(int level, void (*vec)(void))
{
	struct trapvec *tv;
	u_long hi22, lo10;
	int s;

	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		/* Can't wire to softintr slots */
		if (level == 1 || level == 4 || level == 6)
			return;
	}

#ifdef DIAGNOSTIC
	check_tv(level);
#endif

	tv = &trapbase[T_L1INT - 1 + level];
	hi22 = ((u_long)vec) >> 10;
	lo10 = ((u_long)vec) & 0x3ff;
	s = splhigh();

	/* kernel text is write protected -- let us in for a moment */
	pmap_kprotect((vaddr_t)tv & -PAGE_SIZE, PAGE_SIZE,
	    VM_PROT_READ|VM_PROT_WRITE);
	cpuinfo.cache_flush_all();
	tv->tv_instr[0] = I_SETHI(I_L3, hi22);	/* sethi %hi(vec),%l3 */
	tv->tv_instr[1] = I_JMPLri(I_G0, I_L3, lo10);/* jmpl %l3+%lo(vec),%g0 */
	tv->tv_instr[2] = I_RDPSR(I_L0);	/* mov %psr, %l0 */
	pmap_kprotect((vaddr_t)tv & -PAGE_SIZE, PAGE_SIZE, VM_PROT_READ);
	cpuinfo.cache_flush_all();
	fastvec |= 1 << level;
	splx(s);
}

/*
 * Uninstall a fast trap handler.
 */
static void
uninst_fasttrap(int level)
{
	struct trapvec *tv;
	int displ;	/* suspenders, belt, and buttons too */
	int s;

	tv = &trapbase[T_L1INT - 1 + level];
	s = splhigh();
	displ = (CPU_ISSUN4M || CPU_ISSUN4D)
		? &sparc_interrupt4m[0] - &tv->tv_instr[1]
		: &sparc_interrupt44c[0] - &tv->tv_instr[1];

	/* kernel text is write protected -- let us in for a moment */
	pmap_kprotect((vaddr_t)tv & -PAGE_SIZE, PAGE_SIZE,
	    VM_PROT_READ|VM_PROT_WRITE);
	cpuinfo.cache_flush_all();
	tv->tv_instr[0] = I_MOVi(I_L3, level);
	tv->tv_instr[1] = I_BA(0, displ);
	tv->tv_instr[2] = I_RDPSR(I_L0);
	pmap_kprotect((vaddr_t)tv & -PAGE_SIZE, PAGE_SIZE, VM_PROT_READ);
	cpuinfo.cache_flush_all();
	fastvec &= ~(1 << level);
	splx(s);
}

/*
 * Attach an interrupt handler to the vector chain for the given level.
 * This is not possible if it has been taken away as a fast vector.
 */
void
intr_establish(int level, int classipl,
	       struct intrhand *ih, void (*vec)(void))
{
	int s = splhigh();

#ifdef DIAGNOSTIC
	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		/* Check reserved softintr slots */
		if (level == 1 || level == 4 || level == 6)
			panic("intr_establish: reserved softintr level");
	}
#endif

	/*
	 * If a `fast vector' is currently tied to this level, we must
	 * first undo that.
	 */
	if (fastvec & (1 << level)) {
		printf("intr_establish: untie fast vector at level %d\n",
		    level);
		uninst_fasttrap(level);
	} else if (vec != NULL &&
		   intrhand[level] == NULL && sintrhand[level] == NULL) {
		inst_fasttrap(level, vec);
	}

	if (classipl == 0)
		classipl = level;

	/* A requested IPL cannot exceed its device class level */
	if (classipl < level)
		panic("intr_establish: class lvl (%d) < pil (%d)\n",
			classipl, level);

	/* pre-shift to PIL field in %psr */
	ih->ih_classipl = (classipl << 8) & PSR_PIL;

	ih_insert(&intrhand[level], ih);
	splx(s);
}

void
intr_disestablish(int level, struct intrhand *ih)
{

	ih_remove(&intrhand[level], ih);
}

/*
 * This is a softintr cookie.  NB that sic_pilreq MUST be the
 * first element in the struct, because the softintr_schedule()
 * macro in intr.h casts cookies to int * to get it.  On a
 * sun4m, sic_pilreq is an actual processor interrupt level that
 * is passed to raise(), and on a sun4 or sun4c sic_pilreq is a
 * bit to set in the interrupt enable register with ienab_bis().
 */
struct softintr_cookie {
	int sic_pilreq;		/* CPU-specific bits; MUST be first! */
	int sic_pil;		/* Actual machine PIL that is used */
	struct intrhand sic_hand;
};

/*
 * softintr_init(): initialise the MI softintr system.
 */
void
softintr_init(void)
{

	softnet_cookie = softintr_establish(IPL_SOFTNET, softnet, NULL);
#if defined(MULTIPROCESSOR) && (defined(SUN4M) || defined(SUN4D))
	/* Establish a standard soft interrupt handler for cross calls */
	xcall_cookie = softintr_establish(13, xcallintr, NULL);
#endif
}

/*
 * softintr_establish(): MI interface.  establish a func(arg) as a
 * software interrupt.
 */
void *
softintr_establish(int level, void (*fun)(void *), void *arg)
{
	struct softintr_cookie *sic;
	struct intrhand *ih;
	int pilreq;
	int pil;

	/*
	 * On a sun4m, the processor interrupt level is stored
	 * in the softintr cookie to be passed to raise().
	 *
	 * On a sun4 or sun4c the appropriate bit to set
	 * in the interrupt enable register is stored in
	 * the softintr cookie to be passed to ienab_bis().
	 */
	pil = pilreq = level;
	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		/* Select the most suitable of three available softint levels */
		if (level >= 1 && level < 4) {
			pil = 1;
			pilreq = IE_L1;
		} else if (level >= 4 && level < 6) {
			pil = 4;
			pilreq = IE_L4;
		} else {
			pil = 6;
			pilreq = IE_L6;
		}
	}

	sic = malloc(sizeof(*sic), M_DEVBUF, 0);
	sic->sic_pil = pil;
	sic->sic_pilreq = pilreq;
	ih = &sic->sic_hand;
	ih->ih_fun = (int (*)(void *))fun;
	ih->ih_arg = arg;

	/*
	 * Always run the handler at the requested level, which might
	 * be higher than the hardware can provide.
	 *
	 * pre-shift to PIL field in %psr
	 */
	ih->ih_classipl = (level << 8) & PSR_PIL;

	if (fastvec & (1 << pil)) {
		printf("softintr_establish: untie fast vector at level %d\n",
		    pil);
		uninst_fasttrap(level);
	}

	ih_insert(&sintrhand[pil], ih);
	return (void *)sic;
}

/*
 * softintr_disestablish(): MI interface.  disestablish the specified
 * software interrupt.
 */
void
softintr_disestablish(void *cookie)
{
	struct softintr_cookie *sic = cookie;

	ih_remove(&sintrhand[sic->sic_pil], &sic->sic_hand);
	free(cookie, M_DEVBUF);
}

#if 0
void
softintr_schedule(void *cookie)
{
	struct softintr_cookie *sic = cookie;
	if (CPU_ISSUN4M || CPU_ISSUN4D) {
#if defined(SUN4M) || defined(SUN4D)
		extern void raise(int,int);
		raise(0, sic->sic_pilreq);
#endif
	} else {
#if defined(SUN4) || defined(SUN4C)
		ienab_bis(sic->sic_pilreq);
#endif
	}
}
#endif

#ifdef MULTIPROCESSOR
/*
 * Called by interrupt stubs, etc., to lock/unlock the kernel.
 */
void
intr_lock_kernel(void)
{

	KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
}

void
intr_unlock_kernel(void)
{

	KERNEL_UNLOCK();
}
#endif
