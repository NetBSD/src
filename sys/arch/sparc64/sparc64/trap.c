/*	$NetBSD: trap.c,v 1.147.4.2 2009/06/20 07:20:12 yamt Exp $ */

/*
 * Copyright (c) 1996-2002 Eduardo Horvath.  All rights reserved.
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
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
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by Harvard University.
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
 *	@(#)trap.c	8.4 (Berkeley) 9/23/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.147.4.2 2009/06/20 07:20:12 yamt Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_compat_svr4.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/kernel.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/trap.h>
#include <machine/instr.h>
#include <machine/pmap.h>
#include <machine/userret.h>

#ifdef DDB
#include <machine/db_machdep.h>
#else
#include <machine/frame.h>
#endif
#ifdef COMPAT_SVR4
#include <machine/svr4_machdep.h>
#endif
#ifdef COMPAT_SVR4_32
#include <machine/svr4_32_machdep.h>
#endif

#include <sparc/fpu/fpu_extern.h>

#ifndef offsetof
#define	offsetof(s, f) ((size_t)&((s *)0)->f)
#endif

#ifdef TRAPSTATS
/* trapstats */
int protfix = 0;
int udmiss = 0;	/* Number of normal/nucleus data/text miss/protection faults */
int udhit = 0;	
int udprot = 0;
int utmiss = 0;
int kdmiss = 0;
int kdhit = 0;	
int kdprot = 0;
int ktmiss = 0;
int iveccnt = 0; /* number if normal/nucleus interrupt/interrupt vector faults */
int uintrcnt = 0;
int kiveccnt = 0;
int kintrcnt = 0;
int intristk = 0; /* interrupts when already on intrstack */
int intrpoll = 0; /* interrupts not using vector lists */
int wfill = 0;
int kwfill = 0;
int wspill = 0;
int wspillskip = 0;
int rftucnt = 0;
int rftuld = 0;
int rftudone = 0;
int rftkcnt[5] = { 0, 0, 0, 0, 0 };
#endif

#ifdef DEBUG
#define RW_64		0x1
#define RW_ERR		0x2
#define RW_FOLLOW	0x4
int	rwindow_debug = RW_ERR;
#define TDB_ADDFLT	0x1
#define TDB_TXTFLT	0x2
#define TDB_TRAP	0x4
#define TDB_SYSCALL	0x8
#define TDB_FOLLOW	0x10
#define TDB_FRAME	0x20
#define TDB_NSAVED	0x40
#define TDB_TL		0x80
#define TDB_STOPSIG	0x100
#define TDB_STOPCALL	0x200
#define TDB_STOPCPIO	0x400
#define TDB_SYSTOP	0x800
int	trapdebug = 0/*|TDB_SYSCALL|TDB_STOPSIG|TDB_STOPCPIO|TDB_ADDFLT|TDB_FOLLOW*/;
/* #define inline */
#endif

#ifdef DDB
#if 1
#define DEBUGGER(t,f)	do { kdb_trap(t,f); } while (0)
#else
#define DEBUGGER(t,f)	Debugger()
#endif
#else
#define DEBUGGER(t,f)
#define Debugger()
#endif

/*
 * Initial FPU state is all registers == all 1s, everything else == all 0s.
 * This makes every floating point register a signalling NaN, with sign bit
 * set, no matter how it is interpreted.  Appendix N of the Sparc V8 document
 * seems to imply that we should do this, and it does make sense.
 */
const struct fpstate64 initfpstate __aligned(BLOCK_SIZE) = {
	.fs_regs =
	{ ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
	  ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
	  ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
	  ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0 },
	.fs_qsize = 0
};

/*
 * There are more than 100 trap types, but most are unused.
 *
 * Trap type 0 is taken over as an `Asynchronous System Trap'.
 * This is left-over Vax emulation crap that should be fixed.
 *
 * Traps not supported on the spitfire are marked with `*',
 * and additions are marked with `+'
 */
