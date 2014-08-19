/*	$NetBSD: irq_dispatch.S,v 1.12.2.2 2014/08/20 00:02:45 tls Exp $	*/

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"

#include <machine/asm.h>

#include <arm/locore.h>

#include "opt_arm_intr_impl.h"
#ifdef ARM_INTR_IMPL
#include ARM_INTR_IMPL
#else
#error ARM_INTR_IMPL not defined
#endif

#ifndef ARM_IRQ_HANDLER
#error ARM_IRQ_HANDLER not defined
#endif

/*
 * irq_entry:
 *	Main entry point for the IRQ vector.  This is a generic version
 *	which can be used by different platforms.
 */
	.text
	.p2align	5
ARM_ASENTRY_NP(irq_entry)
	sub	lr, lr, #0x00000004	/* Adjust the lr */

	PUSHFRAMEINSVC			/* Push an interrupt frame */
	ENABLE_ALIGNMENT_FAULTS		/* finishes with curcpu() in r4 */

	/*
	 * Increment the interrupt nesting depth and call the interrupt
	 * dispatch routine.  We've pushed a frame, so we can safely use
	 * callee-saved regs here.  We use the following registers, which
	 * we expect to persist:
	 *
	 *	r4	address of current cpu_info
	 *	r6	old value of `ci_intr_depth'
	 */
	ldr	r6, [r4, #CI_INTR_DEPTH]
	add	r1, r6, #1
	str	r1, [r4, #CI_INTR_DEPTH]

	mov	r0, sp			/* arg for dispatcher */
	bl	ARM_IRQ_HANDLER

	/*
	 * Restore the old interrupt depth value (which should be the
	 * same as decrementing it at this point).
	 */
	str	r6, [r4, #CI_INTR_DEPTH]

	LOCK_CAS_CHECK

	DO_AST_AND_RESTORE_ALIGNMENT_FAULTS
	PULLFRAMEFROMSVCANDEXIT
#ifdef __thumb__
	subs	pc, lr, #0		/* Exit */
#else
	movs	pc, lr			/* Exit */
#endif

	.align	0
LOCK_CAS_CHECK_LOCALS

AST_ALIGNMENT_FAULT_LOCALS
ASEND(irq_entry)

	.p2align	5
ARM_ASENTRY_NP(irq_idle_entry)
	PUSHIDLEFRAME

	/*
	 * Increment the interrupt nesting depth and call the interrupt
	 * dispatch routine.  We've pushed a frame, so we can safely use
	 * callee-saved regs here.  We use the following registers, which
	 * we expect to persist:
	 *
	 *	r4	address of current cpu_info
	 *	r6	old value of `ci_intr_depth'
	 */
	GET_CURCPU(r4)
	ldr	r6, [r4, #CI_INTR_DEPTH]
	add	r1, r6, #1
	str	r1, [r4, #CI_INTR_DEPTH]

	mov	r0, sp			/* arg for dispatcher */
	bl	ARM_IRQ_HANDLER

	/*
	 * Restore the old interrupt depth value (which should be the
	 * same as decrementing it at this point).
	 */
	str	r6, [r4, #CI_INTR_DEPTH]

#if defined(__HAVE_FAST_SOFTINTS) && !defined(__HAVE_PIC_FAST_SOFTINTS)
        ldr	r2, [r4, #CI_CPL]	/* Get current priority level */
	ldr	r3, [r4, #CI_SOFTINTS]	/* Get pending softint mask */
#endif

	PULLIDLEFRAME			/* restore r4, r6, sp, lr */

#if defined(__HAVE_FAST_SOFTINTS) && !defined(__HAVE_PIC_FAST_SOFTINTS)
        lsrs	r3, r3, r2		/* shift mask by cpl */
	bne	_C_LABEL(dosoftints)	/* dosoftints(void) */
#endif
	RET
ASEND(irq_idle_entry)
