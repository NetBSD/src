/*	$NetBSD: memcpy.S,v 1.1 2020/08/16 06:43:43 isaki Exp $	*/

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
| void bcopy(const void *src, void *dst, size_t len);
|
ASENTRY_NOPROFILE(bcopy)
		moveml	%sp@,%d0-%d1/%a0-%a1	| %d0: (return address)
						| %d1: src
						| %a0: dst
						| %a1: count
		jbra	memmove_common
|
| void *memcpy(void *dst, const void *src, size_t count);
| void *memmove(void *dst, const void *src, size_t count);
|
ASENTRY_NOPROFILE(memcpy)
ASENTRY_NOPROFILE(memmove)
		moveml	%sp@,%d0-%d1/%a0-%a1	| %d0: (return address)
						| %d1: dst
						| %a0: src
						| %a1: count

		exg	%d1,%a0			| %d1: src
						| %a0: dst
						| %a1: count
memmove_common:
		exg	%d1,%a1			| %d1: count
						| %a0: dst
						| %a1: src
		moveql	#0,%d0
						| %d0: offset in forward mode.
						| %d1: offset in backward mode
						|  and also loop counter.
		jbra	2f
loop:
		| if src(=%a1) > dst(=%a0), then this is forward copy,
		|  %d0 is already forward offset.
		| Otherwise backward copy.  Copy %d1(backward offset) to %d0.
		cmpl	%a1,%a0
		jcs	1f
		movel	%d1,%d0
1:
		moveb	%a1@(%d0.l),%a0@(%d0.l) | %d0: offset
		addql	#1,%d0			| increment forward counter,
						|  though it's pointless in
						|  backward mode
2:
		subql	#1,%d1			| if (--count < 0)
		jcc	loop			|  goto exit
exit:
		rts				| %a0 holds return value (=dst)


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

ASENTRY_NOPROFILE(selftest_memmove)
		moveml	%d2-%d7/%a2-%a6,%sp@-
		PRINT	%pc@(msg_testname)

		TEST	test1
		TEST	test2
		TEST	test3
		TEST	test4

		PRINT	%pc@(msg_crlf)
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts

test:
		movel	%a2@+,buf:W		| initial contents of buffer
		movew	%a2@+,(buf+4):W		|  (6 bytes total)
		movel	%a2@+,%sp@-		| push len
		movel	%a2@+,%sp@-		| push src
		movel	%a2@+,%a3		| keep dst and
		movel	%a3,%sp@-		| push dst
		jbsr	memmove
		leal	%sp@(12),%sp

		cmpal	%a3,%a0			| compare return value
		jne	fail
		movel	%a2@+,%d0		| compare buf[0..3]
		cmpl	buf:W,%d0
		jne	fail
		movew	%a2@+,%d0		| compare buf[4..5]
		cmpw	(buf+4):W,%d0		| compare buf[4..5]
		jne	fail
		PRINT	%pc@(msg_ok)
		rts
fail:
		PRINT	%pc@(msg_fail)
		rts

test1:
		| src=buf+1: 1 2 3 4 5 6
		|               \ \
		| dst=buf+2: 1 2 2 3 5 6
		.byte	1, 2, 3, 4, 5, 6	| initial buf
		.long	2			| len
		.long	buf+1			| src
		.long	buf+2			| dst
		.byte	1, 2, 2, 3, 5, 6	| expected buf

test2:
		| src=buf+2: 1 2 3 4 5 6
		|               / /
		| dst=buf+1: 1 3 4 4 5 6
		.byte	1, 2, 3, 4, 5, 6	| initial buf
		.long	2			| len
		.long	buf+2			| src
		.long	buf+1			| dst
		.byte	1, 3, 4, 4, 5, 6	| expected buf

test3:
		| src == dst
		.byte	1, 2, 3, 4, 5, 6	| initial buf
		.long	2			| len
		.long	buf+1			| src
		.long	buf+1			| dst
		.byte	1, 2, 3, 4, 5, 6	| expected buf

test4:
		| len == 0
		.byte	1, 2, 3, 4, 5, 6	| initial buf
		.long	0			| len
		.long	buf+1			| src
		.long	buf+1			| dst
		.byte	1, 2, 3, 4, 5, 6	| expected buf

msg_testname:
		.asciz	"memmove"
msg_ok:
		.asciz	" ok"
msg_fail:
		.asciz	" fail"
msg_crlf:
		.asciz	"\r\n"

		BSS(buf, 8)
#endif
