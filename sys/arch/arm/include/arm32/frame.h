/*	$NetBSD: frame.h,v 1.1.6.1 2001/09/13 01:13:10 thorpej Exp $	*/

/*
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
 *
 * RiscBSD kernel project
 *
 * frame.h
 *
 * Stack frames structures
 *
 * Created      : 30/09/94
 */

#ifndef _ARM32_FRAME_H_
#define _ARM32_FRAME_H_

#include <arm/frame.h>		/* Common ARM stack frames */

#ifndef _LOCORE

/*
 * System stack frames.
 */

typedef struct irqframe {
	unsigned int if_spsr;
	unsigned int if_r0;
	unsigned int if_r1;
	unsigned int if_r2;
	unsigned int if_r3;
	unsigned int if_r4;
	unsigned int if_r5;
	unsigned int if_r6;
	unsigned int if_r7;
	unsigned int if_r8;
	unsigned int if_r9;
	unsigned int if_r10;
	unsigned int if_r11;
	unsigned int if_r12;
	unsigned int if_usr_sp;
	unsigned int if_usr_lr;
	unsigned int if_svc_sp;
	unsigned int if_svc_lr;
	unsigned int if_pc;
} irqframe_t;

#define clockframe irqframe

/*
 * Switch frame
 */

struct switchframe {
	int	sf_spl;
	u_int	sf_r4;
	u_int	sf_r5;
	u_int	sf_r6;
	u_int	sf_r7;
	u_int	sf_pc;
};
 
/*
 * Stack frame. Used during stack traces (db_trace.c)
 */
struct frame {
	u_int	fr_fp;
	u_int	fr_sp;
	u_int	fr_lr;
	u_int	fr_pc;
};

#ifdef _KERNEL
void validate_trapframe __P((trapframe_t *, int));
#endif /* _KERNEL */

#else /* _LOCORE */

/*
 * ASM macros for pushing and pulling trapframes from the stack
 *
 * These macros are used to handle the irqframe and trapframe structures
 * defined above.
 */

/*
 * PUSHFRAME - macro to push a trap frame on the stack in the current mode
 * Since the current mode is used, the SVC lr field is not defined.
 *
 * NOTE: r13 and r14 are stored separately as a work around for the
 * SA110 rev 2 STM^ bug
 */

#define PUSHFRAME							   \
	str	lr, [sp, #-4]!;		/* Push the return address */	   \
	sub	sp, sp, #(4*17);	/* Adjust the stack pointer */	   \
	stmia	sp, {r0-r12};		/* Push the user mode registers */ \
	add	r0, sp, #(4*13);	/* Adjust the stack pointer */	   \
	stmia	r0, {r13-r14}^;		/* Push the user mode registers */ \
        mov     r0, r0;                 /* NOP for previous instruction */ \
	mrs	r0, spsr_all;		/* Put the SPSR on the stack */	   \
	str	r0, [sp, #-4]!;

/*
 * PULLFRAME - macro to pull a trap frame from the stack in the current mode
 * Since the current mode is used, the SVC lr field is ignored.
 */

#define PULLFRAME							   \
        ldr     r0, [sp], #0x0004;      /* Get the SPSR from stack */	   \
        msr     spsr_all, r0;						   \
        ldmia   sp, {r0-r14}^;		/* Restore registers (usr mode) */ \
        mov     r0, r0;                 /* NOP for previous instruction */ \
	add	sp, sp, #(4*17);	/* Adjust the stack pointer */	   \
 	ldr	lr, [sp], #0x0004;	/* Pull the return address */

/*
 * PUSHFRAMEINSVC - macro to push a trap frame on the stack in SVC32 mode
 * This should only be used if the processor is not currently in SVC32
 * mode. The processor mode is switched to SVC mode and the trap frame is
 * stored. The SVC lr field is used to store the previous value of
 * lr in SVC mode.  
 *
 * NOTE: r13 and r14 are stored separately as a work around for the
 * SA110 rev 2 STM^ bug
 */

#define PUSHFRAMEINSVC							   \
	stmdb	sp, {r0-r3};		/* Save 4 registers */		   \
	mov	r0, lr;			/* Save xxx32 r14 */		   \
	mov	r1, sp;			/* Save xxx32 sp */		   \
	mrs	r3, spsr_all;		/* Save xxx32 spsr */		   \
	mrs     r2, cpsr_all; 		/* Get the CPSR */		   \
	bic     r2, r2, #(PSR_MODE);	/* Fix for SVC mode */		   \
	orr     r2, r2, #(PSR_SVC32_MODE);				   \
	msr     cpsr_all, r2;		/* Punch into SVC mode */	   \
	mov	r2, sp;			/* Save	SVC sp */		   \
	str	r0, [sp, #-4]!;		/* Push return address */	   \
	str	lr, [sp, #-4]!;		/* Push SVC lr */		   \
	str	r2, [sp, #-4]!;		/* Push SVC sp */		   \
	msr     spsr_all, r3;		/* Restore correct spsr */	   \
	ldmdb	r1, {r0-r3};		/* Restore 4 regs from xxx mode */ \
	sub	sp, sp, #(4*15);	/* Adjust the stack pointer */	   \
	stmia	sp, {r0-r12};		/* Push the user mode registers */ \
	add	r0, sp, #(4*13);	/* Adjust the stack pointer */	   \
	stmia	r0, {r13-r14}^;		/* Push the user mode registers */ \
        mov     r0, r0;                 /* NOP for previous instruction */ \
	mrs	r0, spsr_all;		/* Put the SPSR on the stack */	   \
	str	r0, [sp, #-4]!

/*
 * PULLFRAMEFROMSVCANDEXIT - macro to pull a trap frame from the stack
 * in SVC32 mode and restore the saved processor mode and PC.
 * This should be used when the SVC lr register needs to be restored on
 * exit.
 */

#define PULLFRAMEFROMSVCANDEXIT						   \
        ldr     r0, [sp], #0x0004;	/* Get the SPSR from stack */	   \
        msr     spsr_all, r0;		/* restore SPSR */		   \
        ldmia   sp, {r0-r14}^;		/* Restore registers (usr mode) */ \
        mov     r0, r0;	  		/* NOP for previous instruction */ \
	add	sp, sp, #(4*15);	/* Adjust the stack pointer */	   \
	ldmia	sp, {sp, lr, pc}^	/* Restore lr and exit */

#endif /* _LOCORE */

#endif /* _ARM32_FRAME_H_ */
  
/* End of frame.h */
