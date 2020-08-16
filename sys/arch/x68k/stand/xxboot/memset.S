/*	$NetBSD: memset.S,v 1.1 2020/08/16 06:43:43 isaki Exp $	*/

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
| void bzero(void *dst, size_t len)
|
ASENTRY_NOPROFILE(bzero)
		moveml	%sp@,%d0-%d1/%a1	| %d0: (return address)
						| %d1: dst
						| %a1: len

		subal	%a0,%a0			| %a0: c = 0
		jbra	memset_common

|
| void *memset(void *dst, int c, size_t len);
|
ASENTRY_NOPROFILE(memset)
		moveml	%sp@,%d0-%d1/%a0-%a1	| %d0: (return address)
						| %d1: dst
						| %a0: c
						| %a1: len
memset_common:
		addal	%d1,%a1			| %a1: loop start+1 address

		exg	%d1,%a0			| %d1: c
						| %a0: initial dst
						|  for return value and
						|  loop-end condition address
		jbra	2f
1:
		moveb	%d1,%a1@-		| memset in reverse direction
2:
		cmpl	%a1,%a0
		jne	1b			| if condition met, %a0 = dst
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

ASENTRY_NOPROFILE(selftest_memset)
		moveml	%d2-%d7/%a2-%a6,%sp@-
		PRINT	%pc@(msg_testname)

		TEST	test1
		TEST	test2

		PRINT	%pc@(msg_crlf)
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts

test:
		movel	%a2@+,buf:W		| initial contents of buffer
		movel	%a2@+,%sp@-		| push len
		movel	%a2@+,%sp@-		| push c
		movel	%a2@+,%a3		| keep dst and
		movel	%a3,%sp@-		| push dst
		jbsr	memset
		leal	%sp@(12),%sp

		cmpal	%a3,%a0			| compare return value
		jne	fail
		movel	%a2@+,%d0		| compare buf[0..4]
		cmpl	buf:W,%d0
		jne	fail
		PRINT	%pc@(msg_ok)
		rts
fail:
		PRINT	%pc@(msg_fail)
		rts

test1:
		.long	0x11223344		| initial buf
		.long	2			| len
		.long	0xaa			| c
		.long	buf+1			| dst
		.long	0x11aaaa44		| expected buf

test2:
		| len == 0
		.long	0x11223344		| initial buf
		.long	0			| len
		.long	0x55			| c
		.long	buf+1			| dst
		.long	0x11223344		| expected buf

msg_testname:
		.asciz	"memset"
msg_ok:
		.asciz	" ok"
msg_fail:
		.asciz	" fail"
msg_crlf:
		.asciz	"\r\n"

		BSS(buf, 8)
#endif