static const char T[] = "*trap";
const char *trap_type[] = {
	/* non-user vectors */
	"ast",			/* 0 */
	"power on reset",	/* 1 */
	"watchdog reset",	/* 2 */
	"externally initiated reset",/*3 */
	"software initiated reset",/* 4 */
	"RED state exception",	/* 5 */
	T, T,			/* 6..7 */
	"instruction access exception",	/* 8 */
	"*instruction MMU miss",/* 9 */
	"instruction access error",/* 0a */
	T, T, T, T, T,		/* 0b..0f */
	"illegal instruction",	/* 10 */
	"privileged opcode",	/* 11 */
	"*unimplemented LDD",	/* 12 */
	"*unimplemented STD",	/* 13 */
	T, T, T, T,		/* 14..17 */
	T, T, T, T, T, T, T, T, /* 18..1f */
	"fp disabled",		/* 20 */
	"fp exception ieee 754",/* 21 */
	"fp exception other",	/* 22 */
	"tag overflow",		/* 23 */
	"clean window",		/* 24 */
	T, T, T,		/* 25..27 -- trap continues */
	"division by zero",	/* 28 */
	"*internal processor error",/* 29 */
	T, T, T, T, T, T,	/* 2a..2f */
	"data access exception",/* 30 */
	"*data access MMU miss",/* 31 */
	"data access error",	/* 32 */
	"*data access protection",/* 33 */
	"mem address not aligned",	/* 34 */
	"LDDF mem address not aligned",/* 35 */
	"STDF mem address not aligned",/* 36 */
	"privileged action",	/* 37 */
	"LDQF mem address not aligned",/* 38 */
	"STQF mem address not aligned",/* 39 */
	T, T, T, T, T, T,	/* 3a..3f */
	"*async data error",	/* 40 */
	"level 1 int",		/* 41 */
	"level 2 int",		/* 42 */
	"level 3 int",		/* 43 */
	"level 4 int",		/* 44 */
	"level 5 int",		/* 45 */
	"level 6 int",		/* 46 */
	"level 7 int",		/* 47 */
	"level 8 int",		/* 48 */
	"level 9 int",		/* 49 */
	"level 10 int",		/* 4a */
	"level 11 int",		/* 4b */
	"level 12 int",		/* 4c */
	"level 13 int",		/* 4d */
	"level 14 int",		/* 4e */
	"level 15 int",		/* 4f */
	T, T, T, T, T, T, T, T, /* 50..57 */
	T, T, T, T, T, T, T, T, /* 58..5f */
	"+interrupt vector",	/* 60 */
	"+PA_watchpoint",	/* 61 */
	"+VA_watchpoint",	/* 62 */
	"+corrected ECC error",	/* 63 */
	"+fast instruction access MMU miss",/* 64 */
	T, T, T,		/* 65..67 -- trap continues */
	"+fast data access MMU miss",/* 68 */
	T, T, T,		/* 69..6b -- trap continues */
	"+fast data access protection",/* 6c */
	T, T, T,		/* 6d..6f -- trap continues */
	T, T, T, T, T, T, T, T, /* 70..77 */
	T, T, T, T, T, T, T, T, /* 78..7f */
	"spill 0 normal",	/* 80 */
	T, T, T,		/* 81..83 -- trap continues */
	"spill 1 normal",	/* 84 */
	T, T, T,		/* 85..87 -- trap continues */
	"spill 2 normal",	/* 88 */
	T, T, T,		/* 89..8b -- trap continues */
	"spill 3 normal",	/* 8c */
	T, T, T,		/* 8d..8f -- trap continues */
	"spill 4 normal",	/* 90 */
	T, T, T,		/* 91..93 -- trap continues */
	"spill 5 normal",	/* 94 */
	T, T, T,		/* 95..97 -- trap continues */
	"spill 6 normal",	/* 98 */
	T, T, T,		/* 99..9b -- trap continues */
	"spill 7 normal",	/* 9c */
	T, T, T,		/* 9c..9f -- trap continues */
	"spill 0 other",	/* a0 */
	T, T, T,		/* a1..a3 -- trap continues */
	"spill 1 other",	/* a4 */
	T, T, T,		/* a5..a7 -- trap continues */
	"spill 2 other",	/* a8 */
	T, T, T,		/* a9..ab -- trap continues */
	"spill 3 other",	/* ac */
	T, T, T,		/* ad..af -- trap continues */
	"spill 4 other",	/* b0 */
	T, T, T,		/* b1..b3 -- trap continues */
	"spill 5 other",	/* b4 */
	T, T, T,		/* b5..b7 -- trap continues */
	"spill 6 other",	/* b8 */
	T, T, T,		/* b9..bb -- trap continues */
	"spill 7 other",	/* bc */
	T, T, T,		/* bc..bf -- trap continues */
	"fill 0 normal",	/* c0 */
	T, T, T,		/* c1..c3 -- trap continues */
	"fill 1 normal",	/* c4 */
	T, T, T,		/* c5..c7 -- trap continues */
	"fill 2 normal",	/* c8 */
	T, T, T,		/* c9..cb -- trap continues */
	"fill 3 normal",	/* cc */
	T, T, T,		/* cd..cf -- trap continues */
	"fill 4 normal",	/* d0 */
	T, T, T,		/* d1..d3 -- trap continues */
	"fill 5 normal",	/* d4 */
	T, T, T,		/* d5..d7 -- trap continues */
	"fill 6 normal",	/* d8 */
	T, T, T,		/* d9..db -- trap continues */
	"fill 7 normal",	/* dc */
	T, T, T,		/* dc..df -- trap continues */
	"fill 0 other",		/* e0 */
	T, T, T,		/* e1..e3 -- trap continues */
	"fill 1 other",		/* e4 */
	T, T, T,		/* e5..e7 -- trap continues */
	"fill 2 other",		/* e8 */
	T, T, T,		/* e9..eb -- trap continues */
	"fill 3 other",		/* ec */
	T, T, T,		/* ed..ef -- trap continues */
	"fill 4 other",		/* f0 */
	T, T, T,		/* f1..f3 -- trap continues */
	"fill 5 other",		/* f4 */
	T, T, T,		/* f5..f7 -- trap continues */
	"fill 6 other",		/* f8 */
	T, T, T,		/* f9..fb -- trap continues */
	"fill 7 other",		/* fc */
	T, T, T,		/* fc..ff -- trap continues */

	/* user (software trap) vectors */
	"syscall",		/* 100 */
	"breakpoint",		/* 101 */
	"zero divide",		/* 102 */
	"flush windows",	/* 103 */
	"clean windows",	/* 104 */
	"range check",		/* 105 */
	"fix align",		/* 106 */
	"integer overflow",	/* 107 */
	"svr4 syscall",		/* 108 */
	"4.4 syscall",		/* 109 */
	"kgdb exec",		/* 10a */
	T, T, T, T, T,		/* 10b..10f */
	T, T, T, T, T, T, T, T,	/* 11a..117 */
	T, T, T, T, T, T, T, T,	/* 118..11f */
	"svr4 getcc",		/* 120 */
	"svr4 setcc",		/* 121 */
	"svr4 getpsr",		/* 122 */
	"svr4 setpsr",		/* 123 */
	"svr4 gethrtime",	/* 124 */
	"svr4 gethrvtime",	/* 125 */
	T,			/* 126 */
	"svr4 gethrestime",	/* 127 */
	T, T, T, T, T, T, T, T, /* 128..12f */
	T, T,			/* 130..131 */
	"get condition codes",	/* 132 */
	"set condision codes",	/* 133 */
	T, T, T, T,		/* 134..137 */
	T, T, T, T, T, T, T, T, /* 138..13f */
	T, T, T, T, T, T, T, T, /* 140..147 */
	T, T, T, T, T, T, T, T, /* 148..14f */
	T, T, T, T, T, T, T, T, /* 150..157 */
	T, T, T, T, T, T, T, T, /* 158..15f */
	T, T, T, T,		/* 160..163 */
	"SVID syscall64",	/* 164 */
	"SPARC Intl syscall64",	/* 165 */
	"OS vedor spec syscall",/* 166 */
	"HW OEM syscall",	/* 167 */
	"ret from deferred trap",	/* 168 */
};

#define	N_TRAP_TYPES	(sizeof trap_type / sizeof *trap_type)

void trap(struct trapframe64 *, unsigned int, vaddr_t, long);
void data_access_fault(struct trapframe64 *, unsigned int, vaddr_t, vaddr_t,
	vaddr_t, u_long);
void data_access_error(struct trapframe64 *, unsigned int, vaddr_t, u_long,
	vaddr_t, u_long);
void text_access_fault(struct trapframe64 *tf, unsigned int type, vaddr_t pc,
	u_long sfsr);
void text_access_error(struct trapframe64 *, unsigned int, vaddr_t, u_long,
	vaddr_t, u_long);

#ifdef DEBUG
void print_trapframe(struct trapframe64 *);

void
print_trapframe(struct trapframe64 *tf)
{

	printf("Trapframe %p:\ttstate: %lx\tpc: %lx\tnpc: %lx\n",
	       tf, (u_long)tf->tf_tstate, (u_long)tf->tf_pc, (u_long)tf->tf_npc);
	printf("fault: %p\ty: %x\t", 
	       (void *)(u_long)tf->tf_fault, (int)tf->tf_y);
	printf("pil: %d\toldpil: %d\ttt: %x\tGlobals:\n", 
	       (int)tf->tf_pil, (int)tf->tf_oldpil, (int)tf->tf_tt);
	printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
	       (u_int)(tf->tf_global[0]>>32), (u_int)tf->tf_global[0],
	       (u_int)(tf->tf_global[1]>>32), (u_int)tf->tf_global[1],
	       (u_int)(tf->tf_global[2]>>32), (u_int)tf->tf_global[2],
	       (u_int)(tf->tf_global[3]>>32), (u_int)tf->tf_global[3]);
	printf("%08x%08x %08x%08x %08x%08x %08x%08x\nouts:\n",
	       (u_int)(tf->tf_global[4]>>32), (u_int)tf->tf_global[4],
	       (u_int)(tf->tf_global[5]>>32), (u_int)tf->tf_global[5],
	       (u_int)(tf->tf_global[6]>>32), (u_int)tf->tf_global[6],
	       (u_int)(tf->tf_global[7]>>32), (u_int)tf->tf_global[7]);
#ifdef DEBUG
	printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
	       (u_int)(tf->tf_out[0]>>32), (u_int)tf->tf_out[0],
	       (u_int)(tf->tf_out[1]>>32), (u_int)tf->tf_out[1],
	       (u_int)(tf->tf_out[2]>>32), (u_int)tf->tf_out[2],
	       (u_int)(tf->tf_out[3]>>32), (u_int)tf->tf_out[3]);
	printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
	       (u_int)(tf->tf_out[4]>>32), (u_int)tf->tf_out[4],
	       (u_int)(tf->tf_out[5]>>32), (u_int)tf->tf_out[5],
	       (u_int)(tf->tf_out[6]>>32), (u_int)tf->tf_out[6],
	       (u_int)(tf->tf_out[7]>>32), (u_int)tf->tf_out[7]);
