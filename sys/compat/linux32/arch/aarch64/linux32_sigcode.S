/*	$NetBSD: linux32_sigcode.S,v 1.1 2021/11/25 03:08:04 ryo Exp $	*/

/*-
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

RCSID("$NetBSD: linux32_sigcode.S,v 1.1 2021/11/25 03:08:04 ryo Exp $")

#include <compat/linux32/linux32_syscall.h>

/*
 * linux aarch32 Signal trampoline code
 */
	.text
	.section .rodata
	.align 12

	.global	_C_LABEL(linux32_sigcode)
_C_LABEL(linux32_sigcode):
	/* mov r7, #LINUX32_SYS_sigreturn */
	.word	0xe3a07000 + LINUX32_SYS_sigreturn
	.word	0xef000000	/* svc 0   */
	/* NOTREACHED */
	.word	0xe7f000f0	/* udf #0  */

	.global	_C_LABEL(linux32_rt_sigcode)
_C_LABEL(linux32_rt_sigcode):
	/* mov r7, #LINUX32_SYS_rt_sigreturn */
	.word	0xe3a07000 + LINUX32_SYS_rt_sigreturn
	.word	0xef000000	/* svc 0   */
	/* NOTREACHED */
	.word	0xe7f000f0	/* udf #0  */

/*
 * linux/arm kuser_helper
 *   - should be mapped on userspace vaddr 0xffff0f60
 *   - https://www.kernel.org/doc/Documentation/arm/kernel_user_helpers.txt
 */

.kuser_helper_pad:
#define KUSER_HELPER_START	0x00000f60
	.space	KUSER_HELPER_START - (.kuser_helper_pad - _C_LABEL(linux32_sigcode))

/*
 * 0xffff0f60
 * int __kuser_cmpxchg64(const int64_t *oldval, const int64_t *newval, volatile int64_t *ptr);
 */
.__kuser_cmpxchg64:
	.word	0xe92d00f0	/* push {r4, r5, r6, r7}  */
	.word	0xf57ff05f	/* dmb sy                 */
	.word	0xe1c040d0	/* ldrd r4, [r0]          */
	.word	0xe1c160d0	/* ldrd r6, [r1]          */
	.word	0xe1b20f9f	/* ldrexd r0, [r2]        */
	.word	0xe0500004	/* subs r0, r0, r4        */
	.word	0xe0c11005	/* sbc r1, r1, r5         */
	.word	0xe1900001	/* orrs r0, r0, r1        */
	.word	0x01a20f96	/* strexdeq r0, r6, [r2]  */
	.word	0xf57ff05f	/* dmb sy                 */
	.word	0xe8bd00f0	/* pop {r4, r5, r6, r7}   */
	.word	0xe12fff1e	/* bx lr                  */

	.align	5
/*
 * 0xffff0fa0
 * void __kuser_memory_barrier(void);
 */
.__kuser_memory_barrier:
	.word	0xf57ff05f	/* dmb sy  */
	.word	0xe12fff1e	/* bx lr   */

	.align	5
/*
 * 0xffff0fc0
 * int __kuser_cmpxchg(int32_t oldval, int32_t newval, volatile int32_t *ptr);
 */
.__kuser_cmpxchg:
	.word	0xf57ff05f	/* dmb sy                */
	.word	0xe1923f9f	/* ldrex r3, [r2]        */
	.word	0xe0530000	/* subs r0, r3, r0       */
	.word	0x01820f91	/* strexeq r0, r1, [r2]  */
	.word	0xf57ff05f	/* dmb sy                */
	.word	0xe12fff1e	/* bx lr                 */


	.align	5
/* 0xffff0fe0 */
.__kuser_get_tls:
	.word	0xee1d0f70	/* mrc p15, 0, r0, c13, c0, 3  */
	.word	0xe12fff1e	/* bx lr                       */

	.align	4
/* 0xffff0ff0 */
	.word	0
/* 0xffff0ff4 */
	.word	0
/* 0xffff0ff8 */
	.word	0

/* 0xffff0ffc */
.__kuser_helper_version:
	.word	2


	.global	_C_LABEL(linux32_esigcode)
_C_LABEL(linux32_esigcode):
