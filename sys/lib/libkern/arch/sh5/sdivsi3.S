/*	$NetBSD: sdivsi3.S,v 1.1 2002/07/05 13:32:08 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Signed 32-bit division.
 *
 * The SH5 has no native integer divide instruction, which normally doesn't
 * give the compiler a problem as it uses the FPU's divide instruction
 * to fabricate an integer divide.
 *
 * Trouble is, the kernel only saves user FPU state on a context switch and
 * by then there's a fair chance the kernel would have stomped all over it by
 * using floating point instructions itself.
 *
 * The following implementation still uses the FPU's division support, but
 * ensures the affected FP registers are saved and restored before returning.
 *
 * Note: The FP register dirty bits in the USR are not saved/restored as
 * there is a per-process USR stashed in the trapframe on kernel entry which
 * encodes the *real* dirty register state.
 *
 * Note: Division by zero will result in an FPU exception...
 *
 * Note: These don't follow the regular SH-5 ABI calling convention.
 * The dividend is passed in r4 instead of r2, the divisor is passed in
 * r5, and the result is returned in r0. Also, we can only clobber r1-r3.
 */

#include <machine/asm.h>

/*
 * int __divsi3(int dividend, int divisor)
 * {
 *	return (dividend / divisor);
 * }
 *
 * dividend = r4
 *  divisor = r5
 *
 * Return result in r0
 */
ENTRY(__sdivsi3)
	fmov.dq	dr0, r1			/* Preserve dr0 (fr0/fr1) in r1 */
	fmov.dq	dr2, r2			/* Preserve dr2 (fr2/fr3) in r2 */
	fmov.ls	r4, fr0			/* fr0 = bitwise copy of dividend */
	fmov.ls	r5, fr2			/* fr2 = bitwise copy of divisor */
	float.ld fr0, dr0		/* int to double conversion of fr0 */
	float.ld fr2, dr2		/* int to double conversion of fr2 */
	ptabs/l	r18, tr0
	fdiv.d	dr0, dr2, dr0		/* dr0 = dr0 / dr2 */
	fmov.qd	r2, dr2			/* Restore dr2 */
	ftrc.dl	dr0, fr0		/* double to int conversion */
	fmov.sl	fr0, r0			/* Sign-extended result to r0 */
	fmov.qd	r1, dr0			/* Restore dr0 */
	blink	tr0, r63

