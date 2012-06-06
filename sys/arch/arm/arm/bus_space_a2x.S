/*	$NetBSD: bus_space_a2x.S,v 1.1 2012/06/06 20:21:43 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <machine/asm.h>
#include <machine/cpu.h>

RCSID("$NetBSD: bus_space_a2x.S,v 1.1 2012/06/06 20:21:43 skrll Exp $")

/*
 * bus_space_read_[124](void *cookie, bus_space_handle_t handle,
 *     bus_size_t offset);
 */

ENTRY(a2x_bs_r_1)
	ldrb	r0, [r1, r2, lsl #1]
	mov	pc, lr

#if (ARM_ARCH_4 + ARM_ARCH_5 + ARM_ARCH_6 + ARM_ARCH_7) > 0
ENTRY(a2x_bs_r_2)
	lsl	r2, r2, #1
	ldrh	r0, [r1, r2]
	mov	pc, lr
#endif

ENTRY(a2x_bs_r_4)
	ldr	r0, [r1, r2, lsl #1]
	mov	pc, lr

/*
 * bus_space_read_multi_[124](void *cookie, bus_space_handle_t handle,
 *     bus_size_t offset, uint{8,16,32}_t *data, bus_size_t count);
 */

ENTRY(a2x_bs_rm_1)
	lsl	r2, r2, #1
	b	generic_bs_rm_1

#if (ARM_ARCH_4 + ARM_ARCH_5 + ARM_ARCH_6 + ARM_ARCH_7) > 0
ENTRY(a2x_bs_rm_2)
	lsl	r2, r2, #1
	b	generic_armv4_bs_rm_2
#endif

ENTRY(a2x_bs_rm_4)
	lsl	r2, r2, #1
	b	generic_bs_rm_4

/*
 * bus_space_write_[124](void *cookie, bus_space_handle_t handle,
 *     bus_size_t offset, uint{8,16,32}_t value);
 */
ENTRY(a2x_bs_w_1)
	strb	r3, [r1, r2, lsl #1]
	mov	pc, lr

#if (ARM_ARCH_4 + ARM_ARCH_5 + ARM_ARCH_6 + ARM_ARCH_7) > 0
ENTRY(a2x_bs_w_2)
	lsl	r2, r2, #1
	strh	r3, [r1, r2]
	mov	pc, lr
#endif

ENTRY(a2x_bs_w_4)
	str	r3, [r1, r2, lsl #1]
	mov	pc, lr

/*
 * bus_space_write_multi_[124](void *cookie, bus_space_handle_t handle,
 *     bus_size_t offset, uint{8,16,32}_t *data, bus_size_t count);
 */

ENTRY(a2x_bs_wm_1)
	lsl	r2, r2, #1
	b	generic_bs_wm_1

#if (ARM_ARCH_4 + ARM_ARCH_5 + ARM_ARCH_6 + ARM_ARCH_7) > 0
ENTRY(a2x_bs_wm_2)
	lsl	r2, r2, #1
	b	generic_armv4_bs_wm_2
#endif

ENTRY(a2x_bs_wm_4)
	lsl	r2, r2, #1
	b	generic_bs_wm_4
