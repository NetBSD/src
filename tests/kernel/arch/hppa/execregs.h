/*	$NetBSD: execregs.h,v 1.1 2025/02/27 00:55:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#ifndef	TESTS_KERNEL_ARCH_HPPA_EXECREGS_H
#define	TESTS_KERNEL_ARCH_HPPA_EXECREGS_H

#include <sys/cdefs.h>

#define	NEXECREGS	31

#ifndef _LOCORE

#include <unistd.h>

/*
 * Ordered by struct struct trapframe in sys/arch/hppa/include/frame.h
 * for convenience of auditing.  Must match h_execregs.S.
 */
static const char *const regname[] = {
	"t1",
	"t2",
	/* sp: stack pointer */
	"t3",
	/* various privileged stuff */
	"sar",
	"r1",
	"rp",
	/* r3: frame pointer (set to initial stack pointer) */
	"r4",
	"r5",
	"r6",
	"r70",
	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"r15",
	"r16",
	"r17",
	"r18",
	"t4",
	"arg3",
	"arg2",
	"arg1",
	/* arg0: ps_strings */
	"dp",
	"ret0",
	"ret1",
	"r31",
	"cr27",
	"cr28",
};

__CTASSERT(NEXECREGS == __arraycount(regname));

int	execregschild(char *);
pid_t	spawnregschild(char *, int);

#endif	/* _LOCORE */

#endif	/* TESTS_KERNEL_ARCH_HPPA_EXECREGS_H */
