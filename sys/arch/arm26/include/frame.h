/*	$NetBSD: frame.h,v 1.2 2001/01/11 22:03:52 bjh21 Exp $	*/

/*
 * Copyright (c) 1999 Ben Harris.
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * frame.h - Stack frames structures
 */

#ifndef _ARM26_FRAME_H_
#define _ARM26_FRAME_H_

#ifndef _LOCORE

#include <sys/signal.h>

#include <machine/armreg.h>

/*
 * System stack frames.
 */

/*
 * irqframes just contain registers that APCS specifies the callee
 * need not preserve.
 */
typedef struct irqframe {
	union {
		struct {
			register_t if_r0;
			register_t if_r1;
			register_t if_r2;
			register_t if_r3;
			register_t if_r12;
			register_t if_r14;
		} svc;
		struct {
			register_t if_r0;
			register_t if_r1;
			register_t if_r2;
			register_t if_r3;
			register_t if_r11;
			register_t if_r12;
		} usr;
	} if_mode;
	register_t if_r15; /* Must be fixed so we know which branch to use */
} irqframe_t;

#define clockframe irqframe

/* All the CLKF_* macros take a struct clockframe * as an argument. */

/* True if we took the interrupt in user mode */
#define CLKF_USERMODE(frame)	((frame->if_r15 & R15_MODE) == R15_MODE_USR)

/* True if we were at spl0 before the interrupt */
#define CLKF_BASEPRI(frame)	0	/* FIXME */

/* Extract the program counter from a clockframe */
#define CLKF_PC(frame)		(frame->if_r15 & R15_PC)

/* True if we took the interrupt from inside another interrupt handler. */
/* Non-trivial to check because we handle interrupts in SVC mode. */
#define CLKF_INTR(frame)	0	/* FIXME */

typedef struct trapframe {
	register_t tf_r0;
	register_t tf_r1;
	register_t tf_r2;
	register_t tf_r3;
	register_t tf_r4;
	register_t tf_r5;
	register_t tf_r6;
	register_t tf_r7;
	register_t tf_r8;
	register_t tf_r9;
	register_t tf_r10;
	register_t tf_r11;
	register_t tf_r12;
	register_t tf_r13;
	register_t tf_r14;
	register_t tf_r15;
} trapframe_t;

/*
 * Signal frame
 */

struct sigframe {
	int    	sf_signum;
	int	sf_code;
	struct	sigcontext *sf_scp;
	sig_t	sf_handler;
	struct	sigcontext sf_sc;
};

/*
 * Switch frame
 */

struct switchframe {
	register_t	sf_r4;  /* Callee-saved registers */
	register_t	sf_r5;
	register_t	sf_r6;
	register_t	sf_r7;
	register_t	sf_r8;
	register_t	sf_r9;
	register_t	sf_r10;
	register_t	sf_r11; /* Frame pointer */
	register_t	sf_r13; /* Stack pointer */
	register_t	sf_r14; /* Return address */
};

/*
 * Floating-point frame.  Stores the state of the FPU.
 */

struct fpframe {
	register_t	ff_fpsr;
	register_t	ff_regs[8*3];
};
 
/*
 * Stack frame. Used during stack traces (db_trace.c)
 */
struct frame {
	register_t	fr_fp;
	register_t	fr_sp;
	register_t	fr_lr;
	register_t	fr_r15;
};

#ifdef _KERNEL
void validate_trapframe __P((trapframe_t *, int));
#endif /* _KERNEL */

#endif _LOCORE

#endif /* _ARM26_FRAME_H_ */
  
/* End of frame.h */
