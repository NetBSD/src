/*	$NetBSD: atomic_op_cas_impl.h,v 1.1.2.1 2007/04/13 04:28:19 thorpej Exp $	*/

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

#if !defined(_ATOMIC_OP_CAS_IMPL_H_)
#define	_ATOMIC_OP_CAS_IMPL_H_

/*
 * This relies heavily on constant-folding by the compier in order to be
 * efficient.
 */
#define	MASK_FOR_SUBWORD(sw)						\
	(sizeof(sw) == 1 ? (uint32_t)0xff				\
			 : sizeof(sw) == 2 ? (uint32_t)0xffff		\
			 		   : /* sizeof(sw) == 4 */ 0xffffffff)

#define	OP_READ_BARRIER		/* XXX */

#define	OP_LOCALS(type)							\
	volatile uint32_t *wordp;					\
	uint32_t old_word, new_word;					\
	int shift;							\
	type subword

#define	OP_EXTRACT(addr)						\
	wordp = (volatile uint32_t *)((uintptr_t)(addr) & ~(uintptr_t)3);\
	shift = ((uintptr_t)addr - (uintptr_t)addr) * 8;		\
	OP_READ_BARRIER;						\
	old_word = *wordp;						\
	subword = (old_word >> shift) & MASK_FOR_SUBWORD(subword)

#define	OP_INSERT							\
	new_word = (old_word & ~(MASK_FOR_SUBWORD(subword) << shift)) |	\
		   ((uint32_t)subword << shift)

#define	OP_DO(addr, op, val)						\
do {									\
	OP_EXTRACT(addr);						\
	subword op ## = (val);						\
	OP_INSERT;							\
} while (atomic_cas_32(wordp, old_word, new_word) != old_word)

#define	OP_NEW_VALUE	subword

#endif /* _ATOMIC_OP_CAS_IMPL_H_ */
