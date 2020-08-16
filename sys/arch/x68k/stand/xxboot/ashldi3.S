/*	$NetBSD: ashldi3.S,v 1.1 2020/08/16 06:43:43 isaki Exp $	*/

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

ASENTRY_NOPROFILE(__ashldi3)
		moveml	%sp@(4),%d0-%d1/%a0	| %d0:%d1 = quad value
						| %a0     = shift count
		jbra	start
loop:
		lsll	#1,%d1			| X:%d1 =<< 1
		roxll	#1,%d0			| %d0:X =<< 1
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

ASENTRY_NOPROFILE(selftest_ashldi3)
		moveml	%d2-%d7/%a2-%a6,%sp@-
		PRINT	%pc@(msg_testname)

		TEST	test0
		TEST	test1
		TEST	test2
		TEST	test63

		PRINT	%pc@(msg_crlf)
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts

test:
		moveml	%a2@+,%d0-%d2		| %d0:%d1 = value
						| %d2     = count
		moveml	%d0-%d2,%sp@-
		jbsr	__ashldi3
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

test1:		| count = 1
		.long	0x11223344, 0x55667788
		.long	1
		.long	0x22446688, 0xaaccef10

test2:		| count = 2
		.long	0x11223344, 0x55667788
		.long	2
		.long	0x4488cd11, 0x5599de20

test63:		| count = 63
		.long	0x11223344, 0x55667789
		.long	63
		.long	0x80000000, 0x00000000

msg_testname:
		.asciz	"__ashldi3"
msg_ok:
		.asciz	" ok"
msg_fail:
		.asciz	" fail"
msg_crlf:
		.asciz	"\r\n"

#endif
