/*	$NetBSD: cpu.h,v 1.8.4.1 1999/02/13 16:54:27 scw Exp $	*/

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

/*
 * Exported definitions unique to mvme68k/68k cpu support.
 */

/*
 * Get common m68k CPU definitions.
 */
#include <m68k/cpu.h>
#define	M68K_MMU_MOTOROLA

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapin(p)			/* nothing */
#define	cpu_wait(p)			/* nothing */
#define cpu_swapout(p)			/* nothing */

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the mvme68k, we use
 * what the hardware pushes on an interrupt (frame format 0).
 */
struct clockframe {
	u_short	sr;		/* sr at time of interrupt */
	u_long	pc;		/* pc at time of interrupt */
	u_short	vo;		/* vector offset (4-word frame) */
};

#define	CLKF_USERMODE(framep)	(((framep)->sr & PSL_S) == 0)
#define	CLKF_BASEPRI(framep)	(((framep)->sr & PSL_IPL) == 0)
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
extern int want_resched;	/* resched() was called */
#define	need_resched()	{ want_resched++; aston(); }

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	{ (p)->p_flag |= P_OWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

extern int astpending;		/* need to trap before returning to user mode */
#define aston() (astpending++)

/*
 * simulated software interrupt register
 */
extern unsigned char ssir;

#define SIR_NET		0x1
#define SIR_CLOCK	0x2

#define setsoftint(x)	ssir |= (x)
#define siroff(x)	ssir &= ~(x)
#define setsoftnet()	ssir |= SIR_NET
#define setsoftclock()	ssir |= SIR_CLOCK

extern unsigned long allocate_sir();

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_MAXID		2	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

#ifdef _KERNEL
/*
 * Associate MVME models with CPU types.
 */

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
 * MVME-177 (what about 172?); 68060 CPU
 */
#if defined(MVME177) && !defined(M68060)
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
extern	char *intiobase, *intiolimit;
extern	u_int intiobase_phys, intiotop_phys;

struct frame;
void	doboot __P((int)) 
	__attribute__((__noreturn__));
int	badaddr __P((caddr_t, int));
void	nmihand __P((struct frame *));
void	mvme68k_abort __P((const char *));
void	physaccess __P((caddr_t, caddr_t, int, int));
void	physunaccess __P((caddr_t, int));
void	*iomap __P((u_long, size_t));
void	iounmap __P((void *, size_t));
void	child_return __P((void *));
#endif

/* physical memory sections for mvme147 */
#define	INTIOBASE147	(0xfffe0000u)
#define	INTIOTOP147	(0xfffe5000u)

/* ditto for mvme1[67]7 */
#define	INTIOBASE167	(0xfff00000u)
#define	INTIOTOP167	(0xfffd0000u)

/*
 * Internal IO space:
 *
 * Internal IO space is mapped in the kernel from ``intiobase'' to
 * ``intiolimit'' (defined in locore.s).  Since it is always mapped,
 * conversion between physical and kernel virtual addresses is easy.
 */
#define	ISIIOVA(va) \
	((char *)(va) >= intiobase && (char *)(va) < intiolimit)
#define	IIOV(pa)	(((u_int)(pa) - intiobase_phys) + (u_int)intiobase)
#define	IIOP(va)	(((u_int)(va) - (u_int)intiobase) + intiobase_phys)
#define	IIOPOFF(pa)	((u_int)(pa) - intiobase_phys)
