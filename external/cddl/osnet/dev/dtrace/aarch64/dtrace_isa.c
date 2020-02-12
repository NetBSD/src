/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * $FreeBSD$
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/frame.h>
#include <machine/reg.h>

#include <machine/db_machdep.h>
#include <machine/vmparam.h>
#include <ddb/db_sym.h>
#include <ddb/ddb.h>

#include "regset.h"

#define	INKERNEL(va)	((VM_MIN_KERNEL_ADDRESS <= (va)) && ((va) < VM_MAX_KERNEL_ADDRESS))
#define	CURRENT_CPU	cpu_index(curcpu())

struct unwind_state {
	register_t fp;
	register_t sp;
	register_t pc;
};

struct arm64_frame {
	struct arm64_frame	*f_frame;
	uintptr_t		f_retaddr;
};

static int
unwind_frame(struct unwind_state *frame)
{
	uint64_t fp = frame->fp;

	if (!INKERNEL(fp))
		return (-1);

	frame->sp = fp + 0x10;
	/* FP to previous frame (X29) */
	frame->fp = *(uint64_t *)(fp);
	/* LR (X30) */
	frame->pc = *(uint64_t *)(fp + 8) - 4;

	return (0);
}

/*
 * Wee need some reasonable default to prevent backtrace code
 * from wandering too far
 */
#define	MAX_FUNCTION_SIZE 0x10000
#define	MAX_PROLOGUE_SIZE 0x100
#define	MAX_USTACK_DEPTH  2048

uint8_t dtrace_fuword8_nocheck(void *);
uint16_t dtrace_fuword16_nocheck(void *);
uint32_t dtrace_fuword32_nocheck(void *);
uint64_t dtrace_fuword64_nocheck(void *);

void
dtrace_getpcstack(pc_t *pcstack, int pcstack_limit, int aframes,
    uint32_t *intrpc)
{
	extern const char el1_trap_exit[];
	const register_t *fp;
	int i = 0;

	if (intrpc) {
		if (i < pcstack_limit)
			pcstack[i++] = (pc_t)intrpc;
	}

	/*
	 * fp[0] = x29 (saved frame pointer)
	 * fp[1] = x30 (saved link register == return address)
	 */
	fp = __builtin_frame_address(0);
	while (i < pcstack_limit && INKERNEL(fp[0]) && INKERNEL(fp[1])) {
		/* Skip the specified number of artificial frames.  */
		if (aframes > 0)
			aframes--;
		else
			pcstack[i++] = fp[1];

		/* Check whether this frame is handling a trap.  */
		if (fp[1] == (register_t)el1_trap_exit) {
			/*
			 * Trap from kernel.  The trapframe is the
			 * saved frame pointer of the call to the trap
			 * handler whose return address is
			 * el1_trap_exit.  The frame pointer of the
			 * interrupted code is in x29 stashed in the
			 * trapframe, alongside its pc.
			 */
			const struct trapframe *tf = (const void *)fp[0];
			/* x29 = frame pointer */
			fp = (const void *)tf->tf_regs.r_reg[29];
			if (INKERNEL(tf->tf_pc)) {
				if (i >= pcstack_limit)
					break;
				if (aframes > 0)
					aframes--;
				else
					pcstack[i++] = tf->tf_pc;
			}
		} else {
			/*
			 * Not a trap.  Keep going with fp[0] as the
			 * parent frame pointer.
			 */
			fp = (const void *)fp[0];
		}
	}

	/* Zero the rest of the return address stack.  (Paranoia?)  */
	while (i < pcstack_limit)
		pcstack[i++] = 0;
}

static int
dtrace_getustack_common(uint64_t *pcstack, int pcstack_limit, uintptr_t pc,
    uintptr_t fp)
{
	volatile uint16_t *flags =
	    (volatile uint16_t *)&cpu_core[CURRENT_CPU].cpuc_dtrace_flags;
	int ret = 0;
	uintptr_t oldfp = fp;

	ASSERT(pcstack == NULL || pcstack_limit > 0);

	while (pc != 0) {
		/*
		 * We limit the number of times we can go around this
		 * loop to account for a circular stack.
		 */
		if (ret++ >= MAX_USTACK_DEPTH) {
			*flags |= CPU_DTRACE_BADSTACK;
			cpu_core[CURRENT_CPU].cpuc_dtrace_illval = fp;
			break;
		}

		if (pcstack != NULL) {
			*pcstack++ = (uint64_t)pc;
			pcstack_limit--;
			if (pcstack_limit <= 0)
				break;
		}

		if (fp == 0)
			break;

		pc = dtrace_fuword64((void *)(fp +
		    offsetof(struct arm64_frame, f_retaddr)));
		fp = dtrace_fuword64((void *)fp);

		if (fp == oldfp) {
			*flags |= CPU_DTRACE_BADSTACK;
			cpu_core[CURRENT_CPU].cpuc_dtrace_illval = fp;
			break;
		}

		/*
		 * ARM64TODO:
		 *     This workaround might not be necessary. It needs to be
		 *     revised and removed from all architectures if found
		 *     unwanted. Leaving the original x86 comment for reference.
		 *
		 * This is totally bogus:  if we faulted, we're going to clear
		 * the fault and break.  This is to deal with the apparently
		 * broken Java stacks on x86.
		 */
		if (*flags & CPU_DTRACE_FAULT) {
			*flags &= ~CPU_DTRACE_FAULT;
			break;
		}

		oldfp = fp;
	}

	return (ret);
}

