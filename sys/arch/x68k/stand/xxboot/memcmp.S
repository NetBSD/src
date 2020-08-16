/*	$NetBSD: memcmp.S,v 1.1 2020/08/16 06:43:43 isaki Exp $	*/

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
 * Size optimized (but slow) version for primary bootloader.
 */

#include <machine/asm.h>

|
| int memcmp(const void *b1, const void *b2, size_t len)
|
ASENTRY_NOPROFILE(memcmp)
		moveml	%sp@,%d0-%d1/%a0-%a1	| %d0: (return address)
						| %d1: b1
						| %a0: b2
						| %a1: len

		exg	%d1,%a1			| %d1: len
						| %a0: b2
						| %a1: b1
		moveql	#0,%d0
loop:
		subql	#1,%d1			| if (--len < 0)
		jcs	exit			|  goto exit
compare:
		cmpmb	%a0@+,%a1@+
		jeq	loop
		| To comply with standards, recalc exact return value.
		| Although everyone expects only 0 or not...
		moveq	#0,%d1
		moveb	%a1@-,%d0
		moveb	%a0@-,%d1
		subl	%d1,%d0			| (uint)*b1 - (uint)*b2
exit:
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

ASENTRY_NOPROFILE(selftest_memcmp)
		moveml	%d2-%d7/%a2-%a6,%sp@-
		PRINT	%pc@(msg_testname)

		TEST	test1
		TEST	test2
		TEST	test3
		TEST	test4
		TEST	test5

		PRINT	%pc@(msg_crlf)
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts

test:
		movel	%a2@+,buf:W		| contents of b1
		movel	%a2@+,(buf+4):W		| contents of b2
		movel	%a2@+,%sp@-		| push len
		peal	(buf+4):W		| push b2
		peal	(buf):W			| push b1
		jbsr	memcmp
		leal	%sp@(12),%sp

		cmpl	%a2@,%d0		| compare return value
		jne	fail
		PRINT	%pc@(msg_ok)
		rts
fail:
		PRINT	%pc@(msg_fail)
		rts

test1:
		| b1 == b2 within length
		.long 	0x11223344		| b1
		.long	0x11223355		| b2
		.long	3			| len
		.long	0			| expected

test2:
		| b1 > b2 before the last
		.long	0x11813344		| b1
		.long	0x11013344		| b2
		.long	3			| len
		.long	0x00000080		| expected

test3:
		| b1 < b2 in the last byte
		.long	0x11220044		| b1
		.long	0x11220144		| b2
		.long	3			| len
		.long	-1			| expected

test4:		| len == 0
		.long	0x11223344		| b1
		.long	0x11223344		| b2
		.long	0			| len
		.long	0			| expected

test5:		| *b1 - *b2 = 0 - 255 = -255
		.long	0x00000000		| b1
		.long	0xff000000		| b2
		.long	1			| len
		.long	-255			| expected

msg_testname:
		.asciz	"memcmp"
msg_ok:
		.asciz	" ok"
msg_fail:
		.asciz	" fail"
msg_crlf:
		.asciz	"\r\n"

		BSS(buf, 8)
#endif
