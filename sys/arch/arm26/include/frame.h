/*	$NetBSD: frame.h,v 1.7 2001/08/31 04:44:55 simonb Exp $	*/

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

#include <arm/frame.h>

#ifndef _LOCORE

#include <arm/armreg.h>

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
 
#endif /* _LOCORE */

#endif /* _ARM26_FRAME_H_ */
  
/* End of frame.h */
