/*	$NetBSD: frame_regs.h,v 1.2 2008/01/04 23:29:54 dsl Exp $	*/

#ifndef _AMD64_FRAME_REGS_H_
#define _AMD64_FRAME_REGS_H_

/*
 * amd64 registers (and friends) ordered as in a trap/interrupt/syscall frame.
 * Also the indexes into the 'general register state' (__greg_t) passed to
 * userland.
 * Historically they were in the same order, but the order in the frames
 * will be changed to improve syscall efficiency.
 *
 * Notes:
 * 1) gdb (src/gnu/dist/gdb6/gdb/amd64nbsd-tdep.c) has a lookup table that
 *    assumes the __greg_t ordering.
 * 2) src/lib/libc/arch/x86_64/gen/makecontext.c assumes that the first
 *    6 entries in the __greg_t array match the registers used to pass
 *    function arguments.
 * 3) PTHREAD_UCONTEXT_TO_REG (src/lib/libpthread/arch/?*?/pthread_md.h)
 *    assumes that something matches the __greg_t layout, but there
 *    are no (obvious) references.
 * 4) There might be other code out there that relies on the ordering.
 */
#define _FRAME_REG(greg, freg) 	\
	greg(rdi, RDI, 0)	\
	greg(rsi, RSI, 1)	\
	greg(rdx, RDX, 2)	\
	greg(rcx, RCX, 3)	\
	greg(r8,  R8,  4)	\
	greg(r9,  R9,  5)	\
	greg(r10, R10, 6)	\
	greg(r11, R11, 7)	\
	greg(r12, R12, 8)	\
	greg(r13, R13, 9)	\
	greg(r14, R14, 10)	\
	greg(r15, R15, 11)	\
	greg(rbp, RBP, 12)	\
	greg(rbx, RBX, 13)	\
	greg(rax, RAX, 14)	\
	greg(gs,  GS,  15)	\
	greg(fs,  FS,  16)	\
	greg(es,  ES,  17)	\
	greg(ds,  DS,  18)	\
	greg(trapno, TRAPNO, 19)	\
	/* below portion defined in hardware */ \
	greg(err, ERR, 20)	/* Dummy inserted if not defined */ \
	greg(rip, RIP, 21)	\
	greg(cs,  CS,  22)	\
	greg(rflags, RFLAGS, 23)	\
	/* These are pushed unconditionally on the x86-64 */ \
	greg(rsp, RSP, 24)	\
	greg(ss,  SS,  25)

#define _FRAME_NOREG(reg, REG, idx)

#define _FRAME_GREG(greg) _FRAME_REG(greg, _FRAME_NOREG)

#endif
