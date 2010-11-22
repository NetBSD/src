/*	$NetBSD: rump_atomic_cas_up.c,v 1.1 2010/11/22 10:50:51 pooka Exp $	*/

/*-
 * Copyright (c) 2010 Antti Kantee.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rump_atomic_cas_up.c,v 1.1 2010/11/22 10:50:51 pooka Exp $");

/*
 * Uniprocessor version of atomic CAS.  Since there is no preemption
 * in rump, this is a piece of cake.
 */

#include <sys/types.h>

uint32_t rumpup_cas_32_up(volatile uint32_t *ptr, uint32_t, uint32_t);

uint32_t
rumpup_cas_32_up(volatile uint32_t *ptr, uint32_t old, uint32_t new)
{
	uint32_t ret;

	ret = *ptr;
	if (__predict_true(ret == old)) {
		*ptr = new;
	}

	return ret;
}

__strong_alias(atomic_cas_32,rumpup_cas_32_up)
__strong_alias(atomic_cas_uint,rumpup_cas_32_up)
__strong_alias(atomic_cas_ulong,rumpup_cas_32_up)
__strong_alias(atomic_cas_ptr,rumpup_cas_32_up)
__strong_alias(atomic_cas_32_ni,rumpup_cas_32_up)
__strong_alias(atomic_cas_uint_ni,rumpup_cas_32_up)
__strong_alias(atomic_cas_ulong_ni,rumpup_cas_32_up)
__strong_alias(atomic_cas_ptr_ni,rumpup_cas_32_up)
