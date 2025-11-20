/* $NetBSD: cache.h,v 1.1.2.2 2025/11/20 19:14:49 martin Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _WII_CACHE_H
#define _WII_CACHE_H

#if CACHE_LINE_SIZE != 32
#error Incorrect CACHE_LINE_SIZE!
#endif

static inline void
cache_dcbf(void *addr, size_t size)
{
	uint32_t start = ((uint32_t)addr & ~(CACHE_LINE_SIZE - 1));
	uint32_t end = roundup((uint32_t)addr + size, CACHE_LINE_SIZE);

	asm volatile("eieio");
	while (start < end) {
		asm volatile("dcbf 0, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_SIZE;
	}
	asm volatile("sync");
}

static inline void
cache_dcbi(void *addr, size_t size)
{
	uint32_t start = ((uint32_t)addr & ~(CACHE_LINE_SIZE - 1));
	uint32_t end = roundup((uint32_t)addr + size, CACHE_LINE_SIZE);

	asm volatile("eieio");
	while (start < end) {
		asm volatile("dcbi 0, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_SIZE;
	}
	asm volatile("sync");
}

static inline void
cache_icbi(void *addr, size_t size)
{
	uint32_t start = ((uint32_t)addr & ~(CACHE_LINE_SIZE - 1));
	uint32_t end = roundup((uint32_t)addr + size, CACHE_LINE_SIZE);

	asm volatile("eieio");
	while (start < end) {
		asm volatile("icbi 0, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_SIZE;
	}
	asm volatile("sync");
}

#endif /* !_WII_CACHE_H */
