/*	$NetBSD: dbregs.h,v 1.6 2018/07/26 09:29:08 maxv Exp $	*/

/*
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

#ifndef	_X86_DBREGS_H_
#define	_X86_DBREGS_H_

#include <sys/param.h>
#include <sys/types.h>
#include <machine/reg.h>

/*
 * CPU Debug Status Register (DR6)
 *
 * Reserved bits: 4-12 and on x86_64 32-64
 */
#define X86_DR6_DR0_BREAKPOINT_CONDITION_DETECTED	__BIT(0)
#define X86_DR6_DR1_BREAKPOINT_CONDITION_DETECTED	__BIT(1)
#define X86_DR6_DR2_BREAKPOINT_CONDITION_DETECTED	__BIT(2)
#define X86_DR6_DR3_BREAKPOINT_CONDITION_DETECTED	__BIT(3)
#define X86_DR6_DEBUG_REGISTER_ACCESS_DETECTED		__BIT(13)
#define X86_DR6_SINGLE_STEP				__BIT(14)
#define X86_DR6_TASK_SWITCH				__BIT(15)

/*
 * CPU Debug Control Register (DR7)
 *
 * LOCAL_EXACT_BREAKPOINT and GLOBAL_EXACT_BREAKPOINT are no longer used
 * since the P6 processor family - portable code should set these bits
 * unconditionally in order to get exact breakpoints.
 *
 * Reserved bits: 10, 12, 14-15 and on x86_64 32-64.
 */
#define X86_DR7_LOCAL_DR0_BREAKPOINT		__BIT(0)
#define X86_DR7_GLOBAL_DR0_BREAKPOINT		__BIT(1)
#define X86_DR7_LOCAL_DR1_BREAKPOINT		__BIT(2)
#define X86_DR7_GLOBAL_DR1_BREAKPOINT		__BIT(3)
#define X86_DR7_LOCAL_DR2_BREAKPOINT		__BIT(4)
#define X86_DR7_GLOBAL_DR2_BREAKPOINT		__BIT(5)
#define X86_DR7_LOCAL_DR3_BREAKPOINT		__BIT(6)
#define X86_DR7_GLOBAL_DR3_BREAKPOINT		__BIT(7)
#define X86_DR7_LOCAL_EXACT_BREAKPOINT		__BIT(8)
#define X86_DR7_GLOBAL_EXACT_BREAKPOINT		__BIT(9)
#define X86_DR7_RESTRICTED_TRANSACTIONAL_MEMORY	__BIT(11)
#define X86_DR7_GENERAL_DETECT_ENABLE		__BIT(13)

#define X86_DR7_DR0_CONDITION_MASK		__BITS(16, 17)
#define X86_DR7_DR0_LENGTH_MASK			__BITS(18, 19)
#define X86_DR7_DR1_CONDITION_MASK		__BITS(20, 21)
#define X86_DR7_DR1_LENGTH_MASK			__BITS(22, 23)
#define X86_DR7_DR2_CONDITION_MASK		__BITS(24, 25)
#define X86_DR7_DR2_LENGTH_MASK			__BITS(26, 27)
#define X86_DR7_DR3_CONDITION_MASK		__BITS(28, 29)
#define X86_DR7_DR3_LENGTH_MASK			__BITS(30, 31)

/*
 * X86_DR7_CONDITION_IO_READWRITE is currently unused. It requires DE
 * (debug extension) flag in control register CR4 set, and not all CPUs
 * support it.
 */
enum x86_dr7_condition {
	X86_DR7_CONDITION_EXECUTION		= 0x0,
	X86_DR7_CONDITION_DATA_WRITE		= 0x1,
	X86_DR7_CONDITION_IO_READWRITE		= 0x2,
	X86_DR7_CONDITION_DATA_READWRITE	= 0x3
};

/*
 * 0x2 is currently unimplemented - it reflects 8 bytes on modern CPUs.
 */
enum x86_dr7_length {
	X86_DR7_LENGTH_BYTE		= 0x0,
	X86_DR7_LENGTH_TWOBYTES		= 0x1,
	/* 0x2 undefined */
	X86_DR7_LENGTH_FOURBYTES	= 0x3
};

/*
 * The number of available watchpoint/breakpoint registers available since
 * Intel 80386. New CPUs (x86_64) ship with up to 16 Debug Registers but they
 * still offer the same number of watchpoints/breakpoints.
 */
#define X86_DBREGS	4

void x86_dbregs_init(void);
void x86_dbregs_clear(struct lwp *);
void x86_dbregs_abandon(struct lwp *);
void x86_dbregs_read(struct lwp *, struct dbreg *);
void x86_dbregs_store_dr6(struct lwp *);
int x86_dbregs_user_trap(void);
int x86_dbregs_validate(const struct dbreg *);
void x86_dbregs_write(struct lwp *, const struct dbreg *);
void x86_dbregs_switch(struct lwp *, struct lwp *);

#endif /* !_X86_DBREGS_H_ */
