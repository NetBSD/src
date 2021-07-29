/* $NetBSD: atomic_op_asm.h,v 1.6 2021/07/29 10:29:05 skrll Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ATOMIC_OP_ASM_H_
#define	_ATOMIC_OP_ASM_H_

#include <machine/asm.h>

#define	ATOMIC_OP8(OP, INSN)						\
ENTRY_NP(_atomic_##OP##_8)						;\
	mov	x4, x0							;\
1:	ldxrb	w0, [x4]		/* load old value */		;\
	INSN	w2, w0, w1		/* calculate new value */	;\
	stxrb	w3, w2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	ret				/* return old value */		;\
END(_atomic_##OP##_8)

#define	SYNC_FETCH_OP8(OP, INSN)					\
ENTRY_NP(__sync_fetch_and_##OP##_1)					;\
	mov	x4, x0							;\
	dmb	ish							;\
1:	ldxrb	w0, [x4]		/* load old value */		;\
	INSN	w2, w0, w1		/* calculate new value */	;\
	stxrb	w3, w2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	dmb	ish							;\
	ret				/* return old value */		;\
END(__sync_fetch_and_##OP##_1)

#define	ATOMIC_OP8_NV(OP, INSN)						\
ENTRY_NP(_atomic_##OP##_8_nv)						;\
	mov	x4, x0			/* need x0 for return value */	;\
1:	ldxrb	w0, [x4]		/* load old value */		;\
	INSN	w0, w0, w1		/* calc new (return) value */	;\
	stxrb	w3, w0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	ret				/* return new value */		;\
END(_atomic_##OP##_8_nv)

#define	SYNC_OP8_FETCH(OP, INSN)					\
ENTRY_NP(__sync_##OP##_and_fetch_1)					;\
	mov	x4, x0			/* need x0 for return value */	;\
	dmb	ish							;\
1:	ldxrb	w0, [x4]		/* load old value */		;\
	INSN	w0, w0, w1		/* calc new (return) value */	;\
	stxrb	w3, w0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	dmb	ish							;\
	ret				/* return new value */		;\
END(__sync_##OP##_and_fetch_1)

#define	ATOMIC_OP16(OP, INSN)						\
ENTRY_NP(_atomic_##OP##_16)						;\
	mov	x4, x0							;\
1:	ldxrh	w0, [x4]		/* load old value */		;\
	INSN	w2, w0, w1		/* calculate new value */	;\
	stxrh	w3, w2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	ret				/* return old value */		;\
END(_atomic_##OP##_16)

#define	SYNC_FETCH_OP16(OP, INSN)					\
ENTRY_NP(__sync_fetch_and_##OP##_2)					;\
	mov	x4, x0							;\
	dmb	ish							;\
1:	ldxrh	w0, [x4]		/* load old value */		;\
	INSN	w2, w0, w1		/* calculate new value */	;\
	stxrh	w3, w2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	dmb	ish							;\
	ret				/* return old value */		;\
END(__sync_fetch_and_##OP##_2)

#define	ATOMIC_OP16_NV(OP, INSN)					\
ENTRY_NP(_atomic_##OP##_16_nv)						;\
	mov	x4, x0			/* need x0 for return value */	;\
1:	ldxrh	w0, [x4]		/* load old value */		;\
	INSN	w0, w0, w1		/* calc new (return) value */	;\
	stxrh	w3, w0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	ret				/* return new value */		;\
END(_atomic_##OP##_16_nv)

#define	SYNC_OP16_FETCH(OP, INSN)					\
ENTRY_NP(__sync__##OP##_and_fetch_2)					;\
	mov	x4, x0			/* need x0 for return value */	;\
	dmb	ish							;\
1:	ldxrh	w0, [x4]		/* load old value */		;\
	INSN	w0, w0, w1		/* calc new (return) value */	;\
	stxrh	w3, w0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	dmb	ish							;\
	ret				/* return new value */		;\
END(__sync__##OP##_and_fetch_2)

#define	ATOMIC_OP32(OP, INSN)						\
ENTRY_NP(_atomic_##OP##_32)						;\
	mov	x4, x0							;\
1:	ldxr	w0, [x4]		/* load old value */		;\
	INSN	w2, w0, w1		/* calculate new value */	;\
	stxr	w3, w2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	ret				/* return old value */		;\
END(_atomic_##OP##_32)

#define	SYNC_FETCH_OP32(OP, INSN)					\
ENTRY_NP(__sync_fetch_and_##OP##_4)					;\
	mov	x4, x0							;\
	dmb	ish							;\
1:	ldxr	w0, [x4]		/* load old value */		;\
	INSN	w2, w0, w1		/* calculate new value */	;\
	stxr	w3, w2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	dmb	ish							;\
	ret				/* return old value */		;\
END(__sync_fetch_and_##OP##_4)

#define	ATOMIC_OP32_NV(OP, INSN)					\
ENTRY_NP(_atomic_##OP##_32_nv)						;\
	mov	x4, x0			/* need x0 for return value */	;\
1:	ldxr	w0, [x4]		/* load old value */		;\
	INSN	w0, w0, w1		/* calc new (return) value */	;\
	stxr	w3, w0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again? */	;\
	ret				/* return new value */		;\
END(_atomic_##OP##_32_nv)

#define	SYNC_OP32_FETCH(OP, INSN)					\
ENTRY_NP(__sync__##OP##_and_fetch_4)					;\
	mov	x4, x0			/* need x0 for return value */	;\
	dmb	ish							;\
1:	ldxr	w0, [x4]		/* load old value */		;\
	INSN	w0, w0, w1		/* calc new (return) value */	;\
	stxr	w3, w0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again? */	;\
	dmb	ish							;\
	ret				/* return new value */		;\
END(__sync__##OP##_and_fetch_4)

#define	ATOMIC_OP64(OP, INSN)						\
ENTRY_NP(_atomic_##OP##_64)						;\
	mov	x4, x0							;\
1:	ldxr	x0, [x4]		/* load old value */		;\
	INSN	x2, x0, x1		/* calculate new value */	;\
	stxr	w3, x2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	ret				/* return old value */		;\
END(_atomic_##OP##_64)

#define	SYNC_FETCH_OP64(OP, INSN)					\
ENTRY_NP(__sync_fetch_and_##OP##_8)					;\
	mov	x4, x0							;\
	dmb	ish							;\
1:	ldxr	x0, [x4]		/* load old value */		;\
	INSN	x2, x0, x1		/* calculate new value */	;\
	stxr	w3, x2, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again */	;\
	dmb	ish							;\
	ret				/* return old value */		;\
END(__sync_fetch_and_##OP##_8)

#define	ATOMIC_OP64_NV(OP, INSN)					\
ENTRY_NP(_atomic_##OP##_64_nv)						;\
	mov	x4, x0			/* need x0 for return value */	;\
1:	ldxr	x0, [x4]		/* load old value */		;\
	INSN	x0, x0, x1		/* calc new (return) value */	;\
	stxr	w3, x0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again? */	;\
	ret				/* return new value */		;\
END(_atomic_##OP##_64_nv)

#define	SYNC_OP64_FETCH(OP, INSN)					\
ENTRY_NP(__sync_##OP##_and_fetch_8)					;\
	mov	x4, x0			/* need x0 for return value */	;\
	dmb	ish							;\
1:	ldxr	x0, [x4]		/* load old value */		;\
	INSN	x0, x0, x1		/* calc new (return) value */	;\
	stxr	w3, x0, [x4]		/* try to store */		;\
	cbnz	w3, 1b			/*   succeed? no, try again? */	;\
	dmb	ish							;\
	ret				/* return new value */		;\
END(__sync_##OP##_and_fetch_8)

#if defined(_KERNEL)

#define	ATOMIC_OP_ALIAS(a,s)	STRONG_ALIAS(a,s)

#else /* _KERNEL */

#define	ATOMIC_OP_ALIAS(a,s)	WEAK_ALIAS(a,s)

#endif /* _KERNEL */

#endif /* _ATOMIC_OP_ASM_H_ */
