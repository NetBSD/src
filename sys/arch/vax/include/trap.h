/*      $NetBSD: trap.h,v 1.24.58.1 2023/06/21 19:10:28 martin Exp $     */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)trap.h	5.4 (Berkeley) 5/9/91
 */

/*
 * Trap type values
 * also known in trap.c for name strings
 */
#ifndef _VAX_TRAP_H_
#define _VAX_TRAP_H_

#define	T_RESADFLT	0	/* reserved addressing */
#define	T_PRIVINFLT	1	/* privileged instruction */
#define	T_RESOPFLT	2	/* reserved operand */
#define	T_BPTFLT	3	/* breakpoint instruction */
#define	T_XFCFLT	4	/* Customer reserved instruction */
#define	T_SYSCALL	5	/* system call (kcall) */
#define	T_ARITHFLT	6	/* arithmetic trap */
#define	T_ASTFLT	7	/* system forced exception */
#define	T_PTELEN	8	/* Page table length exceeded */
#define	T_TRANSFLT	9	/* translation fault */
#define	T_TRCTRAP	10	/* trace trap */
#define	T_COMPAT	11	/* compatibility mode fault on VAX */
#define	T_ACCFLT	12	/* Access violation fault */
#define	T_KSPNOTVAL	15	/* kernel stack pointer not valid */
#define	T_KDBTRAP	17	/* kernel debugger trap */

/* These gets ORed with the word for page handling routines */
#define	T_WRITE		0x80
#define	T_PTEFETCH	0x40

/* Section 6.4.1 Arithmetic Traps/Faults */
#define ATRP_INTOVF	0x1	/* integer overflow */
#define ATRP_INTDIV	0x2	/* integer divide by zero */
#define ATRP_FLTOVF	0x3	/* floating overflow */
#define ATRP_FLTDIV	0x4	/* floating/decimal divide by zero */
#define ATRP_FLTUND	0x5	/* floating underflow */
#define ATRP_DECOVF	0x6	/* decimal underflow */
#define ATRP_FLTSUB	0x7	/* subscript range */
#define	AFLT_FLTOVF	0x8	/* floating overflow */
#define	AFLT_FLTDIV	0x9	/* floating divide-by-zero */
#define	AFLT_FLTUND	0xa	/* floating underflow */

/* Used by RAS to detect an interrupted CAS */
#define	CASMAGIC	0xFEDABABE /* always invalid space */

/* Trap's coming from user mode */
#define	T_USER	0x100

#ifndef _LOCORE
struct	trapframe {
	long	tf_fp;	/* Stack frame pointer */
	long	tf_ap;     /* Argument pointer on user stack */
	long	tf_sp;	/* Stack pointer */
	long	tf_r0;     /* General registers saved upon trap/syscall */
	long	tf_r1;
	long	tf_r2;
	long	tf_r3;
	long	tf_r4;
	long	tf_r5;
	long	tf_r6;
	long	tf_r7;
	long	tf_r8;
	long	tf_r9;
	long	tf_r10;
	long	tf_r11;
	long	tf_trap;	/* Type of trap */
        long	tf_code;   /* Trap specific code */
        long	tf_pc;     /* User pc */
        long	tf_psl;    /* User psl */
};

#endif /* _LOCORE */

#endif /* _VAX_TRAP_H_ */
