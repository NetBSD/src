/*	$NetBSD: locore.h,v 1.6 2013/03/16 23:04:22 christos Exp $	*/

/*
 * Copyright (c) 1996-2002 Eduardo Horvath
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#undef	CURLWP
#undef	CPCB
#undef	FPLWP

#define	CURLWP	(CPUINFO_VA + CI_CURLWP)
#define	CPCB	(CPUINFO_VA + CI_CPCB)
#define	FPLWP	(CPUINFO_VA + CI_FPLWP)

/* A few convenient abbreviations for trapframe fields. */
#define	TF_G	TF_GLOBAL
#define	TF_O	TF_OUT
#define	TF_L	TF_LOCAL
#define	TF_I	TF_IN

/* Let us use same syntax as C code */
#define Debugger()	ta	1; nop


/*
 * This macro will clear out a cache line before an explicit
 * access to that location.  It's mostly used to make certain
 * loads bypassing the D$ do not get stale D$ data.
 *
 * It uses a register with the address to clear and a temporary
 * which is destroyed.
 */
#ifdef DCACHE_BUG
#define DLFLUSH(a,t) \
	andn	a, 0x3f, t; \
	stxa	%g0, [ t ] ASI_DCACHE_TAG; \
	membar	#Sync
/* The following can be used if the pointer is 32-byte aligned */
#define DLFLUSH2(t) \
	stxa	%g0, [ t ] ASI_DCACHE_TAG; \
	membar	#Sync
#else
#define DLFLUSH(a,t)
#define DLFLUSH2(t)
#endif


/*
 * Combine 2 regs -- used to convert 64-bit ILP32
 * values to LP64.
 */
#define	COMBINE(r1, r2, d)	\
	clruw	r2;		\
	sllx	r1, 32, d;	\
	or	d, r2, d

/*
 * Split 64-bit value in 1 reg into high and low halves.
 * Used for ILP32 return values.
 */
#define	SPLIT(r0, r1)		\
	srl	r0, 0, r1;	\
	srlx	r0, 32, r0


/*
 * A handy macro for maintaining instrumentation counters.
 * Note that this clobbers %o0, %o1 and %o2.  Normal usage is
 * something like:
 *	foointr:
 *		TRAP_SETUP(...)		! makes %o registers safe
 *		INCR(_C_LABEL(cnt)+V_FOO)	! count a foo
 */
#define INCR(what) \
	sethi	%hi(what), %o0; \
	or	%o0, %lo(what), %o0; \
99:	\
	lduw	[%o0], %o1; \
	add	%o1, 1, %o2; \
	casa	[%o0] ASI_P, %o1, %o2; \
	cmp	%o1, %o2; \
	bne,pn	%icc, 99b; \
	 nop

/*
 * A couple of handy macros to save and restore globals to/from
 * locals.  Since udivrem uses several globals, and it's called
 * from vsprintf, we need to do this before and after doing a printf.
 */
#define GLOBTOLOC \
	mov	%g1, %l1; \
	mov	%g2, %l2; \
	mov	%g3, %l3; \
	mov	%g4, %l4; \
	mov	%g5, %l5; \
	mov	%g6, %l6; \
	mov	%g7, %l7

#define LOCTOGLOB \
	mov	%l1, %g1; \
	mov	%l2, %g2; \
	mov	%l3, %g3; \
	mov	%l4, %g4; \
	mov	%l5, %g5; \
	mov	%l6, %g6; \
	mov	%l7, %g7

/* Load strings address into register; NOTE: hidden local label 99 */
#define LOAD_ASCIZ(reg, s)	\
	set	99f, reg ;	\
	.data ;			\
99:	.asciz	s ;		\
	_ALIGN ;		\
	.text

/*
 * Handy stack conversion macros.
 * They correctly switch to requested stack type
 * regardless of the current stack.
 */

#define TO_STACK64(size)					\
	save	%sp, size, %sp;					\
	add	%sp, -BIAS, %o0; /* Convert to 64-bits */	\
	andcc	%sp, 1, %g0; /* 64-bit stack? */		\
	movz	%icc, %o0, %sp

#define TO_STACK32(size)					\
	save	%sp, size, %sp;					\
	add	%sp, +BIAS, %o0; /* Convert to 32-bits */	\
	andcc	%sp, 1, %g0; /* 64-bit stack? */		\
	movnz	%icc, %o0, %sp

#ifdef _LP64
#define	STACKFRAME(size)	TO_STACK64(size)
#else
#define	STACKFRAME(size)	TO_STACK32(size)
#endif

/*
 * Primitives
 */
#ifdef ENTRY
#undef ENTRY
#endif

#ifdef GPROF
	.globl	_mcount
#define	ENTRY(x) \
	.globl _C_LABEL(x); .proc 1; .type _C_LABEL(x),@function; \
_C_LABEL(x): ; \
	.data; \
	.align 8; \
0:	.uaword 0; .uaword 0; \
	.text;	\
	save	%sp, -CC64FSZ, %sp; \
	sethi	%hi(0b), %o0; \
	call	_mcount; \
	or	%o0, %lo(0b), %o0; \
	restore
#else
#define	ENTRY(x)	.globl _C_LABEL(x); .proc 1; \
	.type _C_LABEL(x),@function; _C_LABEL(x):
#endif
#define	ALTENTRY(x)	.globl _C_LABEL(x); _C_LABEL(x):


