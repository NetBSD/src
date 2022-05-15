/*	$NetBSD: atomic_c11_compare_exchange_cas_16.c,v 1.2.20.1 2022/05/15 12:37:00 martin Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann.
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

#include "atomic_op_namespace.h"

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stdbool.h>
#endif
#include <sys/atomic.h>

bool __atomic_compare_exchange_2(volatile void *, void *, uint16_t,
    bool, int, int);

bool
__atomic_compare_exchange_2(volatile void *mem,
    void *expected, uint16_t desired,
    bool weak, int success, int failure)
{
	uint16_t * const ep = expected;
	const uint16_t old = *ep;

	/*
	 * Ignore the details (weak, memory model on success and failure)
	 * and just do the cas. If we get here the compiler couldn't
	 * do better and it mostly will not matter at all.
	 */
	const uint16_t prev = atomic_cas_16(mem, old, desired);
	if (prev == old)
		return true;
	*ep = prev;
	return false;
}