#endif

}
#endif

/*
 * Called from locore.s trap handling, for non-MMU-related traps.
 * (MMU-related traps go through mem_access_fault, below.)
 */
void
trap(struct trapframe64 *tf, unsigned int type, vaddr_t pc, long tstate)
{
	struct lwp *l;
	struct proc *p;
	struct pcb *pcb;
	int64_t n;
	u_quad_t sticks;
	int pstate = tstate >> TSTATE_PSTATE_SHIFT;
	ksiginfo_t ksi;
	int error;
	int code, sig;
#ifdef MULTIPROCESSOR
	int s;
#define	disintr()	s = intr_disable()
#define	rstintr()	intr_restore(s)
#else
#define	disintr()	/* nothing */
#define	rstintr()	/* nothing */
#endif

	/* This steps the PC over the trap. */
#define	ADVANCE (n = tf->tf_npc, tf->tf_pc = n, tf->tf_npc = n + 4)

#ifdef DEBUG
	if (tf->tf_pc == tf->tf_npc) {
		printf("trap: tpc %p == tnpc %p\n",
		    (void *)(u_long)tf->tf_pc, (void *)(u_long)tf->tf_npc);
		Debugger();
	}
	if ((trapdebug & TDB_NSAVED && curpcb->pcb_nsaved) ||
	    trapdebug & (TDB_FOLLOW | TDB_TRAP)) {
		char sbuf[sizeof(PSTATE_BITS) + 64];

		printf("trap: type 0x%x: pc=%lx &tf=%p\n",
		       type, pc, tf);
		snprintb(sbuf, sizeof(sbuf), PSTATE_BITS, pstate);
		printf(" npc=%lx pstate=%s %s\n",
		       (long)tf->tf_npc, sbuf, 
		       type < N_TRAP_TYPES ? trap_type[type] : 
		       ((type == T_AST) ? "ast" : 
			((type == T_RWRET) ? "rwret" : T)));
	}
	if ((trapdebug & (TDB_FOLLOW | TDB_TRAP)) ||
	    ((trapdebug & TDB_TL) && gettl())) {
		char sbuf[sizeof(PSTATE_BITS) + 64];

		extern int trap_trace_dis;
		trap_trace_dis = 1;
		printf("trap: type 0x%x: lvl=%d pc=%lx &tf=%p",
		       type, gettl(), pc, tf);
		snprintb(sbuf, sizeof(sbuf), PSTATE_BITS, pstate);
		printf(" npc=%lx pstate=%s %s\n",
		       (long)tf->tf_npc, sbuf, 
		       type < N_TRAP_TYPES ? trap_type[type] : 
		       ((type == T_AST) ? "ast" : 
			((type == T_RWRET) ? "rwret" : T)));
#ifdef DDB
		kdb_trap(type, tf);
#endif
	}
#endif

	uvmexp.traps++;

	/*
	 * Generally, kernel traps cause a panic.  Any exceptions are
	 * handled early here.
	 */
	if (pstate & PSTATE_PRIV) {
#ifdef DDB
		if (type == T_BREAKPOINT) {
			write_all_windows();
			if (kdb_trap(type, tf)) {
				/* ADVANCE; */
				return;
			}
		}
		if (type == T_PA_WATCHPT || type == T_VA_WATCHPT) {
			if (kdb_trap(type, tf)) {
				/* DDB must turn off watchpoints or something */
				return;
			}
		}
#endif
		/*
		 * The kernel needs to use FPU registers for block
		 * load/store.  If we trap in priviliged code, save
		 * the FPU state if there is any and enable the FPU.
		 *
		 * We rely on the kernel code properly enabling the FPU
		 * in %fprs, otherwise we'll hang here trying to enable
		 * the FPU.
		 */
		if (type == T_FPDISABLED) {
			struct lwp *newfplwp;

			/* New scheme */
			if (CLKF_INTR((struct clockframe *)tf) || !curlwp) {
				newfplwp = &lwp0;
			} else {
				newfplwp = curlwp;
				/* force other cpus to give up this fpstate */
				if (newfplwp->l_md.md_fpstate)
					fpusave_lwp(newfplwp, true);
			}
			if (fplwp != newfplwp) {
				disintr();
				if (fplwp != NULL) {
					/* someone else had it, maybe? */
					KASSERT(fplwp->l_md.md_fpstate != NULL);
					savefpstate(fplwp->l_md.md_fpstate);
					fplwp = NULL;
				}
				rstintr();
				/* If we have an allocated fpstate, load it */
				if (newfplwp->l_md.md_fpstate != NULL) {
					fplwp = newfplwp;
					loadfpstate(fplwp->l_md.md_fpstate);
				} else
					fplwp = NULL;
			}
			/* Enable the FPU */
			tf->tf_tstate |= TSTATE_PEF;
			return;
		}
		goto dopanic;
	}
	if ((l = curlwp) == NULL)
		l = &lwp0;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);
	sticks = p->p_sticks;
	pcb = &l->l_addr->u_pcb;
	l->l_md.md_tf = tf;	/* for ptrace/signals */

	sig = 0;

	switch (type) {

	default:
		if (type < 0x100) {
			extern int trap_trace_dis;
dopanic:
			trap_trace_dis = 1;

			{
				char sb[sizeof(PSTATE_BITS) + 64];

				printf("trap type 0x%x: cpu %d, pc=%lx",
				       type, cpu_number(), pc); 
				snprintb(sb, sizeof(sb), PSTATE_BITS, pstate);
				printf(" npc=%lx pstate=%s\n",
				       (long)tf->tf_npc, sb);
				DEBUGGER(type, tf);
				panic(type < N_TRAP_TYPES ? trap_type[type] : T);
			}
			/* NOTREACHED */
		}
#if defined(COMPAT_SVR4) || defined(COMPAT_SVR4_32)
badtrap:
#endif
		/* the following message is gratuitous */
		/* ... but leave it in until we find anything */
		printf("%s[%d]: unimplemented software trap 0x%x\n",
		    p->p_comm, p->p_pid, type);

		KSI_INIT_TRAP(&ksi);
		sig = SIGILL;
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_ILLTRP;
		ksi.ksi_addr = (void *)pc;
		break;

#if defined(COMPAT_SVR4) || defined(COMPAT_SVR4_32)
	case T_SVR4_GETCC:
	case T_SVR4_SETCC:
	case T_SVR4_GETPSR:
	case T_SVR4_SETPSR:
	case T_SVR4_GETHRTIME:
	case T_SVR4_GETHRVTIME:
	case T_SVR4_GETHRESTIME:
#if defined(COMPAT_SVR4_32)
		if (svr4_32_trap(type, l))
			break;
#endif
#if defined(COMPAT_SVR4)
		if (svr4_trap(type, l))
			break;
#endif
		goto badtrap;
#endif

	case T_AST:
#if 1
		if (want_resched)
			preempt();
		want_ast = 0;
#endif
		break;	/* the work is all in userret() */

	case T_ILLINST:
	case T_INST_EXCEPT:
	case T_TEXTFAULT:
		/* This is not an MMU issue!!!! */
		printf("trap: textfault at %lx!! sending SIGILL due to trap %d: %s\n", 
		       pc, type, type < N_TRAP_TYPES ? trap_type[type] : T);
#if defined(DDB) && defined(DEBUG)
		if (trapdebug & TDB_STOPSIG)
			Debugger();
#endif
		KSI_INIT_TRAP(&ksi);
		sig = SIGILL;
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_PRIVINST:
		printf("trap: privinst!! sending SIGILL due to trap %d: %s\n", 
		       type, type < N_TRAP_TYPES ? trap_type[type] : T);
#if defined(DDB) && defined(DEBUG)
		if (trapdebug & TDB_STOPSIG)
			Debugger();
#endif
		KSI_INIT_TRAP(&ksi);
		sig = SIGILL;
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_PRVOPC;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_PRIVACT:
		KSI_INIT_TRAP(&ksi);
		sig = SIGILL;
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_PRVOPC;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_FPDISABLED: {
		struct fpstate64 *fs = l->l_md.md_fpstate;

		if (fs == NULL) {
			/* NOTE: fpstate must be 64-byte aligned */
			fs = pool_cache_get(fpstate_cache, PR_WAITOK);
			*fs = initfpstate;
			l->l_md.md_fpstate = fs;
		}
		/*
		 * We may have more FPEs stored up and/or ops queued.
		 * If they exist, handle them and get out.  Otherwise,
		 * resolve the FPU state, turn it on, and try again.
		 *
		 * Ultras should never have a FPU queue.
		 */
		if (fs->fs_qsize) {
			printf("trap: Warning fs_qsize is %d\n",fs->fs_qsize);
			fpu_cleanup(l, fs);
			break;
		}
		if (fplwp != l) {		/* we do not have it */
			/* but maybe another CPU has it? */
			fpusave_lwp(l, true);
			disintr();
			if (fplwp != NULL) {	/* someone else had it */
				KASSERT(fplwp->l_md.md_fpstate != NULL);
				savefpstate(fplwp->l_md.md_fpstate);
			}
			loadfpstate(fs);
			fplwp = l;		/* now we do have it */
			rstintr();
		}
		tf->tf_tstate |= TSTATE_PEF;
		break;
	}

	case T_ALIGN:
	case T_LDDF_ALIGN:
	case T_STDF_ALIGN:
		{
		int64_t dsfsr, dsfar=0, isfsr;

		dsfsr = ldxa(SFSR, ASI_DMMU);
		if (dsfsr & SFSR_FV)
			dsfar = ldxa(SFAR, ASI_DMMU);
		isfsr = ldxa(SFSR, ASI_IMMU);
		/* 
		 * If we're busy doing copyin/copyout continue
		 */
		if (l->l_addr && l->l_addr->u_pcb.pcb_onfault) {
			tf->tf_pc = (vaddr_t)l->l_addr->u_pcb.pcb_onfault;
			tf->tf_npc = tf->tf_pc + 4;
			break;
		}
		
#ifdef DEBUG
#define fmt64(x)	(u_int)((x)>>32), (u_int)((x))
		printf("Alignment error: pid=%d.%d comm=%s dsfsr=%08x:%08x "
		       "dsfar=%x:%x isfsr=%08x:%08x pc=%lx\n",
		       l->l_proc->p_pid, l->l_lid, l->l_proc->p_comm, fmt64(dsfsr), fmt64(dsfar),
		       fmt64(isfsr), pc);
#endif
		}
		
#if defined(DDB) && defined(DEBUG)
		if (trapdebug & TDB_STOPSIG) {
			write_all_windows();
			kdb_trap(type, tf);
		}
#endif
		if ((l->l_proc->p_md.md_flags & MDP_FIXALIGN) != 0 && 
		    fixalign(l, tf) == 0) {
			ADVANCE;
			break;
		}
		KSI_INIT_TRAP(&ksi);
		sig = SIGBUS;
		ksi.ksi_trap = type;
		ksi.ksi_code = BUS_ADRALN;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_FP_IEEE_754:
	case T_FP_OTHER:
		/*
		 * Clean up after a floating point exception.
		 * fpu_cleanup can (and usually does) modify the
		 * state we save here, so we must `give up' the FPU
		 * chip context.  (The software and hardware states
		 * will not match once fpu_cleanup does its job, so
		 * we must not save again later.)
		 */
		if (l != fplwp)
			panic("fpe without being the FP user");
		disintr();
		KASSERT(l->l_md.md_fpstate != NULL);
		savefpstate(l->l_md.md_fpstate);
		fplwp = NULL;
		rstintr();
		/* tf->tf_tstate &= ~TSTATE_PEF */ /* share_fpu will do this */
		if (l->l_md.md_fpstate->fs_qsize == 0) {
			error = copyin((void *)pc,
			    &l->l_md.md_fpstate->fs_queue[0].fq_instr,
			    sizeof(int));
			if (error) {
				sig = SIGBUS;
				KSI_INIT_TRAP(&ksi);
				ksi.ksi_trap = type;
				ksi.ksi_code = BUS_OBJERR;
				ksi.ksi_addr = (void *)pc;
				break;
			}
			l->l_md.md_fpstate->fs_qsize = 1;
			code = fpu_cleanup(l, l->l_md.md_fpstate);
			ADVANCE;
		} else
			code = fpu_cleanup(l, l->l_md.md_fpstate);

		if (code != 0) {
			sig = SIGFPE;
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_trap = type;
			ksi.ksi_code = code;
			ksi.ksi_addr = (void *)pc;
		}
		break;

	case T_TAGOF:
		KSI_INIT_TRAP(&ksi);
		sig = SIGEMT;
		ksi.ksi_trap = type;
		ksi.ksi_code = SI_NOINFO;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_BREAKPOINT:
		if (p->p_raslist == NULL ||
		    (ras_lookup(p, (void *)(intptr_t)tf->tf_pc) == (void *)-1)) {
			sig = SIGTRAP;
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_trap = type;
			ksi.ksi_code = TRAP_BRKPT;
			ksi.ksi_addr = (void *)pc;
		}
		break;

	case T_IDIV0:
	case T_DIV0:
		ADVANCE;
		sig = SIGFPE;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = FPE_INTDIV;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_CLEANWIN:
		uprintf("T_CLEANWIN\n");	/* XXX Should not get this */
		ADVANCE;
		break;

	case T_FLUSHWIN:
		/* Software window flush for v8 software */
		write_all_windows();
		ADVANCE;
		break;

	case T_RANGECHECK:
		printf("T_RANGECHECK\n");	/* XXX */
		ADVANCE;
		sig = SIGILL;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = ILL_ILLADR;
		ksi.ksi_addr = (void *)pc;
		break;

	case T_FIXALIGN:
#ifdef DEBUG_ALIGN
		uprintf("T_FIXALIGN\n");
#endif
		/* User wants us to fix alignment faults */
		l->l_proc->p_md.md_flags |= MDP_FIXALIGN;
		ADVANCE;
		break;

	case T_INTOF:
		uprintf("T_INTOF\n");		/* XXX */
		ADVANCE;
		sig = SIGFPE;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type;
		ksi.ksi_code = FPE_INTOVF;
		ksi.ksi_addr = (void *)pc;
		break;
	}
	if (sig != 0) {
		ksi.ksi_signo = sig;
		trapsignal(l, &ksi);
	}
	userret(l, pc, sticks);
	share_fpu(l, tf);
