/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden
 * All rights reserved.
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
 *      This product includes software developed at Ludd, University of Lule}
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: cpu.h,v 1.1 1994/08/02 20:20:13 ragge Exp $
 */
 /* All bugs are subject to removal without further notice */

#include "cdefs.h"
#include "vax/include/mtpr.h"

#define enablertclock()

#define setsoftclock()	mtpr(8,PR_SIRR)

struct clockframe {
        int     pc;
        int     ps;
};

struct cpuops {
	int *nisse;
	int (*cpu_memenable)();
	int (*cpu_memerr)();
	int (*cpu_mchk)();
	int (*cpu_init)();
};

struct percpu {
	int pc_cputype;
	int pc_cpuspeed;
	int pc_nioa;
	struct iobus *pc_io;
	struct cpuops *pc_ops;
};

struct clockops {
	int (*p1)();
	int (*p2)();
	int (*p3)();
};

/*
 * Return timeofdayregister
 */

#define todr()		mfpr(PR_TODR)
/*
 * The following code are from the hp300 port/ragge.
 *      from: Utah Hdr: cpu.h 1.16 91/03/25
 *      from: @(#)cpu.h 7.7 (Berkeley) 6/27/91
 *      $Id: cpu.h,v 1.1 1994/08/02 20:20:13 ragge Exp $
 */
/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */

#define need_resched(){ \
	want_resched++; \
	mtpr(AST_OK,PR_ASTLVL); \
	}

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On hp300, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
/* #define profile_tick(p, framep) { (p)->p_flag |= SOWEUPC; aston(); } */

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */

#define signotify(p)     mtpr(AST_OK,PR_ASTLVL);

/* extern	int     astpending;     /* need to trap before returning to user mode */
extern	int     want_resched;   /* resched() was called */

