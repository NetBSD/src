/*	$NetBSD: average.h,v 1.2 2021/12/19 10:38:38 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_AVERAGE_H_
#define	_LINUX_AVERAGE_H_

#define	DECLARE_EWMA(NAME, PREC, RATE)					      \
struct ewma_##NAME {							      \
	unsigned long ewma_state;					      \
};									      \
									      \
static inline void							      \
ewma_##NAME##_init(struct ewma_##NAME *ewma)				      \
{									      \
	ewma->ewma_state = 0;						      \
}									      \
									      \
static inline unsigned long						      \
ewma_##NAME##_read(struct ewma_##NAME *ewma)				      \
{									      \
	return ewma->ewma_state >> (PREC);				      \
}									      \
									      \
static inline void							      \
ewma_##NAME##_add(struct ewma_##NAME *ewma, unsigned long sample)	      \
{									      \
	const unsigned long shift = ilog2(RATE);			      \
	const unsigned long state = ewma->ewma_state;			      \
									      \
	sample <<= (PREC);						      \
	if (state == 0) {						      \
		ewma->ewma_state = sample;				      \
		return;							      \
	}								      \
	ewma->ewma_state = (((state << shift) - state) + sample) >> shift;    \
}

#endif	/* _LINUX_AVERAGE_H_ */
