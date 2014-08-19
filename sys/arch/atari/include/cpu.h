/*	$NetBSD: cpu.h,v 1.69.6.1 2014/08/20 00:02:48 tls Exp $	*/

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
 * Exported definitions unique to atari/68k cpu support.
 */
#define	M68K_MMU_MOTOROLA

void	cpu_proc_fork(struct proc *, struct proc *);

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
#define	cpu_need_resched(ci,flags)	do {	\
	__USE(flags); 				\
	ci->ci_want_resched = 1;		\
	setsoftast();				\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On hp300, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(l, framep)	((l)->l_pflag |= LP_OWEUPC, setsoftast())
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, setsoftast())

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	setsoftast()

#define setsoftast()	(astpending = 1)

extern int	astpending;	/* need trap before returning to user mode */

/*
 * The rest of this should probably be moved to ../atari/ataricpu.h,
 * although some of it could probably be put into generic 68k headers.
 */
#define	BASEPRI(sr)	((sr & PSL_IPL) == 0)
#endif /* _KERNEL */

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

#if defined(_KERNEL)
extern int machineid;
#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV	1	/* dev_t: console terminal device */
#define CPU_MAXID	2	/* number of valid machdep ids */

#ifdef _KERNEL
/*
 * Prototypes from atari_init.c
 */
int	cpu_dump(int (*)(dev_t, daddr_t, void *, size_t), daddr_t *);
int	cpu_dumpsize(void);

/*
 * Prototypes from autoconf.c
 */
void	config_console(void);

/*
 * Prototypes from fpu.c
 */
const char *fpu_describe(int);

/*
 * Prototypes from locore.s
 */
void	clearseg(paddr_t);
void	doboot(void);
void	loadustp(int);
void	physcopyseg(paddr_t, paddr_t);
u_int	probeva(u_int, u_int);
int	suline(void *, void *);

/*
 * Prototypes from machdep.c:
 */
int	badbaddr(void *, int);
void	consinit(void);
typedef void (*si_farg)(void *, void *);	/* XXX */
void	init_sicallback(void);			/* XXX */
void	add_sicallback(si_farg, void *, void *);
void	rem_sicallback(si_farg);
void	dumpsys(void);
vaddr_t reserve_dumppages(vaddr_t);


/*
 * Prototypes from nvram.c:
 */
struct uio;
int	nvram_uio(struct uio *);

/*
 * Prototypes from pci_machdep.c
 */
void init_pci_bus(void);

#endif /* _KERNEL */
#endif /* !_MACHINE_CPU_H_ */
