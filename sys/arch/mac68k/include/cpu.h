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
 */

/*
 *	Copyright (c) 1992, 1993 BCDL Labs.  All rights reserved.
 *	Allen Briggs, Chris Caputo, Michael Finch, Brad Grantham, Lawrence Kesteloot

 *	Redistribution of this source code or any part thereof is permitted,
 *	 provided that the following conditions are met:
 *	1) Utilized source contains the copyright message above, this list
 *	 of conditions, and the following disclaimer.
 *	2) Binary objects containing compiled source reproduce the
 *	 copyright notice above on startup.
 *
 *	CAVEAT: This source code is provided "as-is" by BCDL Labs, and any
 *	 warranties of ANY kind are disclaimed.  We don't even claim that it
 *	 won't crash your hard disk.  Basically, we want a little credit if
 *	 it works, but we don't want to get mail-bombed if it doesn't. 
 */

/*
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	from: @(#)cpu.h	7.7 (Berkeley) 6/27/91
 *	$Id: cpu.h,v 1.3 1993/12/15 03:22:56 briggs Exp $
 */

/*
   ALICE
	BG -- Sat May 23 23:58:23 EDT 1992
	Exported defines and stuff unique to mac68k.
   A lot of this stuff is really specific to the m68k, not just the macs,
   but there isn't time to do anything about that right now...
 */

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */

/*
 * function vs. inline configuration;
 * these are defined to get generic functions
 * rather than inline or machine-dependent implementations
 */
#define	NEED_MINMAX		/* need {,i,l,ul}{min,max} functions */
#undef	NEED_FFS		/* don't need ffs function */
#undef	NEED_BCMP		/* don't need bcmp function */
#undef	NEED_STRLEN		/* don't need strlen function */

/* ALICE BG -- Sun May 24 11:31:35 EDT 1992 -- what the hell do these things */
/*  do? */
#define	cpu_exec(p)	/* nothing */
#define	cpu_wait(p)	/* nothing */

/*
 * Arguments to hardclock, softclock and gatherstats
 * encapsulate the previous machine state in an opaque
 * clockframe; for hp300, use just what the hardware
 * leaves on the stack.
 */
/* ALICE 05/23/92 BG -- Oh, no.  What does a VIA intleave on the stack? */
/* ALICE 06/27/92 LK -- Make sure hardware clock routine does this: */

typedef struct intrframe {
	int	pc;
	int	ps;
} clockframe;

#define	CLKF_USERMODE(framep)	(((framep)->ps & PSL_S) == 0)
#define	CLKF_BASEPRI(framep)	(((framep)->ps & PSL_IPL7) == 0)
#define	CLKF_PC(framep)		((framep)->pc)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched()	{ want_resched++; aston(); }

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  Request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(p, framep)	{ (p)->p_flag |= SOWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

#define aston() (astpending++)

int	astpending;		/* need to trap before returning to user mode */
int	want_resched;		/* resched() was called */

/* XXX why is this duplicated in mtpr.h? brad@fcr.com */
/*
 * simulated software interrupt register
 */
extern unsigned char ssir;

#define SIR_NET		0x1
#define SIR_CLOCK	0x2
#define SIR_SERIAL	0x4

#define siroff(x)	ssir &= ~(x)
#define setsoftnet()	ssir |= SIR_NET
#define setsoftclock()	ssir |= SIR_CLOCK
#define setsoftserial()	ssir |= SIR_SERIAL


/* values for machineid */
/* BARF MF - some values from the thinkc gesalt include file */
#define MACH_MAC2		6
#define MACH_MAC2X		7
#define MACH_MAC2SI		18
#define MACH_MAC2CI		11
#define MACH_MAC2CX		8
#define MACH_MACSE30		9
#define MACH_MAC2FX		13

/* MF processor passed in */
#define MACH_68020	0
#define MACH_68030	1
#define MACH_68040	2
#define MACH_PENTIUM	3	/* 66 and 99 MHz versions *only* */

/* values for cpuspeed (not really related to clock speed due to caches) */
#define	MHZ_8		1
#define	MHZ_16		2
#define	MHZ_25		3
#define	MHZ_33		4
#define	MHZ_40		5

#ifdef KERNEL
extern	int machineid, ectype;
extern	char *intiobase, *intiolimit;
extern	char *extiobase, *extiolimit;

#endif

/* physical memory sections */
#define	ROMBASE		(0x40000000)
#define INTIOBASE	(0x50000000)
#define INTIOTOP	(0x51000000)	/* ~ 128 K */
#define IIOMAPSIZE	btoc(INTIOTOP - INTIOBASE)

/* ALICE 05/23/92 BG -- These need to be changed. */
#ifdef NO_SUPER_SPACE_YET
#define	NBSBASE		0x60000000	/* NUBUS Super space */
#define	NBSTOP		0xF0000000
#endif
#define NBBASE		0xF9000000	/* NUBUS space */
#define NBTOP		0xFF000000	/* NUBUS space */
#define NBMAPSIZE	btoc(NBTOP-NBBASE)	/* ~ 96 megs */
#define NBMEMSIZE	0x01000000	/* 16 megs per card */
#define NBROMOFFSET	0x00FF0000	/* Last 64K == ROM */

/*
 * IO space:
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

/*
   ALICE 05/24/92,13:25:19 BG -- We need to make sure to map NuBus memory in
    the kernel, too.
   ALICE 06/29/92,20:40:00 LK -- I did that, thank you very much.  Been there.
 */


/*
 * 68851 and 68030 MMU
 */
#define	PMMU_LVLMASK	0x0007
#define	PMMU_INV	0x0400
#define	PMMU_WP		0x0800
#define	PMMU_ALV	0x1000
#define	PMMU_SO		0x2000
#define	PMMU_LV		0x4000
#define	PMMU_BE		0x8000
#define	PMMU_FAULT	(PMMU_WP|PMMU_INV)

/* 680X0 function codes */
#define	FC_USERD	1	/* user data space */
#define	FC_USERP	2	/* user program space */
#define	FC_SUPERD	5	/* supervisor data space */
#define	FC_SUPERP	6	/* supervisor program space */
#define	FC_CPU		7	/* CPU space */

/* fields in the 68020 cache control register */
#define	IC_ENABLE	0x0001	/* enable instruction cache */
#define	IC_FREEZE	0x0002	/* freeze instruction cache */
#define	IC_CE		0x0004	/* clear instruction cache entry */
#define	IC_CLR		0x0008	/* clear entire instruction cache */

/* additional fields in the 68030 cache control register */
#define	IC_BE		0x0010	/* instruction burst enable */
#define	DC_ENABLE	0x0100	/* data cache enable */
#define	DC_FREEZE	0x0200	/* data cache freeze */
#define	DC_CE		0x0400	/* clear data cache entry */
#define	DC_CLR		0x0800	/* clear entire data cache */
#define	DC_BE		0x1000	/* data burst enable */
#define	DC_WA		0x2000	/* write allocate */

#define	CACHE_ON	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	CACHE_OFF	(DC_CLR|IC_CLR)
#define	CACHE_CLR	(CACHE_ON)
#define	IC_CLEAR	(DC_WA|DC_BE|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	DC_CLEAR	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_ENABLE)
