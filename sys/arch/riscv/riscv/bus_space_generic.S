/*	$NetBSD: bus_space_generic.S,v 1.4 2023/05/07 12:41:48 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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


#include <riscv/asm.h>
#include "assym.h"

RCSID("$NetBSD: bus_space_generic.S,v 1.4 2023/05/07 12:41:48 skrll Exp $")


/* void bs_c_1(a0: tag, a1: src, srcoffset, dst, dstoffset, count); */
/* void bs_c_2(a0: tag, a1: src, srcoffset, dst, dstoffset, count); */
/* void bs_c_4(a0: tag, a1: src, srcoffset, dst, dstoffset, count); */
/* void bs_c_8(a0: tag, a1: src, srcoffset, dst, dstoffset, count); */

/* void bs_sr_1(a0: tag, a1: addr, a2: offset, a3: value, a4: count); */
/* void bs_sr_2(a0: tag, a1: addr, a2: offset, a3: value, a4: count); */
/* void bs_sr_4(a0: tag, a1: addr, a2: offset, a3: value, a4: count); */
/* void bs_sr_8(a0: tag, a1: addr, a2: offset, a3: value, a4: count); */


/* uint8_t bs_r_1(a0: tag, a1: addr, a2: offset); */
ENTRY_NP(generic_bs_r_1)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	lbu	a0, 0(a2)		/* load 8-bit */
	ret
END(generic_bs_r_1)


/* uint16_t bs_r_2(a0: tag, a1: addr, a2: offset); */
ENTRY_NP(generic_bs_r_2)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	lhu	a0, 0(a2)		/* load 16-bit */
	ret
END(generic_bs_r_2)


/* uint32_t bs_r_4(a0: tag, a1: addr, a2: offset); */
ENTRY_NP(generic_bs_r_4)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	lw	a0, 0(a2)		/* load 32-bit */
	ret
END(generic_bs_r_4)


#ifdef _LP64
/* uint64_t bs_r_8(a0: tag, a1: addr, a2: offset); */
ENTRY_NP(generic_bs_r_8)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	ld	a0, 0(a2)		/* load 64-bit */
	ret
END(generic_bs_r_8)
#endif


/* void bs_rm_1(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_rm_1)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lbu	a0, 0(a2)		/* load 8-bit */
	sb	a0, 0(a3)
	add	a3, a3, 1
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_rm_1: count == 0"
END(generic_bs_rm_1)


/* void bs_rm_2(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_rm_2)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lhu	a0, 0(a2)		/* load 16-bit */
	sh	a0, 0(a3)		/* store 16-bit */
	add	a3, a3, 2
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_rm_2: count == 0"
END(generic_bs_rm_2)


/* void bs_rm_4(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_rm_4)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lw	a0, 0(a2)		/* load 32-bit */
	sw	a0, 0(a3)		/* store 32-bit */
	add	a3, a3, 4
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_rm_4: count == 0"
END(generic_bs_rm_4)


#ifdef _LP64
/* void bs_rm_8(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_rm_8)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	ld	a0, 0(a2)		/* load 64-bit */
	sd	a0, 0(a3)		/* store 64-bit */
	add	a3, a3, 8
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_rm_8: count == 0"
END(generic_bs_rm_8)
#endif


/* void bs_rr_1(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_rr_1)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	li	t0, 1
	srl	t0, t0, a5		/* delta = 1 << stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lbu	a0, 0(a2)		/* load 8-bit */
	sb	a0, 0(a3)		/* *dst = value */
	add	a2, a2, t0		/* src += delta */
	add	a3, a3, 1		/* dst++ */
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_rr_1: count == 0"
END(generic_bs_rr_1)


/* void bs_rr_2(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_rr_2)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	li	t0, 1
	srl	t0, t0, a5		/* delta = 1 << stride */
// if (delta < 2)
//     delta = 2


	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lhu	a0, 0(a2)		/* load 16-bit */
	sh	a0, 0(a3)		/* *dst = value */
	add	a2, a2, t0		/* src += delta */
	add	a3, a3, 2		/* dst++ */
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_rr_2: count == 0"
END(generic_bs_rr_2)


/* void bs_rr_4(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
/* void bs_rr_8(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */



/* void bs_w_1(a0: tag, a1: addr, a2: offset, a3: value); */
ENTRY_NP(generic_bs_w_1)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	sb	a3, 0(a2)		/* store 8-bit */
	ret
END(generic_bs_w_1)


/* void bs_w_2(a0: tag, a1: addr, a2: offset, a3: value); */
ENTRY_NP(generic_bs_w_2)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	sh	a3, 0(a2)		/* store 16-bit */
	ret
END(generic_bs_w_2)


/* void bs_w_4(a0: tag, a1: addr, a2: offset, a3: value); */
ENTRY_NP(generic_bs_w_4)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	sw	a3, 0(a2)		/* store 32-bit */
	ret
END(generic_bs_w_4)


#ifdef _LP64
/* void bs_w_8(a0: tag, a1: addr, a2: offset, a3: value); */
ENTRY_NP(generic_bs_w_8)
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
	sd	a3, 0(a2)		/* store 64-bit */
	ret
END(generic_bs_w_8)
#endif


/* void bs_wm_1(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_wm_1)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lbu	a0, 0(a3)		/* load 8-bit */
	sb	a0, 0(a2)
	add	a3, a3, 1
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_wm_1: count == 0"
END(generic_bs_wm_1)


/* void bs_wm_2(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_wm_2)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lhu	a0, 0(a3)		/* load 16-bit */
	sh	a0, 0(a2)		/* store 16-bit */
	add	a3, a3, 2
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_wm_2: count == 0"
END(generic_bs_wm_2)


/* void bs_wm_4(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_wm_4)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	lw	a0, 0(a3)		/* load 32-bit */
	sw	a0, 0(a2)		/* store 32-bit */
	add	a3, a3, 4
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_wm_4: count == 0"
END(generic_bs_wm_4)


#ifdef _LP64
/* void bs_wm_8(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
ENTRY_NP(generic_bs_wm_8)
#ifdef DIAGNOSTIC
	beqz	a4, 2f
#endif
	PTR_L	a5, BS_STRIDE(a0)	/* stride */
	PTR_SLL	a2, a2, a5		/* offset <<= stride */
	PTR_ADD	a2, a2, a1		/* add to address */
1:
	ld	a0, 0(a3)		/* load 64-bit */
	sd	a0, 0(a2)		/* store 64-bit */
	add	a3, a3, 8
	add	a4, a4, -1		/* count-- */
	bnez	a4, 1b
	ret
2:
	la	x0, 3f
	tail	_C_LABEL(panic)
3:
	.asciz	"_bs_wm_8: count == 0"
END(generic_bs_wm_8)
#endif

/* void bs_wr_1(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
/* void bs_wr_2(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
/* void bs_wr_4(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
/* void bs_wr_8(a0: tag, a1: addr, a2: offset, a3: datap, a4: count); */
