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
 * $FreeBSD: head/sys/cddl/dev/dtrace/arm/dtrace_isa.c 295882 2016-02-22 09:08:04Z skra $
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

uintptr_t kernelbase = (uintptr_t)KERNEL_BASE;

/* TODO: support AAPCS */
/* XXX: copied from sys/arch/arm/arm/db_trace.c */
#define INKERNEL(va)	(((vaddr_t)(va)) >= VM_MIN_KERNEL_ADDRESS)

#define FR_SCP	(0)
#define FR_RLV	(-1)
#define FR_RSP	(-2)
#define FR_RFP	(-3)

#include "regset.h"

/*
 * Wee need some reasonable default to prevent backtrace code
 * from wandering too far
 */
#define	MAX_FUNCTION_SIZE 0x10000
#define	MAX_PROLOGUE_SIZE 0x100


uint8_t dtrace_fuword8_nocheck(void *);
uint16_t dtrace_fuword16_nocheck(void *);
uint32_t dtrace_fuword32_nocheck(void *);
uint64_t dtrace_fuword64_nocheck(void *);

void
dtrace_getpcstack(pc_t *pcstack, int pcstack_limit, int aframes,
    uint32_t *intrpc)
{
	uint32_t	*frame, *lastframe;
#if 0
	int	scp_offset;
#endif
	int	depth = 0;
	pc_t caller = (pc_t) solaris_cpu[cpu_number()].cpu_dtrace_caller;

	if (intrpc != 0)
		pcstack[depth++] = (pc_t) intrpc;

	aframes++;

	frame = (uint32_t *)__builtin_frame_address(0);;
	lastframe = NULL;
#if 0
	scp_offset = -(get_pc_str_offset() >> 2);
#endif

	while ((frame != NULL) && (depth < pcstack_limit)) {
		db_addr_t	scp;
#if 0 
		uint32_t	savecode;
		int		r;
		uint32_t	*rp;
#endif

		/*
		 * In theory, the SCP isn't guaranteed to be in the function
		 * that generated the stack frame.  We hope for the best.
		 */
		scp = frame[FR_SCP];
		if (aframes > 0) {
			aframes--;
			if ((aframes == 0) && (caller != 0)) {
				pcstack[depth++] = caller;
			}
		}
		else {
			pcstack[depth++] = scp;
		}

#if 0
		savecode = ((uint32_t *)scp)[scp_offset];
		if ((savecode & 0x0e100000) == 0x08000000) {
			/* Looks like an STM */
			rp = frame - 4;
			for (r = 10; r >= 0; r--) {
				if (savecode & (1 << r)) {
					/* register r == *rp-- */
				}
			}
		}
#endif

		/*
		 * Switch to next frame up
		 */
		if (frame[FR_RFP] == 0)
			break; /* Top of stack */

		lastframe = frame;
		frame = (uint32_t *)(frame[FR_RFP]);

		if (INKERNEL((int)frame)) {
			/* staying in kernel */
			if (frame <= lastframe) {
				/* bad frame pointer */
				break;
			}
		}
		else
			break;
	}

	for (; depth < pcstack_limit; depth++) {
		pcstack[depth] = 0;
	}
}

void
dtrace_getupcstack(uint64_t *pcstack, int pcstack_limit)
{
	printf("IMPLEMENT ME: %s\n", __func__);
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
/*	struct arm_frame *fp = (struct arm_frame *)dtrace_getfp();*/

	printf("IMPLEMENT ME: %s\n", __func__);
	return (0);
}

int
dtrace_getstackdepth(int aframes)
{
	uint32_t	*frame, *lastframe;
	int	depth = 1;

	frame = (uint32_t *)__builtin_frame_address(0);;
	lastframe = NULL;

	while (frame != NULL) {
#if 0 
		uint32_t	savecode;
		int		r;
		uint32_t	*rp;
#endif

		depth++;

		/*
		 * Switch to next frame up
		 */
		if (frame[FR_RFP] == 0)
			break; /* Top of stack */

		lastframe = frame;
		frame = (uint32_t *)(frame[FR_RFP]);

		if (INKERNEL((int)frame)) {
			/* staying in kernel */
			if (frame <= lastframe) {
				/* bad frame pointer */
				break;
			}
		}
		else
			break;
	}

	if (depth < aframes)
		return 0;
	else
		return depth - aframes;

}

ulong_t
dtrace_getreg(struct trapframe *rp, uint_t reg)
{
	printf("IMPLEMENT ME: %s\n", __func__);

	return (0);
}

static int
dtrace_copycheck(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{

	if (uaddr + size > VM_MAXUSER_ADDRESS || uaddr + size < uaddr) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_number()].cpuc_dtrace_illval = uaddr;
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
		cpu_core[cpu_number()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword8_nocheck(uaddr));
}

uint16_t
dtrace_fuword16(void *uaddr)
{
	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_number()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword16_nocheck(uaddr));
}

uint32_t
dtrace_fuword32(void *uaddr)
{
	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_number()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword32_nocheck(uaddr));
}

uint64_t
dtrace_fuword64(void *uaddr)
{
	if ((uintptr_t)uaddr > VM_MAXUSER_ADDRESS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_number()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword64_nocheck(uaddr));
}
