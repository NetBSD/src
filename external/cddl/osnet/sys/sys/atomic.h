/*	$NetBSD: atomic.h,v 1.1 2009/08/07 20:57:57 haad Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _CDDL_SYS_ATOMIC_H_
#define	_CDDL_SYS_ATOMIC_H_

#include_next <sys/atomic.h>

#define casptr(_a, _b, _c)      \
	atomic_cas_ptr((volatile uint64_t *) _a, _b, _c);



static __inline void
atomic_or_8(volatile uint8_t *ptr, uint8_t val)
{
	uint32_t mask;

#if _BYTE_ORDER == _LITTLE_ENDIAN
	switch ((uintptr_t)ptr & 3) {
	case 0:
		mask = (uint32_t)val;
		break;
	case 1:
		mask = (uint32_t)val << 8;
		break;
	case 2:
		mask = (uint32_t)val << 16;
		break;
	case 3:
		mask = (uint32_t)val << 24;
		break;
	}
#elif _BYTE_ORDER == _BIG_ENDIAN
	switch ((uintptr_t)ptr & 3) {
	case 3:
		mask = (uint32_t)val;
		break;
	case 2:
		mask = (uint32_t)val << 8;
		break;
	case 1:
		mask = (uint32_t)val << 16;
		break;
	case 0:
		mask = (uint32_t)val << 24;
		break;
	}
#else
#error What byte order?
#endif

	atomic_or_32((uint32_t *)((uintptr_t)ptr & (uintptr_t)~3ULL), mask);
}

#endif	/* _CDDL_SYS_ATOMIC_H */
