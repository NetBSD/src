/*	$NetBSD: atomic_and.c,v 1.1.2.1 2007/04/27 05:57:38 thorpej Exp $	*/

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
atomic_and_8_nv(volatile uint8_t *addr, uint8_t val)
{
	uint8_t old, new;

	do {
		OP_READ_BARRIER;
		old = *addr;

		new = old & val;
	} while (atomic_cas_8(addr, old, new) != old);

	return (new);
}

#undef atomic_and_8_nv
atomic_op_alias(atomic_and_8_nv,_atomic_and_8_nv)

#undef atomic_and_char_nv
atomic_op_alias(atomic_and_uchar_nv,_atomic_and_8_nv)
__strong_alias(_atomic_and_uchar_nv,_atomic_and_8_nv)

#undef atomic_and_8
atomic_op_alias(atomic_and_8,_atomic_and_8_nv)
__strong_alias(_atomic_and_8,_atomic_and_8_nv)

#undef atomic_and_char
atomic_op_alias(atomic_and_uchar,_atomic_and_8_nv)
__strong_alias(_atomic_and_uchar,_atomic_and_8_nv)

uint16_t
atomic_and_16_nv(volatile uint16_t *addr, uint16_t val)
{
	uint16_t old, new;

	do {
		OP_READ_BARRIER;
		old = *addr;

		new = old & val;
	} while (atomic_cas_16(addr, old, new) != old);

	return (new)
}

#undef atomic_and_16_nv
atomic_op_alias(atomic_and_16_nv,_atomic_and_16_nv)

#undef atomic_and_short_nv
atomic_op_alias(atomic_and_ushort_nv,_atomic_and_16_nv)
__strong_alias(_atomic_and_ushort_nv,_atomic_and_16_nv)

#undef atomic_and_16
atomic_op_alias(atomic_and_16,_atomic_and_16_nv)
__strong_alias(_atomic_and_16,_atomic_and_16_nv)

#undef atomic_and_short
atomic_op_alias(atomic_and_ushort,_atomic_and_16_nv)
__strong_alias(_atomic_and_ushort,_atomic_and_16_nv)

uint32_t
atomic_and_32_nv(volatile uint32_t *addr, uint32_t val)
{
	uint32_t old, new;

	do {
		OP_READ_BARRIER;
		old = *addr;

		new = old & val;
	} while (atomic_cas_32(addr, old, new) != old);

	return (new);
}

#undef atomic_and_32_nv
atomic_op_alias(atomic_and_32_nv,_atomic_and_32_nv)

#undef atomic_and_int_nv
atomic_op_alias(atomic_and_uint_nv,_atomic_and_32_nv)
__strong_alias(_atomic_and_uint_nv,_atomic_and_32_nv)

#undef atomic_and_long_nv
atomic_op_alias(atomic_and_ulong_nv,_atomic_and_32_nv)
__strong_alias(_atomic_and_ulong_nv,_atomic_and_32_nv)

#undef atomic_and_32
atomic_op_alias(atomic_and_32,_atomic_and_32_nv)
__strong_alias(_atomic_and_32,_atomic_and_32_nv)

#undef atomic_and_int
atomic_op_alias(atomic_and_uint,_atomic_and_32_nv)
__strong_alias(_atomic_and_uint,_atomic_and_32_nv)

#undef atomic_and_long
atomic_op_alias(atomic_and_ulong,_atomic_and_32_nv)
__strong_alias(_atomic_and_ulong,_atomic_and_32_nv)