void
dtrace_getupcstack(uint64_t *pcstack, int pcstack_limit)
{
	proc_t *p = curproc;
	struct trapframe *tf;
	uintptr_t pc, sp, fp;
	volatile uint16_t *flags =
	    (volatile uint16_t *)&cpu_core[CURRENT_CPU].cpuc_dtrace_flags;
	int n;

	if (*flags & CPU_DTRACE_FAULT)
		return;

	if (pcstack_limit <= 0)
		return;

	/*
	 * If there's no user context we still need to zero the stack.
	 */
	if (p == NULL || (tf = curlwp->l_md.md_utf) == NULL)
		goto zero;

	*pcstack++ = (uint64_t)p->p_pid;
	pcstack_limit--;

	if (pcstack_limit <= 0)
		return;

	pc = tf->tf_pc;
	sp = tf->tf_sp;
	fp = tf->tf_regs.r_reg[29];

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
		/*
		 * In an entry probe.  The frame pointer has not yet been
		 * pushed (that happens in the function prologue).  The
		 * best approach is to add the current pc as a missing top
		 * of stack and back the pc up to the caller, which is stored
		 * at the current stack pointer address since the call
		 * instruction puts it there right before the branch.
		 */

		*pcstack++ = (uint64_t)pc;
		pcstack_limit--;
		if (pcstack_limit <= 0)
			return;

		pc = tf->tf_lr;
	}

	n = dtrace_getustack_common(pcstack, pcstack_limit, pc, fp);
	ASSERT(n >= 0);
	ASSERT(n <= pcstack_limit);

	pcstack += n;
	pcstack_limit -= n;

zero:
	while (pcstack_limit-- > 0)
		*pcstack++ = 0;
}

int
dtrace_getustackdepth(void)
{

	printf("IMPLEMENT ME: %s\n", __func__);

	return (0);
}

void
dtrace_getufpstack(uint64_t *pcstack, uint64_t *fpstack, int pcstack_limit)
{

	printf("IMPLEMENT ME: %s\n", __func__);
}

/*ARGSUSED*/
uint64_t
dtrace_getarg(int arg, int aframes)
{
	extern const char el1_trap_exit[];
	const register_t *fp;
	const struct trapframe *tf = NULL;
	int i = 0;

	/*
	 * The first arguments are passed in x0,...,x7.  The rest are
	 * on the stack, too much trouble to figure out.
	 *
	 * XXX Shouldn't we ask ctf or dwarf or something to figure
	 * this stuff out for us?
	 */
	KASSERT(arg >= 0);
	if (arg >= 8)
		return 0;

	fp = __builtin_frame_address(0);
	while (i < 1000 && INKERNEL(fp[0]) && INKERNEL(fp[1])) {
		if (aframes > 0)
			aframes--;
		else
			i++;
		if (fp[1] == (register_t)el1_trap_exit) {
			tf = (const void *)fp[0];
			break;
		} else {
			fp = (const void *)fp[0];
		}
	}

	/* If we didn't find a trap frame, give up.  */
	if (tf == NULL)
		return 0;

	/* Arg0, arg1, ..., arg7 are in registers x0, x1, ..., x7.  */
	return tf->tf_regs.r_reg[arg];
}

int
dtrace_getstackdepth(int aframes)
{
	extern const char el1_trap_exit[];
	const register_t *fp;
	int i = 0;

	fp = __builtin_frame_address(0);
	while (i < 1000 && INKERNEL(fp[0]) && INKERNEL(fp[1])) {
		if (aframes > 0)
			aframes--;
		else
			i++;
		if (fp[1] == (register_t)el1_trap_exit) {
			const struct trapframe *tf = (const void *)fp[0];
			fp = (const void *)tf->tf_regs.r_reg[29];
			if (aframes > 0)
				aframes--;
			else
				i++;
		} else {
			fp = (const void *)fp[0];
		}
	}

	return i;
}

ulong_t
dtrace_getreg(struct trapframe *tf, uint_t reg)
{

	if (reg < 32)
		return tf->tf_regs.r_reg[reg];

	return (0);
}

static int
dtrace_copycheck(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{

	if (uaddr + size > VM_MAXUSER_ADDRESS || uaddr + size < uaddr) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CURRENT_CPU].cpuc_dtrace_illval = uaddr;
		return (0);
	}

	return (1);
}

void
dtrace_copyin(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
{

	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copy(uaddr, kaddr, size);
}

void
dtrace_copyout(uintptr_t kaddr, uintptr_t uaddr, size_t size,
    volatile uint16_t *flags)
{

	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copy(kaddr, uaddr, size);
}

void
dtrace_copyinstr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
{

	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copystr(uaddr, kaddr, size, flags);
}

void
dtrace_copyoutstr(uintptr_t kaddr, uintptr_t uaddr, size_t size,
    volatile uint16_t *flags)
{

	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copystr(kaddr, uaddr, size, flags);
}

uint8_t
dtrace_fuword8(void *uaddr)
{

	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CURRENT_CPU].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}

	return (dtrace_fuword8_nocheck(uaddr));
}

uint16_t
dtrace_fuword16(void *uaddr)
{

	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CURRENT_CPU].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}

	return (dtrace_fuword16_nocheck(uaddr));
}

uint32_t
dtrace_fuword32(void *uaddr)
{

	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CURRENT_CPU].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}

	return (dtrace_fuword32_nocheck(uaddr));
}

uint64_t
dtrace_fuword64(void *uaddr)
{

	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CURRENT_CPU].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}

	return (dtrace_fuword64_nocheck(uaddr));
}
