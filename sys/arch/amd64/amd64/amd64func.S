/*	$NetBSD: amd64func.S,v 1.5 2008/04/28 20:23:12 martin Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Functions to provide access to amd64-specific instructions.
 *
 * These are explicitly _not_ shared with xen/amd64.
 */

#include <machine/asm.h>

#include "assym.h"

ENTRY(tsc_get_timecount)
	jmp	_C_LABEL(tsc_slow_timecount)
	movq	CPUVAR(CURLWP), %rsi
1:
	movl	L_NCSW(%rsi), %ecx	/* gen = l_ncsw */
	movq	L_CPU(%rsi), %rdi
	addq	$CPU_INFO_CC, %rdi
	cmpl	$0, CC_DENOM(%rdi)	/* if (first time) */
	jz	2f			/*    return tsc_slow_timecount() */
	addl	CC_GEN(%rdi), %ecx	/* gen += cc_gen */
	movl	CC_DELTA+4(%rdi), %eax	/* if (cc_delta > 0xffffffff || */
	orl	CC_DENOM+4(%rdi), %eax	/*     cc_denom > 0xffffffff) */
	jnz	2f			/*    return tsc_slow_timecount() */
	rdtsc				/* cc = cpu_counter32(); */
	subl	CC_CC(%rdi), %eax	/* cc -= cc_cc; (can wrap) */
	mull	CC_DELTA(%rdi)		/* rcc = (cc * cc_delta) */
	divl	CC_DENOM(%rdi)		/*     / cc_denom */
	addl	CC_VAL(%rdi), %eax	/*     + cc_val; */
	subl	CC_GEN(%rdi), %ecx	/* if (gen != cc_gen + */
	cmpl	L_NCSW(%rsi), %ecx	/*     l_ncsw) */
	jne	3f			/*    continue; */
	ret				/* return rcc; */
2:
	jmp	_C_LABEL(tsc_slow_timecount)
3:
	jmp	1b