#undef ADVANCE
#ifdef DEBUG
	if (trapdebug & (TDB_FOLLOW | TDB_TRAP)) {
		printf("trap: done\n");
		/* if (type != T_BREAKPOINT) Debugger(); */
	}
#endif
}

/*
 * Save windows from PCB into user stack, and return 0.  This is used on
 * window overflow pseudo-traps (from locore.s, just before returning to
 * user mode) and when ptrace or sendsig needs a consistent state.
 * As a side effect, rwindow_save() always sets pcb_nsaved to 0.
 *
 * If the windows cannot be saved, pcb_nsaved is restored and we return -1.
 * 
 * XXXXXX This cannot work properly.  I need to re-examine this register
 * window thing entirely.  
 */
int
rwindow_save(struct lwp *l)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct rwindow64 *rw = &pcb->pcb_rw[0];
	uint64_t rwdest;
	int i, j;

	i = pcb->pcb_nsaved;
#ifdef DEBUG
	if (rwindow_debug & RW_FOLLOW)
		printf("rwindow_save(%p): nsaved %d\n", l, i);
#endif
	if (i == 0)
		return (0);
#ifdef DEBUG
	if (rwindow_debug & RW_FOLLOW)
		printf("%s[%d]: rwindow: pcb->stack:", l->l_proc->p_comm, l->l_proc->p_pid);
