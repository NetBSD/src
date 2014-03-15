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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#define _ASM
#define _LOCORE
#define LOCORE

#include <sys/cpuvar_defs.h>
#include <sys/dtrace.h>

#include <machine/asm.h>
#include <arm/armreg.h>

/*
void dtrace_membar_producer(void)
*/
ENTRY(dtrace_membar_producer)
	RET
END(dtrace_membar_producer)

/*
void dtrace_membar_consumer(void)
*/
ENTRY(dtrace_membar_consumer)
	RET
END(dtrace_membar_consumer)

/*
dtrace_icookie_t dtrace_interrupt_disable(void)
*/
ENTRY(dtrace_interrupt_disable)
	mrs	r0, cpsr
	mov	r1, r0
	orr	r1, r1, #(I32_bit|F32_bit)
	msr	cpsr_c, r1
	RET
END(dtrace_interrupt_disable)

/*
void dtrace_interrupt_enable(dtrace_icookie_t cookie)
*/
ENTRY(dtrace_interrupt_enable)
	and	r0, r0, #(I32_bit|F32_bit)
	mrs	r1, cpsr
	bic	r1, r1, #(I32_bit|F32_bit)
	orr	r1, r1, r0
	msr	cpsr_c, r1
	RET
END(dtrace_interrupt_enable)

/*
uint32_t dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
XXX: just disable interrupts for now, add proper implementation for
ARMv6/ARMv7 later
*/
ENTRY_NP(dtrace_casptr)
ENTRY(dtrace_cas32)
	stmfd	sp!, {r4, r5}

	mrs	r3, cpsr
	mov	r4, r3
	orr	r4, r4, #(I32_bit|F32_bit)
	msr	cpsr_c, r4

	ldr	r5, [r0]
	cmp	r5, r1
	movne	r0, r5
	bne	2f

	str	r2, [r0]
	mov	r0, r5

2:
	msr	cpsr_c, r3
	ldmfd	sp!, {r4, r5}
	RET
END(dtrace_cas32)
END(dtrace_casptr)

/*
uint8_t
dtrace_fuword8_nocheck(void *addr)
*/
ENTRY(dtrace_fuword8_nocheck)
	ldrb	r3, [r0]
	mov 	r0, r3
	RET
END(dtrace_fuword8_nocheck)

/*
uint16_t
dtrace_fuword16_nocheck(void *addr)
*/
ENTRY(dtrace_fuword16_nocheck)
	ldrh	r3, [r0]
	mov 	r0, r3
	RET
END(dtrace_fuword16_nocheck)

/*
uint32_t
dtrace_fuword32_nocheck(void *addr)
*/
ENTRY(dtrace_fuword32_nocheck)
	ldr	r3, [r0]
	mov 	r0, r3
	RET
END(dtrace_fuword32_nocheck)

/*
uint64_t
dtrace_fuword64_nocheck(void *addr)
XXX: add byteorder check
*/
ENTRY(dtrace_fuword64_nocheck)
	ldm	r0, {r2, r3}

	mov	r0, r2
	mov	r1, r3
#if 0
/* little endian */
	mov	r0, r2
	mov	r1, r3
/* big endian */
	mov	r0, r3
	mov	r1, r2
#endif
	RET
END(dtrace_fuword64_nocheck)

/*
void
dtrace_copy(uintptr_t uaddr, uintptr_t kaddr, size_t size)
*/
ENTRY(dtrace_copy)
	stmfd   sp!, {r4-r5}			/* stack is 8 byte aligned */
	teq     r2, #0x00000000
	mov     r5, #0x00000000
	beq     2f

1:	ldrb    r4, [r0], #0x0001
	add     r5, r5, #0x00000001
	strb    r4, [r1], #0x0001
	teqne   r5, r2
	bne     1b

2:	ldmfd   sp!, {r4-r5}			/* stack is 8 byte aligned */
	RET
END(dtrace_copy)


/*
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
XXX: Check for flags?
*/
ENTRY(dtrace_copystr)
	stmfd   sp!, {r4-r5}			/* stack is 8 byte aligned */
	teq     r2, #0x00000000
	mov     r5, #0x00000000
	beq     2f

1:	ldrb    r4, [r0], #0x0001
	add     r5, r5, #0x00000001
	teq     r4, #0x00000000
	strb    r4, [r1], #0x0001
	teqne   r5, r2
	bne     1b

2:	ldmfd   sp!, {r4-r5}			/* stack is 8 byte aligned */
	RET
END(dtrace_copystr)


/*
void dtrace_invop_init(void)
*/
ENTRY(dtrace_invop_init)
	ldr	r1, .Ldtrace_invop
	ldr	r2, .Ldtrace_invop_jump_addr
	str	r1, [r2]
	RET
	.align	0
.Ldtrace_invop:
	.word	dtrace_invop
.Ldtrace_invop_jump_addr:
	.word	dtrace_invop_jump_addr
END(dtrace_invop_init)

/*
void dtrace_invop_uninit(void)
*/
ENTRY(dtrace_invop_uninit)
	mov	r0, #0
	ldr	r1, .Ldtrace_invop_jump_addr
	str	r0, [r1]
	RET
END(dtrace_invop_uninit)

/*
void
vpanic(const char *format, va_list alist)
*/
ENTRY(vpanic)				/* Initial stack layout: */
vpanic_common:
	RET
END(vpanic)

/*
void
dtrace_vpanic(const char *format, va_list alist)
*/
ENTRY(dtrace_vpanic)			/* Initial stack layout: */
	RET
END(dtrace_vpanic)

/*
uintptr_t
dtrace_caller(int aframes)
*/
ENTRY(dtrace_caller)
	mov	r0, #-1
	RET
END(dtrace_caller)
