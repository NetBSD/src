/*	$NetBSD: cpu.h,v 1.37.4.1 2001/10/01 12:38:11 fvdl Exp $	*/

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
 *
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	7.7 (Berkeley) 6/27/91
 */

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

/*
 * Exported definitions unique to atari/68k cpu support.
 */

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>
#define	M68K_MMU_MOTOROLA

#include <sys/sched.h>
struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};

#ifdef _KERNEL
extern struct cpu_info cpu_info_store;

#define	curcpu()	(&cpu_info_store)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapin(p)			/* nothing */
#define	cpu_wait(p)			/* nothing */
#define cpu_swapout(p)			/* nothing */
#define	cpu_number()			0

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the hp300, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_int	cf_regs[4];	/* d0,d1,a0,a1 (see locore.s)	*/
	u_short	cf_sr;		/* sr at time of interrupt	*/
	u_long	cf_pc;		/* pc at time of interrupt	*/
	u_short	cf_vo;		/* vector offset (4-word frame)	*/
} __attribute__((packed));

#define	CLKF_USERMODE(framep)	(((framep)->cf_sr & PSL_S) == 0)
#define	CLKF_BASEPRI(framep)	(((framep)->cf_sr & PSL_IPL) == 0)
#define	CLKF_PC(framep)		((framep)->cf_pc)
#if 0
/* We would like to do it this way... */
#define	CLKF_INTR(framep)	(((framep)->cf_sr & PSL_M) == 0)
#else
/* but until we start using PSL_M, we have to do this instead */
#define	CLKF_INTR(framep)	(0)	/* XXX */
#endif


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched(ci)	{want_resched = 1; setsoftast();}

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On hp300, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(p, framep)	((p)->p_flag |= P_OWEUPC, setsoftast())
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, setsoftast())

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	setsoftast()

#define setsoftast()	(astpending = 1)

extern int	astpending;	/* need trap before returning to user mode */
extern int	want_resched;	/* resched() was called */

#endif /* _KERNEL */

/* include support for software interrupts */
#include <machine/mtpr.h>

/*
 * The rest of this should probably be moved to ../atari/ataricpu.h,
 * although some of it could probably be put into generic 68k headers.
 */
#define	BASEPRI(sr)	((sr & PSL_IPL) == 0)

/*
 * Values for machineid.
 */
#define	ATARI_68000	1		/* 68000 CPU			*/
#define	ATARI_68010	(1<<1)		/* 68010 CPU			*/
#define ATARI_68020	(1L<<2)		/* 68020 CPU			*/
#define ATARI_68030	(1L<<3)		/* 68030 CPU			*/
#define ATARI_68040	(1L<<4)		/* 68040 CPU			*/
#define ATARI_68060	(1L<<6)		/* 68060 CPU			*/
#define	ATARI_TT	(1L<<11)	/* This is a TT030		*/
#define	ATARI_FALCON	(1L<<12)	/*           Falcon		*/
#define	ATARI_HADES	(1L<<13)	/*           Hades		*/
#define	ATARI_MILAN	(1L<<14)	/*           Milan		*/

#define	ATARI_CLKBROKEN	(1L<<16)

#define	ATARI_ANYCPU	(ATARI_68000|ATARI_68010|ATARI_68020|ATARI_68030 \
			|ATARI_68040|ATARI_68060)

#define	ATARI_ANYMACH	(ATARI_TT|ATARI_FALCON|ATARI_HADES|ATARI_MILAN)

#ifdef _KERNEL
extern int machineid;
#endif

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV	1	/* dev_t: console terminal device */
#define CPU_MAXID	2	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

#ifdef _KERNEL
/*
 * Prototypes from atari_init.c
 */
int	cpu_dump __P((int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *));
int	cpu_dumpsize __P((void));

/*
 * Prototypes from autoconf.c
 */
void	config_console __P((void));

/*
 * Prototypes from clock.c
 */
long	clkread __P((void));

/*
 * Prototypes from disksubr.c
 */
struct buf;
struct disklabel;
int	bounds_check_with_label __P((struct buf *, struct disklabel *, int));

/*
 * Prototypes from fpu.c
 */
char	*fpu_describe __P((int));
int	fpu_probe __P((void));

/*
 * Prototypes from vm_machdep.c
 */
int	badbaddr __P((caddr_t, int));
void	consinit __P((void));
void	dumpconf __P((void));
paddr_t	kvtop __P((caddr_t));
void	physaccess __P((caddr_t, caddr_t, int, int));
void	physunaccess __P((caddr_t, int));
void	setredzone __P((u_int *, caddr_t));

/*
 * Prototypes from locore.s
 */
struct fpframe;
struct user;
struct pcb;

void	clearseg __P((paddr_t));
void	doboot __P((void));
void	loadustp __P((int));
void	m68881_save __P((struct fpframe *));
void	m68881_restore __P((struct fpframe *));
void	physcopyseg __P((paddr_t, paddr_t));
u_int	probeva __P((u_int, u_int));
void	proc_trampoline __P((void));
void	savectx __P((struct pcb *));
int	suline __P((caddr_t, caddr_t));
void	switch_exit __P((struct proc *));
void	DCIAS __P((vaddr_t));
void	DCIA __P((void));
void	DCIS __P((void));
void	DCIU __P((void));
void	ICIA __P((void));
void	ICPA __P((void));
void	PCIA __P((void));
void	TBIA __P((void));
void	TBIS __P((vaddr_t));
void	TBIAS __P((void));
void	TBIAU __P((void));

#if defined(M68040) || defined(M68060)
void	DCFA __P((void));
void	DCFP __P((paddr_t));
void	DCFL __P((paddr_t));
void	DCPL __P((paddr_t));
void	DCPP __P((paddr_t));
void	ICPL __P((paddr_t));
void	ICPP __P((paddr_t));
#endif

/*
 * Prototypes from machdep.c:
 */
typedef void (*si_farg)(void *, void *);	/* XXX */
void	add_sicallback __P((si_farg, void *, void *));
void	rem_sicallback __P((si_farg));
void	cpu_startup __P((void));
void	dumpsys __P((void));
vaddr_t reserve_dumppages __P((vaddr_t));
void	softint __P((void));


/*
 * Prototypes from nvram.c:
 */
struct uio;
int	nvram_uio __P((struct uio *));

/*
 * Prototypes from sys_machdep.c:
 */
int	cachectl1 __P((unsigned long, vaddr_t, size_t, struct proc *));
int	dma_cachectl __P((caddr_t, int));

/*
 * Prototypes from pci_machdep.c
 */
void init_pci_bus __P((void));

#endif /* _KERNEL */
#endif /* !_MACHINE_CPU_H_ */
