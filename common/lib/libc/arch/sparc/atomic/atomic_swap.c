/*	$NetBSD: atomic_swap.c,v 1.1.2.1 2007/04/27 05:57:39 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "../../../atomic/atomic_op_namespace.h"

#include <sys/atomic.h>
#include "../../../atomic/atomic_op_cas_impl.h"

uint8_t
atomic_swap_8(volatile uint8_t *addr, uint8_t new)
{
	uint8_t old;

	do {
		OP_READ_BARRIER;
		old = *addr;
	} while (atomic_cas_8(addr, old, new) != old);

	return (old);
}

#undef atomic_swap_8
atomic_op_alias(atomic_swap_8,_atomic_swap_8)

#undef atomic_swap_uchar
atomic_op_alias(atomic_swap_uchar,_atomic_swap_8)
__strong_alias(_atomic_swap_uchar,_atomic_swap_8)

uint16_t
atomic_swap_16(volatile uint16_t *addr, uint16_t new)
{
	uint16_t old;

	do {
		OP_READ_BARRIER;
		old = *addr;
	} while (atomic_cas_16(addr, old, new) != old);

	return (old)
}

#undef atomic_swap_16
atomic_op_alias(atomic_swap_16,_atomic_swap_16)

#undef atomic_swap_ushort
atomic_op_alias(atomic_swap_ushort,_atomic_swap_16)
__strong_alias(_atomic_swap_ushort,_atomic_swap_16)

uint32_t
atomic_swap_32(volatile uint32_t *addr, uint32_t new)
{
	uint32_t old;

	do {
		OP_READ_BARRIER;
		old = *addr;
	} while (atomic_cas_32(addr, old, new) != old);

	return (old);
}

#undef atomic_swap_32
atomic_op_alias(atomic_swap_32,_atomic_swap_32)

#undef atomic_swap_uint
atomic_op_alias(atomic_swap_uint,_atomic_swap_32)
__strong_alias(_atomic_swap_uint,_atomic_swap_32)

#undef atomic_swap_ulong
atomic_op_alias(atomic_swap_ulong,_atomic_swap_32)
__strong_alias(_atomic_swap_ulong,_atomic_swap_32)

#undef atomic_swap_ptr
atomic_op_alias(atomic_swap_ptr,_atomic_swap_32)
__strong_alias(_atomic_swap_ptr,_atomic_swap_32)
