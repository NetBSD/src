/*	$NetBSD: cpu.h,v 1.52 2007/05/18 10:15:10 tsutsui Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _HP300_CPU_H_
#define	_HP300_CPU_H_

#if defined(_KERNEL)

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/*
 * Exported definitions unique to hp300/68k cpu support.
 */

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>
#include <machine/hp300spu.h>

/*
 * Get interrupt glue.
 */
#include <machine/intr.h>

#include <sys/cpu_data.h>
struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	int	ci_mtx_count;
	int	ci_mtx_oldspl;
	int	ci_want_resched;
};

extern struct cpu_info cpu_info_store;

#define	curcpu()	(&cpu_info_store)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapin(p)			/* nothing */
#define	cpu_swapout(p)			/* nothing */
#define	cpu_number()			0

void	cpu_proc_fork(struct proc *, struct proc *);

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the hp300, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	vo;		/* vector offset (4-word frame) */
};

#define	CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->pc)
#if 0
/* We would like to do it this way... */
#define	CLKF_INTR(framep)	(((framep)->sr & PSL_M) == 0)
#else
/* but until we start using PSL_M, we have to do this instead */
#define	CLKF_INTR(framep)	(0)	/* XXX */
#endif


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci, flags)	\
	do { ci->ci_want_resched = 1; aston(); } while (/* CONSTCOND */0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)	\
	do { (l)->l_flag |= LP_OWEUPC; aston(); } while (/* CONSTCOND */0)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	aston()

extern int astpending;		/* need to trap before returning to user mode */
#define aston() (astpending++)

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_MAXID		2	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

/*
 * The rest of this should probably be moved to <machine/hp300spu.h>,
 * although some of it could probably be put into generic 68k headers.
 */

#ifdef _KERNEL
extern	char *intiobase, *intiolimit, *extiobase;
extern	void (*vectab[])(void);

struct frame;
struct fpframe;
struct pcb;

/* locore.s functions */
void	m68881_save(struct fpframe *);
void	m68881_restore(struct fpframe *);
int	suline(void *, void *);
void	savectx(struct pcb *);
void	proc_trampoline(void);
void	loadustp(int);

void	doboot(void) __attribute__((__noreturn__));
void	ecacheon(void);
void	ecacheoff(void);

/* clock.c functions */
void	hp300_calibrate_delay(void);

/* machdep.c functions */
int	badaddr(void *);
int	badbaddr(void *);

/* sys_machdep.c functions */
int	cachectl1(unsigned long, vaddr_t, size_t, struct proc *);

/* vm_machdep.c functions */
void	physaccess(void *, void *, int, int);
void	physunaccess(void *, int);
int	kvtop(void *);

/* what is this supposed to do? i.e. how is it different than startrtclock? */
#define	enablertclock()

#endif

/* physical memory sections */
#define	ROMBASE		(0x00000000)
#define	INTIOBASE	(0x00400000)
#define	INTIOTOP	(0x00600000)
#define	EXTIOBASE	(0x00600000)
#define	EXTIOTOP	(0x20000000)
#define	MAXADDR		(0xFFFFF000)

/*
 * Internal IO space:
 *
 * Ranges from 0x400000 to 0x600000 (IIOMAPSIZE).
 *
 * Internal IO space is mapped in the kernel from ``intiobase'' to
 * ``intiolimit'' (defined in locore.s).  Since it is always mapped,
 * conversion between physical and kernel virtual addresses is easy.
 */
#define	ISIIOVA(va) \
	((char *)(va) >= intiobase && (char *)(va) < intiolimit)
#define	IIOV(pa)	((int)(pa)-INTIOBASE+(int)intiobase)
#define	IIOP(va)	((int)(va)-(int)intiobase+INTIOBASE)
#define	IIOPOFF(pa)	((int)(pa)-INTIOBASE)
#define	IIOMAPSIZE	btoc(INTIOTOP-INTIOBASE)	/* 2mb */

/*
 * External IO space:
 *
 * DIO ranges from select codes 0-63 at physical addresses given by:
 *	0x600000 + (sc - 32) * 0x10000
 * DIO cards are addressed in the range 0-31 [0x600000-0x800000) for
 * their control space and the remaining areas, [0x200000-0x400000) and
 * [0x800000-0x1000000), are for additional space required by a card;
 * e.g. a display framebuffer.
 *
 * DIO-II ranges from select codes 132-255 at physical addresses given by:
 *	0x1000000 + (sc - 132) * 0x400000
 * The address range of DIO-II space is thus [0x1000000-0x20000000).
 *
 * DIO/DIO-II space is too large to map in its entirety, instead devices
 * are mapped into kernel virtual address space allocated from a range
 * of EIOMAPSIZE pages (vmparam.h) starting at ``extiobase''.
 */
#define	DIOBASE		(0x600000)
#define	DIOTOP		(0x1000000)
#define	DIOCSIZE	(0x10000)
#define	DIOIIBASE	(0x01000000)
#define	DIOIITOP	(0x20000000)
#define	DIOIICSIZE	(0x00400000)

/*
 * HP MMU
 */
#define	MMUBASE		IIOPOFF(0x5F4000)
#define	MMUSSTP		0x0
#define	MMUUSTP		0x4
#define	MMUTBINVAL	0x8
#define	MMUSTAT		0xC
#define	MMUCMD		MMUSTAT

#define	MMU_UMEN	0x0001	/* enable user mapping */
#define	MMU_SMEN	0x0002	/* enable supervisor mapping */
#define	MMU_CEN		0x0004	/* enable data cache */
#define	MMU_BERR	0x0008	/* bus error */
#define	MMU_IEN		0x0020	/* enable instruction cache */
#define	MMU_FPE		0x0040	/* enable 68881 FP coprocessor */
#define	MMU_WPF		0x2000	/* write protect fault */
#define	MMU_PF		0x4000	/* page fault */
#define	MMU_PTF		0x8000	/* page table fault */

#define	MMU_FAULT	(MMU_PTF|MMU_PF|MMU_WPF|MMU_BERR)
#define	MMU_ENAB	(MMU_UMEN|MMU_SMEN|MMU_IEN|MMU_FPE)

#if defined(CACHE_HAVE_PAC) || defined(CACHE_HAVE_VAC)
#define M68K_CACHEOPS_MACHDEP
#endif

#ifdef CACHE_HAVE_PAC
#define M68K_CACHEOPS_MACHDEP_PCIA
#endif

#ifdef CACHE_HAVE_VAC
#define M68K_CACHEOPS_MACHDEP_DCIA
#define M68K_CACHEOPS_MACHDEP_DCIS
#define M68K_CACHEOPS_MACHDEP_DCIU
#define M68K_CACHEOPS_MACHDEP_TBIA
#define M68K_CACHEOPS_MACHDEP_TBIS
#define M68K_CACHEOPS_MACHDEP_TBIAS
#define M68K_CACHEOPS_MACHDEP_TBIAU
#endif

#endif /* _HP300_CPU_H_ */
