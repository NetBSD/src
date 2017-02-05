/*	$NetBSD: cpu.h,v 1.80.6.1 2017/02/05 13:40:02 skrll Exp $	*/

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
 *	@(#)cpu.h	7.7 (Berkeley) 6/27/91
 */

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>

#if defined(_KERNEL)
/*
 * Exported definitions unique to amiga/68k cpu support.
 */
#define	M68K_MMU_MOTOROLA

extern volatile unsigned int interrupt_depth;
/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the amiga, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	vo;		/* vector offset (4-word frame) */
};

#define	CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->pc)
#define	CLKF_INTR(framep)	(interrupt_depth > 1)


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci,flags)	do {	\
	__USE(flags); 				\
	ci->ci_want_resched = 1;		\
	setsoftast();				\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On the amiga, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(l, framep)	((l)->l_pflag |= LP_OWEUPC, setsoftast())
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, setsoftast())

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	setsoftast()

extern int astpending;		/* need trap before returning to user mode */
#define setsoftast()		(astpending = 1)

/*
 * The rest of this should probably be moved to ../amiga/amigacpu.h,
 * although some of it could probably be put into generic 68k headers.
 */

/* values for machineid (happen to be AFF_* settings of AttnFlags) */
#define AMIGA_68020	(1L<<1)
#define AMIGA_68030	(1L<<2)
#define AMIGA_68040	(1L<<3)
#define AMIGA_68881	(1L<<4)
#define AMIGA_68882	(1L<<5)
#define	AMIGA_FPU40	(1L<<6)
#define AMIGA_68060	(1L<<7)

extern int machineid;

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV	1	/* dev_t: console terminal device */
#define CPU_MAXID	2	/* number of valid machdep ids */

#ifdef _KERNEL
/*
 * Prototypes from amiga_init.c
 */
void	*alloc_z2mem(long);

/*
 * Prototypes from autoconf.c
 */
int	is_a600(void);
int	is_a1200(void);
int	is_a3000(void);
int	is_a4000(void);
#ifdef DRACO
#define	is_draco() ((machineid >> 24) == 0x7d ? (machineid >> 16) & 0xff : 0)
#endif

#ifdef DRACO
/*
 * Prototypes from kbd.c
 */
void	drkbdintr(void);

/*
 * Prototypes from drsc.c
 */
void	drsc_handler(void);
#endif

/*
 * Prototypes from locore.s
 */
void	clearseg(vm_offset_t);
void	doboot(void) __attribute__((__noreturn__));
void	loadustp(int);
void	physcopyseg(vm_offset_t, vm_offset_t);
u_int	probeva(u_int, u_int);

/*
 * Prototypes from machdep.c
 */
int	badaddr(void *);
int	badbaddr(void *);
void	bootsync(void);

/*
 * Prototypes from pmap.c:
 */
void	pmap_bootstrap(vm_offset_t, vm_offset_t);

#endif /* _KERNEL */

/*
 * Reorder protection when accessing device registers.
 */
#define amiga_membarrier()

/*
 * Finish all bus operations and flush pipelines.
 */
#define amiga_cpu_sync() __asm volatile ("nop")

#endif /* !_MACHINE_CPU_H_ */
