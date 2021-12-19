/*	$NetBSD: io.h,v 1.7 2021/12/19 10:48:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _ASM_IO_H_
#define _ASM_IO_H_

#include <sys/cdefs.h>
#include <sys/systm.h>

#include <linux/string.h>

/*
 * XXX This is bollocks, and is wrong on various architectures (should
 * work for x86; who knows what else), but bus_space_barrier won't work
 * because we have no bus space tag or handle or offset or anything.
 */
#define	mmiowb()	membar_sync()

#define	memcpy_fromio(d,s,n)	memcpy((d),__UNVOLATILE(s),(n))
#define	memcpy_toio(d,s,n)	memcpy(__UNVOLATILE(d),(s),(n))

#if defined(__NetBSD__) && defined(__aarch64__)
static inline void *
memset_io(volatile void *b, int c, size_t len)
{
	volatile uint8_t *ptr = b;

	while (len > 0) {
		*ptr++ = c;
		len--;
	}

	return b;
}
#else
#define	memset_io(b,c,n)	memset(__UNVOLATILE(b),(c),(n))
#endif

/* XXX wrong place */
#define	__force

#endif  /* _ASM_IO_H_ */