#endif

	 while (i > 0) {
		rwdest = rw[i--].rw_in[6];
#ifdef DEBUG
		if (rwindow_debug & RW_FOLLOW)
			printf("window %d at %lx\n", i, (long)rwdest);
#endif
		if (rwdest & 1) {
#ifdef DEBUG
			if (rwindow_debug & RW_64) {
				printf("rwindow_save: 64-bit tf to %p+BIAS "
				       "or %p\n", 
				       (void *)(long)rwdest,
				       (void *)(long)(rwdest+BIAS));
				Debugger();
			}
#endif
			rwdest += BIAS;
			if (copyout((void *)&rw[i], (void *)(u_long)rwdest,
				    sizeof(*rw))) {
#ifdef DEBUG
			if (rwindow_debug & (RW_ERR | RW_64))
				printf("rwindow_save: 64-bit pcb copyout "
				       "to %p failed\n", 
				       (void *)(long)rwdest);
#endif
				return (-1);
			}
#ifdef DEBUG
			if (rwindow_debug & RW_64) {
				printf("Finished copyout(%p, %p, %lx)\n",
					(void *)&rw[i], (void *)(long)rwdest,
                                	sizeof(*rw));
				Debugger();
			}
#endif
		} else {
			struct rwindow32 rwstack;

			/* 32-bit window */
			for (j = 0; j < 8; j++) { 
				rwstack.rw_local[j] = (int)rw[i].rw_local[j];
				rwstack.rw_in[j] = (int)rw[i].rw_in[j];
			}
			/* Must truncate rwdest */
			if (copyout(&rwstack, (void *)(u_long)(u_int)rwdest,
				    sizeof(rwstack))) {
#ifdef DEBUG
				if (rwindow_debug & RW_ERR)
					printf("rwindow_save: 32-bit pcb "
					       "copyout to %p (%p) failed\n", 
					       (void *)(u_long)(u_int)rwdest,
					       (void *)(u_long)rwdest);
#endif
				return (-1);
			}
		}
	}
	pcb->pcb_nsaved = 0;
#ifdef DEBUG
	if (rwindow_debug & RW_FOLLOW) {
		printf("\n");
		Debugger();
	}
#endif
	return (0);
}

/*
 * Kill user windows (before exec) by writing back to stack or pcb
 * and then erasing any pcb tracks.  Otherwise we might try to write
 * the registers into the new process after the exec.
 */
void
kill_user_windows(struct lwp *l)
{

	write_user_windows();
	l->l_addr->u_pcb.pcb_nsaved = 0;
}

/*
 * This routine handles MMU generated faults.  About half
 * of them could be recoverable through uvm_fault.
 */
void
data_access_fault(struct trapframe64 *tf, unsigned int type, vaddr_t pc,
	vaddr_t addr, vaddr_t sfva, u_long sfsr)
{
	uint64_t tstate;
	struct lwp *l;
	struct proc *p;
	struct vmspace *vm;
	vaddr_t va;
	int rv;
	vm_prot_t access_type;
	vaddr_t onfault;
	u_quad_t sticks;
	ksiginfo_t ksi;
#ifdef DEBUG
	static int lastdouble;
#endif

#ifdef DEBUG
	if (tf->tf_pc == tf->tf_npc) {
		printf("data_access_fault: tpc %lx == tnpc %lx\n", 
		       (long)tf->tf_pc, (long)tf->tf_npc);
		Debugger();
	}
	write_user_windows();
	if ((curpcb->pcb_nsaved > 8) ||
	    (trapdebug & TDB_NSAVED && curpcb->pcb_nsaved) ||
	    (trapdebug & (TDB_ADDFLT | TDB_FOLLOW))) {
		printf("%ld: data_access_fault(%p, %x, %p, %p, %lx, %lx) "
			"nsaved=%d\n",
			(long)(curproc?curproc->p_pid:-1), tf, type,
			(void *)addr, (void *)pc,
			sfva, sfsr, (int)curpcb->pcb_nsaved);
#ifdef DDB
		if ((trapdebug & TDB_NSAVED && curpcb->pcb_nsaved))
			Debugger();
#endif
	}
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
	if ((trapdebug & TDB_TL) && gettl()) {
		printf("%ld: data_access_fault(%p, %x, %p, %p, %lx, %lx) "
			"nsaved=%d\n",
			(long)(curproc?curproc->p_pid:-1), tf, type,
			(void*)addr, (void*)pc,
			sfva, sfsr, (int)curpcb->pcb_nsaved);
		Debugger();
	}
	if (trapdebug & TDB_STOPCALL) {
		Debugger();
	}
#endif

	uvmexp.traps++;
	if ((l = curlwp) == NULL)	/* safety check */
		l = &lwp0;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);
	sticks = p->p_sticks;
	tstate = tf->tf_tstate;

	/* Find the faulting va to give to uvm_fault */
	va = trunc_page(addr);

#ifdef DEBUG
	if (lastdouble) {
		printf("cpu%d: stacked data fault @ %lx (pc %lx);",
		       cpu_number(), addr, pc);
		lastdouble = 0;
		if (curproc == NULL)
			printf("NULL proc\n");
		else
			printf("pid %d(%s); sigmask %x, sigcatch %x\n",
			       l->l_proc->p_pid, l->l_proc->p_comm,
				/* XXX */
			       l->l_sigmask.__bits[0], 
			       l->l_proc->p_sigctx.ps_sigcatch.__bits[0]);
	}
