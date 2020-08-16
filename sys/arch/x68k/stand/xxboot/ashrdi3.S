/*	$NetBSD: ashrdi3.S,v 1.1 2020/08/16 06:43:43 isaki Exp $	*/

/*
 * Copyright (C) 2020 Tetsuya Isaki. All rights reserved.
 * Copyright (C) 2020 Y.Sugahara (moveccr). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Size optimized version for primary bootloader.
 */

#include <machine/asm.h>

ASENTRY_NOPROFILE(__ashrdi3)
		moveml	%sp@(4),%d0-%d1/%a0	| %d0:%d1 = quad value
						| %a0     = shift count
		jbra	start
loop:
		asrl	#1,%d0			| %d0:X >>>= 1
		roxrl	#1,%d1			| X:%d1 >>= 1
start:
		subql	#1,%a0			| sub %a0 doesn't affect ccr,
		tstl	%a0			|  but this extra TST op is
						|  smaller than push/pop %d2.
		jpl	loop
		rts


#if defined(SELFTEST)
#include "iocscall.h"
		.macro	PRINT	msg
		leal	\msg,%a1
		IOCS(__B_PRINT)
		.endm

		.macro	TEST	name
		leal	\name,%a2
		jbsr	test
		.endm

ASENTRY_NOPROFILE(selftest_ashrdi3)
		moveml	%d2-%d7/%a2-%a6,%sp@-
		PRINT	%pc@(msg_testname)

		TEST	test0
		TEST	test1p
		TEST	test1m
		TEST	test4p
		TEST	test4m
		TEST	test63p
		TEST	test63m

		PRINT	%pc@(msg_crlf)
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts

test:
		moveml	%a2@+,%d0-%d2		| %d0:%d1 = value
						| %d2     = count
		moveml	%d0-%d2,%sp@-
		jbsr	__ashrdi3
		leal	%sp@(12),%sp

		cmpl	%a2@+,%d0		| compare high word
		jne	fail
		cmpl	%a2@+,%d1		| compare low word
		jne	fail
		PRINT	%pc@(msg_ok)
		rts
fail:
		PRINT	%pc@(msg_fail)
		rts

test0:		| count = 0
		.long	0x11223344, 0x55667788
		.long	0
		.long	0x11223344, 0x55667788

test1p:		| count = 1
		.long	0x11223344, 0x55667788
		.long	1
		.long	0x089119a2, 0x2ab33bc4

test1m:		| count = 1 (negative value)
		.long	0x91223344, 0x55667788
		.long	1
		.long	0xc89119a2, 0x2ab33bc4

test4p:		| count = 4
		.long	0x11223344, 0x55667788
		.long	4
		.long	0x01122334, 0x45566778

test4m:		| count = 4 (negative value)
		.long	0x91223344, 0x55667788
		.long	4
		.long	0xf9122334, 0x45566778

test63p:	| count = 63
		.long	0x41223344, 0x55667788
		.long	63
		.long	0x00000000, 0x00000000

test63m:	| count = 63 (negative value)
		.long	0x91223344, 0x55667788
		.long	63
		.long	0xffffffff, 0xffffffff

msg_testname:
		.asciz	"__ashrdi3"
msg_ok:
		.asciz	" ok"
msg_fail:
		.asciz	" fail"
msg_crlf:
		.asciz	"\r\n"

#endif
