/*	$NetBSD: startprog.S,v 1.4.18.1 2009/09/16 19:23:18 matt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch and Simon Burge.
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
 * startprog(entry, stack, argc, argv, prom_magic, prom_ptr, bi_magic, bi_ptr);
 *
 * load new stack pointer, then call
 *  entry(argc, argv, prom_magic, prom_ptr, bi_magic, bi_ptr);
 */

#include <mips/asm.h>

LEAF(startprog)
        subu    sp,sp,CALLFRAME_SIZ
	addu	t2,a1,-CALLFRAME_SIZ	# new stack value
	move	t9,a0
	move	a0,a2
	move	a1,a3

#ifdef __mips_o32
	lw	a2,48(sp)		# load everything from old stack we need
	lw	a3,52(sp)
	lw	$8,56(sp)		# use a4/t0 in case N32/N64 kernel
	lw	$9,60(sp)		# use a5/t1 in case N32/N64 kernel
#else
	move	a2,a4
	move	a3,a5
	move	a4,a6
	move	a5,a7
#endif

	move	sp,t2			# new stack pointer
	sw	ra,CALLFRAME_RA(sp)

	sw	$8,16(sp)		# save on stack for O32
	sw	$9,20(sp)		# save on stack for O32
	jal	ra,t9
	nop				# BDslot

	lw	ra,CALLFRAME_RA(sp)	# should not get back here!
	j	ra
	addu	sp,sp,CALLFRAME_SIZ
END(startprog)
