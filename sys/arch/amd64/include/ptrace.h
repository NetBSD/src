/*	$NetBSD: ptrace.h,v 1.3.100.4 2017/02/05 13:40:02 skrll Exp $	*/

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _AMD64_PTRACE_H_
#define _AMD64_PTRACE_H_

#ifdef __x86_64__
/*
 * i386-dependent ptrace definitions
 */
#define	PT_STEP			(PT_FIRSTMACH + 0)
#define	PT_GETREGS		(PT_FIRSTMACH + 1)
#define	PT_SETREGS		(PT_FIRSTMACH + 2)
#define	PT_GETFPREGS		(PT_FIRSTMACH + 3)
#define	PT_SETFPREGS		(PT_FIRSTMACH + 4)
#define	PT_READ_WATCHPOINT	(PT_FIRSTMACH + 5)
#define	PT_WRITE_WATCHPOINT	(PT_FIRSTMACH + 6)
#define	PT_COUNT_WATCHPOINTS	(PT_FIRSTMACH + 7)

#define PT_MACHDEP_STRINGS \
	"PT_STEP", \
	"PT_GETREGS", \
	"PT_SETREGS", \
	"PT_GETFPREGS", \
	"PT_SETFPREGS", \
	"PT_READ_WATCHPOINT", \
	"PT_WRITE_WATCHPOINT", \
	"PT_COUNT_WATCHPOINTS"

#include <machine/reg.h>
#define PTRACE_REG_PC(r)	(r)->regs[_REG_RIP]
#define PTRACE_REG_SET_PC(r, v)	(r)->regs[_REG_RIP] = (v)
#define PTRACE_REG_SP(r)	(r)->regs[_REG_RSP]
#define PTRACE_REG_INTRV(r)	(r)->regs[_REG_RAX]

#define PTRACE_BREAKPOINT	((const uint8_t[]) { 0xcc })
#define PTRACE_BREAKPOINT_SIZE	1
#define PTRACE_BREAKPOINT_ADJ	1

#define __HAVE_PTRACE_WATCHPOINTS

/*
 * The current list of supported hardware watchpoints
 */
#define PTRACE_PW_TYPE_DBREGS	1

struct mdpw {
	union {
		/* Debug Registers DR0-3, DR6, DR7 */
		struct {
			void	*_md_address;
			int	 _md_condition;
			int	 _md_length;
		} _dbregs;
	} _type;
};

/*
 * This MD structure translates into x86_hw_watchpoint
 *
 * pw_address - 0 represents disabled hardware watchpoint
 *
 * conditions:
 *     0b00 - execution
 *     0b01 - data write
 *     0b10 - io read/write (not implemented)
 *     0b11 - data read/write
 *
 * length:
 *     0b00 - 1 byte
 *     0b01 - 2 bytes
 *     0b10 - undefined (8 bytes in modern CPUs - not implemented)
 *     0b11 - 4 bytes
 *
 * Helper symbols for conditions and length are available in <x86/dbregs.h>
 *
 */

#define	md_address	_type._dbregs._md_address
#define	md_condition	_type._dbregs._md_condition
#define	md_length	_type._dbregs._md_length


#ifdef _KERNEL_OPT
#include "opt_compat_netbsd32.h"

#ifdef COMPAT_NETBSD32
#include <machine/netbsd32_machdep.h>

#define process_read_regs32	netbsd32_process_read_regs
#define process_read_fpregs32	netbsd32_process_read_fpregs

#define process_write_regs32	netbsd32_process_write_regs
#define process_write_fpregs32	netbsd32_process_write_fpregs

#define process_write_watchpoint32	netbsd32_process_write_watchpoint
#define process_read_watchpoint32	netbsd32_process_read_watchpoint
#define process_count_watchpoint32	netbsd32_process_count_watchpoint

#define process_reg32		struct reg32
#define process_fpreg32		struct fpreg32
#define process_watchpoint32	struct ptrace_watchpoint32
#endif	/* COMPAT_NETBSD32 */
#endif	/* _KERNEL_OPT */

#else	/* !__x86_64__ */

#include <i386/ptrace.h>

#endif	/* __x86_64__ */

#endif	/* _AMD64_PTRACE_H_ */
