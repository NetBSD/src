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

#ifndef	TESTS_KERNEL_ARCH_VAX_EXECREGS_H
#define	TESTS_KERNEL_ARCH_VAX_EXECREGS_H

#include <sys/cdefs.h>

#define	NEXECREGS	12

#ifndef _LOCORE

#include <unistd.h>

/*
 * The order matches that in struct trapframe in
 * sys/arch/vax/include/trap.h
 *
 * Must match h_execregs.S.
 *
 * See also sys/arch/vax/vax/trap.c:setregs()
 */
static const char *const regname[] = {
	"fp",	       /* Stack frame pointer */
	"ap",	       /* Argument pointer on user stack */
	/* sp: stack pointer */
	"r0",	       /* General registers saved upon trap/syscall */
	"r1",
	"r2",
	"r3",
	"r4",
	"r5",
	/* r6: initial stack pointer */
	"r7",
	"r8",
	/* r9: ps_strings */
	"r10",
	"r11",
	/* trap: type of trap, not a register */
	/* code: trap specific code, not a register */
	/* pc: user PC */
	/* psl: processor status longword */
};

__CTASSERT(NEXECREGS == __arraycount(regname));

int	execregschild(char *);
pid_t	spawnregschild(char *, int);

#endif	/* _LOCORE */

#endif	/* TESTS_KERNEL_ARCH_VAX_EXECREGS_H */