#endif

	/* 
	 * Now munch on protections.
	 *
	 * If it was a FAST_DATA_ACCESS_MMU_MISS we have no idea what the
	 * access was since the SFSR is not set.  But we should never get
	 * here from there.
	 */
	if (type == T_FDMMU_MISS || (sfsr & SFSR_FV) == 0) {
		/* Punt */
		access_type = VM_PROT_READ;
	} else {
		access_type = (sfsr & SFSR_W) ? VM_PROT_WRITE : VM_PROT_READ;
	}
	if (tstate & TSTATE_PRIV) {
		extern char Lfsbail[];

		/*
		 * If this was an access that we shouldn't try to page in,
		 * resume at the fault handler without any action.
		 */
		if (l->l_addr && l->l_addr->u_pcb.pcb_onfault == Lfsbail)
			goto kfault;

		/*
		 * During autoconfiguration, faults are never OK unless
		 * pcb_onfault is set.  Once running normally we must allow
		 * exec() to cause copy-on-write faults to kernel addresses.
		 */
		if (cold)
			goto kfault;
		if (!(addr & TLB_TAG_ACCESS_CTX)) {
			/* CTXT == NUCLEUS */
#ifdef DIAGNOSTIC
			/*
			 * XXX We would panic later on anyway, but with no
			 * meaningfull message and the details are sometims
			 * hard to find, so better panic now with a helpfull
			 * message.
			 */
			/*
			 * XXXMRG in yamt-idlelwp world this seems unlikely?
			 */
			if (curlwp == NULL) {
				panic("cpu%d: kernel data access fault "
				    "accessing 0x%lx at pc 0x%lx\n",
				    cpu_number(), va, (long)tf->tf_pc);
			}
#endif
			rv = uvm_fault(kernel_map, va, access_type);
#ifdef DEBUG
			if (trapdebug & (TDB_ADDFLT | TDB_FOLLOW))
				printf("cpu%d: data_access_fault: kernel "
					"uvm_fault(%p, %lx, %x) "
					"sez %x -- %s\n", cpu_number(),
					kernel_map, va, access_type, rv,
					rv ? "failure" : "success");
#endif
			if (rv == 0)
				return;
			goto kfault;
		}
	} else {
		l->l_md.md_tf = tf;
		/*
		 * WRS: Can drop LP_SA_NOBLOCK test iff can only get
		 * here from a usermode-initiated access. LP_SA_NOBLOCK
		 * should never be set there - it's kernel-only.
		 */
		if ((l->l_flag & LW_SA)
		    && (~l->l_pflag & LP_SA_NOBLOCK)) {
			l->l_savp->savp_faultaddr = addr;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}
	}

	vm = p->p_vmspace;
	/* alas! must call the horrible vm code */
	onfault = (vaddr_t)l->l_addr->u_pcb.pcb_onfault;
	l->l_addr->u_pcb.pcb_onfault = NULL;
	rv = uvm_fault(&vm->vm_map, va, access_type);
	l->l_addr->u_pcb.pcb_onfault = (void *)onfault;

#ifdef DEBUG
	if (trapdebug & (TDB_ADDFLT | TDB_FOLLOW))
		printf("cpu%d: data_access_fault: %s uvm_fault(%p, %lx, %x) "
			"sez %x -- %s\n", cpu_number(),
			&vm->vm_map == kernel_map ? "kernel!!!" : "user",
			&vm->vm_map, va, access_type, rv,
			rv ? "failure" : "success");
#endif

	/*
	 * If this was a stack access we keep track of the maximum
	 * accessed stack size.  Also, if uvm_fault gets a protection
	 * failure it is due to accessing the stack region outside
	 * the current limit and we need to reflect that as an access
	 * error.
	 */
	if ((void *)va >= vm->vm_maxsaddr) {
		if (rv == 0)
			uvm_grow(p, va);
		else if (rv == EACCES)
			rv = EFAULT;
	}
	if (rv != 0) {

		/*
		 * Pagein failed.  If doing copyin/out, return to onfault
		 * address.  Any other page fault in kernel, die; if user
		 * fault, deliver SIGSEGV.
		 */
		if (tstate & TSTATE_PRIV) {
kfault:
			onfault = l->l_addr ?
			    (long)l->l_addr->u_pcb.pcb_onfault : 0;
			if (!onfault) {
				extern int trap_trace_dis;

				/* Disable traptrace for printf */
				trap_trace_dis = 1;
				(void) splhigh();
				printf("cpu%d: data fault: pc=%lx addr=%lx\n",
				    cpu_number(), pc, addr);
				DEBUGGER(type, tf);
				panic("kernel fault");
				/* NOTREACHED */
			}
#ifdef DEBUG
			if (trapdebug &
			    (TDB_ADDFLT | TDB_FOLLOW | TDB_STOPCPIO)) {
				printf("data_access_fault: copyin/out of %p "
				       "fault -- recover\n", (void *)addr);
				DEBUGGER(type, tf);
			}
#endif
			tf->tf_pc = onfault;
			tf->tf_npc = onfault + 4;
			return;
		}
#ifdef DEBUG
		if (trapdebug & (TDB_ADDFLT | TDB_STOPSIG)) {
			extern int trap_trace_dis;
			trap_trace_dis = 1;
			printf("cpu%d: data_access_fault at addr %p: "
			    "sending SIGSEGV\n", cpu_number(), (void *)addr);
			printf("%ld: data_access_fault(%p, %x, %p, %p, "
			       "%lx, %lx) nsaved=%d\n",
				(long)(curproc ? curproc->p_pid : -1), tf, type,
				(void *)addr, (void *)pc,
				sfva, sfsr, (int)curpcb->pcb_nsaved);
			Debugger();
		}
#endif
		KSI_INIT_TRAP(&ksi);
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       l->l_cred ?
			       kauth_cred_geteuid(l->l_cred) : -1);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = SI_NOINFO;
		} else {
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = (rv == EACCES
				? SEGV_ACCERR : SEGV_MAPERR);
		}
		ksi.ksi_trap = type;
		ksi.ksi_addr = (void *)sfva;
		trapsignal(l, &ksi);
	}
	if ((tstate & TSTATE_PRIV) == 0) {
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		userret(l, pc, sticks);
		share_fpu(l, tf);
	}
#ifdef DEBUG
	if (trapdebug & (TDB_ADDFLT | TDB_FOLLOW))
		printf("data_access_fault: done\n");
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
	if (trapdebug & (TDB_ADDFLT | TDB_FOLLOW)) {
		extern void *return_from_trap(void);

		if ((void *)(u_long)tf->tf_pc == (void *)return_from_trap) {
			printf("Returning from stack datafault\n");
		}
	}
#endif
}

/*
 * This routine handles deferred errors caused by the memory
 * or I/O bus subsystems.  Most of these are fatal, and even
 * if they are not, recovery is painful.  Also, the TPC and
 * TNPC values are probably not valid if we're not doing a
 * special PEEK/POKE code sequence.
 */
