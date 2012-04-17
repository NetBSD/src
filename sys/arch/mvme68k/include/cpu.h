/*	$NetBSD: cpu.h,v 1.46.4.1 2012/04/17 00:06:41 yamt Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#include "opt_m68k_arch.h"
#endif

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>

#if defined(_KERNEL)
/*
 * Exported definitions unique to mvme68k/68k cpu support.
 */
#define	M68K_MMU_MOTOROLA

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the mvme68k, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	fmt:4,
		vec:12;		/* vector offset (4-word frame) */
} __attribute__((packed));

#define	CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->pc)

/*
 * The clock interrupt handler can determine if it's a nested
 * interrupt by checking for interrupt_depth > 1.
 * (Remember, the clock interrupt handler itself will cause the
 * depth counter to be incremented).
 */
extern volatile unsigned int interrupt_depth;
#define	CLKF_INTR(framep)	(interrupt_depth > 1)


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci, flags)	\
	do { ci->ci_want_resched++; aston(); } while (/* CONSTCOND */0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)	\
	do { (l)->l_pflag |= LP_OWEUPC; aston(); } while (/* CONSTCOND */0)

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

#ifdef _KERNEL
/*
 * Associate MVME models with CPU types.
 */
#define	MVME68K		1	

/*
 * MVME-147; 68030 CPU
 */
#if defined(MVME147) && !defined(M68030)
#define M68030
#endif

/*
 * MVME-162/166/167; 68040 CPU
 */
#if (defined(MVME162) || defined(MVME167)) && !defined(M68040)
#define M68040
#endif

/*
 * MVME-172/177; 68060 CPU
 */
#if (defined(MVME172) || defined(MVME177)) && !defined(M68060)
#define M68060
#endif
#endif /* _KERNEL */

/*
 * Values for machineid; these match the Bug's values.
 */
#define	MVME_147	0x147
#define	MVME_162	0x162
#define	MVME_166	0x166
#define	MVME_167	0x167
#define	MVME_172	0x172
#define	MVME_177	0x177

#ifdef _KERNEL
extern	int machineid;
extern	int cpuspeed;
extern	char *intiobase, *intiolimit;
extern	u_int intiobase_phys, intiotop_phys;
extern	u_long ether_data_buff_size;
extern	u_char mvme_ea[6];

void	doboot(int) 
	__attribute__((__noreturn__));
int	nmihand(void *);
void	mvme68k_abort(const char *);
void	*iomap(u_long, size_t);
void	iounmap(void *, size_t);
void	loadustp(paddr_t);

/* physical memory addresses where mvme147's onboard devices live */
#define	INTIOBASE147	(0xfffe0000u)
#define	INTIOTOP147	(0xfffe5000u)

/* ditto for mvme1[67][27] */
#define	INTIOBASE1xx	(0xfff40000u)
#define	INTIOTOP1xx	(0xfffd0000u)

#endif /* _KERNEL */

#endif /* _MACHINE_CPU_H_ */
