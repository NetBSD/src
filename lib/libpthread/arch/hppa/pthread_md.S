/*	$NetBSD: pthread_md.S,v 1.1 2009/05/16 22:20:40 ad Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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

LEAF_ENTRY(pthread__ras_simple_lock_init)
	ldi	1,%ret0			/* 1 == unlocked */
	stw	%ret0,0(%arg0)
	stw	%ret0,4(%arg0)
	stw	%ret0,8(%arg0)
	stw	%ret0,12(%arg0)
	bv,n	%r0(%rp)
EXIT(pthread__ras_simple_lock_init)

	.global pthread__lock_ras_start
	.global pthread__lock_ras_end

LEAF_ENTRY(pthread__ras_simple_lock_try)
	ldo	15(%arg0),%arg0
	depi	0,31,4,%arg0

pthread__lock_ras_start:
	ldw	0(%arg0),%ret0
	stw	%r0,0(%arg0)		/* 0 == locked */
pthread__lock_ras_end:

	comiclr,= 0,%ret0,%ret0		/* if locked return 0 */
	ldi	1,%ret0			/* else return 1 */
	bv,n	%r0(%rp)
EXIT(pthread__ras_simple_lock_try)


LEAF_ENTRY(pthread__ras_simple_unlock)
	ldo	15(%arg0),%arg0
	depi	0,31,4,%arg0
	ldi	1,%ret0			/* 1 == unlocked */
	bv	%r0(%rp)
	stw	%ret0,0(%arg0)
EXIT(pthread__ras_simple_unlock)