void
data_access_error(struct trapframe64 *tf, unsigned int type, vaddr_t afva,
	u_long afsr, vaddr_t sfva, u_long sfsr)
{
	u_long pc;
	uint64_t tstate;
	struct lwp *l;
	vaddr_t onfault;
	u_quad_t sticks;
	ksiginfo_t ksi;
#ifdef DEBUG
	static int lastdouble;
#endif

#ifdef DEBUG
	if (tf->tf_pc == tf->tf_npc) {
		printf("data_access_error: tpc %lx == tnpc %lx\n", 
		       (long)tf->tf_pc, (long)tf->tf_npc);
		Debugger();
	}
	write_user_windows();
	if ((trapdebug & TDB_NSAVED && curpcb->pcb_nsaved) || 
	    trapdebug & (TDB_ADDFLT | TDB_FOLLOW)) {
		char buf[768];

		snprintb(buf, sizeof buf, SFSR_BITS, sfsr);
		printf("%d data_access_error(%lx, %lx, %lx, %p)=%lx @ %p %s\n",
		       curproc?curproc->p_pid:-1, 
		       (long)type, (long)sfva, (long)afva, tf,
		       (long)tf->tf_tstate, 
		       (void *)(u_long)tf->tf_pc, buf);
	}
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
	if ((trapdebug & TDB_TL) && gettl()) {
		char buf[768];
		snprintb(buf, sizeof buf, SFSR_BITS, sfsr);

		printf("%d tl %d data_access_error(%lx, %lx, %lx, %p)="
		       "%lx @ %lx %s\n",
		       curproc ? curproc->p_pid : -1, gettl(),
		       (long)type, (long)sfva, (long)afva, tf,
		       (long)tf->tf_tstate, 
		       (long)tf->tf_pc, buf);
		Debugger();
	}
	if (trapdebug & TDB_STOPCALL) {
		Debugger();
	}
#endif

	uvmexp.traps++;
	if ((l = curlwp) == NULL)	/* safety check */
		l = &lwp0;
	LWP_CACHE_CREDS(l, l->l_proc);
	sticks = l->l_proc->p_sticks;

	pc = tf->tf_pc;
	tstate = tf->tf_tstate;

	onfault = l->l_addr ? (long)l->l_addr->u_pcb.pcb_onfault : 0;
	printf("data error type %x sfsr=%lx sfva=%lx afsr=%lx afva=%lx tf=%p\n",
		type, sfsr, sfva, afsr, afva, tf);

	if (afsr == 0) {
		printf("data_access_error: no fault\n");
		goto out;	/* No fault. Why were we called? */
	}

#ifdef DEBUG
	if (lastdouble) {
		printf("stacked data error @ %lx (pc %lx); sfsr %lx",
		       sfva, pc, sfsr);
		lastdouble = 0;
		if (curproc == NULL)
			printf("NULL proc\n");
		else
			printf("pid %d(%s); sigmask %x, sigcatch %x\n",
			       curproc->p_pid, curproc->p_comm,
				/* XXX */
			       curlwp->l_sigmask.__bits[0], 
			       curproc->p_sigctx.ps_sigcatch.__bits[0]);
	}
#endif

	if (tstate & TSTATE_PRIV) {
		if (!onfault) {
			extern int trap_trace_dis;
			char buf[768];

			trap_trace_dis = 1; /* Disable traptrace for printf */
			snprintb(buf, sizeof buf, SFSR_BITS, sfsr);
			(void) splhigh();
			printf("data fault: pc=%lx addr=%lx sfsr=%s\n",
				(u_long)pc, (long)sfva, buf);
			DEBUGGER(type, tf);
			panic("kernel fault");
			/* NOTREACHED */
		}

		/*
		 * If this was a priviliged error but not a probe, we
		 * cannot recover, so panic.
		 */
		if (afsr & ASFR_PRIV) {
			char buf[128];

			snprintb(buf, sizeof(buf), AFSR_BITS, afsr);
			panic("Privileged Async Fault: AFAR %p AFSR %lx\n%s",
				(void *)afva, afsr, buf);
			/* NOTREACHED */
		}
#ifdef DEBUG
		if (trapdebug & (TDB_ADDFLT | TDB_FOLLOW | TDB_STOPCPIO)) {
			printf("data_access_error: kern fault -- "
			       "skipping instr\n");
			if (trapdebug & TDB_STOPCPIO) {
				DEBUGGER(type, tf);
			}
		}
#endif
		tf->tf_pc = onfault;
		tf->tf_npc = onfault + 4;
		return;
	}
#ifdef DEBUG
	if (trapdebug & (TDB_ADDFLT | TDB_STOPSIG)) {
		extern int trap_trace_dis;

		trap_trace_dis = 1;
		printf("data_access_error at %p: sending SIGSEGV\n",
			(void *)(u_long)afva);
		Debugger();
	}
#endif
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGSEGV;
	ksi.ksi_code = SEGV_MAPERR;
	ksi.ksi_trap = type;
	ksi.ksi_addr = (void *)afva;
	trapsignal(l, &ksi);
out:
	if ((tstate & TSTATE_PRIV) == 0) {
		userret(l, pc, sticks);
		share_fpu(l, tf);
	}
#ifdef DEBUG
	if (trapdebug & (TDB_ADDFLT | TDB_FOLLOW))
		printf("data_access_error: done\n");
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
#endif
}

/*
 * This routine handles MMU generated faults.  About half
 * of them could be recoverable through uvm_fault.
 */
void
text_access_fault(struct trapframe64 *tf, unsigned int type, vaddr_t pc,
	u_long sfsr)
{
	uint64_t tstate;
	struct lwp *l;
	struct proc *p;
	struct vmspace *vm;
	vaddr_t va;
	int rv;
	vm_prot_t access_type;
	u_quad_t sticks;
	ksiginfo_t ksi;

#ifdef DEBUG
	if (tf->tf_pc == tf->tf_npc) {
		printf("text_access_fault: tpc %p == tnpc %p\n",
		    (void *)(u_long)tf->tf_pc, (void *)(u_long)tf->tf_npc);
		Debugger();
	}
	write_user_windows();
	if (((trapdebug & TDB_NSAVED) && curpcb->pcb_nsaved) || 
	    (trapdebug & (TDB_TXTFLT | TDB_FOLLOW)))
		printf("%d text_access_fault(%x, %lx, %p)\n",
		       curproc?curproc->p_pid:-1, type, pc, tf); 
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
	if ((trapdebug & TDB_TL) && gettl()) {
		printf("%d tl %d text_access_fault(%x, %lx, %p)\n",
		       curproc?curproc->p_pid:-1, gettl(), type, pc, tf); 
		Debugger();
	}
	if (trapdebug & TDB_STOPCALL) { 
		Debugger();
	}
#endif

	uvmexp.traps++;
	if ((l = curlwp) == NULL)	/* safety check */
		l = &lwp0;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);
	sticks = p->p_sticks;
	tstate = tf->tf_tstate;
	va = trunc_page(pc);

	/* Now munch on protections... */

	access_type = VM_PROT_EXECUTE;
	if (tstate & TSTATE_PRIV) {
		extern int trap_trace_dis;
		trap_trace_dis = 1; /* Disable traptrace for printf */
		(void) splhigh();
		printf("text_access_fault: pc=%lx va=%lx\n", pc, va);
		DEBUGGER(type, tf);
		panic("kernel fault");
		/* NOTREACHED */
	} else
		l->l_md.md_tf = tf;

	vm = p->p_vmspace;
	/* alas! must call the horrible vm code */
	rv = uvm_fault(&vm->vm_map, va, access_type);

#ifdef DEBUG
	if (trapdebug & (TDB_TXTFLT | TDB_FOLLOW))
		printf("text_access_fault: uvm_fault(%p, %lx, %x) sez %x\n",
		       &vm->vm_map, va, access_type, rv);
#endif
	/*
	 * If this was a stack access we keep track of the maximum
	 * accessed stack size.  Also, if uvm_fault gets a protection
	 * failure it is due to accessing the stack region outside
	 * the current limit and we need to reflect that as an access
	 * error.
	 */
	if ((void *)va >= vm->vm_maxsaddr) {
		if (rv == 0)
			uvm_grow(p, va);
	}
	if (rv != 0) {

		/*
		 * Pagein failed. Any other page fault in kernel, die; if user
		 * fault, deliver SIGSEGV.
		 */
		if (tstate & TSTATE_PRIV) {
			extern int trap_trace_dis;
			trap_trace_dis = 1; /* Disable traptrace for printf */
			(void) splhigh();
			printf("text fault: pc=%llx\n", (unsigned long long)pc);
			DEBUGGER(type, tf);
			panic("kernel fault");
			/* NOTREACHED */
		}
#ifdef DEBUG
		if (trapdebug & (TDB_TXTFLT | TDB_STOPSIG)) {
			extern int trap_trace_dis;
			trap_trace_dis = 1;
			printf("text_access_fault at %p: sending SIGSEGV\n",
			    (void *)(u_long)va);
			Debugger();
		}
#endif
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_code = (rv == EACCES ? SEGV_ACCERR : SEGV_MAPERR);
		ksi.ksi_trap = type;
		ksi.ksi_addr = (void *)pc;
		trapsignal(l, &ksi);
	}
	if ((tstate & TSTATE_PRIV) == 0) {
		userret(l, pc, sticks);
		share_fpu(l, tf);
	}
