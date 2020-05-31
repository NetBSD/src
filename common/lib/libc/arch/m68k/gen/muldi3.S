/*	$NetBSD: muldi3.S,v 1.2 2020/05/31 12:37:07 rin Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rin Okuyama.
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

#include <machine/asm.h>

RCSID("$NetBSD: muldi3.S,v 1.2 2020/05/31 12:37:07 rin Exp $")

| int64_t __muldi3(int64_t X, int64_t Y);
|
| * Return lower 64bit of (X * Y) into %d0:%d1.
|
| * Intended for 68060:
|   - GCC does not emit __muldi3() for 68020-40, that have 32 * 32 --> 64 mulul.
|   - mulsl (and moveml) are not implemented for 68010.
|
| * Notation:
|   - H32:L32 --> higher:lower 32bit of variable
|   - H:L     --> higher:lower 16bit of variable/register

#ifdef __mc68010__
#error "not for 68010"
#endif

#define X_H32 (4 * 4)
#define X_L32 (X_H32 + 4)
#define Y_H32 (X_L32 + 4)
#define Y_L32 (Y_H32 + 4)

ENTRY(__muldi3)
	moveml	%d2-%d4, -(%sp)	| push %d2-%d4

| First, calculate (X_L32 * Y_L32) as a 64bit integer.

	movel	X_L32(%sp), %a0	| save X_L32
	movel	Y_L32(%sp), %a1	| save Y_L32

	movel	%a0, %d2	| prepare for X_L32(H) in L
	movel	%a1, %d3	| prepare for Y_L32(H) in L

	movel	%a0, %d4	| X_L32(L) in L
	movel	%a1, %d1	| Y_L32(L) in L
	movel	%a0, %d0	| X_L32(L) in L

	swap	%d2		| X_L32(H) in L
	swap	%d3 		| Y_L32(H) in L

	muluw	%d1, %d4	| A = X_L32(L) * Y_L32(L)
	muluw	%d2, %d1	| B = X_L32(H) * Y_L32(L)
	muluw	%d3, %d2	| C = X_L32(H) * Y_L32(H)
	muluw	%d0, %d3	| D = X_L32(L) * Y_L32(H)

	movel	%d4, %d0	| extract A(H)
	clrw	%d0
	swap	%d0

	addl	%d0, %d1	| B += A(H) (no carry; max 0xffff0000)

	addl	%d3, %d1	| B += D
	bccs	1f		| if (carry)
	addil	#0x10000, %d2	| 	C += 0x10000

1:	swap	%d1		| B(H) <--> B(L)

| (%d0), (%d1), %d2 = C, %d3 = free, %d4 = A

	clrl	%d3		| extract B(H)
	movew	%d1, %d3

	movew	%d4, %d1	| %d1 = (B(L) << 16) + A(L)

	addl	%d3, %d2	| C += B(H)

| We have (X_L32 * Y_L32) in %d2:%d1. Lower 32bit was completed.
| Add (X_L32 * Y_H32 + X_H32 * Y_L32) to higher 32bit.
|
| (%d0), (%d1), %d2 = C, %d3 = free, %d4 = free

	movel	%a0, %d0	| restore X_L32
	movel	%a1, %d3	| restore Y_L32
	mulsl	Y_H32(%sp), %d0	| E = X_L32 * Y_H32
	mulsl	X_H32(%sp), %d3 | F = X_H32 * Y_L32
	addl	%d2, %d0	| E += C
	addl	%d3, %d0	| %d0 = E + F

	moveml	(%sp)+, %d2-%d4	| pop %d2-%d4
	rts
END(__muldi3)