#ifdef DEBUG
	if (trapdebug & (TDB_TXTFLT | TDB_FOLLOW)) {
		printf("text_access_fault: done\n");
		/* kdb_trap(T_BREAKPOINT, tf); */
	}
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
#endif
}


/*
 * This routine handles deferred errors caused by the memory
 * or I/O bus subsystems.  Most of these are fatal, and even
 * if they are not, recovery is painful.  Also, the TPC and
 * TNPC values are probably not valid if we're not doing a
 * special PEEK/POKE code sequence.
 */
void
text_access_error(struct trapframe64 *tf, unsigned int type, vaddr_t pc,
	u_long sfsr, vaddr_t afva, u_long afsr)
{
	int64_t tstate;
	struct lwp *l;
	struct proc *p;
	struct vmspace *vm;
	vaddr_t va;
	int rv;
	vm_prot_t access_type;
	u_quad_t sticks;
	ksiginfo_t ksi;
#ifdef DEBUG
	static int lastdouble;
#endif
	char buf[768];
	
#ifdef DEBUG
	if (tf->tf_pc == tf->tf_npc) {
		printf("text_access_error: tpc %p == tnpc %p\n",
		    (void *)(u_long)tf->tf_pc, (void *)(u_long)tf->tf_npc);
		Debugger();
	}
	write_user_windows();
	if ((trapdebug & TDB_NSAVED && curpcb->pcb_nsaved) ||
	    trapdebug & (TDB_TXTFLT | TDB_FOLLOW)) {
		snprintb(buf, sizeof buf, SFSR_BITS, sfsr);
		printf("%ld text_access_error(%lx, %lx, %lx, %p)=%lx @ %lx %s\n",
		       (long)(curproc?curproc->p_pid:-1), 
		       (long)type, pc, (long)afva, tf, (long)tf->tf_tstate, 
		       (long)tf->tf_pc, buf); 
	}
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
	if ((trapdebug & TDB_TL) && gettl()) {
		snprintb(buf, sizeof buf, SFSR_BITS, sfsr);
		printf("%d tl %d text_access_error(%lx, %lx, %lx, %p)=%lx @ %lx %s\n",
		       curproc?curproc->p_pid:-1, gettl(),
		       (long)type, (long)pc, (long)afva, tf, 
		       (long)tf->tf_tstate, (long)tf->tf_pc, buf); 
		Debugger();
	}
	if (trapdebug & TDB_STOPCALL) { 
		Debugger();
	}
#endif
	uvmexp.traps++;
	if ((l = curlwp) == NULL)	/* safety check */
		l = &lwp0;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);
	sticks = p->p_sticks;

	tstate = tf->tf_tstate;

	if ((afsr) != 0) {
		extern int trap_trace_dis;

		trap_trace_dis++; /* Disable traptrace for printf */
		printf("text_access_error: memory error...\n");
		printf("text memory error type %d sfsr=%lx sfva=%lx afsr=%lx afva=%lx tf=%p\n",
		       type, sfsr, pc, afsr, afva, tf);
		trap_trace_dis--; /* Reenable traptrace for printf */

		if (tstate & TSTATE_PRIV)
			panic("text_access_error: kernel memory error");

		/* User fault -- Berr */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_code = BUS_OBJERR;
		ksi.ksi_trap = type;
		ksi.ksi_addr = (void *)pc;
		trapsignal(l, &ksi);
	}

	if ((sfsr & SFSR_FV) == 0 || (sfsr & SFSR_FT) == 0)
		goto out;	/* No fault. Why were we called? */

	va = trunc_page(pc);

#ifdef DEBUG
	if (lastdouble) {
		printf("stacked text error @ pc %lx; sfsr %lx", pc, sfsr);
		lastdouble = 0;
		if (curproc == NULL)
			printf("NULL proc\n");
		else
			printf("pid %d(%s); sigmask %x, sigcatch %x\n",
			       curproc->p_pid, curproc->p_comm,
			       curlwp->l_sigmask.__bits[0], 
			       curproc->p_sigctx.ps_sigcatch.__bits[0]);
	}
#endif
	/* Now munch on protections... */

	access_type = VM_PROT_EXECUTE;
	if (tstate & TSTATE_PRIV) {
		extern int trap_trace_dis;
		trap_trace_dis = 1; /* Disable traptrace for printf */
		snprintb(buf, sizeof buf, SFSR_BITS, sfsr);
		(void) splhigh();
		printf("text error: pc=%lx sfsr=%s\n", pc, buf);
		DEBUGGER(type, tf);
		panic("kernel fault");
		/* NOTREACHED */
	} else
		l->l_md.md_tf = tf;

	vm = p->p_vmspace;
	/* alas! must call the horrible vm code */
	rv = uvm_fault(&vm->vm_map, va, access_type);

	/*
	 * If this was a stack access we keep track of the maximum
	 * accessed stack size.  Also, if uvm_fault gets a protection
	 * failure it is due to accessing the stack region outside
	 * the current limit and we need to reflect that as an access
	 * error.
	 */
	if ((void *)va >= vm->vm_maxsaddr) {
		if (rv == 0)
			uvm_grow(p, va);
		else if (rv == EACCES)
			rv = EFAULT;
	}
	if (rv != 0) {
		/*
		 * Pagein failed.  If doing copyin/out, return to onfault
		 * address.  Any other page fault in kernel, die; if user
		 * fault, deliver SIGSEGV.
		 */
		if (tstate & TSTATE_PRIV) {
			extern int trap_trace_dis;
			trap_trace_dis = 1; /* Disable traptrace for printf */
			snprintb(buf, sizeof buf, SFSR_BITS, sfsr);
			(void) splhigh();
			printf("text error: pc=%lx sfsr=%s\n", pc, buf);
			DEBUGGER(type, tf);
			panic("kernel fault");
			/* NOTREACHED */
		}
#ifdef DEBUG
		if (trapdebug & (TDB_TXTFLT | TDB_STOPSIG)) {
			extern int trap_trace_dis;
			trap_trace_dis = 1;
			printf("text_access_error at %p: sending SIGSEGV\n",
			    (void *)(u_long)va);
			Debugger();
		}
#endif
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_code = (rv == EACCES ? SEGV_ACCERR : SEGV_MAPERR);
		ksi.ksi_trap = type;
		ksi.ksi_addr = (void *)pc;
		trapsignal(l, &ksi);
	}
out:
	if ((tstate & TSTATE_PRIV) == 0) {
		userret(l, pc, sticks);
		share_fpu(l, tf);
	}
#ifdef DEBUG
	if (trapdebug & (TDB_TXTFLT | TDB_FOLLOW))
		printf("text_access_error: done\n");
	if (trapdebug & TDB_FRAME) {
		print_trapframe(tf);
	}
#endif
}
